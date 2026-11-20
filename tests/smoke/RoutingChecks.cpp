#include "ContentLibrary.h"
#include "DiagnosticRunner.h"
#include "EditorDocument.h"
#include "MediaLibrary.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "ProgramChecks.h"
#include "ProgramStore.h"
#include "PrototypeChecks.h"
#include "SlideFactory.h"
#include <QDebug>
#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>

int startRoutingChecks(QGuiApplication &app, EditorDocument &document, MediaLibrary &library,
    ContentLibrary &content, OutputManager &outputs, NdiController &ndi, ProgramStore &programs,
    QQmlApplicationEngine &engine) {
    const int routingIndex = app.arguments().indexOf("--routing-smoke");
    if (routingIndex >= 0) {
        if (routingIndex + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 20;
        }
        auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        const auto folder = app.arguments()[routingIndex + 1];
        QDir().mkpath(folder);
        document.setNotes("Speak slowly. Welcome new visitors.");
        outputs.takeLive(document.slides(), 0);
        document.updateElement(1, { { "text", "Draft stays off the output" } });
        outputs.setStageMessage("2 minutes remaining");
        outputs.startCountdown(20);
        outputs.preview("audience");
        outputs.preview("stage");
        const auto sample = folder + "/test-card.png";
        if (QFileInfo::exists(sample)) {
            document.addMedia(QUrl::fromLocalFile(sample));
        }
        QTimer::singleShot(1400, &app, [&, window, folder] {
            if (!outputs.audienceEnabled() || !outputs.stageEnabled()
                || outputs.liveText() != "You belong\nhere.") {
                app.exit(21);
                return;
            }
            for (auto w : QGuiApplication::topLevelWindows()) {
                auto quick = qobject_cast<QQuickWindow *>(w);
                if (!quick) {
                    continue;
                }
                if (w->title().contains("Audience output")) {
                    quick->grabWindow().save(folder + "/audience-output.png");
                }
                if (w->title().contains("Stage output")) {
                    quick->grabWindow().save(folder + "/stage-output.png");
                }
            }
            outputs.blackout("audience", true);
            QTimer::singleShot(150, &app, [&, window, folder] {
                bool checked = false;
                for (auto w : QGuiApplication::topLevelWindows()) {
                    if (w->title().contains("Audience output")) {
                        auto frame = qobject_cast<QQuickWindow *>(w)->grabWindow();
                        checked = !frame.isNull()
                            && frame.pixelColor(frame.width() / 2, frame.height() / 2)
                                == QColor(Qt::black);
                    }
                }
                if (!checked || !outputs.stageEnabled()) {
                    app.exit(22);
                    return;
                }
                outputs.blackout("audience", false);
                outputs.stop("audience");
                if (outputs.audienceEnabled() || !outputs.stageEnabled()) {
                    app.exit(23);
                    return;
                }
                outputs.stop("stage");
                QJSValue object = engine.newQObject(window);
                object.property("showLibrary").callWithInstance(object);
                QTimer::singleShot(400, &app, [&, window, folder] {
                    window->grabWindow().save(folder + "/media-library.png");
                    // Close the library via its root property, then open output setup.
                    QJSValue object = engine.newQObject(window);
                    object.property("showOutputs").callWithInstance(object);
                    QTimer::singleShot(300, &app, [&, window, folder] {
                        window->grabWindow().save(folder + "/output-settings.png");
                        qInfo() << "Routing checks passed: audience/stage windows, draft "
                                   "isolation, blackout pixels, and independent stop.";
                        app.exit(0);
                    });
                });
            });
        });
    }

    return 0;
}
