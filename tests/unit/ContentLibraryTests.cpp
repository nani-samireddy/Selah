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
    auto tagged = library.song(id);
    tagged["tags"] = " Worship, Holy Spirit, ఆరాధన, worship ;  ";
    if (library.saveSong(tagged) != id
        || library.parseSongTags(library.song(id)["tags"])
            != QStringList { "Worship", "Holy Spirit", "ఆరాధన" }
        || library.searchSongs("", "WORSHIP").size() != 1
        || library.searchSongs("పాడుదము", "ఆరాధన").size() != 1
        || !library.searchSongs("", "Holy").isEmpty()
        || !library.searchSongs("missing lyric", "Worship").isEmpty()
        || !library.songTags().contains("Holy Spirit")) {
        return 11;
    }
    // Structured imports and older comma-separated metadata share the same persistent format.
    tagged["tags"] = QVariantList { "Praise", "praise", "Holy Spirit" };
    if (library.saveSong(tagged) != id
        || library.parseSongTags(library.song(id)["tags"])
            != QStringList { "Praise", "Holy Spirit" }
        || !library.exportSong(id, file) || !library.importSong(file)
        || library.searchSongs("", "Praise").size() != 2) {
        return 12;
    }
    {
        ContentLibrary reopened(nullptr, directory.path());
        if (reopened.searchSongs("", "holy spirit").size() != 2
            || reopened.songTags().contains("Worship")) {
            return 13;
        }
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
    const auto quickId = library.createQuickSong("Quick Telugu song",
        "\r\n  మొదటి పంక్తి\r\nరెండవ పంక్తి\r\n \t\r\nChorus line\r\n\r\n\r\nFinal section\n",
        QVariantList { "Praise", "praise", "ఆరాధన" }, "Telugu");
    const auto quick = library.song(quickId);
    const auto quickSections = quick["sections"].toList();
    if (quickId.isEmpty() || quickSections.size() != 3
        || quickSections[0].toMap()["primary"] != "మొదటి పంక్తి\nరెండవ పంక్తి"
        || quickSections[1].toMap()["primary"] != "Chorus line"
        || quick["arrangement"].toList() != QVariantList { "s1", "s2", "s3" }
        || quick["primaryLanguage"] != "Telugu"
        || library.parseSongTags(quick["tags"]).size() != 2) {
        return 14;
    }
    const auto songCount = library.songs().size();
    if (!library.createQuickSong("Empty", " \n\n\t ", {}, "English").isEmpty()
        || library.songs().size() != songCount) {
        return 15;
    }
    qInfo() << "Content library checks passed: three offline translations, Unicode indexed search, "
               "songs, arrangements, and import/export.";
    return 0;
}
