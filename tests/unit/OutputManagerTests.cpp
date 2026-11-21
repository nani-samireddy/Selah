#include "OutputManager.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QGuiApplication>
#include <QSettings>

int OutputManager::selfTest() {
    OutputManager outputs;
    QVariantList slides { QVariantMap { { "name", "First" }, { "notes", "Private notes" },
                              { "elements",
                                  QVariantList { QVariantMap { { "kind", "text" },
                                      { "text", "Live text" }, { "fontSize", 90 } } } } },
        QVariantMap { { "name", "Next" }, { "elements", QVariantList {} } } };
    int snapshotChanges = 0;
    QObject::connect(
        &outputs, &OutputManager::snapshotChanged, &outputs, [&] { ++snapshotChanges; });
    outputs.takeLive(slides, 0);
    slides[0] = QVariantMap { { "name", "Edited draft" } };
    if (outputs.liveTitle() != "First" || outputs.liveText() != "Live text"
        || outputs.nextTitle() != "Next" || outputs.liveNotes() != "Private notes") {
        return 1;
    }
    const auto order = outputs.liveOrder();
    if (order.size() != 2 || order[0].toMap()["title"] != "First"
        || order[0].toMap().contains("notes") || order[0].toMap().contains("elements")) {
        return 8;
    }
    outputs.advance(1);
    outputs.advance(1);
    if (snapshotChanges != 1 || outputs.liveOrder() != order) {
        return 9;
    }
    if (outputs.liveIndex() != 1) {
        return 2;
    }
    outputs.blackout("audience", true);
    outputs.clearText(true);
    if (!outputs.audienceBlack() || outputs.stageBlack() || !outputs.textCleared()) {
        return 3;
    }
    outputs.startCountdown(2);
    if (outputs.countdown() != "02:00") {
        return 4;
    }
    outputs.stopCountdown();
    outputs.setStageMessage("Stage only");
    if (outputs.stageMessage() != "Stage only") {
        return 5;
    }
    QWindow audience, stage, editor;
    outputs.bindWindows(&audience, &stage, &editor);
    if (!outputs.displays().isEmpty()
        && outputs.route("audience", outputs.displays().first().toMap()["id"].toString())) {
        return 6;
    }
    outputs.preview("audience");
    outputs.preview("stage");
    outputs.screenRemoved(audience.screen());
    if (outputs.audienceEnabled() || outputs.stageEnabled() || audience.isVisible()
        || stage.isVisible()) {
        return 7;
    }
    qInfo() << "Output checks passed: live snapshots, next slide, notes, boundaries, independent "
               "blackout, countdown, and operator-screen protection.";
    return 0;
}
