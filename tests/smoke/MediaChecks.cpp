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

int startMediaChecks(QGuiApplication &app, EditorDocument &document, MediaLibrary &library,
    ContentLibrary &content, OutputManager &outputs, NdiController &ndi, ProgramStore &programs,
    QQmlApplicationEngine &engine) {
    const int smokeIndex = app.arguments().indexOf("--media-smoke");
    if (smokeIndex >= 0) {
        if (smokeIndex + 3 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 3;
        }
        auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        const QString captureFolder = app.arguments()[smokeIndex + 3];
        QDir().mkpath(captureFolder);
        if (!document.addMedia(QUrl::fromLocalFile(app.arguments()[smokeIndex + 1]))) {
            return 4;
        }
        document.updateElement(
            0, { { "x", 1400 }, { "y", 760 }, { "width", 400 }, { "height", 250 } });
        if (!document.addMedia(QUrl::fromLocalFile(app.arguments()[smokeIndex + 2]))) {
            return 4;
        }
        window->setProperty("videosPlaying", true);
        auto probe = [&engine, window] {
            QJSValue object = engine.newQObject(window);
            return object.property("rendererProbe").callWithInstance(object).toVariant().toList();
        };
        QTimer::singleShot(1800, &app, [&, window, probe, captureFolder] {
            const auto before = probe();
            QVariantMap video;
            for (const auto &v : before) {
                if (v.toMap()["kind"] == "video") {
                    video = v.toMap();
                }
            }
            if (video.isEmpty() || video["frames"].toInt() < 2
                || !video["problem"].toString().isEmpty()) {
                qCritical() << "Video did not decode" << before;
                app.exit(5);
                return;
            }
            document.updateElement(3, { { "text", "Shared renderer / తెలుగు / हिंदी" } });
            QTimer::singleShot(600, &app, [&, window, probe, captureFolder, video] {
                QVariantMap after;
                for (const auto &v : probe()) {
                    if (v.toMap()["kind"] == "video") {
                        after = v.toMap();
                    }
                }
                if (after["token"] != video["token"]
                    || after["position"].toInt() <= video["position"].toInt()) {
                    qCritical() << "Video restarted after text edit" << video << after;
                    app.exit(6);
                    return;
                }
                window->grabWindow().save(captureFolder + "/media-editor.png");
                QJSValue object = engine.newQObject(window);
                object.property("preview").callWithInstance(object);
                window->setProperty("videosPlaying", false);
                QTimer::singleShot(500, &app, [&, probe, window, captureFolder] {
                    const auto paused = probe();
                    QTimer::singleShot(400, &app, [&, probe, window, captureFolder, paused] {
                        const auto later = probe();
                        for (int i = 0; i < paused.size(); i++) {
                            if (paused[i].toMap()["kind"] == "video"
                                && std::abs(paused[i].toMap()["position"].toInt()
                                       - later[i].toMap()["position"].toInt())
                                    > 100) {
                                qCritical() << "Pause did not hold";
                                app.exit(7);
                                return;
                            }
                        }
                        document.updateElement(3, { { "text", "Draft only" } });
                        for (auto output : QGuiApplication::topLevelWindows()) {
                            if (output->title().contains("Output preview")) {
                                auto preview = qobject_cast<QQuickWindow *>(output);
                                bool snapshotPreserved = false;
                                for (const auto &v :
                                    preview->property("contentSnapshot").toList()) {
                                    if (v.toMap()["text"].toString().contains("Shared renderer")) {
                                        snapshotPreserved = true;
                                    }
                                }
                                if (!snapshotPreserved) {
                                    qCritical() << "Draft changed output snapshot";
                                    app.exit(8);
                                    return;
                                }
                                preview->grabWindow().save(captureFolder + "/media-output.png");
                            }
                        }
                        qInfo() << "Media checks passed: video frames, persistent playback through "
                                   "text edits, pause, and isolated output snapshot.";
                        app.exit(0);
                    });
                });
            });
        });
    }

    return 0;
}
