#include "FontPreviewChecks.h"
#include "EditorDocument.h"
#include "FontLibrary.h"
#include "OutputManager.h"
#include "VisualLookup.h"
#include <memory>
#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QTimer>

void runFontPreviewChecks(FontLibrary &fonts, EditorDocument &document, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    auto picker = window ? window->findChild<QObject *>("fontPicker") : nullptr;
    if (!picker) {
        qApp->exit(300);
        return;
    }
    QDir().mkpath(folder);
    QString family;
    for (const QString &candidate : { "Timmana", "Ponnala", "Suranna" }) {
        if (!fonts.available(candidate)) {
            family = candidate;
            break;
        }
    }
    if (family.isEmpty()) {
        qCritical() << "Preview test needs an uninstalled Telugu family";
        qApp->exit(301);
        return;
    }
    const auto draft = document.slides();
    outputs.takeLive(draft, 0, "font-preview-baseline");
    const auto snapshot = outputs.snapshotId();
    picker->setProperty("currentFamily", family);
    picker->setProperty("previewText", "యేసయ్యా నీ కృప నన్ను నడిపించును\nYour grace will guide me");
    picker->setProperty("script", "telugu");
    QMetaObject::invokeMethod(picker, "open");

    struct State {
        int phase = 0;
        bool painted = false;
        bool valid = false;
    };

    auto state = std::make_shared<State>();
    auto timer = new QTimer(&engine);
    timer->setInterval(300);
    QObject::connect(timer, &QTimer::timeout, &engine,
        [&, window, picker, timer, state, family, draft, snapshot, folder] {
            auto fail = [&](const QString &error) {
                qCritical() << "Font preview check failed:" << error << fonts.previewStatus();
                timer->stop();
                QMetaObject::invokeMethod(picker, "close");
                qApp->exit(302);
            };
            if (state->phase == 0) {
                if (fonts.previewFamily() != family || fonts.previewBusy()) {
                    return;
                }
                if (!fonts.previewAvailable()) {
                    fail("Native face unavailable");
                    return;
                }
                auto item = VisualLookup::find(window->contentItem(), "downloadableFontPreview");
                if (!item || !item->isVisible() || item->implicitHeight() < 30
                    || item->property("missingCharacters").toBool()) {
                    fail("Indic preview geometry or glyph coverage");
                    return;
                }
                auto grab = item->grabToImage();
                if (!grab) {
                    fail("Preview capture unavailable");
                    return;
                }
                QObject::connect(
                    grab.data(), &QQuickItemGrabResult::ready, &engine,
                    [grab, state, folder] {
                        const auto image = grab->image();
                        int pixels = 0;
                        for (int y = 0; y < image.height(); ++y) {
                            for (int x = 0; x < image.width(); ++x) {
                                if (image.pixelColor(x, y).alpha() > 32) {
                                    ++pixels;
                                }
                            }
                        }
                        state->valid = pixels > 40;
                        state->painted = true;
                        grab->saveToFile(folder + "/font-canvas.png");
                    },
                    Qt::SingleShotConnection);
                window->grabWindow().save(folder + "/font-picker.png");
                ++state->phase;
            } else if (state->phase == 1) {
                if (!state->painted) {
                    return;
                }
                if (!state->valid || fonts.available(family)
                    || QFontDatabase::families().contains(family, Qt::CaseInsensitive)
                    || document.slides() != draft || outputs.snapshotId() != snapshot) {
                    fail("Preview must paint privately without applying/installing or altering "
                         "live content");
                    return;
                }
                QMetaObject::invokeMethod(picker, "close");
                ++state->phase;
            } else {
                if (fonts.previewBusy() || fonts.previewAvailable()) {
                    fail("Closing must release the active preview");
                    return;
                }
                timer->stop();
                qInfo() << "Font preview checks passed: auto-loaded Indic/English face, painted "
                           "pixels, no registration, no offline install, protected draft/live "
                           "snapshot and cleanup.";
                qApp->exit(0);
            }
        });
    timer->start();
    QTimer::singleShot(60000, &engine, [] { qApp->exit(303); });
}
