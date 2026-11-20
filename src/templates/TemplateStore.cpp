#include "TemplateStore.h"
#include "EditorDocument.h"
#include "TemplateComposer.h"
#include "TemplateDefinition.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

TemplateStore::TemplateStore(QObject *parent, const QString &directory)
    : QObject(parent) {
    const auto folder = directory.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : directory;
    QDir().mkpath(folder);
    m_db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    m_db.setDatabaseName(folder + "/templates.sqlite");
    m_db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    if (!m_db.open()) {
        m_status = m_db.lastError().text();
        return;
    }
    QSqlQuery statement(m_db);
    if (!statement.exec("CREATE TABLE IF NOT EXISTS templates(id TEXT PRIMARY KEY,name TEXT NOT "
                        "NULL,data TEXT NOT NULL)")) {
        m_status = statement.lastError().text();
        return;
    }
    for (const auto &kind : { QString("song"), QString("bilingual"), QString("scripture") }) {
        const auto id = "starter-" + kind;
        if (layout(id).isEmpty()) {
            auto definition = TemplateDefinition::starter(kind);
            definition["id"] = id;
            if (persist(definition).isEmpty()) {
                return;
            }
        }
    }
    m_status = "Templates ready";
}

TemplateStore::~TemplateStore() {
    const auto connection = m_db.connectionName();
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

QVariantList TemplateStore::layouts() const {
    QVariantList result;
    QSqlQuery statement(m_db);
    if (statement.exec("SELECT data FROM templates ORDER BY name COLLATE NOCASE")) {
        while (statement.next()) {
            result << QJsonDocument::fromJson(statement.value(0).toByteArray())
                          .object()
                          .toVariantMap();
        }
    }
    return result;
}

QVariantMap TemplateStore::layout(const QString &id) const {
    QSqlQuery statement(m_db);
    statement.prepare("SELECT data FROM templates WHERE id=?");
    statement.addBindValue(id);
    return statement.exec() && statement.next()
        ? QJsonDocument::fromJson(statement.value(0).toByteArray()).object().toVariantMap()
        : QVariantMap();
}

QString TemplateStore::persist(QVariantMap definition) {
    QString error;
    if (!TemplateDefinition::validate(definition, &error)) {
        setStatus(error);
        return {};
    }
    auto id = definition["id"].toString();
    if (id.isEmpty()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    definition["id"] = id;
    definition["revision"] = layout(id).value("revision", 0).toInt() + 1;
    definition["slide"] = EditorDocument::validatedSlides({ definition["slide"] })[0];
    QSqlQuery statement(m_db);
    statement.prepare("INSERT INTO templates VALUES(?,?,?) ON CONFLICT(id) DO UPDATE SET "
                      "name=excluded.name,data=excluded.data");
    statement.addBindValue(id);
    statement.addBindValue(definition["name"]);
    statement.addBindValue(QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(definition)).toJson(QJsonDocument::Compact)));
    if (!statement.exec()) {
        setStatus("Could not save template: " + statement.lastError().text());
        return {};
    }
    setStatus("Template saved · existing services keep their layout snapshots");
    return id;
}

QString TemplateStore::saveTemplate(
    const QString &name, const QString &kind, const QVariantMap &slide, const QString &id) {
    if (!id.isEmpty() && layout(id).isEmpty()) {
        setStatus("The template to update could not be found");
        return {};
    }
    return persist(
        { { "id", id }, { "name", name.trimmed() }, { "kind", kind }, { "slide", slide } });
}

QString TemplateStore::createTemplate(const QString &name, const QString &kind) {
    auto definition = TemplateDefinition::starter(kind);
    definition["name"] = name.trimmed();
    return persist(definition);
}

QVariantMap TemplateStore::preview(const QString &id) const {
    const auto definition = layout(id);
    QString error;
    QVariantList slides;
    if (definition["kind"] == "scripture") {
        slides = TemplateComposer::scriptureSlides(
            { QVariantMap { { "text",
                                "Sample scripture text flows inside the text box you designed. "
                                "Longer passages continue on the following slides." },
                { "reference", "Sample 1:1" }, { "abbreviation", "Preview" } } },
            4, definition, &error);
    } else {
        QVariantMap section { { "key", "v1" }, { "label", "Verse 1" },
            { "primary",
                "We gather in Your light\nWe lift our hearts in praise\nWe share a living hope" } };
        if (definition["kind"] == "bilingual") {
            section["secondary"] = "నీ వెలుగులో కూడుదము\nస్తుతితో హృదయాలను పైకెత్తుదము\nజీవముగల ఆశను పంచుదము";
        }
        slides = TemplateComposer::songSlides(
            { { "title", "Together in hope" }, { "credits", "Original preview lyrics" },
                { "sections", QVariantList { section } },
                { "arrangement", QVariantList { "v1" } } },
            3, definition, &error);
    }
    return { { "slides", slides }, { "error", error },
        { "missingMedia", TemplateDefinition::missingMedia(definition) } };
}

bool TemplateStore::importTemplate(const QUrl &url) {
    QFile file(url.toLocalFile());
    if (!url.isLocalFile() || !file.open(QIODevice::ReadOnly) || file.size() > 5 * 1024 * 1024) {
        setStatus("Choose a local template file under 5 MB");
        return false;
    }
    const auto root = QJsonDocument::fromJson(file.readAll()).object();
    if (root["format"] != "churchdeck-template-1") {
        setStatus("Invalid Church Deck template format");
        return false;
    }
    auto definition = root["template"].toObject().toVariantMap();
    definition.remove("id");
    definition.remove("revision");
    return !persist(definition).isEmpty();
}

bool TemplateStore::exportTemplate(const QString &id, const QUrl &url) {
    const auto definition = layout(id);
    if (definition.isEmpty() || !url.isLocalFile()) {
        setStatus("Choose a template and a local export file");
        return false;
    }
    QSaveFile file(url.toLocalFile());
    const auto data = QJsonDocument(QJsonObject { { "format", "churchdeck-template-1" },
                                        { "template", QJsonObject::fromVariantMap(definition) } })
                          .toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        setStatus("Could not export template");
        return false;
    }
    setStatus("Template exported · media files remain local");
    return true;
}
