#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include "SlideTransition.h"
#include <QDebug>
#include <QTemporaryDir>

int runTransitionChecks() {
    EditorDocument document(nullptr, false);
    if (document.defaultTransition() != SlideTransition::cut()
        || !document.setDefaultTransition("fade", 700)
        || !document.setSlideTransition("cut", 300)) {
        return 1;
    }
    document.addSlide();
    if (SlideTransition::resolve(document.slides().last().toMap())["durationMs"] != 700
        || !document.slideTransition().isEmpty()) {
        return 2;
    }
    if (document.setSlideTransition("fade", -1) || document.setDefaultTransition("zoom", 500)) {
        return 3;
    }
    QTemporaryDir directory;
    const auto file = QUrl::fromLocalFile(directory.filePath("transitions.churchdeck"));
    if (!document.saveProject(file)) {
        return 4;
    }
    EditorDocument reopened(nullptr, false);
    if (!reopened.openProject(file) || reopened.defaultTransition()["durationMs"] != 700) {
        return 5;
    }
    auto invalid = reopened.slides();
    auto page = invalid.first().toMap();
    page["transition"] = QVariantMap { { "type", "fade" }, { "durationMs", 0 } };
    invalid[0] = page;
    if (reopened.replaceSlides(invalid) || reopened.slides() != document.slides()) {
        return 6;
    }

    OutputManager outputs;
    ProgramStore programs(&outputs, nullptr, directory.path());
    if (!programs.createProgram("Transitions", "")
        || !programs.addPresentation(document.slides(), "Slides") || !programs.present()) {
        return 7;
    }
    const auto originalSnapshot = outputs.snapshotId();
    if (!programs.setTransition("fade", 500) || outputs.snapshotId() != originalSnapshot
        || SlideTransition::resolve(programs.selectedSlides().first().toMap())["durationMs"]
            != 500) {
        return 8;
    }
    programs.undo();
    if (SlideTransition::resolve(programs.selectedSlides().first().toMap())["type"] != "cut") {
        return 9;
    }
    programs.redo();
    const auto programId = programs.id();
    if (!programs.save() || !programs.openProgram(programId)
        || programs.selectedItem()["transition"].toMap()["durationMs"] != 500) {
        return 10;
    }
    if (!programs.setTransition("inherit", 300)
        || SlideTransition::resolve(programs.selectedSlides().first().toMap())["type"] != "cut") {
        return 11;
    }

    QVariantMap red { { "name", "Red" }, { "background", "#FF0000" }, { "ambientOpacity", 0 },
        { "elements", QVariantList {} } };
    auto blue = red;
    blue["name"] = "Blue";
    blue["background"] = "#0000FF";
    blue["transition"] = QVariantMap { { "type", "fade" }, { "durationMs", 500 } };
    outputs.takeLive({ red, blue, red }, 0);
    outputs.advance(1);
    if (!outputs.transitioning() || outputs.transitionProgress() != 0) {
        return 12;
    }
    outputs.advance(1);
    if (outputs.transitioning() || outputs.transitionProgress() != 1 || outputs.liveIndex() != 2) {
        return 13;
    }
    outputs.advance(-1);
    outputs.blackout("audience", true);
    if (outputs.transitioning() || !outputs.audienceBlack()) {
        return 14;
    }
    outputs.blackout("audience", false);
    outputs.advance(-1);
    outputs.advance(1);
    outputs.clearText(true);
    if (outputs.transitioning() || !outputs.textCleared()) {
        return 15;
    }
    outputs.clearText(false);
    outputs.advance(-1);
    outputs.advance(1);
    const auto snapshot = outputs.snapshotId();
    outputs.stopPresentation();
    if (outputs.transitioning() || outputs.playing() || outputs.snapshotId() != snapshot
        || outputs.liveIndex() != 1 || outputs.liveCount() != 3) {
        return 16;
    }
    for (const auto &type : SlideTransition::effects()) {
        if (!document.setDefaultTransition(type, 600) || !document.saveProject(file)
            || !reopened.openProject(file) || reopened.defaultTransition().value("type") != type) {
            return 20;
        }
        auto target = blue;
        target["transition"] = QVariantMap { { "type", type }, { "durationMs", 600 } };
        outputs.takeLive({ red, target }, 0);
        outputs.advance(1);
        if (!outputs.transitioning()
            || outputs.transitionEffect() != SlideTransition::effects().indexOf(type)) {
            return 21;
        }
        outputs.stopPresentation();
    }
    qInfo() << "Transition checks passed: defaults, overrides, validation, persistence, undo, "
               "protected snapshots and interrupted/safety cuts.";
    return 0;
}
