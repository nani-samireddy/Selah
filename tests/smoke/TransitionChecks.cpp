#include "TransitionChecks.h"
#include "DocumentElements.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "SlideTransition.h"
#include <memory>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

// Real GPU capture: exact endpoint colors, a linear midpoint, transparent alpha,
// instant blackout, rapid navigation and the same fade on the audience window.
void runTransitionOutputChecks(OutputManager &outputs, NdiController &ndi,
    QQmlApplicationEngine &engine, const QString &folder) {
    QDir().mkpath(folder);
    QVariantMap red { { "name", "Red" }, { "background", "#FF0000" }, { "ambientOpacity", 0 },
        { "elements", QVariantList {} } };
    auto blue = red;
    blue["name"] = "Blue";
    blue["background"] = "#0000FF";
    blue["transition"] = QVariantMap { { "type", "fade" }, { "durationMs", 1200 } };
    auto empty = blue;
    empty["elements"] = QVariantList {};
    auto graphic = DocumentElements::text("Test graphic", "", 0, 0, 1920, 1080, 36, "Arial");
    // A local SVG image exercises translucent pixels without font rasterization.
    const auto svg = QDir(folder).absoluteFilePath("alpha.svg");
    QFile f(svg);
    if (!f.open(QIODevice::WriteOnly)) {
        qApp->exit(294);
        return;
    }
    f.write("<svg xmlns='http://www.w3.org/2000/svg' width='1920' height='1080'><rect width='1920' "
            "height='1080' fill='red' fill-opacity='0.5'/></svg>");
    f.close();
    graphic["kind"] = "image";
    graphic["source"] = QUrl::fromLocalFile(svg).toString();
    graphic["fit"] = "stretch";
    red["elements"] = QVariantList { graphic };
    // Solid color phase uses the backgrounds; alpha phase uses only this image.
    const auto alphaSlide = red;
    red["elements"] = QVariantList {};
    const QVariantList slides { red, blue, red };
    outputs.takeLive(slides, 0);
    outputs.preview("audience");
    if (!ndi.start("Transition verification", 1280, 720, 60, false, true)) {
        qCritical() << ndi.status();
        qApp->exit(291);
        return;
    }

    struct State {
        QVariantMap videoBefore;
        int phase = 0;
        int waits = 0;
        int effectIndex = 0;
        QImage baseline;
    };

    auto state = std::make_shared<State>();
    auto timer = new QTimer(&engine);
    timer->setInterval(450);
    auto audience = []() -> QQuickWindow * {
        for (auto w : QGuiApplication::topLevelWindows()) {
            if (w->title().contains("Audience output")) {
                return qobject_cast<QQuickWindow *>(w);
            }
        }
        return nullptr;
    };
    QObject::connect(timer, &QTimer::timeout, &engine,
        [&, timer, state, slides, alphaSlide, empty, folder, audience] {
            auto fail = [&](const QString &error) {
                qCritical() << "Transition output check failed:" << error << ndi.status();
                timer->stop();
                ndi.stop();
                outputs.stop("audience");
                qApp->exit(292);
            };
            auto pixel = [&] {
                return ndi.lastFrame().isNull() ? QColor() : ndi.lastFrame().pixelColor(640, 360);
            };
            if (state->phase == 0) {
                if (!ndi.active() || ndi.framesCaptured() < 5) {
                    if (++state->waits > 12) {
                        fail("Initial frames unavailable");
                    }
                    return;
                }
                if (pixel() != QColor("#FF0000")) {
                    fail("Initial slide must be red");
                    return;
                }
                outputs.advance(1);
            } else if (state->phase == 1) {
                const auto p = pixel();
                if (!outputs.transitioning() || p.red() < 50 || p.blue() < 30
                    || qAbs(p.red() + p.blue() - 255) > 5 || p.alpha() != 255) {
                    fail(QString("Linear fade midpoint: %1,%2,%3,%4")
                            .arg(p.red())
                            .arg(p.green())
                            .arg(p.blue())
                            .arg(p.alpha()));
                    return;
                }
                ndi.lastFrame().save(folder + "/ndi-midfade.png");
                const auto physical = audience()->grabWindow();
                const auto q = physical.pixelColor(physical.width() / 2, physical.height() / 2);
                if (qAbs(q.red() - p.red()) > 45 || qAbs(q.blue() - p.blue()) > 45) {
                    fail("Audience/NDI fade mismatch");
                    return;
                }
                physical.save(folder + "/audience-midfade.png");
            } else if (state->phase == 2) {
                // Allow the shared 1200ms timeline to finish.
            } else if (state->phase == 3) {
                if (outputs.transitioning() || pixel() != QColor("#0000FF")) {
                    fail("Fade endpoint must be blue");
                    return;
                }
                outputs.advance(1);
            } else if (state->phase == 4) {
                if (pixel() != QColor("#FF0000") || outputs.transitioning()) {
                    fail("Cut must be immediate");
                    return;
                }
                outputs.advance(-1);
                outputs.advance(-1);
                if (outputs.transitioning() || outputs.liveIndex() != 0) {
                    fail("Rapid navigation must settle");
                    return;
                }
                outputs.advance(1);
                outputs.blackout("audience", true);
            } else if (state->phase == 5) {
                if (pixel() != QColor(Qt::black) || outputs.transitioning()) {
                    fail("Blackout must cut immediately");
                    return;
                }
                outputs.blackout("audience", false);
                ndi.stop();
                outputs.takeLive({ alphaSlide, empty }, 0);
            } else if (state->phase == 6) {
                if (ndi.busy()) {
                    return;
                }
                // This is a new scene with no previous texture; its first frame must cut.
                if (!ndi.start("Alpha transition verification", 1280, 720, 60, true, true)) {
                    fail("Alpha scene restart");
                    return;
                }
            } else if (state->phase == 7) {
                if (!ndi.active() || ndi.lastFrame().isNull()) {
                    fail("Alpha scene did not start");
                    return;
                }
                const auto p = pixel();
                if (qAbs(p.alpha() - 128) > 3 || p.red() < 245) {
                    fail("Initial overlay alpha");
                    return;
                }
                outputs.advance(1);
            } else if (state->phase == 8) {
                const auto p = pixel();
                if (p.alpha() < 30 || p.alpha() > 115 || p.red() < 240 || p.blue() > 5) {
                    fail("Premultiplied alpha crossfade");
                    return;
                }
                ndi.lastFrame().save(folder + "/ndi-alpha-midfade.png");
            } else if (state->phase == 9) {
            } else if (state->phase == 10) {
                if (pixel().alpha() != 0) {
                    fail("Uncovered canvas must fade to transparent");
                    return;
                }
                if (qApp->arguments().contains("--transition-video")) {
                    ndi.stop();
                    const auto arg = qApp->arguments().indexOf("--transition-video");
                    auto video = DocumentElements::text("Video", "", 0, 0, 1920, 1080, 36, "Arial");
                    video["kind"] = "video";
                    video["source"] = QUrl::fromLocalFile(
                        QFileInfo(qApp->arguments().value(arg + 1)).absoluteFilePath())
                                          .toString();
                    video["fit"] = "contain";
                    video["loop"] = true;
                    video["muted"] = true;
                    auto first = alphaSlide;
                    first["elements"] = QVariantList { video };
                    auto second = first;
                    second["transition"]
                        = QVariantMap { { "type", "fade" }, { "durationMs", 1200 } };
                    outputs.takeLive({ first, second }, 0);
                } else {
                    state->phase = 13;
                }
            } else if (state->phase == 11) {
                if (ndi.busy()) {
                    return;
                }
                if (!ndi.start("Video transition verification", 1280, 720, 60, false, true)) {
                    fail("Video scene restart");
                    return;
                }
            } else if (state->phase == 12) {
                for (const auto &layer : ndi.rendererProbe()) {
                    if (layer.toMap()["kind"] == "video") {
                        state->videoBefore = layer.toMap();
                    }
                }
                if (state->videoBefore["frames"].toInt() < 2) {
                    if (++state->waits > 20) {
                        fail("Video did not decode before fading");
                    }
                    return;
                }
                outputs.advance(1);
            } else if (state->phase == 13) {
                QVariantMap after;
                for (const auto &layer : ndi.rendererProbe()) {
                    if (layer.toMap()["kind"] == "video") {
                        after = layer.toMap();
                    }
                }
                if (after["token"] != state->videoBefore["token"]
                    || after["frames"].toInt() <= state->videoBefore["frames"].toInt()) {
                    fail("Video restarted or stopped during the slide fade");
                    return;
                }
                ndi.lastFrame().save(folder + "/ndi-video-midfade.png");
            } else if (state->phase == 14) {
                // Exercise every effect through the real GPU diagnostic scene.
                ndi.stop();
            } else if (state->phase == 15) {
                if (ndi.busy()) {
                    return;
                }
                outputs.takeLive(slides, 0);
                if (!ndi.start("Transition catalogue verification", 1280, 720, 30, false, true)) {
                    fail("Catalogue scene restart");
                    return;
                }
            } else if (state->phase == 16) {
                auto first = slides.first().toMap();
                auto second = slides[1].toMap();
                second["transition"]
                    = QVariantMap { { "type", SlideTransition::effects()[state->effectIndex] },
                          { "durationMs", 1200 } };
                outputs.takeLive({ first, second }, 0);
            } else if (state->phase == 17) {
                state->baseline = ndi.lastFrame();
                outputs.advance(1);
            } else if (state->phase == 18) {
                const auto frame = ndi.lastFrame();
                const auto effect = SlideTransition::effects()[state->effectIndex];
                if (!outputs.transitioning() || frame.isNull() || frame == state->baseline) {
                    fail("Effect must visibly progress: " + effect);
                    return;
                }
                if (frame.pixelColor(5, 5).alpha() != 255
                    || frame.pixelColor(640, 360).alpha() != 255) {
                    fail("Opaque slides must stay opaque during " + effect);
                    return;
                }
                frame.save(folder + "/" + effect + "-midpoint.png");
                if (effect == "wipe-left"
                    && (frame.pixelColor(10, 360) != QColor("red")
                        || frame.pixelColor(1270, 360) != QColor("blue"))) {
                    fail("Wipe direction must reveal the incoming slide from the right");
                    return;
                }
            } else if (state->phase == 19) {
                // Wait for endpoint.
            } else {
                if (outputs.transitioning() || pixel() != QColor("blue")) {
                    fail("Effect endpoint must be the incoming slide");
                    return;
                }
                if (++state->effectIndex < SlideTransition::effects().size()) {
                    state->phase = 16;
                    return;
                }
                timer->stop();
                ndi.stop();
                outputs.stop("audience");
                qInfo() << "Transition output checks passed: every GPU effect, endpoints, shared "
                           "audience/NDI clock, rapid navigation, blackout and transparent alpha.";
                qApp->exit(0);
                return;
            }
            ++state->phase;
        });
    timer->start();
    QTimer::singleShot(80000, &engine, [] { qApp->exit(293); });
}
