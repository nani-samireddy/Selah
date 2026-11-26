#include "DocumentElements.h"
#include "EditorDocument.h"
#include "TemplateComposer.h"
#include "TemplateDefinition.h"
#include "TemplateTextLayout.h"
#include "TextFormatting.h"
#include "TextLayoutMetrics.h"
#include <QDebug>
#include <QTemporaryDir>

int runTextLayoutChecks() {
    TextFormatting formatting;
    auto authored = DocumentElements::text("Fit",
        QString("God gives us hope and strength through every season. ").repeated(2), 100, 100, 700,
        200, 96, "Arial");
    authored["textSizing"] = "shrink";
    authored["minFontSize"] = 32;
    auto rich = formatting.apply(
        authored, { { "fontWeight", 700 }, { "color", "#FF3344" }, { "fontSize", 120 } }, 0, 3);
    const auto fitted = formatting.display(rich);
    const auto metrics = TextLayoutMetrics::measure(fitted["text"].toString(), fitted, 692);
    if (fitted["text"] != rich["text"] || fitted["fontSize"].toInt() >= 96
        || fitted["fontSize"].toInt() < 32 || metrics.height > 192 || metrics.width > 692.5
        || rich["fontSize"].toInt() != 96
        || formatting.styleAt(fitted, 0, 3)["fontWeight"].toInt() != 700
        || formatting.styleAt(fitted, 0, 3)["color"].toString() != "#ffff3344") {
        return 1;
    }
    auto blocked = rich;
    blocked["minFontSize"] = 96;
    blocked["maxLines"] = 1;
    const auto overflow = formatting.display(blocked);
    if (!overflow["_fitOverflow"].toBool() || overflow["text"] != rich["text"]
        || overflow["fontSize"].toInt() != 96) {
        return 2;
    }
    auto shortText = authored;
    auto fractional = blocked;
    fractional["fontSize"] = 96.5;
    fractional["minFontSize"] = 96.5;
    if (formatting.display(fractional)["fontSize"].toDouble() > 96.5) {
        return 10;
    }
    shortText["text"] = "Hope";
    if (formatting.display(shortText)["fontSize"].toInt() != 96) {
        return 3;
    }

    auto definition = TemplateDefinition::starter("scripture");
    auto slide = definition["slide"].toMap();
    auto elements = slide["elements"].toList();
    for (auto &value : elements) {
        auto box = value.toMap();
        if (box["binding"] == "Scripture text") {
            box["width"] = 600;
            box["height"] = 150;
            box["fontSize"] = 48;
            box["textSizing"] = "fixed";
        }
        value = box;
    }
    slide["elements"] = elements;
    definition["slide"] = slide;
    QString error;
    for (const auto &source :
        QStringList { "God is our refuge. He gives us strength when we are weary.",
            "The Lord is my shepherd; I shall not want; he restores my soul.",
            "ప్రభువు మనకు ఆశను ఇస్తాడు. ప్రభువు మనకు ఆశను ఇస్తాడు. ప్రభువు మనకు ఆశను ఇస్తాడు." }) {
        const auto pages = TemplateComposer::scriptureSlides(
            { QVariantMap { { "text", source }, { "reference", "Test 1:1" },
                { "translation", "test" }, { "abbreviation", "TEST" } } },
            2, definition, &error);
        QString restored;
        if (!error.isEmpty() || pages.size() < 2) {
            qWarning() << error << source;
            return 4;
        }
        for (int i = 0; i < pages.size(); ++i) {
            for (const auto &value : pages[i].toMap()["elements"].toList()) {
                const auto box = value.toMap();
                if (box["binding"] != "Scripture text") {
                    continue;
                }
                const auto text = box["text"].toString();
                if (!TemplateTextLayout::fits(text, box, 2)) {
                    return 5;
                }
                if (i + 1 < pages.size() && !text.trimmed().endsWith('.')
                    && !text.trimmed().endsWith(';')) {
                    return 6;
                }
                restored += text;
            }
        }
        if (restored != source) {
            return 7;
        }
    }
    // Without a complete phrase that fits, fail explicitly rather than silently slicing a word.
    QString remaining = QString("unpunctuated ").repeated(30);
    const auto original = remaining;
    if (!TemplateTextLayout::takePrefix(remaining, "Scripture text", definition, 2).isEmpty()
        || remaining != original) {
        return 8;
    }
    remaining = QString("1,000 ").repeated(30);
    const auto numeric = remaining;
    if (!TemplateTextLayout::takePrefix(remaining, "Scripture text", definition, 2).isEmpty()
        || remaining != numeric) {
        return 9;
    }
    return 0;
}
