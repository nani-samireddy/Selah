#include "ScriptureImporter.h"
#include "BibleXmlReader.h"
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace {
QString importDevice(const QString &path, QIODevice &input, const QUrl &source) {
    QString parseError;
    auto prefix = input.peek(256).trimmed();
    if (prefix.startsWith("\xEF\xBB\xBF")) {
        prefix.remove(0, 3);
    }
    QJsonObject root;
    if (source.path().endsWith(".xml", Qt::CaseInsensitive) || prefix.trimmed().startsWith('<')) {
        root = BibleXmlReader::read(input, source, &parseError);
    } else {
        QJsonParseError jsonError;
        root = QJsonDocument::fromJson(input.readAll(), &jsonError).object();
        if (jsonError.error != QJsonParseError::NoError) {
            parseError = "Invalid Bible JSON: " + jsonError.errorString();
        }
    }
    if (!parseError.isEmpty()) {
        return parseError;
    }
    const auto metadata = root["translation"].toObject().toVariantMap();
    const auto rows = root["verses"].toArray();
    const QString id = metadata["id"].toString();
    if (root["format"] != "churchdeck-bible-1"
        || !QRegularExpression("^[a-z0-9_-]{1,48}$").match(id).hasMatch() || rows.isEmpty()
        || rows.size() > 100000) {
        return "Invalid Bible package format, ID, or verse count";
    }
    for (const auto &field :
        { "title", "abbreviation", "language", "copyright", "license", "source" }) {
        if (metadata.value(field).toString().trimmed().isEmpty()) {
            return "Bible metadata must include title, abbreviation, language, copyright, license, "
                   "and source";
        }
    }
    QString error;
    const auto connection = QUuid::createUuid().toString();
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
        db.setDatabaseName(path);
        db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
        if (!db.open()) {
            error = db.lastError().text();
        } else if (!db.transaction()) {
            error = "Could not begin Bible import: " + db.lastError().text();
        } else {
            QSqlQuery statement(db);
            statement.prepare("INSERT INTO translations VALUES(?,?,?,?,?,?,?)");
            for (const auto &field :
                { "id", "title", "abbreviation", "language", "copyright", "license", "source" }) {
                statement.addBindValue(metadata.value(field));
            }
            if (!statement.exec()) {
                error = "Translation ID already exists or could not be saved";
            }
            QMap<QString, QVariantMap> bookDefinitions;
            QSqlQuery known(db);
            known.exec("SELECT code,ordinal,name FROM books WHERE translation='engwebp'");
            while (known.next()) {
                bookDefinitions[known.value(0).toString()]
                    = { { "ordinal", known.value(1) }, { "name", known.value(2) } };
            }
            for (const auto &book : root["books"].toArray()) {
                const auto b = book.toObject();
                if (bookDefinitions.contains(b["code"].toString())
                    && !b["name"].toString().isEmpty()) {
                    bookDefinitions[b["code"].toString()]["name"] = b["name"].toString();
                }
            }
            QSet<QString> inserted;
            QSqlQuery insert(db);
            insert.prepare(
                "INSERT INTO verses(translation,book,chapter,verse,text) VALUES(?,?,?,?,?)");
            for (const auto &record : rows) {
                if (!error.isEmpty()) {
                    break;
                }
                const auto row = record.toObject();
                const auto book = row["book"].toString().toUpper();
                if (!bookDefinitions.contains(book) || !row["chapter"].isDouble()
                    || !row["verse"].isDouble()
                    || row["chapter"].toDouble() != row["chapter"].toInt()
                    || row["verse"].toDouble() != row["verse"].toInt() || row["chapter"].toInt() < 1
                    || row["chapter"].toInt() > 200 || row["verse"].toInt() < 0
                    || row["verse"].toInt() > 200 || row["text"].toString().trimmed().isEmpty()
                    || row["text"].toString().size() > 20000) {
                    error = "Invalid verse record; import rolled back";
                    break;
                }
                insert.bindValue(0, id);
                insert.bindValue(1, book);
                insert.bindValue(2, row["chapter"].toInt());
                insert.bindValue(3, row["verse"].toInt());
                insert.bindValue(4, row["text"].toString());
                if (!insert.exec()) {
                    error = "Duplicate verse or storage error; import rolled back";
                    break;
                }
                if (!inserted.contains(book)) {
                    QSqlQuery b(db);
                    b.prepare("INSERT INTO books VALUES(?,?,?,?)");
                    b.addBindValue(id);
                    b.addBindValue(book);
                    b.addBindValue(bookDefinitions[book]["ordinal"]);
                    b.addBindValue(bookDefinitions[book]["name"]);
                    if (!b.exec()) {
                        error = b.lastError().text();
                        break;
                    }
                    inserted.insert(book);
                }
            }
            if (error.isEmpty()) {
                QSqlQuery fts(db);
                fts.prepare("INSERT INTO verses_fts(rowid,text) SELECT rowid,text FROM verses "
                            "WHERE translation=?");
                fts.addBindValue(id);
                if (!fts.exec()) {
                    error = fts.lastError().text();
                }
            }
            if (error.isEmpty()) {
                if (!db.commit()) {
                    error = db.lastError().text();
                    db.rollback();
                }
            } else {
                db.rollback();
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connection);
    return error;
}

}

QString ScriptureImporter::importFile(const QString &path, const QUrl &source) {
    QFile file(source.toLocalFile());
    if (!source.isLocalFile() || !file.open(QIODevice::ReadOnly)
        || file.size() > 100 * 1024 * 1024) {
        return "Choose a local Bible JSON or XML file under 100 MB";
    }
    return importDevice(path, file, source);
}

QString ScriptureImporter::importData(
    const QString &path, const QByteArray &data, const QUrl &source) {
    if (data.isEmpty() || data.size() > 100 * 1024 * 1024) {
        return "Invalid Bible download size";
    }
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);
    return importDevice(path, buffer, source);
}
