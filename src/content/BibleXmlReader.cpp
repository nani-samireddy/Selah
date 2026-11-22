#include "BibleXmlReader.h"
#include <QCryptographicHash>
#include <QFileInfo>
#include <QJsonArray>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QXmlStreamReader>

namespace {
const QStringList codes = QString(
    "GEN EXO LEV NUM DEU JOS JDG RUT 1SA 2SA 1KI 2KI 1CH 2CH EZR NEH EST JOB PSA PRO ECC SNG ISA "
    "JER LAM EZK DAN HOS JOL AMO OBA JON MIC NAM HAB ZEP HAG ZEC MAL MAT MRK LUK JHN ACT ROM 1CO "
    "2CO GAL EPH PHP COL 1TH 2TH 1TI 2TI TIT PHM HEB JAS 1PE 2PE 1JN 2JN 3JN JUD REV")
                              .split(' ');

// Both source formats use the standard 1..66 book numbering, including NT-only files.
class Reader {
public:
    explicit Reader(QIODevice &input)
        : xml(&input) { }

    QXmlStreamReader xml;
    QJsonArray books, verses;
    QMap<QString, QString> metadata;
    QSet<int> bookNumbers;
    bool zefania = false;

    int number(const QString &name, int low, int high) {
        bool ok = false;
        const int value = xml.attributes().value(name).toInt(&ok);
        if (!ok || value < low || value > high) {
            xml.raiseError("Invalid " + name + " (expected " + QString::number(low) + ".."
                + QString::number(high) + ")");
        }
        return value;
    }

    void book() {
        const int ordinal = number(zefania ? "bnumber" : "number", 1, codes.size());
        if (xml.hasError()) {
            return;
        }
        if (bookNumbers.contains(ordinal)) {
            xml.raiseError("Duplicate Bible book");
            return;
        }
        bookNumbers.insert(ordinal);
        books.append(QJsonObject { { "code", codes[ordinal - 1] },
            { "name", xml.attributes().value(zefania ? "bname" : "name").toString() } });
        QSet<int> chapters;
        while (!xml.hasError() && xml.readNextStartElement()) {
            if (xml.name() != (zefania ? QLatin1String("CHAPTER") : QLatin1String("chapter"))) {
                xml.raiseError("Expected a chapter inside a book");
                break;
            }
            const int chapter = number(zefania ? "cnumber" : "number", 1, 200);
            if (chapters.contains(chapter)) {
                xml.raiseError("Duplicate chapter");
                break;
            }
            chapters.insert(chapter);
            while (!xml.hasError() && xml.readNextStartElement()) {
                if (xml.name() != (zefania ? QLatin1String("VERS") : QLatin1String("verse"))) {
                    xml.raiseError("Expected a verse inside a chapter");
                    break;
                }
                const int verse = number(zefania ? "vnumber" : "number", 0, 200);
                const auto text
                    = xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed();
                if (xml.hasError()) {
                    break;
                }
                if (text.size() > 20000 || verses.size() >= 100000) {
                    xml.raiseError("Verse text or count exceeds import limits");
                    break;
                }
                // Some editions deliberately leave an omitted verse empty. Keep numbering gaps.
                if (!text.isEmpty()) {
                    verses.append(QJsonObject { { "book", codes[ordinal - 1] },
                        { "chapter", chapter }, { "verse", verse }, { "text", text } });
                }
            }
        }
    }

    void parse() {
        while (!xml.atEnd() && !xml.isStartElement()) {
            xml.readNext();
            if (xml.isDTD()) {
                xml.raiseError("DTD declarations are not supported");
            }
        }
        zefania = xml.name() == QLatin1String("XMLBIBLE");
        if (!zefania && xml.name() != QLatin1String("bible")) {
            xml.raiseError("Expected a bible or XMLBIBLE document");
            return;
        }
        metadata["title"]
            = xml.attributes().value(zefania ? "biblename" : "translation").toString();
        metadata["info"] = xml.attributes().value("info").toString();
        metadata["link"] = xml.attributes().value("link").toString();
        while (!xml.hasError() && xml.readNextStartElement()) {
            if (zefania && xml.name() == QLatin1String("INFORMATION")) {
                while (xml.readNextStartElement()) {
                    const auto field = xml.name().toString().toLower();
                    metadata[field]
                        = xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed();
                }
            } else if (zefania && xml.name() == QLatin1String("BIBLEBOOK")) {
                book();
            } else if (!zefania && xml.name() == QLatin1String("testament")) {
                while (!xml.hasError() && xml.readNextStartElement()) {
                    if (xml.name() != QLatin1String("book")) {
                        xml.raiseError("Expected a book inside a testament");
                        break;
                    }
                    book();
                }
            } else {
                xml.raiseError("Unsupported Bible XML structure");
            }
        }
        // Reject malformed trailing content as well as malformed verses.
        while (!xml.atEnd()) {
            xml.readNext();
        }
    }

    QString value(const QString &key, const QString &fallback) const {
        const auto text = metadata.value(key).trimmed();
        return text.isEmpty() ? fallback : text;
    }
};
}

QJsonObject BibleXmlReader::read(QIODevice &input, const QUrl &source, QString *error) {
    Reader reader(input);
    reader.parse();
    if (reader.xml.hasError() || reader.verses.isEmpty()) {
        *error = QString("Bible XML error at line %1: %2")
                     .arg(reader.xml.lineNumber())
                     .arg(reader.xml.hasError() ? reader.xml.errorString() : "No verses found");
        return {};
    }
    const auto filename = QFileInfo(source.path()).completeBaseName();
    const auto title = reader.value("title", filename.isEmpty() ? "Imported Bible" : filename);
    auto slug = filename.toLower();
    slug.replace(QRegularExpression("[^a-z0-9]+"), "-");
    const auto digest
        = QCryptographicHash::hash(filename.toUtf8(), QCryptographicHash::Sha256).toHex().left(8);
    const auto id = "xml-" + slug.left(31) + "-" + QString::fromLatin1(digest);
    auto abbreviation = reader.value("identifier", "");
    if (abbreviation.isEmpty()) {
        // Keep the version label readable: e.g. Telugu (BSI) 1880 instead of opaque initials.
        abbreviation = title.size() <= 32 ? title : title.left(29) + "…";
    }
    const auto nameLanguage = QRegularExpression("^[A-Z][a-z]+").match(filename).captured();
    const auto language
        = reader.value("language", nameLanguage.isEmpty() ? "Unspecified" : nameLanguage);
    auto origin = source.isLocalFile()
        ? "Local XML import: " + QFileInfo(source.toLocalFile()).fileName()
        : source.toString();
    if (!reader.value("link", "").isEmpty()) {
        origin += "\nOriginal source: " + reader.value("link", "");
    }
    const auto info = reader.value("info", "Copyright information not provided in XML source");
    const auto rights = reader.value("rights", reader.value("copyright", info));
    const auto license = reader.value("license",
        info.contains("license", Qt::CaseInsensitive) ? info : "Not specified in XML source");
    return { { "format", "churchdeck-bible-1" }, { "books", reader.books },
        { "verses", reader.verses },
        { "translation",
            QJsonObject { { "id", id }, { "title", title }, { "abbreviation", abbreviation },
                { "language", language }, { "copyright", rights }, { "license", license },
                { "source", origin } } } };
}
