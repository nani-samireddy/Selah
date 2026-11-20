#include "ScriptureImporter.h"
#include <QFile>
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

QString ScriptureImporter::importFile(const QString &path, const QUrl &url) {
    QFile file(url.toLocalFile());
    if (!url.isLocalFile() || !file.open(QIODevice::ReadOnly) || file.size() > 100 * 1024 * 1024) {
        return "Choose a local Bible JSON file under 100 MB";
    }
    const auto root = QJsonDocument::fromJson(file.readAll()).object();
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
        } else {
            db.transaction();
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
