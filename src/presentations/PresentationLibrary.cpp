#include "PresentationLibrary.h"
#include "EditorDocument.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

PresentationLibrary::PresentationLibrary(QObject *parent, const QString &directory)
    : QObject(parent) {
    const auto folder = directory.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : directory;
    QDir().mkpath(folder);
    m_db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    m_db.setDatabaseName(folder + "/presentations.sqlite");
    m_db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    if (!m_db.open()) {
        m_status = m_db.lastError().text();
        return;
    }
    QSqlQuery query(m_db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS presentations(id TEXT PRIMARY KEY,title TEXT NOT "
                    "NULL,data TEXT NOT NULL)")) {
        m_status = query.lastError().text();
    }
}

PresentationLibrary::~PresentationLibrary() {
    const auto connection = m_db.connectionName();
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

void PresentationLibrary::setStatus(const QString &message) {
    m_status = message;
    emit changed();
}

QVariantList PresentationLibrary::presentations() const {
    QVariantList result;
    QSqlQuery query(m_db);
    if (query.exec("SELECT data FROM presentations ORDER BY title COLLATE NOCASE")) {
        while (query.next()) {
            result.append(
                QJsonDocument::fromJson(query.value(0).toByteArray()).object().toVariantMap());
        }
    }
    return result;
}

QVariantMap PresentationLibrary::presentation(const QString &id) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT data FROM presentations WHERE id=?");
    query.addBindValue(id);
    if (query.exec() && query.next()) {
        return QJsonDocument::fromJson(query.value(0).toByteArray()).object().toVariantMap();
    }
    return {};
}

QString PresentationLibrary::save(
    const QString &title, const QVariantList &slides, const QString &id) {
    const auto name = title.trimmed();
    if (name.isEmpty() || name.size() > 200) {
        setStatus("Enter a presentation name of 1–200 characters");
        return {};
    }
    const auto validated = EditorDocument::validatedSlides(slides);
    if (validated.isEmpty()) {
        setStatus("The presentation must contain valid slides");
        return {};
    }
    if (!id.isEmpty() && presentation(id).isEmpty()) {
        setStatus("Presentation not found. Save a new copy instead.");
        return {};
    }
    const auto key = id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id;
    const QVariantMap data { { "id", key }, { "title", name }, { "slides", validated } };
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO presentations VALUES(?,?,?) ON CONFLICT(id) DO UPDATE SET "
                  "title=excluded.title,data=excluded.data");
    query.addBindValue(key);
    query.addBindValue(name);
    query.addBindValue(
        QString::fromUtf8(QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact)));
    if (!query.exec()) {
        setStatus(query.lastError().text());
        return {};
    }
    setStatus("Presentation saved");
    return key;
}
