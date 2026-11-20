#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramSources.h"
#include "ProgramStore.h"
#include "TemplateComposer.h"
#include "TemplateDefinition.h"
#include "TemplateStore.h"
#include "TemplateTextLayout.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>
#include <QTextBoundaryFinder>

namespace {
QVariantMap song() {
    return { { "title", "Together in hope" }, { "credits", "Original test lyrics" },
        { "sections",
            QVariantList { QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                { "primary", "One\nTwo\nThree\nFour" },
                { "secondary", "ఒకటి\nరెండు\nమూడు\nనాలుగు" } } } },
        { "arrangement", QVariantList { "v1", "v1" } },
        { "arrangements",
            QVariantList {
                QVariantMap { { "name", "Full" }, { "order", QVariantList { "v1", "v1" } } },
                QVariantMap { { "name", "Short" }, { "order", QVariantList { "v1" } } } } },
        { "activeArrangement", 0 } };
}

QString text(const QVariantMap &slide, const QString &binding) {
    for (const auto &value : slide["elements"].toList()) {
        const auto element = value.toMap();
        if (element["binding"] == binding) {
            return element["text"].toString();
        }
    }
    return {};
}

bool fitsAll(const QVariantList &pages, int maximumLines) {
    for (const auto &value : pages) {
        for (const auto &element : value.toMap()["elements"].toList()) {
            const auto box = element.toMap();
            const auto binding = box["binding"].toString();
            if (binding == "None" || binding.isEmpty()) {
                continue;
            }
            if (!TemplateTextLayout::fits(box["text"].toString(), box,
                    binding == "Primary lyrics" || binding == "Translated lyrics"
                            || binding == "Scripture text"
                        ? maximumLines
                        : 10000)) {
                return false;
            }
        }
    }
    return true;
}
}

int TemplateStore::selfTest() {
    auto check = [](bool condition, const char *name) {
        if (!condition) {
            qCritical() << "Template check failed:" << name;
        }
        return condition;
    };
    QTemporaryDir directory;
    TemplateStore library(nullptr, directory.path());
    if (!check(library.layouts().size() == 3, "Three starter layouts")) {
        return 1;
    }
    auto definition = library.layout("starter-bilingual");
    QString error;
    auto pages = TemplateComposer::songSlides(song(), 2, definition, &error);
    if (!check(error.isEmpty() && pages.size() == 4 && fitsAll(pages, 2),
            "Bilingual wrapping and arrangement order")) {
        return 2;
    }
    QStringList primary, secondary;
    QSet<QString> ids;
    for (const auto &page : pages) {
        primary << text(page.toMap(), "Primary lyrics");
        secondary << text(page.toMap(), "Translated lyrics");
        for (const auto &element : page.toMap()["elements"].toList()) {
            const auto id = element.toMap()["uid"].toString();
            if (!check(!ids.contains(id), "Fresh renderer identities")) {
                return 3;
            }
            ids.insert(id);
        }
    }
    if (!check(primary.join('\n') == "One\nTwo\nThree\nFour\nOne\nTwo\nThree\nFour"
                && secondary.join('\n').contains("నాలుగు"),
            "No loss in paired lyrics")) {
        return 4;
    }
    auto small = definition;
    auto slide = small["slide"].toMap();
    auto elements = slide["elements"].toList();
    for (auto &value : elements) {
        auto box = value.toMap();
        if (box["binding"] == "Primary lyrics") {
            box["width"] = 300;
            box["height"] = 150;
            box["fontSize"] = 100;
        }
        value = box;
    }
    auto decoration = elements.first().toMap();
    decoration["uid"] = "static-label";
    decoration["binding"] = "None";
    decoration["text"] = "Static label";
    elements << decoration;
    slide["elements"] = elements;
    small["slide"] = slide;
    auto longSong = song();
    auto section = longSong["sections"].toList()[0].toMap();
    section["primary"] = "We gather in hope and offer a song of thanks together";
    section["secondary"] = "ఆశతో కలిసి కృతజ్ఞతా గీతం పాడుదము";
    longSong["sections"] = QVariantList { section };
    longSong["arrangement"] = QVariantList { "v1" };
    pages = TemplateComposer::songSlides(longSong, 6, small, &error);
    QString joined;
    for (const auto &page : pages) {
        joined += text(page.toMap(), "Primary lyrics");
    }
    if (!check(error.isEmpty() && pages.size() > 1 && joined == section["primary"]
                && fitsAll(pages, 6)
                && pages[0].toMap()["elements"].toList().last().toMap()["text"] == "Static label",
            "Box dimensions drive pagination and preserve static layers")) {
        return 5;
    }
    auto bad = small;
    slide = bad["slide"].toMap();
    elements = slide["elements"].toList();
    auto box = elements[1].toMap();
    box["width"] = 40;
    box["height"] = 30;
    box["fontSize"] = 400;
    elements[1] = box;
    slide["elements"] = elements;
    bad["slide"] = slide;
    if (!check(TemplateComposer::songSlides(longSong, 6, bad, &error).isEmpty() && !error.isEmpty(),
            "Too-small boxes fail without clipping")) {
        return 6;
    }
    if (!check(TemplateComposer::songSlides(song(), 3, library.layout("starter-song"), &error)
                   .isEmpty(),
            "Reject lost translations")) {
        return 7;
    }
    const auto original = QString("ప్రభువు మనకు ఆశను ఇస్తాడు. ").repeated(40);
    const QVariantList verses { QVariantMap { { "text", original }, { "reference", "Test 1:1" },
        { "translation", "test" }, { "abbreviation", "TEST" }, { "copyright", "Test copyright" },
        { "license", "CC0" } } };
    pages
        = TemplateComposer::scriptureSlides(verses, 3, library.layout("starter-scripture"), &error);
    joined.clear();
    for (const auto &page : pages) {
        joined += text(page.toMap(), "Scripture text");
    }
    if (!check(error.isEmpty() && joined == original && pages.size() > 1 && fitsAll(pages, 3)
                && pages[0].toMap()["sourceLicense"] == "CC0",
            "Lossless shaped scripture and attribution")) {
        return 8;
    }
    const auto id
        = library.saveTemplate("My bilingual layout", "bilingual", definition["slide"].toMap());
    if (!check(!id.isEmpty() && library.layout(id)["revision"] == 1, "Save custom layout")) {
        return 9;
    }
    const auto snapshot = library.layout(id);
    library.saveTemplate("Updated layout", "bilingual", definition["slide"].toMap(), id);
    {
        TemplateStore reopened(nullptr, directory.path());
        if (!check(reopened.layout(id)["revision"] == 2 && reopened.layouts().size() == 4,
                "Revision and persistence")) {
            return 10;
        }
    }
    const auto file = QUrl::fromLocalFile(directory.filePath("test.churchtemplate"));
    if (!check(library.exportTemplate(id, file) && library.importTemplate(file)
                && library.layouts().size() == 5,
            "Independent import/export")) {
        return 11;
    }
    OutputManager outputs;
    ProgramStore programs(&outputs, nullptr, directory.path());
    programs.createProgram("Service", "");
    programs.addSong(song(), 3);
    programs.present(0);
    const auto live = outputs.liveElements();
    const auto before = programs.items();
    if (!check(programs.applyLayout(snapshot)
                && programs.selectedItem()["layoutTemplate"].toMap()["revision"] == 1
                && outputs.liveElements() == live,
            "Apply snapshot without changing live")) {
        return 12;
    }
    const auto applied = programs.items();
    programs.undo();
    if (!check(programs.items() == before, "Undo template application")) {
        return 13;
    }
    programs.redo();
    if (!check(programs.items() == applied, "Redo template application")) {
        return 14;
    }
    programs.save();
    const auto programId = programs.id();
    programs.openProgram(programId);
    if (!check(programs.selectedItem()["layoutTemplate"].toMap() == snapshot,
            "Program retains old template revision")) {
        return 15;
    }
    if (!check(programs.setArrangement(1)
                && programs.selectedItem()["layoutTemplate"].toMap() == snapshot
                && fitsAll(programs.selectedSlides(), 3),
            "Arrangement changes retain their layout snapshot")) {
        return 19;
    }
    programs.addScripture(verses, 3);
    if (!check(programs.applyLayout(library.layout("starter-scripture")),
            "Bible source snapshot supports reflow")) {
        return 16;
    }
    EditorDocument document(nullptr, false);
    if (!check(document.addSongSlides(song(), 2, snapshot), "Editor template insertion")) {
        return 17;
    }
    const auto legacyItem = programs.selectedItem();
    auto withoutSource = legacyItem;
    withoutSource.remove("verses");
    QString legacyError;
    const auto recovered = ProgramSources::scripture(withoutSource, &legacyError);
    QString recoveredText;
    for (const auto &verse : recovered) {
        recoveredText += verse.toMap()["text"].toString();
    }
    if (!check(legacyError.isEmpty()
                && recoveredText.simplified().remove(' ') == original.simplified().remove(' '),
            "Recover prepared legacy passages without dropping text")) {
        return 18;
    }
    qInfo() << "Template checks passed: persistence/revisions, type and fit validation, "
               "layout-driven bilingual/scripture pagination, static layers, source attribution, "
               "program snapshots and undo.";
    return 0;
}
