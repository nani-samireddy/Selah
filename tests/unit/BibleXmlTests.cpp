#include "BibleRepository.h"
#include "BibleXmlReader.h"
#include "ContentLibrary.h"
#include "ScriptureImporter.h"
#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

namespace {
QJsonObject parse(const QByteArray &xml, const QUrl &source, QString *error) {
    QBuffer buffer;
    buffer.setData(xml);
    buffer.open(QIODevice::ReadOnly);
    return BibleXmlReader::read(buffer, source, error);
}
}

int runBibleXmlChecks(const QString &realFile = QString()) {
    QTemporaryDir directory;
    ContentLibrary library(nullptr, directory.path());
    const auto path = directory.filePath("content-library.sqlite");
    const QUrl source("https://raw.githubusercontent.com/nani-samireddy/Holy-Bible-XML-Format/"
                      "master/TeluguBible.xml");
    const QByteArray fixture = QString(
        "<bible translation=\"Telugu test\" info=\"Original test fixture\" "
        "link=\"https://example.org/source\"><testament name=\"Old\"><book number=\"1\"><chapter "
        "number=\"1\"><verse number=\"1\">పరీక్ష కొరకు తెలుగు వాక్యము</verse><verse "
        "number=\"2\"></verse><verse number=\"3\">Second <style>test</style> "
        "line</verse></chapter></book></testament><testament name=\"New\"><book "
        "number=\"40\"><chapter number=\"1\"><verse number=\"1\">Original next testament "
        "text</verse></chapter></book></testament></bible>")
                                   .toUtf8();
    QString error;
    const auto package = parse(fixture, source, &error);
    const auto metadata = package["translation"].toObject();
    const auto id = metadata["id"].toString();
    if (!error.isEmpty() || package["verses"].toArray().size() != 3
        || metadata["title"] != "Telugu test" || metadata["language"] != "Telugu"
        || metadata["copyright"] != "Original test fixture"
        || metadata["license"] != "Not specified in XML source"
        || !metadata["source"].toString().contains(source.toString())) {
        return 1;
    }
    if (!ScriptureImporter::importData(path, fixture, source).isEmpty()
        || library.books(id).size() != 2 || library.chapters(id, "GEN").size() != 1
        || library.passage(id, "GEN", 1, 1, 3).size() != 2
        || library.searchVerses(id, "పరీక్ష").size() != 1
        || library.passage(id, "MAT", 1, 1, 1).size() != 1
        || library.passage(id, "GEN", 1, 3, 3)[0].toMap()["text"] != "Second test line") {
        return 2;
    }
    if (ScriptureImporter::importData(path, fixture, source).isEmpty()
        || library.translations().size() != 4) {
        return 3;
    }
    const QByteArray zefania
        = "\xEF\xBB\xBF<?xml version=\"1.0\"?><XMLBIBLE><BIBLEBOOK bnumber=\"66\" "
          "bname=\"Revelation\"><CHAPTER cnumber=\"22\"><VERS vnumber=\"21\">Original Zefania "
          "fixture</VERS></CHAPTER></BIBLEBOOK></XMLBIBLE>";
    if (!ScriptureImporter::importData(path, zefania, QUrl("file:///Zefania.xml")).isEmpty()) {
        return 4;
    }
    const QList<QByteArray> invalid { "<bible><testament><book number=\"67\"/></testament></bible>",
        "<bible><testament><book number=\"1\"><chapter number=\"0\"/></book></testament></bible>",
        "<bible><testament><book number=\"1\"><chapter number=\"1\"><verse "
        "number=\"1\">One</verse><verse "
        "number=\"1\">Two</verse></chapter></book></testament></bible>",
        "<bible><testament><book number=\"1\"><chapter number=\"1\"><verse "
        "number=\"1\">First</verse></chapter><chapter number=\"1\"/></book></testament></bible>",
        "<bible><testament><book number=\"1\"><chapter number=\"1\"><verse "
        "number=\"1\">First</verse></chapter></book></testament></bible>broken",
        "<!DOCTYPE bible [<!ENTITY x 'Entity'>]><bible><testament><book number=\"1\"><chapter "
        "number=\"1\"><verse number=\"1\">&x;</verse></chapter></book></testament></bible>" };
    const auto before = library.translations();
    int index = 0;
    for (const auto &bad : invalid) {
        const auto url = QUrl("file:///Invalid" + QString::number(index++) + ".xml");
        if (ScriptureImporter::importData(path, bad, url).isEmpty()
            || library.translations() != before) {
            return 5;
        }
    }
    const auto raw = BibleRepository::rawUrl(QUrl(
        "https://github.com/nani-samireddy/Holy-Bible-XML-Format/blob/master/TeluguBible.xml"));
    if (raw != source || !BibleRepository::rawUrl(QUrl("https://example.org/Test.xml")).isEmpty()
        || !BibleRepository::rawUrl(QUrl("http://raw.githubusercontent.com/nani-samireddy/"
                                         "Holy-Bible-XML-Format/master/TeluguBible.xml"))
            .isEmpty()
        || library.repositoryBibles().size() != 1048) {
        return 6;
    }
    library.importBible(QUrl("https://example.org/Test.xml"));
    if (library.busy() || !library.bibleImportPhase().isEmpty()
        || library.translations() != before) {
        return 10;
    }
    if (!realFile.isEmpty()) {
        QFile actual(realFile);
        if (!actual.open(QIODevice::ReadOnly)) {
            return 7;
        }
        const auto bytes = actual.readAll();
        error.clear();
        const auto expected = parse(bytes, source, &error);
        QTemporaryDir actualDirectory;
        ContentLibrary actualLibrary(nullptr, actualDirectory.path());
        if (!error.isEmpty() || expected["books"].toArray().size() != 66
            || expected["verses"].toArray().size() != 31100
            || !ScriptureImporter::importData(
                actualDirectory.filePath("content-library.sqlite"), bytes, source)
                .isEmpty()) {
            return 8;
        }
        const auto realId = expected["translation"].toObject()["id"].toString();
        const auto first = actualLibrary.passage(realId, "GEN", 1, 1, 1);
        if (actualLibrary.books(realId).size() != 66 || first.size() != 1
            || first[0].toMap()["text"]
                != expected["verses"].toArray()[0].toObject()["text"].toString()
            || actualLibrary.passage(realId, "REV", 22, 21, 21).size() != 1
            || actualLibrary.searchVerses(realId, "దేవుడు").isEmpty()) {
            return 9;
        }
        qInfo() << "Full repository Telugu Bible passed: 66 books, 31,100 nonempty verses, indexed "
                   "Unicode search and final passage.";
    }
    qInfo() << "Bible XML checks passed: Beblia/Zefania, metadata, NT numbering, omitted verses, "
               "duplicates, rollback, malformed XML and repository URLs.";
    return 0;
}
