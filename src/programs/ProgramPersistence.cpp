#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramSlides.h"
#include "ProgramStore.h"
#include "SlideTransition.h"
#include "TemplateDefinition.h"
#include <QDate>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace {
QString uid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

}

bool ProgramStore::validate(QVariantMap &program, bool newIds) {
    if (program["id"].toString().isEmpty() || program["title"].toString().trimmed().isEmpty()
        || program["title"].toString().size() > 200) {
        return false;
    }
    const auto date = program["date"].toString();
    if (!date.isEmpty() && !QDate::fromString(date, Qt::ISODate).isValid()) {
        return false;
    }
    if (program["items"].metaType().id() != QMetaType::QVariantList) {
        return false;
    }
    QStringList sections { "Worship", "Sermon", "Closing" };
    if (program.contains("sections")) {
        const auto value = program["sections"];
        if (value.metaType().id() != QMetaType::QVariantList
            && value.metaType().id() != QMetaType::QStringList) {
            return false;
        }
        sections.clear();
        for (const auto &entry : value.toList()) {
            const auto name = entry.toString();
            if (entry.metaType().id() != QMetaType::QString || name.trimmed().isEmpty()
                || name != name.trimmed() || name.size() > 100 || name.contains('\n')
                || name.contains('\r') || sections.contains(name) || sections.size() >= 40) {
                return false;
            }
            sections << name;
        }
    }
    auto items = program["items"].toList();
    if (items.size() > 200) {
        return false;
    }
    int count = 0;
    QSet<QString> ids;
    for (auto &v : items) {
        auto item = v.toMap();
        if (item.contains("section")
            && (item["section"].metaType().id() != QMetaType::QString
                || (!item["section"].toString().isEmpty()
                    && !sections.contains(item["section"].toString())))) {
            return false;
        }
        if (item.contains("transition") && !SlideTransition::valid(item["transition"])) {
            return false;
        }
        if (item["id"].toString().isEmpty() || ids.contains(item["id"].toString())
            || item["title"].toString().trimmed().isEmpty() || item["title"].toString().size() > 200
            || item["notes"].toString().size() > 10000
            || !QStringList { "song", "scripture", "presentation", "media" }.contains(
                item["kind"].toString())
            || !QStringList { "original", "midnight", "paper" }.contains(
                item.value("template", "original").toString())) {
            return false;
        }
        if (!item.value("layoutTemplate").toMap().isEmpty()) {
            QString error;
            if (!TemplateDefinition::validate(item.value("layoutTemplate").toMap(), &error)) {
                return false;
            }
        }
        ids.insert(item["id"].toString());
        auto slides = EditorDocument::validatedSlides(item["slides"].toList());
        if (slides.isEmpty()) {
            return false;
        }
        count += slides.size();
        if (count > 2000) {
            return false;
        }
        if (newIds) {
            item["id"] = uid();
            slides = ProgramSlides::cloneWithFreshIds(slides);
        }
        item["slides"] = slides;
        v = item;
    }
    if (newIds) {
        program["id"] = uid();
    }
    program["items"] = items;
    return true;
}

bool ProgramStore::exportProgram(const QUrl &url) {
    if (id().isEmpty() || !url.isLocalFile()) {
        return false;
    }
    if (m_dirty && !save()) {
        return false;
    }
    QSaveFile file(url.toLocalFile());
    const auto bytes = QJsonDocument(QJsonObject { { "format", "churchdeck-program-1" },
                                         { "program", QJsonObject::fromVariantMap(m_program) } })
                           .toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        statusMessage("Could not export program");
        return false;
    }
    statusMessage("Program exported · media files stay on this computer");
    return true;
}

bool ProgramStore::importProgram(const QUrl &url) {
    if (url.toLocalFile().endsWith(".selahbundle", Qt::CaseInsensitive)) {
        return importBundle(url);
    }
    QFile file(url.toLocalFile());
    if (!url.isLocalFile() || !file.open(QIODevice::ReadOnly) || file.size() > 100 * 1024 * 1024) {
        statusMessage("Choose a local program file under 100 MB");
        return false;
    }
    const auto root = QJsonDocument::fromJson(file.readAll()).object();
    auto candidate = root["program"].toObject().toVariantMap();
    if (root["format"] != "churchdeck-program-1" || !validate(candidate, true)) {
        statusMessage("Invalid program; current service kept");
        return false;
    }
    if (m_dirty && !save()) {
        return false;
    }
    const auto previous = m_program;
    m_program = candidate;
    m_dirty = true;
    if (!save()) {
        m_program = previous;
        m_dirty = false;
        emit changed();
        return false;
    }
    m_undo.clear();
    m_redo.clear();
    m_selected = items().isEmpty() ? -1 : 0;
    emit changed();
    emit liveChanged();
    return true;
}

ProgramStore::ProgramStore(OutputManager *outputs, QObject *parent, const QString &directory)
    : QObject(parent)
    , m_outputs(outputs) {
    const auto folder = directory.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : directory;
    QDir().mkpath(folder);
    m_db = QSqlDatabase::addDatabase("QSQLITE", uid());
    m_db.setDatabaseName(folder + "/programs.sqlite");
    m_db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    if (m_db.open()) {
        QSqlQuery statement(m_db);
        statement.exec("PRAGMA journal_mode=WAL");
        if (!statement.exec(
                "CREATE TABLE IF NOT EXISTS programs(id TEXT PRIMARY KEY,title TEXT NOT "
                "NULL,date TEXT NOT NULL,data TEXT NOT NULL,updated INTEGER NOT NULL)")) {
            m_status = statement.lastError().text();
        }
        statement.exec("CREATE TABLE IF NOT EXISTS preferences(key TEXT PRIMARY KEY,value TEXT)");
        if (statement.exec("SELECT value FROM preferences WHERE key='current'")
            && statement.next()) {
            openProgram(statement.value(0).toString());
        }
    } else {
        m_status = m_db.lastError().text();
    }
    m_autosave.setSingleShot(true);
    m_autosave.setInterval(450);
    connect(&m_autosave, &QTimer::timeout, this, &ProgramStore::save);
    if (outputs) {
        connect(outputs, &OutputManager::liveChanged, this, &ProgramStore::liveChanged);
    }
}

ProgramStore::~ProgramStore() {
    const bool pendingBundle = m_bundleTimer.isActive();
    cancelBundle();
    m_bundleWatcher.waitForFinished();
    if (pendingBundle) {
        const auto directory = m_bundleWatcher.result().directory;
        if (!directory.isEmpty()) {
            QDir(directory).removeRecursively();
        }
    }
    if (m_dirty) {
        save();
    }
    const auto name = m_db.connectionName();
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

QVariantList ProgramStore::programs() const {
    QVariantList result;
    QSqlQuery statement(m_db);
    if (statement.exec("SELECT id,title,date FROM programs ORDER BY updated DESC,rowid DESC")) {
        while (statement.next()) {
            result << QVariantMap { { "id", statement.value(0) }, { "title", statement.value(1) },
                { "date", statement.value(2) } };
        }
    }
    return result;
}

bool ProgramStore::save() {
    // The snapshot and last-opened preference must succeed or roll back together.
    if (id().isEmpty()) {
        return true;
    }
    m_autosave.stop();
    if (!m_db.isOpen()) {
        statusMessage("Program storage unavailable");
        return false;
    }
    if (!m_db.transaction()) {
        statusMessage(m_db.lastError().text());
        return false;
    }
    QSqlQuery statement(m_db);
    statement.prepare(
        "INSERT INTO programs VALUES(?,?,?,?,unixepoch()) ON CONFLICT(id) DO UPDATE SET "
        "title=excluded.title,date=excluded.date,data=excluded.data,updated=excluded.updated");
    statement.addBindValue(id());
    statement.addBindValue(title());
    statement.addBindValue(date());
    statement.addBindValue(QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(m_program)).toJson(QJsonDocument::Compact)));
    bool ok = statement.exec();
    if (ok) {
        statement.prepare(
            "INSERT INTO preferences VALUES('current',?) ON CONFLICT(key) DO UPDATE SET "
            "value=excluded.value");
        statement.addBindValue(id());
        ok = statement.exec();
    }
    if (!ok || !m_db.commit()) {
        m_db.rollback();
        statusMessage("Could not save program: " + statement.lastError().text());
        return false;
    }
    m_dirty = false;
    statusMessage("Program saved on this computer");
    return true;
}

bool ProgramStore::createProgram(const QString &title, const QString &date) {
    if (title.trimmed().isEmpty() || title.size() > 200
        || (!date.isEmpty() && !QDate::fromString(date, Qt::ISODate).isValid())) {
        statusMessage("Enter a title and a valid date (YYYY-MM-DD)");
        return false;
    }
    if (m_dirty && !save()) {
        return false;
    }
    const auto previous = m_program;
    m_program = { { "id", uid() }, { "title", title.trimmed() }, { "date", date },
        { "items", QVariantList() },
        { "sections", QStringList { "Worship", "Sermon", "Closing" } } };
    m_dirty = true;
    if (!save()) {
        m_program = previous;
        m_dirty = false;
        emit changed();
        return false;
    }
    m_undo.clear();
    m_redo.clear();
    m_selected = -1;
    emit changed();
    emit liveChanged();
    return true;
}

bool ProgramStore::openProgram(const QString &id) {
    if (m_dirty && !save()) {
        return false;
    }
    QSqlQuery statement(m_db);
    statement.prepare("SELECT data FROM programs WHERE id=?");
    statement.addBindValue(id);
    if (!statement.exec() || !statement.next()) {
        statusMessage("Program could not be opened");
        return false;
    }
    auto candidate
        = QJsonDocument::fromJson(statement.value(0).toByteArray()).object().toVariantMap();
    if (!validate(candidate, false)) {
        statusMessage("Invalid program; current service kept");
        return false;
    }
    m_program = candidate;
    m_selected = items().isEmpty() ? -1 : 0;
    m_undo.clear();
    m_redo.clear();
    m_dirty = false;
    statement.prepare("INSERT INTO preferences VALUES('current',?) ON CONFLICT(key) DO UPDATE SET "
                      "value=excluded.value");
    statement.addBindValue(id);
    statement.exec();
    statusMessage("Program opened");
    emit liveChanged();
    return true;
}
