#include "PrototypeChecks.h"
#include "EditorDocument.h"
#include "NdiController.h"
#include "OutputManager.h"
#include <memory>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

namespace {
struct State {
    int phase = 0;
    QElapsedTimer clock;
    QJsonObject report;
    bool video = false;
};

QJsonObject metrics(const NdiController &ndi);

void deferExit(int code) {
    QTimer::singleShot(0, qGuiApp, [code] { qGuiApp->exit(code); });
}

bool allTransparent(const QImage &image) {
    if (image.isNull()) {
        return false;
    }
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            if (image.pixelColor(x, y).alpha() != 0) {
                return false;
            }
        }
    }
    return true;
}

void runPhysicalDisplayChecksImpl(NdiController &ndi, OutputManager &outputs,
    EditorDocument &document, QQmlApplicationEngine &engine, const QString &folder) {
    QDir().mkpath(folder);
    auto state = std::make_shared<State>();
    auto editor = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    auto operatorScreen = editor->screen();
    QString target;
    for (const auto &v : outputs.displays()) {
        if (!v.toMap()["operator"].toBool()) {
            target = v.toMap()["id"].toString();
            break;
        }
    }
    state->report = { { "displays", QJsonArray::fromVariantList(outputs.displays()) },
        { "physicalHotplugVerified", false }, { "windowsVerified", false },
        { "networkTransmissionVerified", false } };
    auto saveReport = [folder](const QJsonObject &value) {
        QFile file(folder + "/physical-display-report.json");
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(value).toJson());
        }
    };
    if (target.isEmpty()) {
        state->report["result"] = "No non-operator display available";
        saveReport(state->report);
        qInfo() << "Physical display check skipped: connect a second display.";
        deferExit(0);
        return;
    }
    document.setNotes("Physical stage display verification");
    outputs.takeLive(document.slides(), 0);
    outputs.setStageMessage("Display check · stage only");
    outputs.startCountdown(2);
    if (!outputs.route("audience", target)
        || !ndi.start("Display capture diagnostic", 1280, 720, 30, false, true)) {
        qCritical() << outputs.status() << ndi.status();
        deferExit(50);
        return;
    }
    state->clock.start();
    auto timer = new QTimer(qGuiApp);
    timer->setInterval(100);
    auto windowFor = [](const QString &role) -> QQuickWindow * {
        for (auto window : QGuiApplication::topLevelWindows()) {
            if (window->title().contains(role + " output", Qt::CaseInsensitive)) {
                return qobject_cast<QQuickWindow *>(window);
            }
        }
        return nullptr;
    };
    QObject::connect(timer, &QTimer::timeout, qGuiApp,
        [&, state, timer, target, editor, operatorScreen, saveReport, windowFor, folder] {
            auto fail = [&](const QString &message) {
                state->report["error"] = message;
                saveReport(state->report);
                qCritical() << message;
                timer->stop();
                outputs.stop("audience");
                outputs.stop("stage");
                ndi.stop();
                qGuiApp->exit(51);
            };
            if (state->clock.elapsed() > 12000) {
                fail("Fullscreen transition timed out");
                return;
            }
            if (state->phase == 0 && state->clock.elapsed() > 2200) {
                auto window = windowFor("Audience");
                state->report["observedAudienceState"] = window ? int(window->visibility()) : -1;
                state->report["observedAudienceScreen"]
                    = window && window->screen() ? window->screen()->name() : "none";
                state->report["observedEditorScreen"]
                    = editor->screen() ? editor->screen()->name() : "none";
                state->report["expectedOperatorScreen"]
                    = operatorScreen ? operatorScreen->name() : "none";
                if (!window || window->visibility() != QWindow::FullScreen
                    || window->screen() == operatorScreen || editor->screen() != operatorScreen) {
                    fail("Audience fullscreen routing did not stay on the assigned display");
                    return;
                }
                const auto image = window->grabWindow();
                if (image.isNull()) {
                    fail("No audience framebuffer");
                    return;
                }
                image.save(folder + "/physical-audience.png");
                state->report["audienceFullscreenVerified"] = true;
                state->report["audienceDisplay"] = window->screen()->name();
                state->report["framebufferWidth"] = image.width();
                state->report["framebufferHeight"] = image.height();
                outputs.blackout("audience", true);
                state->phase = 1;
                state->clock.restart();
            } else if (state->phase == 1 && state->clock.elapsed() > 300) {
                auto frame = windowFor("Audience")->grabWindow();
                if (frame.pixelColor(frame.width() / 2, frame.height() / 2) != QColor(Qt::black)) {
                    fail("Physical audience blackout failed");
                    return;
                }
                outputs.blackout("audience", false);
                outputs.stop("audience");
                if (!outputs.route("stage", target)) {
                    fail("Could not route stage");
                    return;
                }
                state->phase = 2;
                state->clock.restart();
            } else if (state->phase == 2 && state->clock.elapsed() > 2200) {
                auto window = windowFor("Stage");
                if (!window || window->visibility() != QWindow::FullScreen
                    || window->screen() == operatorScreen || editor->screen() != operatorScreen) {
                    fail("Stage fullscreen routing failed");
                    return;
                }
                window->grabWindow().save(folder + "/physical-stage.png");
                state->report["stageFullscreenVerified"] = true;
                state->report["stageDisplay"] = window->screen()->name();
                state->report["captureConcurrentWithFullscreen"] = metrics(ndi);
                if (!ndi.active() || ndi.framesCaptured() < 20) {
                    fail("Offscreen capture stalled during fullscreen output");
                    return;
                }
                outputs.stop("stage");
                ndi.stop();
                state->phase = 3;
                state->clock.restart();
            } else if (state->phase == 3 && !ndi.busy()) {
                if (outputs.audienceEnabled() || outputs.stageEnabled()
                    || editor->screen() != operatorScreen) {
                    fail("Output cleanup failed");
                    return;
                }
                state->report["operatorDisplayPreserved"] = true;
                state->report["physicalSequentialFullscreenPassed"] = true;
                saveReport(state->report);
                qInfo().noquote() << QJsonDocument(state->report).toJson(QJsonDocument::Compact);
                timer->stop();
                qGuiApp->exit(0);
            }
        });
    timer->start();
}

QJsonObject metrics(const NdiController &ndi) {
    return { { "backend", ndi.backendName() }, { "frames", int(ndi.framesCaptured()) },
        { "captureFps", ndi.captureFps() }, { "meanRenderMs", ndi.averageRenderMs() },
        { "droppedFrames", int(ndi.droppedFrames()) } };
}
}

void runPhysicalDisplayChecks(NdiController &ndi, OutputManager &outputs, EditorDocument &document,
    QQmlApplicationEngine &engine, const QString &folder) {
    runPhysicalDisplayChecksImpl(ndi, outputs, document, engine, folder);
}

void runNdiCaptureChecks(NdiController &ndi, OutputManager &outputs, EditorDocument &document,
    QQmlApplicationEngine &engine, const QString &folder) {
    QDir().mkpath(folder);
    auto state = std::make_shared<State>();
    const auto videoPath = folder + "/test-loop.mp4";
    if (QFileInfo::exists(videoPath)) {
        state->video = document.addMedia(QUrl::fromLocalFile(videoPath));
    }
    outputs.takeLive(document.slides(), 0);
    if (!ndi.start("Selah Capture Check", 1280, 720, 30, false, true)) {
        qCritical() << ndi.status();
        deferExit(40);
        return;
    }
    ndi.setVerificationPattern(true);
    state->clock.start();
    state->report = { { "networkTransmissionVerified", false }, { "sdkCompiled", ndi.sdkEnabled() },
        { "physicalHotplugVerified", false },
        { "displays", QJsonArray::fromVariantList(outputs.displays()) } };
    auto timer = new QTimer(qGuiApp);
    timer->setInterval(100);
    QObject::connect(timer, &QTimer::timeout, qGuiApp, [&, state, timer, folder] {
        auto fail = [&](const QString &message) {
            qCritical() << message << ndi.status();
            timer->stop();
            ndi.stop();
            qGuiApp->exit(41);
        };
        const auto age = state->clock.elapsed();
        if (age > 12000) {
            fail("Prototype check timed out");
            return;
        }
        if (state->phase == 0 && age > 2200) {
            auto frame = ndi.lastFrame();
            if (!ndi.active() || frame.size() != QSize(1280, 720) || ndi.framesCaptured() < 20
                || !ndi.offscreenHidden()) {
                fail("Offscreen frame production failed");
                return;
            }
            if (frame.pixelColor(8, 8) != QColor(Qt::red)
                || frame.pixelColor(8, frame.height() - 8) != QColor(Qt::blue)) {
                fail("RGBA channel order or vertical orientation is incorrect");
                return;
            }
            if (state->video) {
                bool decoded = false;
                for (const auto &v : ndi.rendererProbe()) {
                    if (v.toMap()["kind"] == "video") {
                        decoded = v.toMap()["frames"].toInt() > 3
                            && v.toMap()["position"].toInt() > 500;
                    }
                }
                if (!decoded) {
                    fail("Offscreen video playback failed");
                    return;
                }
            }
            state->report["full720"] = metrics(ndi);
            frame.save(folder + "/ndi-full-720.png");
            state->report["videoDecodedOffscreen"] = state->video;
            ndi.stop();
            state->phase = 1;
            state->clock.restart();
        } else if (state->phase == 1 && !ndi.busy()) {
            // Keep the antialiased-text probe independent of authored media.
            auto slide = document.slides().first().toMap();
            auto elements = slide["elements"].toList();
            elements.removeIf(
                [](const QVariant &element) { return element.toMap()["kind"] != "text"; });
            slide["elements"] = elements;
            outputs.takeLive({ slide }, 0);
            if (!ndi.start("Alpha check", 1280, 720, 30, true, true)) {
                fail("Could not start overlay capture");
                return;
            }
            state->phase = 2;
            state->clock.restart();
        } else if (state->phase == 2 && age > 1500) {
            const auto frame = ndi.lastFrame();
            if (frame.isNull() || frame.pixelColor(0, 0).alpha() != 0) {
                fail("Overlay background is not transparent");
                return;
            }
            int visible = 0, edges = 0, straight = 0, coloredEdges = 0, checkedEdges = 0;
            for (int y = 0; y < frame.height(); y++) {
                for (int x = 0; x < frame.width(); x++) {
                    const auto color = frame.pixelColor(x, y);
                    if (color.alpha() > 0) {
                        visible++;
                    }
                    if (color.alpha() > 0 && color.alpha() < 180) {
                        edges++;
                        if (color.red() > color.alpha()) {
                            straight++;
                        }
                    }
                    if (color.alpha() > 64 && color.alpha() < 240) {
                        checkedEdges++;
                        if (qMax(color.red(), qMax(color.green(), color.blue()))
                                - qMin(color.red(), qMin(color.green(), color.blue()))
                            > 65) {
                            coloredEdges++;
                        }
                    }
                }
            }
            if (visible < 100 || edges < 10 || straight < 10) {
                fail("Text alpha or unpremultiplication failed");
                return;
            }
            if (checkedEdges > 0 && coloredEdges * 10 > checkedEdges) {
                fail("Transparent text has unexpected color fringes");
                return;
            }
            state->report["alphaColorFringesChecked"] = true;
            state->report["overlay720"] = metrics(ndi);
            frame.save(folder + "/ndi-transparent-text.png");
            state->report["straightAlphaVerified"] = true;
            outputs.clearText(true);
            state->phase = 3;
            state->clock.restart();
        } else if (state->phase == 3 && age > 350) {
            if (!allTransparent(ndi.lastFrame())) {
                fail("Clear text retained overlay pixels");
                return;
            }
            outputs.clearText(false);
            // An authored graphic has opaque, translucent, and uncovered regions.
            QImage graphic(600, 120, QImage::Format_RGBA8888);
            graphic.fill(Qt::transparent);
            for (int y = 0; y < graphic.height(); ++y) {
                for (int x = 0; x < 300; ++x) {
                    graphic.setPixelColor(x, y, QColor(0, 255, 0, x < 150 ? 255 : 128));
                }
            }
            const auto path = folder + "/overlay-graphic.png";
            graphic.save(path);
            QVariantList layers { QVariantMap { { "uid", "overlay-text" }, { "kind", "text" },
                                      { "text", "Lower third" }, { "x", 300 }, { "y", 450 },
                                      { "width", 600 }, { "height", 100 }, { "fontSize", 60 } },
                QVariantMap { { "uid", "overlay-graphic" }, { "kind", "image" },
                    { "source", QUrl::fromLocalFile(path).toString() }, { "x", 300 }, { "y", 600 },
                    { "width", 600 }, { "height", 120 } } };
            if (state->video) {
                layers.append(QVariantMap { { "uid", "overlay-video" }, { "kind", "video" },
                    { "source", QUrl::fromLocalFile(folder + "/test-loop.mp4").toString() },
                    { "x", 1200 }, { "y", 600 }, { "width", 480 }, { "height", 270 } });
            }
            outputs.takeLive({ QVariantMap { { "elements", layers }, { "background", "#FF00FF" },
                                 { "ambientOpacity", 1.0 } } },
                0);
            state->phase = 8;
            state->clock.restart();
        } else if (state->phase == 8 && age > 1500) {
            const auto frame = ndi.lastFrame();
            const auto opaque = frame.pixelColor(230, 430);
            const auto translucent = frame.pixelColor(330, 430);
            if (opaque != QColor(0, 255, 0) || qAbs(translucent.alpha() - 128) > 2
                || translucent.green() < 250 || frame.pixelColor(500, 430).alpha() != 0
                || frame.pixelColor(0, 0).alpha() != 0) {
                fail("Transparent canvas lost authored graphic alpha or retained its base fill");
                return;
            }
            if (state->video) {
                bool decoded = false;
                for (const auto &v : ndi.rendererProbe()) {
                    const auto layer = v.toMap();
                    if (layer["uid"] == "overlay-video") {
                        decoded = layer["frames"].toInt() > 3 && layer["position"].toInt() > 500;
                    }
                }
                if (!decoded || frame.pixelColor(900, 450).alpha() != 255) {
                    fail("Transparent canvas excluded video");
                    return;
                }
            }
            frame.save(folder + "/ndi-transparent-canvas.png");
            state->report["transparentCanvasMediaAndAlphaVerified"] = true;
            outputs.clearText(true);
            state->phase = 9;
            state->clock.restart();
        } else if (state->phase == 9 && age > 350) {
            const auto frame = ndi.lastFrame();
            if (frame.pixelColor(230, 430) != QColor(0, 255, 0)) {
                fail("Clear text incorrectly hid graphic layers");
                return;
            }
            for (int y = 300; y < 365; ++y) {
                for (int x = 200; x < 600; ++x) {
                    if (frame.pixelColor(x, y).alpha() != 0) {
                        fail("Clear text retained lower-third text");
                        return;
                    }
                }
            }
            outputs.clearText(false);
            ndi.setBlackout(true);
            state->phase = 4;
            state->clock.restart();
        } else if (state->phase == 4 && age > 350) {
            if (!allTransparent(ndi.lastFrame())) {
                fail("Overlay blackout retained pixels");
                return;
            }
            ndi.setBlackout(false);
            ndi.stop();
            state->phase = 5;
            state->clock.restart();
        } else if (state->phase == 5 && !ndi.busy()) {
            if (!ndi.start("1080 capture check", 1920, 1080, 30, false, true)) {
                fail("Could not start 1080 capture");
                return;
            }
            state->phase = 6;
            state->clock.restart();
        } else if (state->phase == 6 && age > 1800) {
            if (ndi.lastFrame().size() != QSize(1920, 1080) || ndi.framesCaptured() < 15) {
                fail("1080 frame production failed");
                return;
            }
            state->report["full1080"] = metrics(ndi);
            ndi.lastFrame().save(folder + "/ndi-full-1080.png");
            state->report["captureOnlyPassed"] = true;
            auto root = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            auto object = engine.newQObject(root);
            object.property("showNdi").callWithInstance(object);
            state->phase = 7;
            state->clock.restart();
        } else if (state->phase == 7 && age > 300) {
            qobject_cast<QQuickWindow *>(engine.rootObjects().first())
                ->grabWindow()
                .save(folder + "/ndi-settings.png");
            QFile report(folder + "/ndi-prototype-report.json");
            if (report.open(QIODevice::WriteOnly)) {
                report.write(QJsonDocument(state->report).toJson());
            }
            qInfo().noquote() << QJsonDocument(state->report).toJson(QJsonDocument::Compact);
            timer->stop();
            ndi.stop();
            qGuiApp->exit(0);
        }
    });
    timer->start();
}
