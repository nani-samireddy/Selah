#include "BookMatcher.h"
#include "ContentLibrary.h"
#include <QDebug>
#include <QTemporaryDir>

int runBookMatcherChecks() {
    QTemporaryDir folder;
    ContentLibrary library(nullptr, folder.path());
    for (const auto &pair : QList<QPair<QString, QString>> { { "adikandam 1:1", "GEN 1:1" },
             { "aadikandamu 1:1", "GEN 1:1" }, { "ādikāṇḍam ౧:౧", "GEN 1:1" },
             { "yohanu 3:16", "JHN 3:16" }, { "యోహాను 3:16 చూపించు", "JHN 3:16" },
             { "utpatti 1:1", "GEN 1:1" }, { "उत्पत्ति १:१", "GEN 1:1" },
             { "yuhanna 3:16", "JHN 3:16" }, { "genses 1:1", "GEN 1:1" },
             { "show adikandam 1:1 చూపించు", "GEN 1:1" }, { "1 yohanu 3:16", "1JN 3:16" } }) {
        const auto resolved = library.resolveReference("tel2017", pair.first);
        if (resolved["query"] != pair.second) {
            qCritical() << "Book match failed" << pair.first << resolved;
            return 1;
        }
        if (library.searchVerses("tel2017", pair.first).isEmpty()) {
            qCritical() << "No saved verse" << pair.first;
            return 2;
        }
    }
    if (!library.resolveReference("tel2017", "show John")["handled"].toBool()
        || library.resolveReference("tel2017", "the mark of the beast")["handled"].toBool()) {
        return 7;
    }
    const auto exact = library.passage("tel2017", "GEN", 1, 1, 1);
    if (library.searchVerses("tel2017", "adikandam 1:1") != exact) {
        return 3;
    }
    for (const auto &input :
        { "John or James 3:16", "3:16", "adikandam 1:0", "adikandam 1:5-2", "John 3:16 and 4:1" }) {
        if (library.resolveReference("tel2017", input)["error"].toString().isEmpty()) {
            return 4;
        }
    }
    // Imported native names join the same index, independent of the target Bible language.
    auto names = library.books("engwebp");
    names.append(QVariantMap { { "code", "GEN" }, { "name", "ஆதியாகமம்" } });
    names.append(QVariantMap { { "code", "GEN" }, { "name", "Génesis" } });
    if (BookMatcher::resolve("Génesis 1:1", names, library.books("tel2017")).code != "GEN"
        || BookMatcher::resolve("ஆதியாகமம் 1:1", names, library.books("tel2017")).code != "GEN") {
        return 5;
    }
    // Tied spellings must expose choices rather than selecting a book by iteration order.
    const QVariantList ambiguous { QVariantMap { { "code", "AAA" }, { "name", "Calen" } },
        QVariantMap { { "code", "BBB" }, { "name", "Caren" } } };
    const auto match = BookMatcher::resolve("caten 1:1", ambiguous, ambiguous);
    if (match.error.isEmpty() || match.choices.size() != 2 || !match.query.isEmpty()) {
        return 6;
    }
    return 0;
}
