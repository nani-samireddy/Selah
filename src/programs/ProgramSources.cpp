#include "ProgramSources.h"

QVariantList ProgramSources::scripture(const QVariantMap &item, QString *error) {
    if (!item["verses"].toList().isEmpty()) {
        return item["verses"].toList();
    }
    QVariantList verses;
    for (const auto &value : item["slides"].toList()) {
        const auto slide = value.toMap();
        const auto reference = slide["sourceReference"].toString(),
                   translation = slide["sourceId"].toString();
        QString body, version = translation;
        for (const auto &elementValue : slide["elements"].toList()) {
            const auto element = elementValue.toMap();
            if (element["binding"] == "Scripture text" && body.isEmpty()) {
                body = element["text"].toString();
            }
            if (element["binding"] == "Scripture reference") {
                const auto footer = element["text"].toString();
                const auto marker = reference + " · ";
                if (footer.startsWith(marker)) {
                    version = footer.mid(marker.size());
                }
            }
        }
        if (reference.isEmpty() || translation.isEmpty() || body.trimmed().isEmpty()) {
            *error = "This passage has no recoverable source text. Add it again from the Bible "
                     "library to choose a layout.";
            return {};
        }
        if (!verses.isEmpty() && verses.last().toMap()["reference"] == reference
            && verses.last().toMap()["translation"] == translation) {
            auto verse = verses.last().toMap();
            verse["text"] = verse["text"].toString() + '\n' + body;
            verses.last() = verse;
        } else {
            verses << QVariantMap { { "reference", reference }, { "translation", translation },
                { "text", body }, { "abbreviation", version },
                { "copyright", slide["sourceCopyright"] }, { "license", slide["sourceLicense"] } };
        }
    }
    return verses;
}
