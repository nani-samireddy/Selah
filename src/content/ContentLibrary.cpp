#include "ContentLibrary.h"
#include "ScriptureImporter.h"
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QTemporaryDir>
#include <QTimer>
#include <QUuid>

namespace {
QString normalized(QString value) {
    return value.normalized(QString::NormalizationForm_C);
}

QVariantMap verseRow(const QSqlQuery &statement) {
    return { { "book", statement.value(0) }, { "chapter", statement.value(1) },
        { "verse", statement.value(2) }, { "text", statement.value(3) },
        { "reference",
            statement.value(4).toString() + " " + statement.value(1).toString() + ":"
                + statement.value(2).toString() },
        { "translation", statement.value(5) }, { "abbreviation", statement.value(6) },
        { "language", statement.value(7) }, { "copyright", statement.value(8) },
        { "license", statement.value(9) } };
}

const QString verseSelect
    = "SELECT "
      "v.book,v.chapter,v.verse,v.text,b.name,t.id,t.abbreviation,t.language,t.copyright,t.license "
      "FROM verses v JOIN translations t ON t.id=v.translation JOIN books b ON "
      "b.translation=v.translation AND b.code=v.book ";

}

ContentLibrary::ContentLibrary(QObject *parent, const QString &directory)
    : QObject(parent) {
    const auto folder = directory.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : directory;
    QDir().mkpath(folder);
    m_path = folder + "/content-library.sqlite";
    // Seed only a new library; bundled updates must never replace user songs/translations.
    if (!QFile::exists(m_path)) {
        if (!QFile::copy(":/qt/qml/ChurchDeck/assets/scriptures/content-seed.sqlite", m_path)) {
            m_status = "Could not initialize the offline scripture library";
            return;
        }
        QFile::setPermissions(m_path, QFile::ReadOwner | QFile::WriteOwner);
    }
    m_db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    m_db.setDatabaseName(m_path);
    m_db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    if (!m_db.open()) {
        m_status = m_db.lastError().text();
        return;
    }
    QSqlQuery statement(m_db);
    statement.exec("PRAGMA journal_mode=WAL");
    if (!statement.exec(
            "CREATE TABLE IF NOT EXISTS songs(id TEXT PRIMARY KEY,title TEXT NOT NULL,search "
            "TEXT NOT NULL,data TEXT NOT NULL)")) {
        m_status = statement.lastError().text();
    }
}

ContentLibrary::~ContentLibrary() {
    const auto name = m_db.connectionName();
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

QVariantList ContentLibrary::songs() const {
    return searchSongs("");
}

QVariantList ContentLibrary::searchSongs(const QString &query) const {
    QVariantList result;
    if (!m_db.isOpen()) {
        return result;
    }
    QSqlQuery statement(m_db);
    statement.prepare(
        "SELECT data FROM songs WHERE instr(search,?)>0 ORDER BY title COLLATE NOCASE LIMIT 500");
    statement.addBindValue(normalized(query).toCaseFolded());
    if (statement.exec()) {
        while (statement.next()) {
            result.append(
                QJsonDocument::fromJson(statement.value(0).toByteArray()).object().toVariantMap());
        }
    }
    return result;
}

QVariantMap ContentLibrary::song(const QString &id) const {
    QSqlQuery statement(m_db);
    statement.prepare("SELECT data FROM songs WHERE id=?");
    statement.addBindValue(id);
    if (statement.exec() && statement.next()) {
        return QJsonDocument::fromJson(statement.value(0).toByteArray()).object().toVariantMap();
    }
    return {};
}

QString ContentLibrary::saveSong(const QVariantMap &input) {
    if (!m_db.isOpen()) {
        setStatus("Content database unavailable");
        return {};
    }
    auto data = input;
    auto id = data["id"].toString();
    if (id.isEmpty()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    const auto title = data["title"].toString().trimmed();
    const auto sections = data["sections"].toList();
    if (title.isEmpty() || title.size() > 200 || sections.isEmpty() || sections.size() > 100) {
        setStatus("Enter a title and between 1 and 100 song sections");
        return {};
    }
    QSet<QString> keys;
    QString searchable = title + " " + data["tags"].toString() + " " + data["credits"].toString();
    for (const auto &value : sections) {
        const auto section = value.toMap();
        const auto key = section["key"].toString();
        const auto primary = section["primary"].toString();
        const auto secondary = section["secondary"].toString();
        if (!QRegularExpression("^[a-z][a-z0-9_-]{0,31}$").match(key).hasMatch()
            || keys.contains(key) || primary.trimmed().isEmpty() || primary.size() > 30000
            || secondary.size() > 30000) {
            setStatus("Each section needs a unique key and primary lyrics");
            return {};
        }
        if (!secondary.trimmed().isEmpty()
            && primary.split('\n').size() != secondary.split('\n').size()) {
            setStatus(section["label"].toString()
                + ": align primary and translated lyrics to the same number of lines");
            return {};
        }
        keys.insert(key);
        searchable += " " + primary + " " + secondary;
    }
    auto arrangement = data["arrangement"].toList();
    if (arrangement.isEmpty()) {
        for (const auto &value : sections) {
            arrangement.append(value.toMap()["key"]);
        }
    }
    if (arrangement.size() > 300) {
        setStatus("Arrangement is too long");
        return {};
    }
    for (const auto &key : arrangement) {
        if (!keys.contains(key.toString())) {
            setStatus("Arrangement refers to an unknown section: " + key.toString());
            return {};
        }
    }
    const auto arrangements = data["arrangements"].toList();
    QSet<QString> arrangementNames;
    if (arrangements.size() > 30) {
        setStatus("Use at most 30 arrangements per song");
        return {};
    }
    for (const auto &item : arrangements) {
        const auto a = item.toMap();
        const auto name = a["name"].toString().trimmed();
        const auto order = a["order"].toList();
        if (name.isEmpty() || name.size() > 80 || arrangementNames.contains(name.toCaseFolded())
            || order.isEmpty() || order.size() > 300) {
            setStatus("Each arrangement needs a unique name and 1 to 300 sections");
            return {};
        }
        arrangementNames.insert(name.toCaseFolded());
        for (const auto &key : order) {
            if (!keys.contains(key.toString())) {
                setStatus("Arrangement refers to an unknown section");
                return {};
            }
        }
    }
    if (!arrangements.isEmpty()) {
        const auto active = data["activeArrangement"].toInt();
        if (active < 0 || active >= arrangements.size()) {
            setStatus("Select a valid arrangement");
            return {};
        }
        arrangement = arrangements[active].toMap()["order"].toList();
    }
    data["id"] = id;
    data["title"] = title;
    data["arrangement"] = arrangement;
    QSqlQuery statement(m_db);
    statement.prepare("INSERT INTO songs VALUES(?,?,?,?) ON CONFLICT(id) DO UPDATE SET "
                      "title=excluded.title,search=excluded.search,data=excluded.data");
    statement.addBindValue(id);
    statement.addBindValue(title);
    statement.addBindValue(normalized(searchable).toCaseFolded());
    statement.addBindValue(QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(data)).toJson(QJsonDocument::Compact)));
    if (!statement.exec()) {
        setStatus(statement.lastError().text());
        return {};
    }
    setStatus("Song saved");
    return id;
}

bool ContentLibrary::importSong(const QUrl &url) {
    QFile f(url.toLocalFile());
    if (!url.isLocalFile() || !f.open(QIODevice::ReadOnly) || f.size() > 2 * 1024 * 1024) {
        setStatus("Choose a local song JSON file under 2 MB");
        return false;
    }
    auto root = QJsonDocument::fromJson(f.readAll()).object();
    if (root["format"] != "churchdeck-song-1") {
        setStatus("Invalid Church Deck song format");
        return false;
    }
    auto data = root["song"].toObject().toVariantMap();
    data.remove("id");
    return !saveSong(data).isEmpty();
}

bool ContentLibrary::exportSong(const QString &id, const QUrl &url) {
    auto data = song(id);
    if (data.isEmpty() || !url.isLocalFile()) {
        return false;
    }
    QSaveFile f(url.toLocalFile());
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }
    auto bytes = QJsonDocument(QJsonObject { { "format", "churchdeck-song-1" },
                                   { "song", QJsonObject::fromVariantMap(data) } })
                     .toJson();
    if (f.write(bytes) != bytes.size() || !f.commit()) {
        setStatus("Could not export song");
        return false;
    }
    setStatus("Song exported");
    return true;
}

QVariantList ContentLibrary::translations() const {
    QVariantList result;
    if (!m_db.isOpen()) {
        return result;
    }
    QSqlQuery statement(m_db);
    if (statement.exec(
            "SELECT id,title,abbreviation,language,copyright,license,source FROM translations "
            "ORDER BY id")) {
        while (statement.next()) {
            result.append(QVariantMap { { "id", statement.value(0) },
                { "title", statement.value(1) }, { "abbreviation", statement.value(2) },
                { "language", statement.value(3) }, { "copyright", statement.value(4) },
                { "license", statement.value(5) }, { "source", statement.value(6) } });
        }
    }
    return result;
}

QVariantList ContentLibrary::books(const QString &translation) const {
    QVariantList result;
    QSqlQuery statement(m_db);
    statement.prepare("SELECT code,name FROM books WHERE translation=? ORDER BY ordinal");
    statement.addBindValue(translation);
    if (statement.exec()) {
        while (statement.next()) {
            result.append(
                QVariantMap { { "code", statement.value(0) }, { "name", statement.value(1) } });
        }
    }
    return result;
}

QVariantList ContentLibrary::chapters(const QString &translation, const QString &book) const {
    QVariantList result;
    QSqlQuery statement(m_db);
    statement.prepare(
        "SELECT chapter,MIN(verse),MAX(verse) FROM verses WHERE translation=? AND book=? "
        "GROUP BY chapter ORDER BY chapter");
    statement.addBindValue(translation);
    statement.addBindValue(book);
    if (statement.exec()) {
        while (statement.next()) {
            result.append(QVariantMap { { "chapter", statement.value(0) },
                { "first", statement.value(1) }, { "last", statement.value(2) } });
        }
    }
    return result;
}

QVariantList ContentLibrary::passage(
    const QString &translation, const QString &book, int chapter, int first, int last) const {
    QVariantList result;
    if (first < 0 || last < first || last - first > 200) {
        return result;
    }
    QSqlQuery statement(m_db);
    statement.prepare(verseSelect
        + "WHERE v.translation=? AND v.book=? AND v.chapter=? AND v.verse BETWEEN ? AND ? ORDER BY "
          "v.verse");
    statement.addBindValue(translation);
    statement.addBindValue(book);
    statement.addBindValue(chapter);
    statement.addBindValue(first);
    statement.addBindValue(last);
    if (statement.exec()) {
        while (statement.next()) {
            result.append(verseRow(statement));
        }
    }
    return result;
}

QVariantList ContentLibrary::searchVerses(const QString &translation, const QString &query) const {
    QVariantList result;
    const auto terms = normalized(query).trimmed();
    if (terms.size() < 2) {
        return result;
    }
    const auto ref
        = QRegularExpression("^(.+?)\\s+(\\d+)(?::(\\d+)(?:[-–](\\d+))?)?$").match(terms);
    if (ref.hasMatch()) {
        for (const auto &value : books(translation)) {
            const auto b = value.toMap();
            if (b["name"].toString().compare(ref.captured(1), Qt::CaseInsensitive) == 0
                || b["code"].toString().compare(ref.captured(1), Qt::CaseInsensitive) == 0) {
                return passage(translation, b["code"].toString(), ref.captured(2).toInt(),
                    ref.captured(3).isEmpty() ? 0 : ref.captured(3).toInt(),
                    ref.captured(4).isEmpty()
                        ? (ref.captured(3).isEmpty() ? 200 : ref.captured(3).toInt())
                        : ref.captured(4).toInt());
            }
        }
        return {};
    }
    QSqlQuery statement(m_db);
    QString escaped = terms;
    escaped.replace('"', "\"\"");
    statement.prepare(verseSelect
        + "WHERE v.translation=? AND v.rowid IN (SELECT rowid FROM verses_fts WHERE verses_fts "
          "MATCH ?) ORDER BY b.ordinal,v.chapter,v.verse LIMIT 100");
    statement.addBindValue(translation);
    statement.addBindValue('"' + escaped + '"');
    if (statement.exec()) {
        while (statement.next()) {
            result.append(verseRow(statement));
        }
    }
    return result;
}

void ContentLibrary::importBible(const QUrl &url) {
    if (m_busy) {
        return;
    }
    m_busy = true;
    setStatus("Importing and indexing translation…");
    auto watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher] {
        auto error = watcher->result();
        watcher->deleteLater();
        m_busy = false;
        setStatus(error.isEmpty() ? "Translation imported" : error);
    });
    const auto path = m_path;
    watcher->setFuture(
        QtConcurrent::run([path, url] { return ScriptureImporter::importFile(path, url); }));
}
