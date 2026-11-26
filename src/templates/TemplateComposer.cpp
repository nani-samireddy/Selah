#include "TemplateComposer.h"
#include "TemplateDefinition.h"
#include "TemplateTextLayout.h"
#include "TextFormatting.h"
#include <algorithm>
#include <QMap>
#include <QSet>
#include <QUuid>

namespace {
QVariantMap populate(const QVariantMap &definition, const QVariantMap &context, const QString &name,
    QString *error, int maximumLines) {
    auto slide = definition["slide"].toMap();
    auto elements = slide["elements"].toList();
    for (auto &value : elements) {
        auto box = value.toMap();
        box["uid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const auto binding = box.value("binding", "None").toString();
        if (!binding.isEmpty() && binding != "None") {
            const auto text = context.value(binding).toString();
            if (text.size() > 30000) {
                *error = binding + " exceeds the template text limit";
                return {};
            }
            box["text"] = text;
            // Placeholder character ranges do not describe newly bound content.
            box.remove("richText");
            box.remove("textRuns");
            const bool body = binding == "Primary lyrics" || binding == "Translated lyrics"
                || binding == "Scripture text" || binding == "Secondary scripture text";
            if (body && box["textSizing"] == "shrink") {
                box = TextFormatting::fitted(box, maximumLines);
            }
            // Metadata keeps its existing fit behavior; body text respects its selected mode.
            if (!body && box.value("visible", true).toBool()) {
                while (!TemplateTextLayout::fits(text, box) && box["fontSize"].toInt() > 8) {
                    box["fontSize"] = box["fontSize"].toInt() - 1;
                }
                if (!TemplateTextLayout::fits(text, box)) {
                    *error = "Enlarge the " + binding + " box so its text fits";
                    return {};
                }
            }
        }
        value = box;
    }
    slide["elements"] = elements;
    slide["name"] = name;
    slide["layoutTemplateId"] = definition.value("id", "").toString();
    slide["layoutTemplateName"] = definition["name"];
    slide["layoutTemplateRevision"] = definition.value("revision", 1).toInt();
    return slide;
}

bool appendPage(QVariantList &result, QVariantMap page, QString *error) {
    if (page.isEmpty()) {
        return false;
    }
    if (result.size() >= 500) {
        *error = "This content would exceed 500 slides";
        return false;
    }
    result << page;
    return true;
}
}

QVariantList TemplateComposer::songSlides(
    const QVariantMap &song, int maximumLines, const QVariantMap &definition, QString *error) {
    error->clear();
    const auto sectionList = song["sections"].toList();
    if (sectionList.isEmpty() || sectionList.size() > 100) {
        *error = "Choose a song with 1 to 100 sections";
        return {};
    }
    QMap<QString, QVariantMap> sections;
    bool bilingual = false;
    QSet<QString> keys;
    for (const auto &value : sectionList) {
        const auto section = value.toMap();
        const auto primary = section["primary"].toString(),
                   secondary = section["secondary"].toString();
        if (primary.trimmed().isEmpty() || primary.size() > 30000 || secondary.size() > 30000) {
            *error = "Each section needs primary lyrics of up to 30,000 characters";
            return {};
        }
        const auto key = section["key"].toString();
        if (key.isEmpty() || keys.contains(key)) {
            *error = "Song sections need unique, non-empty keys";
            return {};
        }
        keys.insert(key);
        sections[key] = section;
        bilingual |= !secondary.trimmed().isEmpty();
    }
    if (!TemplateDefinition::supports(definition, "song", bilingual, error)) {
        return {};
    }
    auto order = song["arrangement"].toList();
    if (order.isEmpty()) {
        for (const auto &value : sectionList) {
            order << value.toMap()["key"];
        }
    }
    if (order.size() > 300) {
        *error = "The arrangement is too long";
        return {};
    }
    maximumLines = qBound(1, maximumLines, 6);
    QVariantList result;
    for (const auto &key : order) {
        if (!sections.contains(key.toString())) {
            *error = "Arrangement contains an unknown section";
            return {};
        }
        const auto section = sections[key.toString()];
        const auto primaryRows = section["primary"].toString().split('\n');
        const auto secondaryRows = section["secondary"].toString().split('\n');
        const bool paired = !section["secondary"].toString().trimmed().isEmpty();
        if (paired && primaryRows.size() != secondaryRows.size()) {
            *error = "Align both languages to the same number of rows";
            return {};
        }
        QString primaryPage, secondaryPage;
        int part = 0;
        auto flush = [&]() {
            if (primaryPage.isEmpty() && secondaryPage.isEmpty()) {
                return true;
            }
            const auto name = song["title"].toString() + " · " + section["label"].toString() + " · "
                + QString::number(++part);
            const QVariantMap context { { "Title", song["title"] },
                { "Section name", section["label"] }, { "Primary lyrics", primaryPage },
                { "Translated lyrics", secondaryPage }, { "Credits", song["credits"] } };
            auto page = populate(definition, context, name, error, maximumLines);
            if (page.isEmpty()) {
                return false;
            }
            page["sourceKind"] = "song";
            page["sourceId"] = song["id"];
            page["sourceSection"] = key;
            page["sourceCredits"] = song["credits"];
            const bool appended = appendPage(result, page, error);
            primaryPage.clear();
            secondaryPage.clear();
            return appended;
        };
        for (int row = 0; row < primaryRows.size(); ++row) {
            const auto primary = primaryRows[row],
                       secondary = paired ? secondaryRows[row] : QString();
            if (primary.trimmed().isEmpty() && secondary.trimmed().isEmpty()) {
                if (!flush()) {
                    return {};
                }
                continue;
            }
            const auto candidatePrimary
                = primaryPage.isEmpty() ? primary : primaryPage + '\n' + primary;
            const auto candidateSecondary
                = secondaryPage.isEmpty() ? secondary : secondaryPage + '\n' + secondary;
            if (TemplateTextLayout::fitsBinding(
                    candidatePrimary, "Primary lyrics", definition, maximumLines)
                && TemplateTextLayout::fitsBinding(
                    candidateSecondary, "Translated lyrics", definition, maximumLines)) {
                primaryPage = candidatePrimary;
                secondaryPage = candidateSecondary;
                continue;
            }
            if (!flush()) {
                return {};
            }
            QString remainingPrimary = primary, remainingSecondary = secondary;
            while (!remainingPrimary.isEmpty() || !remainingSecondary.isEmpty()) {
                const bool primaryPending = !remainingPrimary.isEmpty(),
                           secondaryPending = !remainingSecondary.isEmpty();
                primaryPage = TemplateTextLayout::takePrefix(
                    remainingPrimary, "Primary lyrics", definition, maximumLines);
                secondaryPage = TemplateTextLayout::takePrefix(
                    remainingSecondary, "Translated lyrics", definition, maximumLines);
                if ((primaryPending && primaryPage.isEmpty())
                    || (secondaryPending && secondaryPage.isEmpty())) {
                    *error = "Enlarge the lyric boxes or reduce their font sizes; a text cluster "
                             "cannot fit";
                    return {};
                }
                if (!remainingPrimary.isEmpty() || !remainingSecondary.isEmpty()) {
                    if (!flush()) {
                        return {};
                    }
                }
            }
        }
        if (!flush()) {
            return {};
        }
    }
    return result;
}

QVariantList TemplateComposer::scriptureSlides(
    const QVariantList &verses, int maximumLines, const QVariantMap &definition, QString *error) {
    error->clear();
    if (!TemplateDefinition::supports(definition, "scripture", false, error)) {
        return {};
    }
    if (verses.isEmpty() || verses.size() > 200) {
        *error = "Choose between 1 and 200 verses";
        return {};
    }
    const bool dual = std::any_of(verses.cbegin(), verses.cend(),
        [](const QVariant &value) { return !value.toMap()["secondary"].toMap().isEmpty(); });
    if (dual && !TemplateDefinition::hasVisibleBinding(definition, "Secondary scripture text")) {
        *error = "Choose a dual Bible template with a visible Secondary scripture text box";
        return {};
    }
    maximumLines = qBound(1, maximumLines, 6);
    QVariantList result;
    for (const auto &value : verses) {
        const auto verse = value.toMap();
        QString remaining = verse["text"].toString();
        if (remaining.trimmed().isEmpty() || remaining.size() > 30000) {
            *error = "Choose a verse with up to 30,000 characters";
            return {};
        }
        const auto secondary = verse["secondary"].toMap();
        QString secondRemaining = secondary["text"].toString();
        if (dual && (secondRemaining.trimmed().isEmpty() || secondRemaining.size() > 30000)) {
            *error = "Every selected verse needs text from the second Bible";
            return {};
        }
        if (dual && verse.contains("book")
            && (secondary["book"] != verse["book"] || secondary["chapter"] != verse["chapter"]
                || secondary["verse"] != verse["verse"]
                || secondary["translation"] == verse["translation"])) {
            *error
                = "Both Bibles must use the same book, chapter and verse in different translations";
            return {};
        }
        QStringList chunks, secondChunks;
        while (!remaining.isEmpty()) {
            const auto prefix = TemplateTextLayout::takePrefix(
                remaining, "Scripture text", definition, maximumLines);
            if (prefix.isEmpty()) {
                *error
                    = "A complete scripture phrase cannot fit. Enlarge the Scripture text box, "
                      "allow more lines, or enable Shrink to fit with a lower minimum font size.";
                return {};
            }
            chunks << prefix;
            if (chunks.size() + result.size() > 500) {
                *error = "This passage would exceed 500 slides";
                return {};
            }
        }
        while (dual && !secondRemaining.isEmpty()) {
            const auto prefix = TemplateTextLayout::takePrefix(
                secondRemaining, "Secondary scripture text", definition, maximumLines);
            if (prefix.isEmpty()) {
                *error = "A complete phrase cannot fit in the second Bible box. Enlarge it, allow "
                         "more lines, or lower its Shrink to fit minimum font size.";
                return {};
            }
            secondChunks << prefix;
            if (secondChunks.size() + result.size() > 500) {
                *error = "This passage would exceed 500 slides";
                return {};
            }
        }
        const int count = qMax(chunks.size(), secondChunks.size());
        for (int i = 0; i < count; ++i) {
            const auto reference = verse["reference"].toString();
            const auto name = reference
                + (count > 1 ? " · " + QString::number(i + 1) + "/" + QString::number(count) : "");
            // Keep the shorter translation visible while the longer one continues.
            const QVariantMap context { { "Title", reference },
                { "Scripture text", chunks[qMin(i, int(chunks.size()) - 1)] },
                { "Secondary scripture text",
                    dual ? secondChunks[qMin(i, int(secondChunks.size()) - 1)] : QString() },
                { "Secondary scripture reference", secondary["reference"] },
                { "Secondary version", secondary["abbreviation"] },
                { "Secondary credits", secondary["copyright"] },
                { "Scripture reference", reference }, { "Version", verse["abbreviation"] },
                { "Credits", verse["copyright"] } };
            auto page = populate(definition, context, name, error, maximumLines);
            if (page.isEmpty()) {
                return {};
            }
            page["sourceKind"] = "scripture";
            page["sourceId"] = verse["translation"];
            page["sourceReference"] = reference;
            page["sourceCopyright"] = verse["copyright"];
            page["sourceLicense"] = verse["license"];
            if (dual) {
                page["sourceSecondary"] = secondary;
            }
            if (!appendPage(result, page, error)) {
                return {};
            }
        }
    }
    return result;
}
