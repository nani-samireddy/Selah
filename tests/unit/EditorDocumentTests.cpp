#include "EditorDocument.h"
#include "SlideFactory.h"
#include <algorithm>
#include <cmath>
#include <QColor>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFutureWatcher>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QTemporaryDir>
#include <QUuid>

int EditorDocument::selfTest() {
    EditorDocument d(nullptr, false);
    QTemporaryDir dir;
    auto check = [](bool ok, const char *message) {
        if (!ok) {
            qCritical() << message;
        }
        return ok;
    };
    if (!check(d.elements().size() == 4, "Initial elements")) {
        return 1;
    }
    d.updateElement(1, { { "text", "తెలుగు / हिंदी / English" }, { "x", 99999 } });
    if (!check(d.elements()[1].toMap()["x"].toDouble() == 1880, "Coordinate bounds")) {
        return 1;
    }
    d.undo();
    if (!check(d.elements()[1].toMap()["text"].toString() == "You belong\nhere.", "Undo")) {
        return 1;
    }
    d.redo();
    if (!check(d.elements()[1].toMap()["text"].toString().contains("తెలుగు"), "Redo Unicode")) {
        return 1;
    }
    d.setSelected(1);
    d.duplicateSelected();
    if (!check(d.elements().size() == 5, "Duplicate")) {
        return 1;
    }
    d.deleteSelected();
    if (!check(d.elements().size() == 4, "Delete")) {
        return 1;
    }
    d.setNotes("Speaker notes");
    d.addSlide();
    d.setCurrentSlide(0);
    const auto url = QUrl::fromLocalFile(dir.filePath("test.churchdeck"));
    if (!check(d.saveProject(url), "Save")) {
        return 1;
    }
    EditorDocument loaded(nullptr, false);
    if (!check(loaded.openProject(url), "Load")) {
        return 1;
    }
    if (!check(loaded.slides().size() == 4 && loaded.notes() == "Speaker notes"
                && loaded.elements() == d.elements(),
            "Roundtrip")) {
        return 1;
    }
    QFile invalid(dir.filePath("invalid"));
    if (!invalid.open(QIODevice::WriteOnly)) {
        return 1;
    }
    invalid.write("{\"format\":1,\"slides\":[{}]}");
    invalid.close();
    if (!check(!loaded.openProject(QUrl::fromLocalFile(invalid.fileName()))
                && loaded.slides().size() == 4,
            "Reject invalid document without data loss")) {
        return 1;
    }
    QImage image(64, 48, QImage::Format_RGB32);
    image.fill(QColor("#2288AA"));
    const auto imagePath = dir.filePath("image.png");
    if (!image.save(imagePath)) {
        return 1;
    }
    if (!check(loaded.addMedia(QUrl::fromLocalFile(imagePath)), "Image import")) {
        return 1;
    }
    const auto media = loaded.selectedElement();
    if (!check(media["kind"] == "image"
                && QFile::exists(QUrl(media["source"].toString()).toLocalFile()),
            "Managed media copy")) {
        return 1;
    }
    QFile::remove(imagePath);
    if (!check(QFile::exists(QUrl(media["source"].toString()).toLocalFile()),
            "Independent of original media")) {
        return 1;
    }
    loaded.updateElement(0, { { "fit", "cover" }, { "loop", false } });
    loaded.moveSelected(1);
    if (!check(loaded.selected() == 1 && loaded.selectedElement()["uid"] == media["uid"],
            "Layer ordering preserves identity")) {
        return 1;
    }
    loaded.duplicateSelected();
    if (!check(loaded.selectedElement()["uid"] != media["uid"],
            "Duplicate has independent identity")) {
        return 1;
    }
    if (!check(loaded.saveProject(url), "Save media project")) {
        return 1;
    }
    EditorDocument roundtrip(nullptr, false);
    if (!check(roundtrip.openProject(url) && roundtrip.elements() == loaded.elements(),
            "Media project roundtrip")) {
        return 1;
    }
    if (!check(!loaded.addMedia(QUrl("https://example.com/video.mp4")), "Reject remote import")) {
        return 1;
    }
    const int targetCount = loaded.elements().size();
    QEventLoop importLoop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(5000);
    connect(&timeout, &QTimer::timeout, &importLoop, &QEventLoop::quit);
    connect(&loaded, &EditorDocument::importingChanged, &importLoop, [&] {
        if (!loaded.importing()) {
            importLoop.quit();
        }
    });
    loaded.importMedia(QUrl(media["source"].toString()));
    loaded.setCurrentSlide(1);
    timeout.start();
    importLoop.exec();
    if (!check(!loaded.importing() && loaded.currentSlide() == 1
                && loaded.slides()[0].toMap()["elements"].toList().size() == targetCount + 1,
            "Background import preserves target slide")) {
        return 1;
    }
    const auto beforeLibraryOnly = loaded.elements();
    bool registered = false;
    connect(&loaded, &EditorDocument::mediaImported, &importLoop,
        [&](const QVariantMap &) { registered = true; });
    loaded.importMedia(QUrl(media["source"].toString()), false);
    timeout.start();
    importLoop.exec();
    if (!check(registered && loaded.elements() == beforeLibraryOnly && !loaded.importing(),
            "Library-only import does not alter slides")) {
        return 1;
    }
    const auto initial = loaded.slides();
    if (!check(
            loaded.addScriptureSlides(
                { QVariantMap { { "text", QString("తెలుగు వాక్యం ").repeated(50) },
                    { "reference", "Test 1:1" }, { "translation", "test" }, { "abbreviation", "T" },
                    { "copyright", "Test copyright" }, { "license", "CC0" } } },
                3),
            "Generate scripture")) {
        return 1;
    }
    const auto generated = loaded.slides();
    loaded.undo();
    if (!check(loaded.slides() == initial, "Batch undo")) {
        return 1;
    }
    loaded.redo();
    if (!check(loaded.slides() == generated, "Batch redo")) {
        return 1;
    }
    if (!check(loaded.saveProject(url) && roundtrip.openProject(url)
                && roundtrip.slides() == generated
                && roundtrip.slides().last().toMap()["sourceLicense"] == "CC0",
            "Generated slide persistence and source attribution")) {
        return 1;
    }
    EditorDocument layers(nullptr, false);
    layers.setSelected(1);
    const auto selectedId = layers.selectedElement()["uid"];
    const auto originalLayers = layers.slides();
    const auto firstId = layers.elements().first().toMap()["uid"];
    if (!check(layers.moveElementTo(0, layers.elements().size() - 1)
                && layers.elements().last().toMap()["uid"] == firstId
                && layers.selectedElement()["uid"] == selectedId,
            "Layer reorder preserves selected identity")) {
        return 1;
    }
    const auto reorderedLayers = layers.slides();
    layers.undo();
    if (!check(layers.slides() == originalLayers, "Layer reorder is one undoable move")) {
        return 1;
    }
    layers.redo();
    if (!check(layers.slides() == reorderedLayers, "Layer reorder redo")) {
        return 1;
    }
    if (!check(!layers.moveElementTo(-1, 0) && !layers.moveElementTo(0, 0)
                && !layers.moveElementTo(0, 1000) && layers.slides() == reorderedLayers,
            "Invalid layer drops are ignored")) {
        return 1;
    }
    qInfo() << "Document checks passed: edits, bounds, undo/redo, Unicode, duplicate/delete, "
               "notes, slides, save/load, invalid input.";
    return 0;
}
