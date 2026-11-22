#include "ContentLibrary.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include "SlideFactory.h"
#include "TemplateComposer.h"
#include "TemplateDefinition.h"
#include "TemplateTextLayout.h"
#include <QDebug>
#include <QTemporaryDir>

namespace {
QString boundText(const QVariantMap &slide, const QString &binding) {
    for (const auto &value : slide["elements"].toList()) {
        const auto box = value.toMap();
        if (box["binding"] == binding) {
            return box["text"].toString();
        }
    }
    return {};
}
}

int runDualScriptureChecks() {
    QTemporaryDir directory;
    ContentLibrary library(nullptr, directory.path());
    const auto primary = library.passage("engwebp", "JHN", 3, 16, 17);
    const auto paired = library.pairScripture(primary, "tel2017");
    const auto verses = paired["verses"].toList();
    if (!paired["error"].toString().isEmpty() || verses.size() != 2) {
        return 1;
    }
    for (const auto &value : verses) {
        const auto verse = value.toMap(), secondary = verse["secondary"].toMap();
        if (secondary["book"] != verse["book"] || secondary["chapter"] != verse["chapter"]
            || secondary["verse"] != verse["verse"] || secondary["translation"] != "tel2017"
            || secondary["text"].toString().isEmpty()) {
            return 2;
        }
    }
    if (library.pairScripture(primary, "missing")["error"].toString().isEmpty()
        || library.pairScripture(primary, "engwebp")["error"].toString().isEmpty()) {
        return 3;
    }
    QString error;
    const auto definition = TemplateDefinition::dualScripture();
    auto pages = SlideFactory::scriptureSlides(verses, 4, &error);
    if (!error.isEmpty() || pages.isEmpty()
        || boundText(pages.first().toMap(), "Secondary scripture text").isEmpty()
        || pages.first().toMap()["sourceSecondary"].toMap()["license"].toString().isEmpty()) {
        return 4;
    }
    if (!TemplateComposer::scriptureSlides(
            verses, 4, TemplateDefinition::starter("scripture"), &error)
            .isEmpty()
        || error.isEmpty()) {
        return 5;
    }
    auto mismatch = verses.first().toMap();
    auto second = mismatch["secondary"].toMap();
    second["verse"] = 99;
    mismatch["secondary"] = second;
    if (!TemplateComposer::scriptureSlides({ mismatch }, 4, definition, &error).isEmpty()) {
        return 6;
    }

    // Both texts must survive pagination, including Indic clusters and unequal page counts.
    auto longVerse = verses.first().toMap();
    second = longVerse["secondary"].toMap();
    const auto original = second["text"].toString().repeated(12);
    second["text"] = original;
    longVerse["text"] = "Short primary verse.";
    longVerse["secondary"] = second;
    pages = TemplateComposer::scriptureSlides({ longVerse }, 2, definition, &error);
    QString reconstructed;
    if (!error.isEmpty() || pages.size() < 2) {
        return 7;
    }
    for (const auto &page : pages) {
        const auto slide = page.toMap();
        if (boundText(slide, "Scripture text") != "Short primary verse.") {
            return 8;
        }
        reconstructed += boundText(slide, "Secondary scripture text");
        for (const auto &value : slide["elements"].toList()) {
            const auto box = value.toMap();
            if (box["binding"] == "Scripture text"
                || box["binding"] == "Secondary scripture text") {
                if (!TemplateTextLayout::fits(box["text"].toString(), box, 2)) {
                    return 9;
                }
            }
        }
    }
    if (reconstructed != original) {
        return 10;
    }

    OutputManager outputs;
    outputs.takeLive(pages, 0, "dual-bible-test");
    if (!outputs.liveText().contains(boundText(pages.first().toMap(), "Scripture text"))
        || !outputs.liveText().contains(
            boundText(pages.first().toMap(), "Secondary scripture text"))) {
        return 14;
    }
    ProgramStore programs(&outputs, nullptr, directory.path());
    if (!programs.createProgram("Dual Bible", "2026-11-22") || !programs.addScripture(verses, 4)
        || !programs.save()) {
        return 11;
    }
    const auto id = programs.id();
    const auto before = programs.items();
    if (programs.applyLayout(TemplateDefinition::starter("scripture"))
        || programs.items() != before) {
        return 12;
    }
    ProgramStore reopened(&outputs, nullptr, directory.path());
    if (!reopened.openProgram(id) || reopened.items() != before
        || reopened.selectedItem()["verses"].toList() != verses) {
        return 13;
    }
    qInfo()
        << "Dual scripture checks passed: canonical pairing, missing-reference rejection, "
           "two template bindings, unequal pagination, attribution and saved program snapshots.";
    return 0;
}
