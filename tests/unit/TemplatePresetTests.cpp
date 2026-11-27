#include "PresentationLibrary.h"
#include "TemplateDefinition.h"
#include "TemplatePresets.h"
#include "TemplateStore.h"
#include <QDebug>
#include <QFile>
#include <QFontDatabase>
#include <QImageReader>
#include <QTemporaryDir>
#include <QUrl>

int TemplatePresets::selfTest() {
    for (const auto &family : { "Public Sans", "Cormorant Garamond", "Antic Didone",
             "Bebas Neue", "Allura", "Patrick Hand", "Barlow Condensed", "Lato",
             "Freckle Face", "Noto Sans Telugu" }) {
        if (!QFontDatabase::families().contains(QString::fromLatin1(family))) {
            qCritical() << "Missing offline template font:" << family;
            return 10;
        }
    }
    QTemporaryDir directory;
    TemplateStore store(nullptr, directory.path());
    PresentationLibrary presentations(nullptr, directory.path());
    QString error = install(store, nullptr, directory.path());
    if (!error.isEmpty() || store.layouts().size() != 23
        || !presentations.presentations().isEmpty()) {
        qCritical() << "Template preset installation failed:" << error;
        return 1;
    }
    QString mediaPath;
    for (const auto &value : store.layouts()) {
        const auto definition = value.toMap();
        if (!TemplateDefinition::validate(definition, &error)
            || !TemplateDefinition::missingMedia(definition).isEmpty()) {
            qCritical() << definition["name"] << error;
            return 2;
        }
        const auto preview = store.preview(definition["id"].toString());
        if (!preview["error"].toString().isEmpty() || preview["slides"].toList().isEmpty()) {
            qCritical() << "Preset preview failed:" << definition["name"] << preview["error"];
            return 3;
        }
        for (const auto &element : definition["slide"].toMap()["elements"].toList()) {
            const auto layer = element.toMap();
            if (layer["kind"] == "image") {
                const auto path = QUrl(layer["source"].toString()).toLocalFile();
                QImageReader reader(path);
                if (!reader.canRead()) {
                    qCritical() << "Unreadable template asset:" << path;
                    return 4;
                }
                mediaPath = path;
            }
        }
    }
    auto original = store.layout("preset-selected-a01");
    auto customized = original["slide"].toMap();
    customized["background"] = "#F1EADD";
    if (store.saveTemplate("My welcome", "presentation", customized, original["id"].toString())
            .isEmpty()) {
        return 5;
    }
    // Startup restores bundled files but preserves saved designs and their revisions.
    if (!QFile::remove(mediaPath) || !install(store, nullptr, directory.path()).isEmpty()
        || !QFile::exists(mediaPath) || store.layouts().size() != 23
        || store.layout("preset-selected-a01")["name"] != "My welcome"
        || store.layout("preset-selected-a01")["revision"].toInt() != 2) {
        return 6;
    }
    auto incompatible = TemplateDefinition::starter("presentation");
    auto slide = incompatible["slide"].toMap();
    auto layers = slide["elements"].toList();
    auto layer = layers[0].toMap();
    layer["binding"] = "Primary lyrics";
    layers[0] = layer;
    slide["elements"] = layers;
    incompatible["slide"] = slide;
    if (TemplateDefinition::validate(incompatible, &error)
        || TemplateDefinition::supports(original, "song", false, &error)
        || !presentations.presentations().isEmpty()) {
        return 7;
    }
    // Retirement only touches the former Meadow/Garden pack, not this collection.
    auto legacy = TemplateDefinition::starter("presentation");
    legacy["id"] = "preset-meadow-untouched";
    if (!store.installPreset(legacy)) {
        return 8;
    }
    legacy["id"] = "preset-garden-edited";
    if (!store.installPreset(legacy)
        || store.saveTemplate("My old design", "presentation", legacy["slide"].toMap(),
               legacy["id"].toString()).isEmpty()
        || !store.retireLegacyPresets() || store.layouts().size() != 24
        || !store.layout("preset-meadow-untouched").isEmpty()
        || store.layout("preset-garden-edited")["name"] != "My old design"
        || store.layout("preset-selected-a01")["name"] != "My welcome"
        || !QFile::exists(mediaPath) || !store.retireLegacyPresets()
        || store.layouts().size() != 24) {
        return 9;
    }
    qInfo() << "Selected template collection passed: 19 layouts, original local backgrounds, "
               "content bindings, previews, repeatable installation, media repair, "
               "edit preservation and legacy retirement.";
    return 0;
}
