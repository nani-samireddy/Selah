#include "ContentLibrary.h"

QVariantMap ContentLibrary::pairScripture(
    const QVariantList &verses, const QString &secondaryTranslation) const {
    QVariantList paired;
    if (secondaryTranslation.isEmpty()) {
        return { { "verses", verses }, { "error", "" } };
    }
    if (verses.isEmpty() || verses.size() > 200) {
        return { { "error", "Choose between 1 and 200 verses" } };
    }

    // Match canonical coordinates, never translated book names or search-result positions.
    for (const auto &value : verses) {
        auto primary = value.toMap();
        if (primary["translation"] == secondaryTranslation) {
            return { { "error", "Choose two different Bible translations" } };
        }
        const auto matches = passage(secondaryTranslation, primary["book"].toString(),
            primary["chapter"].toInt(), primary["verse"].toInt(), primary["verse"].toInt());
        if (matches.size() != 1 || matches.first().toMap()["text"].toString().trimmed().isEmpty()) {
            return { { "error",
                "The second Bible does not contain " + primary["reference"].toString()
                    + ". Choose another translation or remove this verse." } };
        }
        primary["secondary"] = matches.first();
        paired << primary;
    }
    return { { "verses", paired }, { "error", "" } };
}
