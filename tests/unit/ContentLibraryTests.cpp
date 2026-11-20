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

int ContentLibrary::selfTest() {
    QTemporaryDir directory;
    ContentLibrary library(nullptr, directory.path());
    if (library.translations().size() != 3) {
        return 1;
    }
    if (library.books("engwebp").size() != 66 || library.books("tel2017").size() != 66) {
        return 2;
    }
    auto verse = library.passage("engwebp", "JHN", 3, 16, 16);
    if (verse.size() != 1 || !verse[0].toMap()["text"].toString().contains("God so loved")) {
        return 3;
    }
    if (library.searchVerses("engwebp", "God so loved").isEmpty()
        || library.searchVerses("tel2017", "దేవుడు").isEmpty()
        || library.searchVerses("hin2017", "परमेश्वर").isEmpty()) {
        return 4;
    }
    QVariantMap song { { "title", "Test gathering" }, { "primaryLanguage", "English" },
        { "secondaryLanguage", "తెలుగు" },
        { "sections",
            QVariantList { QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                { "primary", "We gather in hope\nWe sing together" },
                { "secondary", "ఆశతో కూడుదము\nకలిసి పాడుదము" } } } },
        { "arrangement", QVariantList { "v1", "v1" } } };
    auto id = library.saveSong(song);
    if (id.isEmpty() || library.searchSongs("పాడుదము").size() != 1) {
        return 5;
    }
    auto invalid = song;
    invalid["arrangement"] = QVariantList { "missing" };
    if (!library.saveSong(invalid).isEmpty() || library.songs().size() != 1) {
        return 6;
    }
    auto file = QUrl::fromLocalFile(directory.filePath("song.json"));
    if (!library.exportSong(id, file) || !library.importSong(file) || library.songs().size() != 2) {
        return 7;
    }
    if (library.searchVerses("engwebp", "John 3:16-18").size() != 3
        || !library.searchVerses("engwebp", "John 3:18-16").isEmpty()) {
        return 8;
    }
    const auto biblePath = directory.filePath("bible.json");
    auto writeBible = [&](const QJsonArray &rows) {
        QFile f(biblePath);
        if (!f.open(QIODevice::WriteOnly)) {
            return false;
        }
        f.write(QJsonDocument(
            QJsonObject { { "format", "churchdeck-bible-1" },
                { "translation",
                    QJsonObject { { "id", "test" }, { "title", "Test" }, { "abbreviation", "T" },
                        { "language", "English" }, { "copyright", "Original test" },
                        { "license", "CC0" }, { "source", "Local test" } } },
                { "verses", rows } })
                .toJson());
        return true;
    };
    const auto testVerse = QJsonObject { { "book", "JHN" }, { "chapter", 3 }, { "verse", 16 },
        { "text", "An original import test" } };
    if (!writeBible({ testVerse, testVerse })
        || ScriptureImporter::importFile(library.m_path, QUrl::fromLocalFile(biblePath)).isEmpty()
        || library.translations().size() != 3) {
        return 9;
    }
    if (!writeBible({ testVerse })
        || !ScriptureImporter::importFile(library.m_path, QUrl::fromLocalFile(biblePath)).isEmpty()
        || library.translations().size() != 4
        || library.searchVerses("test", "original import").size() != 1) {
        return 10;
    }
    qInfo() << "Content library checks passed: three offline translations, Unicode indexed search, "
               "songs, arrangements, and import/export.";
    return 0;
}
