#include "PresentChecks.h"
#include "EditorDocument.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include "VisualLookup.h"
#include <memory>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

void runPresentChecks(OutputManager &outputs, ProgramStore &programs, EditorDocument &document,
    NdiController &ndi, QQmlApplicationEngine &engine, const QString &folder) {
    QDir().mkpath(folder);
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(220);
        return;
    }
    outputs.removeDefaultImage();
    window->resize(1120, 760);
    QMetaObject::invokeMethod(window, "switchWorkspace", Q_ARG(QVariant, QVariant("present")));
    auto style = engine.singletonInstance<QObject *>("ChurchDeck", "LibraryStyle");
    auto appearance = [&engine, style](const QString &theme) {
        auto wrapped = engine.newQObject(style);
        wrapped.property("setAppearance").callWithInstance(wrapped, { QJSValue(theme) });
    };
    appearance("dark");

    struct State {
        int phase = 0;
        QString snapshot;
        int index = 0;
    };

    auto state = std::make_shared<State>();
    auto timer = new QTimer(&engine);
    timer->setInterval(1000);
    QObject::connect(
        timer, &QTimer::timeout, &engine, [&, window, state, timer, folder, appearance] {
            auto fail = [&](const QString &error) {
                qCritical() << "Present check failed:" << error;
                timer->stop();
                ndi.stop();
                outputs.stop("audience");
                qApp->exit(221);
            };
            auto transport = window->findChild<QQuickItem *>("presentTransport");
            auto workspace = window->findChild<QQuickItem *>("presentWorkspace");
            if (!transport || !workspace || transport->height() < 40
                || transport->mapToItem(workspace, QPointF(0, transport->height())).y()
                    > workspace->height() + 1) {
                fail("Transport stays within the workspace at compact sizes");
                return;
            }
            auto stop = window->findChild<QQuickItem *>("stopPresentingButton");
            if (!stop || !stop->isVisible()
                || stop->mapToItem(workspace, QPointF(stop->width(), stop->height())).x()
                    > workspace->width()) {
                fail("Stop presenting stays visible in the pinned toolbar");
                return;
            }
            auto audience = []() -> QQuickWindow * {
                for (auto candidate : QGuiApplication::topLevelWindows()) {
                    if (candidate->title().contains("Audience output")) {
                        return qobject_cast<QQuickWindow *>(candidate);
                    }
                }
                return nullptr;
            };
            if (window->findChild<QObject *>("onAirFrame")) {
                fail("Live state must not add a full-window outline");
                return;
            }
            if (state->phase == 0) {
                window->grabWindow().save(folder + "/present-empty.png");
                const auto image = folder + "/default-image.jpg";
                QFile::remove(image);
                if (!QFile::copy(":/template-presets/meadow/media/meadow-wide.jpg", image)
                    || !outputs.setDefaultImage(QUrl::fromLocalFile(image))) {
                    fail("Default image import");
                    return;
                }
                const QVariantMap song { { "title", "Together in Your light" },
                    { "sections",
                        QVariantList {
                            QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                                { "primary",
                                    "We gather in Your light\nWe lift our hearts in praise\nWe "
                                    "share a living hope\nYour love will guide our way" } },
                            QVariantMap { { "key", "c1" }, { "label", "Chorus" },
                                { "primary",
                                    "Your mercy carries me\nYour faithfulness remains" } } } },
                    { "arrangement", QVariantList { "v1", "c1", "v1" } } };
                document.setNotes(
                    "Welcome the congregation.\nPause before moving into the first song.");
                if (!programs.createProgram("Evening service", "2026-11-25")
                    || !programs.addPresentation(document.slides(), "Evening welcome")
                    || !programs.createProgram("Sunday Worship", "")
                    || !programs.addPresentation(document.slides(), "Welcome & announcements")
                    || !programs.addSong(song, 2)) {
                    fail(programs.status());
                    return;
                }
                programs.setSelected(0);
                auto selection
                    = VisualLookup::find(window->contentItem(), "presentProgramSelector");
                auto load = VisualLookup::find(window->contentItem(), "loadPresentProgramButton");
                const auto beforeSelection = outputs.snapshotId();
                if (!selection || !load || !selection->isVisible()) {
                    fail("Present page program picker missing");
                    return;
                }
                selection->setProperty("currentIndex", 1);
                QMetaObject::invokeMethod(selection, "activated", Q_ARG(int, 1));
                if (outputs.snapshotId() != beforeSelection) {
                    fail("Choosing a program must not change live output");
                    return;
                }
                selection->setProperty("currentIndex", 0);
                QMetaObject::invokeMethod(selection, "activated", Q_ARG(int, 0));
                if (!QMetaObject::invokeMethod(load, "clicked")
                    || outputs.liveSource() != "program:" + programs.id()) {
                    fail("Load program must present the selected program from this page");
                    return;
                }
                outputs.setStageMessage("Two minutes to begin");
                state->snapshot = outputs.snapshotId();
                state->index = outputs.liveIndex();
            } else if (state->phase == 1) {
                if (window->property("onAir").toBool()) {
                    fail("A prepared snapshot without an enabled audience output must show Ready");
                    return;
                }
                window->grabWindow().save(folder + "/present-dark-compact.png");
                if (!outputs.showDefaultImage(true)) {
                    fail(outputs.status());
                    return;
                }
                outputs.preview("audience");
                if (!ndi.start("Present verification", 1280, 720, 30, false, true)) {
                    fail(ndi.status());
                    return;
                }
            } else if (state->phase == 2) {
                if (!window->property("onAir").toBool()) {
                    fail("An enabled audience output must set the live ambience");
                    return;
                }
                window->grabWindow().save(folder + "/present-default-image.png");
                const auto frame = ndi.lastFrame();
                QImage source(folder + "/default-image.jpg");
                const auto expected
                    = source.scaled(1280, 720, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                          .pixelColor(20, 20);
                const auto actual = frame.isNull() ? QColor() : frame.pixelColor(20, 20);
                if (!actual.isValid() || qAbs(actual.red() - expected.red()) > 8
                    || qAbs(actual.green() - expected.green()) > 8
                    || qAbs(actual.blue() - expected.blue()) > 8 || !audience()) {
                    fail("Default image on audience and NDI");
                    return;
                }
                frame.save(folder + "/default-image-ndi.png");
                audience()->grabWindow().save(folder + "/default-image-audience.png");
                outputs.blackout("audience", true);
            } else if (state->phase == 3) {
                if (!window->property("liveBlackout").toBool()) {
                    fail("Blackout must be reflected in the live indicator");
                    return;
                }
                if (!audience() || audience()->grabWindow().pixelColor(20, 20) != QColor(Qt::black)
                    || !outputs.defaultImageShowing()) {
                    fail("Blackout priority");
                    return;
                }
                outputs.blackout("audience", false);
                outputs.showDefaultImage(false);
                if (outputs.snapshotId() != state->snapshot
                    || outputs.liveIndex() != state->index) {
                    fail("Resume preserves the selected slide");
                    return;
                }
                appearance("light");
                window->resize(1440, 900);
            } else if (state->phase == 4) {
                window->grabWindow().save(folder + "/present-light-wide.png");
                appearance("dark");
                outputs.stop("audience");
                if (window->property("onAir").toBool()) {
                    fail("NDI diagnostic capture alone must not set Live");
                    return;
                }
                outputs.preview("audience");
                outputs.preview("stage");
                if (!stop->property("enabled").toBool()
                    || !QMetaObject::invokeMethod(stop, "clicked") || outputs.audienceEnabled()
                    || outputs.stageEnabled() || outputs.playing() || ndi.active()
                    || window->property("onAir").toBool() || outputs.snapshotId() != state->snapshot
                    || outputs.liveIndex() != state->index) {
                    fail("One Stop presenting action must stop both displays, NDI and media while "
                         "keeping the prepared snapshot");
                    return;
                }
            } else if (state->phase == 5) {
                if (ndi.busy()) {
                    return;
                }
                if (stop->property("enabled").toBool()) {
                    fail("Stop presenting must be disabled after all outputs stop");
                    return;
                }
                window->grabWindow().save(folder + "/present-stopped.png");
                if (!ndi.start("Stop during startup", 1280, 720, 30, false, true)
                    || !stop->property("enabled").toBool()
                    || !QMetaObject::invokeMethod(stop, "clicked")) {
                    fail("Stop must also cancel NDI startup");
                    return;
                }
            } else if (state->phase == 6) {
                if (ndi.busy()) {
                    return;
                }
                if (ndi.active() || ndi.framesCaptured() != 0
                    || stop->property("enabled").toBool()) {
                    fail("A queued sender-ready signal must not reactivate stopped output");
                    return;
                }
                timer->stop();
                qInfo()
                    << "Present checks passed: compact/wide themes, standby/resume, audience/NDI "
                       "capture, blackout and one-click stop including NDI startup cancellation.";
                qApp->exit(0);
                return;
            }
            ++state->phase;
        });
    timer->start();
}
