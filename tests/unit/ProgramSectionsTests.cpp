#include "OutputManager.h"
#include "ProgramStore.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

int runProgramSectionChecks() {
    QTemporaryDir directory;
    OutputManager outputs;
    ProgramStore store(&outputs, nullptr, directory.filePath("source"));
    const QVariantList slides { QVariantMap {
        { "name", "Content" }, { "background", "#101010" }, { "elements", QVariantList {} } } };
    if (!store.createProgram("Section rehearsal", "2026-11-26")
        || !store.addPresentation(slides, "Song") || !store.setItemSection("Worship")
        || !store.addPresentation(slides, "Bible") || !store.setItemSection("Sermon")
        || !store.addPresentation(slides, "Blessing") || !store.setItemSection("Closing")) {
        return 1;
    }
    if (store.items().size() != 3 || !store.present() || outputs.liveCount() != 3
        || outputs.liveOrder()[1].toMap()["section"] != "Sermon") {
        return 2;
    }
    const auto live = outputs.liveSlides();
    store.setSelected(1);
    if (!store.renameSection("Sermon", "Message") || outputs.liveSlides() != live
        || store.selectedItem()["section"] != "Message" || store.addSection("Worship")
        || store.setItemSection("Unknown")) {
        return 3;
    }
    if (!store.removeSection("Message") || store.items().size() != 3
        || store.selectedItem()["title"] != "Bible" || store.selected() != 0) {
        return 4;
    }
    store.undo();
    if (!store.sections().contains("Message") || store.selectedItem()["title"] != "Bible"
        || store.selectedItem()["section"] != "Message") {
        return 5;
    }
    store.redo();
    store.undo();
    if (!store.addSection("Prayer") || !store.setItemSection("Prayer") || store.selected() != 2
        || store.selectedItem()["title"] != "Bible") {
        return 6;
    }
    store.moveItem(-1);
    if (store.selectedItem()["section"] != "Closing" || store.selectedItem()["title"] != "Bible"
        || outputs.liveSlides() != live) {
        return 7;
    }
    const auto file = QUrl::fromLocalFile(directory.filePath("service.churchprogram"));
    if (!store.exportProgram(file)) {
        return 8;
    }
    ProgramStore imported(&outputs, nullptr, directory.filePath("imported"));
    if (!imported.importProgram(file) || imported.sections() != store.sections()
        || imported.items().size() != store.items().size()
        || imported.items()[1].toMap()["section"] != store.items()[1].toMap()["section"]
        || imported.items()[1].toMap()["id"] == store.items()[1].toMap()["id"]
        || outputs.liveSlides() != live) {
        return 9;
    }
    QFile bad(file.toLocalFile());
    if (!bad.open(QIODevice::ReadOnly)) {
        return 10;
    }
    auto root = QJsonDocument::fromJson(bad.readAll()).object();
    bad.close();
    auto program = root["program"].toObject();
    program["sections"] = QJsonArray { "Duplicate", "Duplicate" };
    root["program"] = program;
    if (!bad.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return 11;
    }
    bad.write(QJsonDocument(root).toJson());
    bad.close();
    const auto previous = imported.id();
    if (imported.importProgram(file) || imported.id() != previous) {
        return 12;
    }
    qInfo("Program section checks passed: ordering, assignment, rename/remove, undo/redo, live "
          "isolation and exchange validation.");
    return 0;
}
