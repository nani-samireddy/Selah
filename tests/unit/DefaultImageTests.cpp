#include "OutputManager.h"
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QSettings>
#include <QTemporaryDir>

namespace {
struct ImagePreferences {
    QSettings settings;
    QVariantMap previous;
    QStringList keys { "outputs/defaultImage", "outputs/defaultImageName",
        "outputs/defaultImageFit" };

    ImagePreferences() {
        for (const auto &key : keys) {
            if (settings.contains(key)) {
                previous[key] = settings.value(key);
            }
            settings.remove(key);
        }
    }

    ~ImagePreferences() {
        for (const auto &key : keys) {
            if (previous.contains(key)) {
                settings.setValue(key, previous[key]);
            } else {
                settings.remove(key);
            }
        }
    }
};
}

int runDefaultImageChecks() {
    ImagePreferences preferences;
    QTemporaryDir directory;
    QImage image(160, 90, QImage::Format_RGB32);
    image.fill(QColor("#234832"));
    const auto path = directory.filePath("Welcome.png");
    if (!image.save(path)) {
        return 1;
    }
    OutputManager outputs;
    int changes = 0;
    QObject::connect(&outputs, &OutputManager::liveChanged, &outputs, [&] { ++changes; });
    if (outputs.showDefaultImage(true) || !outputs.setDefaultImage(QUrl::fromLocalFile(path))
        || changes || outputs.hasAudienceContent() || outputs.defaultImageShowing()) {
        return 2;
    }
    const auto saved = outputs.defaultImageSource();
    QFile::remove(path);
    if (!QFile::exists(saved.toLocalFile()) || !outputs.showDefaultImage(true)
        || outputs.liveCount() || !outputs.hasAudienceContent()
        || outputs.audienceElements().size() != 1 || outputs.audienceAmbientOpacity() != 0) {
        return 3;
    }
    outputs.showDefaultImage(false);
    outputs.takeLive({ QVariantMap { { "name", "Current" }, { "notes", "Speaker notes" },
                           { "elements", QVariantList {} } },
                         QVariantMap { { "name", "Next" }, { "elements", QVariantList {} } } },
        0, "service");
    const auto snapshot = outputs.snapshotId();
    const auto slides = outputs.liveSlides();
    outputs.setPlaying(false);
    outputs.blackout("audience", true);
    outputs.clearText(true);
    outputs.showDefaultImage(true);
    const auto shown = outputs.audienceElements();
    if (outputs.snapshotId() != snapshot || outputs.liveSlides() != slides || outputs.liveIndex()
        || outputs.liveNotes() != "Speaker notes" || outputs.playing() || !outputs.audienceBlack()
        || !outputs.textCleared()) {
        return 4;
    }
    outputs.setDefaultImageFit("cover");
    if (outputs.audienceElements() != shown) {
        return 5;
    }
    outputs.showDefaultImage(false);
    if (outputs.audienceElements() != outputs.liveElements() || outputs.liveIndex()) {
        return 6;
    }
    outputs.showDefaultImage(true);
    if (outputs.audienceElements()[0].toMap()["fit"] != "cover") {
        return 7;
    }
    outputs.advance(1);
    if (outputs.defaultImageShowing() || outputs.liveIndex() != 1
        || outputs.snapshotId() != snapshot) {
        return 8;
    }
    if (outputs.setDefaultImage(QUrl("https://example.invalid/image.jpg"))
        || outputs.defaultImageSource() != saved) {
        return 9;
    }
    OutputManager reopened;
    if (reopened.defaultImageSource() != saved || reopened.defaultImageFit() != "cover"
        || reopened.defaultImageShowing()) {
        return 10;
    }
    outputs.showDefaultImage(true);
    outputs.removeDefaultImage();
    if (outputs.defaultImageAvailable() || !outputs.defaultImageShowing()
        || outputs.audienceElements().size() != 1) {
        return 11;
    }
    outputs.showDefaultImage(false);
    if (outputs.showDefaultImage(true)) {
        return 12;
    }
    outputs.setDefaultImage(saved);
    outputs.showDefaultImage(true);
    outputs.takeLive(slides, 0);
    if (outputs.defaultImageShowing()) {
        return 13;
    }
    QFile::remove(saved.toLocalFile());
    if (outputs.showDefaultImage(true) || outputs.defaultImageShowing()) {
        return 14;
    }
    qInfo() << "Default image checks passed: durable import, persistence, explicit show/resume, "
               "snapshot isolation, fit, missing files, navigation and blackout priority.";
    return 0;
}
