#include "AppBackup.h"
#include <functional>
#include <QDataStream>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSet>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QtConcurrentRun>
#include <QTemporaryDir>
#include <QUuid>

namespace {
QString unique() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString quote(QString name) {
    return '"' + name.replace('"', "\"\"") + '"';
}

bool under(const QString &path, const QString &root) {
    return path == root || path.startsWith(root + '/');
}

QVariant relocated(const QVariant &value, const QMap<QString, QString> &replacements) {
    if (value.metaType().id() == QMetaType::QVariantMap) {
        auto map = value.toMap();
        for (auto &item : map) {
            item = relocated(item, replacements);
        }
        return map;
    }
    if (value.metaType().id() == QMetaType::QVariantList) {
        auto list = value.toList();
        for (auto &item : list) {
            item = relocated(item, replacements);
        }
        return list;
    }
    if (value.metaType().id() == QMetaType::QStringList) {
        QStringList result;
        for (const auto &item : value.toStringList()) {
            result.append(relocated(item, replacements).toString());
        }
        return result;
    }
    if (value.metaType().id() == QMetaType::QString) {
        auto text = value.toString();
        for (auto it = replacements.cbegin(); it != replacements.cend(); ++it) {
            text.replace(it.key(), it.value());
        }
        return text;
    }
    return value;
}

QString relocatedText(const QString &text, const QMap<QString, QString> &replacements) {
    const auto doc = QJsonDocument::fromJson(text.toUtf8());
    if (!doc.isNull()) {
        const auto next = relocated(doc.toVariant(), replacements);
        if (next == doc.toVariant()) {
            return text;
        }
        return QString::fromUtf8(QJsonDocument::fromVariant(next).toJson(QJsonDocument::Compact));
    }
    return relocated(text, replacements).toString();
}

bool settingsFile(const QString &path, QVariantMap &settings, bool writing) {
    QFile file(path);
    if (!file.open(writing ? QIODevice::WriteOnly : QIODevice::ReadOnly)
        || (!writing && file.size() > 4 * 1024 * 1024)) {
        return false;
    }
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_6_0);
    if (writing) {
        stream << settings;
    } else {
        stream >> settings;
    }
    return stream.status() == QDataStream::Ok && (writing || file.atEnd());
}

bool database(const QString &path, const std::function<bool(QSqlDatabase &)> &run) {
    const auto connection = unique();
    bool ok = false;
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
        db.setDatabaseName(path);
        db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
        if (db.open()) {
            ok = run(db);
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connection);
    return ok;
}

bool safeRelative(const QString &relative) {
    static const QRegularExpression reserved(
        "^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\.|$)", QRegularExpression::CaseInsensitiveOption);
    for (const auto &part : relative.split('/')) {
        if (part.endsWith(' ') || part.endsWith('.') || reserved.match(part).hasMatch()) {
            return false;
        }
    }
    return !relative.isEmpty() && !QDir::isAbsolutePath(relative) && !relative.contains('\\')
        && !relative.contains(':') && relative != ".." && !relative.startsWith("../")
        && QDir::cleanPath(relative) == relative && !relative.startsWith('/');
}

// Only local media references in JSON values are discovered; no network activity occurs.
void localReferences(const QVariant &value, QSet<QString> &urls) {
    if (value.metaType().id() == QMetaType::QVariantMap) {
        const auto map = value.toMap();
        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            if (it.key() == "source" && QUrl(it.value().toString()).isLocalFile()) {
                urls.insert(it.value().toString());
            } else {
                localReferences(it.value(), urls);
            }
        }
    } else if (value.metaType().id() == QMetaType::QVariantList) {
        for (const auto &item : value.toList()) {
            localReferences(item, urls);
        }
    }
}

QStringList ordinaryTables(QSqlDatabase &db) {
    QStringList tables;
    QSqlQuery query(db);
    if (query.exec("PRAGMA table_list")) {
        while (query.next()) {
            if (query.value(2) == "table" && !query.value(1).toString().startsWith("sqlite_")) {
                tables.append(query.value(1).toString());
            }
        }
    }
    return tables;
}
}

BundleResult AppBackup::write(const QString &root, const QVariantMap &settings, const QString &path,
    const std::shared_ptr<BundleProgress> &progress) {
    if (under(QFileInfo(path).absoluteFilePath(), root)) {
        return { {}, {}, "Save the backup outside Selah's data folder" };
    }
    QTemporaryDir staging;
    if (!staging.isValid()) {
        return { {}, {}, "Could not create backup workspace" };
    }
    QVariantList files, external;
    QSet<QString> references;
    QDirIterator entries(
        root, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (entries.hasNext()) {
        const auto source = entries.next();
        const auto relative = QDir(root).relativeFilePath(source);
        if (source.endsWith("-wal") || source.endsWith("-shm") || source.endsWith("-journal")
            || source.endsWith(".lock") || source.endsWith(".part") || source.endsWith(".tmp")
            || source.endsWith(".DS_Store")) {
            continue;
        }
        if (!safeRelative(relative) || progress->cancelled) {
            return { {}, {}, "Backup cancelled or invalid file path" };
        }
        QString snapshot = source;
        if (source.endsWith(".sqlite") || source.endsWith(".db")) {
            snapshot = staging.filePath(relative);
            if (!QDir().mkpath(QFileInfo(snapshot).absolutePath())
                || !database(source, [&](QSqlDatabase &db) {
                       QSqlQuery vacuum(db);
                       if (!vacuum.exec(
                               "VACUUM INTO '" + QString(snapshot).replace('\'', "''") + "'")) {
                           return false;
                       }
                       for (const auto &table : ordinaryTables(db)) {
                           QSqlQuery rows(db);
                           if (!rows.exec("SELECT * FROM " + quote(table))) {
                               return false;
                           }
                           while (rows.next()) {
                               for (int column = 0; column < rows.record().count(); ++column) {
                                   const auto text = rows.value(column).toString();
                                   if (!text.contains("file:")) {
                                       continue;
                                   }
                                   const auto doc = QJsonDocument::fromJson(text.toUtf8());
                                   localReferences(doc.toVariant(), references);
                                   if (QUrl(text).isLocalFile()) {
                                       references.insert(text);
                                   }
                               }
                           }
                       }
                       return true;
                   })) {
                return { {}, {}, "Could not snapshot database: " + relative };
            }
        }
        if (source.endsWith(".json")) {
            QFile json(source);
            if (json.open(QIODevice::ReadOnly) && json.size() <= 100 * 1024 * 1024) {
                localReferences(QJsonDocument::fromJson(json.readAll()).toVariant(), references);
            }
        }
        files.append(QVariantMap { { "relative", relative },
            { "permissions", int(QFile::permissions(source)) },
            { "source", QUrl::fromLocalFile(snapshot).toString() } });
    }
    // Settings may refer to user-selected models or runtimes outside the managed profile.
    for (const auto &value : settings) {
        const auto text = value.toString();
        if (QUrl(text).isLocalFile()) {
            references.insert(text);
        } else if (QDir::isAbsolutePath(text) && QFileInfo(text).isFile()) {
            references.insert(QUrl::fromLocalFile(text).toString());
        }
    }
    for (const auto &url : references) {
        const auto source = QUrl(url).toLocalFile();
        if (under(QFileInfo(source).absoluteFilePath(), root)) {
            continue;
        }
        if (!QFileInfo(source).isFile()) {
            return { {}, {}, "A referenced external file is missing: " + source };
        }
        external.append(QVariantMap { { "original", url },
            { "permissions", int(QFile::permissions(source)) }, { "source", url } });
    }
    auto prefs = settings;
    const auto prefsPath = staging.filePath("preferences.bin");
    if (!settingsFile(prefsPath, prefs, true)) {
        return { {}, {}, "Could not snapshot preferences" };
    }
    return PortableBundle::write(
        { { "appBackupFormat", 1 }, { "root", root }, { "files", files }, { "external", external },
            { "preferences",
                QVariantMap { { "source", QUrl::fromLocalFile(prefsPath).toString() } } } },
        path, progress);
}

BundleResult AppBackup::prepare(
    const QString &path, const QString &root, const std::shared_ptr<BundleProgress> &progress) {
    if (QFile::exists(root + ".restore-request")) {
        return { {}, {}, "A restore is already prepared. Restart Selah to finish it." };
    }
    QTemporaryDir workspace(QFileInfo(root).absolutePath() + "/selah-backup-import-XXXXXX");
    if (!workspace.isValid()) {
        return { {}, {}, "Could not create restore workspace" };
    }
    const auto bundle = PortableBundle::read(path, workspace.path(), progress);
    if (!bundle.error.isEmpty()) {
        return bundle;
    }
    const auto data = bundle.program;
    const auto files = data["files"].toList();
    if (data["appBackupFormat"].toInt() != 1 || data["root"].toString().size() < 2
        || files.isEmpty()) {
        return { {}, {}, "Choose a full Selah backup, not a program bundle" };
    }
    const auto staging = root + ".restore-" + unique();
    if (!QDir().mkpath(staging)) {
        return { {}, {}, "Could not stage restored data" };
    }
    bool keep = false;
    const auto cleanup = qScopeGuard([&] {
        if (!keep) {
            QDir(staging).removeRecursively();
        }
    });
    QSet<QString> names;
    auto stagedSource = [&](const QVariant &value) {
        const auto url = QUrl(value.toString());
        const auto source = QFileInfo(url.toLocalFile()).absoluteFilePath();
        return url.isLocalFile() && under(source, bundle.directory) && QFileInfo(source).isFile()
            ? source
            : QString();
    };
    for (const auto &value : files) {
        const auto file = value.toMap();
        const auto relative = file["relative"].toString();
        const auto source = stagedSource(file["source"]);
        if (!safeRelative(relative) || names.contains(relative.toCaseFolded())) {
            return { {}, {}, "Invalid or duplicate backup path: " + relative };
        }
        if (progress->cancelled) {
            return { {}, {}, "Restore cancelled" };
        }
        if (source.isEmpty()) {
            return { {}, {},
                "Missing staged asset: " + file["source"].toString() + " in " + bundle.directory };
        }
        QDir().mkpath(QFileInfo(staging + '/' + relative).absolutePath());
        QFile inputFile(source);
        if (!inputFile.rename(staging + '/' + relative)) {
            return { {}, {},
                "Could not stage restored file: " + relative + " · " + inputFile.errorString() };
        }
        QFile::setPermissions(
            staging + '/' + relative, QFileDevice::Permissions(file["permissions"].toInt()));
        names.insert(relative.toCaseFolded());
    }
    const auto oldRoot = data["root"].toString();
    QMap<QString, QString> replacements { { oldRoot, root },
        { QUrl::fromLocalFile(oldRoot).toString(), QUrl::fromLocalFile(root).toString() } };
    for (const auto &value : data["external"].toList()) {
        const auto file = value.toMap();
        const auto original = file["original"].toString();
        const auto source = stagedSource(file["source"]);
        const auto relative = "restored-external/" + QFileInfo(source).fileName();
        QDir().mkpath(staging + "/restored-external");
        if (!QUrl(original).isLocalFile() || source.isEmpty()
            || (!QFile::exists(staging + '/' + relative)
                && !QFile::rename(source, staging + '/' + relative))) {
            return { {}, {}, "Invalid external backup asset" };
        }
        QFile::setPermissions(
            staging + '/' + relative, QFileDevice::Permissions(file["permissions"].toInt()));
        replacements[original] = QUrl::fromLocalFile(root + '/' + relative).toString();
        replacements[QUrl(original).toLocalFile()] = root + '/' + relative;
    }
    QVariantMap settings;
    if (!settingsFile(stagedSource(data["preferences"].toMap()["source"]), settings, false)) {
        return { {}, {}, "Invalid backup preferences" };
    }
    settings = relocated(settings, replacements).toMap();
    if (!settingsFile(staging + "/restore-preferences.bin", settings, true)) {
        return { {}, {}, "Could not stage preferences" };
    }
    for (const auto &value : files) {
        const auto relative = value.toMap()["relative"].toString();
        if (relative.endsWith(".json")) {
            QFile input(staging + '/' + relative);
            if (!input.open(QIODevice::ReadOnly) || input.size() > 100 * 1024 * 1024) {
                return { {}, {}, "Could not verify restored JSON data" };
            }
            const auto bytes = input.readAll();
            input.close();
            const auto next = relocatedText(QString::fromUtf8(bytes), replacements).toUtf8();
            if (next != bytes) {
                QSaveFile output(input.fileName());
                if (!output.open(QIODevice::WriteOnly) || output.write(next) != next.size()
                    || !output.commit()) {
                    return { {}, {}, "Could not relocate restored JSON data" };
                }
            }
        }
        if (!relative.endsWith(".sqlite") && !relative.endsWith(".db")) {
            continue;
        }
        if (progress->cancelled || !database(staging + '/' + relative, [&](QSqlDatabase &db) {
                QSqlQuery integrity(db);
                if (!integrity.exec("PRAGMA integrity_check") || !integrity.next()
                    || integrity.value(0) != "ok" || !db.transaction()) {
                    return false;
                }
                for (const auto &table : ordinaryTables(db)) {
                    const auto fields = db.record(table);
                    for (int column = 0; column < fields.count(); ++column) {
                        const auto name = quote(fields.fieldName(column));
                        QSqlQuery rows(db);
                        rows.prepare("SELECT DISTINCT " + name + " FROM " + quote(table)
                            + " WHERE typeof(" + name + ")='text' AND (instr(" + name
                            + ",?)>0 OR instr(" + name + ",'file:')>0)");
                        rows.addBindValue(oldRoot);
                        if (!rows.exec()) {
                            db.rollback();
                            return false;
                        }
                        QStringList values;
                        while (rows.next()) {
                            values.append(rows.value(0).toString());
                        }
                        rows.finish();
                        for (const auto &text : values) {
                            const auto next = relocatedText(text, replacements);
                            if (next == text) {
                                continue;
                            }
                            QSqlQuery update(db);
                            update.prepare("UPDATE " + quote(table) + " SET " + name + "=? WHERE "
                                + name + "=?");
                            update.addBindValue(next);
                            update.addBindValue(text);
                            if (!update.exec()) {
                                db.rollback();
                                return false;
                            }
                        }
                    }
                }
                return db.commit();
            })) {
            return { {}, {}, "Backup cancelled or restored database could not be verified" };
        }
    }
    // A file is the handoff to the next launch; no open database is replaced in this process.
    if (progress->cancelled) {
        return { {}, {}, "Restore cancelled" };
    }
    QSaveFile request(root + ".restore-request");
    if (!request.open(QIODevice::WriteOnly)
        || request.write(staging.toUtf8()) != staging.toUtf8().size() || !request.commit()) {
        return { {}, {}, "Could not prepare restore for next launch" };
    }
    keep = true;
    return { {}, staging, {} };
}

QString AppBackup::applyPendingRestore(const QString &root, bool applySettings) {
    QFile request(root + ".restore-request");
    if (!request.exists()) {
        return {};
    }
    if (!request.open(QIODevice::ReadOnly) || request.size() > 8192) {
        return "Could not read pending restore";
    }
    const auto staging = QString::fromUtf8(request.readAll());
    request.close();
    if (!staging.startsWith(root + ".restore-") || staging.contains("..")
        || !QFileInfo(staging).isDir()) {
        return "Invalid pending restore path";
    }
    QVariantMap settings;
    if (!settingsFile(staging + "/restore-preferences.bin", settings, false)) {
        return "Invalid pending restore preferences";
    }
    const auto previous = root + ".before-restore-" + unique();
    QDir directory;
    if (!directory.rename(root, previous)) {
        return "Could not preserve the current profile; restore was not applied";
    }
    if (!directory.rename(staging, root)) {
        directory.rename(previous, root);
        return "Could not apply restored profile; original data kept";
    }
    if (applySettings) {
        QSettings prefs;
        QVariantMap prior;
        for (const auto &key : prefs.allKeys()) {
            prior[key] = prefs.value(key);
        }
        settingsFile(previous + "/restore-preferences.bin", prior, true);
        prefs.clear();
        for (auto it = settings.cbegin(); it != settings.cend(); ++it) {
            prefs.setValue(it.key(), it.value());
        }
        prefs.sync();
    }
    QFile::remove(root + "/restore-preferences.bin");
    QFile::remove(root + ".restore-request");
    // Checkpointed sessions may be offered, but all output controllers start inactive.
    return {};
}

AppBackup::AppBackup(const QString &directory, QObject *parent)
    : QObject(parent) {
    m_root = directory.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                 : directory;
    QDir().mkpath(m_root);
    m_timer.setInterval(100);
    connect(&m_timer, &QTimer::timeout, this, &AppBackup::changed);
    connect(&m_watcher, &QFutureWatcher<BundleResult>::finished, this, [this] {
        m_timer.stop();
        const auto result = m_watcher.result();
        m_status = !result.error.isEmpty() ? result.error
            : result.directory.isEmpty()   ? "Full backup exported"
                                         : "Backup verified. Quit and reopen Selah to restore it. "
                                           "Your current data will be preserved.";
        emit changed();
    });
}

AppBackup::~AppBackup() {
    cancel();
    m_watcher.waitForFinished();
}

bool AppBackup::restoreReady() const {
    return QFile::exists(m_root + ".restore-request");
}

void AppBackup::cancel() {
    if (m_progress) {
        m_progress->cancelled = true;
    }
}

bool AppBackup::exportBackup(const QUrl &url) {
    return start(url, false);
}

bool AppBackup::importBackup(const QUrl &url) {
    return start(url, true);
}

bool AppBackup::start(const QUrl &url, bool importing) {
    if (!url.isLocalFile() || busy() || restoreReady()) {
        return false;
    }
    QVariantMap settings;
    QSettings prefs;
    prefs.sync();
    for (const auto &key : prefs.allKeys()) {
        settings[key] = prefs.value(key);
    }
    m_progress = std::make_shared<BundleProgress>();
    const auto progress = m_progress;
    const auto root = m_root;
    const auto path = url.toLocalFile();
    m_status
        = importing ? "Verifying full backup…" : "Backing up libraries, media and preferences…";
    m_watcher.setFuture(QtConcurrent::run([root, path, settings, progress, importing] {
        return importing ? prepare(path, root, progress) : write(root, settings, path, progress);
    }));
    m_timer.start();
    emit changed();
    return true;
}

void AppBackup::cancelRestore() {
    if (busy()) {
        return;
    }
    QFile request(m_root + ".restore-request");
    if (!request.open(QIODevice::ReadOnly) || request.size() > 8192) {
        return;
    }
    const auto staging = QString::fromUtf8(request.readAll());
    request.close();
    if (staging.startsWith(m_root + ".restore-") && !staging.contains("..")) {
        QDir(staging).removeRecursively();
        QFile::remove(request.fileName());
        m_status = "Prepared restore cancelled. Current data kept.";
        emit changed();
    }
}
