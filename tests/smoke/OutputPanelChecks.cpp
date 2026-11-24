#include "OutputPanelChecks.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "VisualLookup.h"
#include <memory>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QTimer>

void runOutputPanelChecks(EditorDocument &document, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(270);
        return;
    }
    window->resize(1120, 760);
    QDir().mkpath(folder);
    const auto draft = document.slides();
    outputs.takeLive(draft, 0, "output-ux-baseline");
    const auto baseline = outputs.snapshotId();
    QMetaObject::invokeMethod(window, "showOutputs", Q_ARG(QVariant, QVariant("displays")));

    struct State {
        int phase = 0;
        int captures = 0;
    };

    auto state = std::make_shared<State>();
    auto timer = new QTimer(&engine);
    timer->setInterval(600);
    QObject::connect(
        timer, &QTimer::timeout, &engine, [&, window, folder, state, timer, draft, baseline] {
            auto fail = [&](const QString &message) {
                qCritical() << "Output settings check failed:" << message;
                timer->stop();
                qApp->exit(271);
            };
            auto find = [&](const QString &name) {
                return VisualLookup::find(window->contentItem(), name);
            };
            auto click = [&](const QString &name) {
                auto button = find(name);
                if (!button || !QMetaObject::invokeMethod(button, "clicked")) {
                    fail("Missing control: " + name);
                    return false;
                }
                return true;
            };
            auto capture = [&](const QString &name) {
                const auto grab = window->contentItem()->grabToImage();
                if (!grab) {
                    fail("Capture unavailable");
                    return;
                }
                ++state->captures;
                QObject::connect(
                    grab.data(), &QQuickItemGrabResult::ready, &engine,
                    [grab, state, folder, name] {
                        grab->saveToFile(folder + "/" + name + ".png");
                        --state->captures;
                    },
                    Qt::SingleShotConnection);
            };
            auto panel = window->findChild<QObject *>("presentOutputControls");
            if (!panel || window->property("activeWorkspace") != "present"
                || outputs.snapshotId() != baseline || document.slides() != draft
                || outputs.audienceEnabled() || outputs.stageEnabled()) {
                fail("Opening/settings must not route a screen or replace live/draft content");
                return;
            }
            if (state->phase == 0) {
                if (panel->property("section") != "displays" || !find("audienceDisplayCard")
                    || !find("stageDisplayCard")) {
                    fail("Display setup must be the initial section");
                    return;
                }
                auto combo = find("stageDisplayChoice");
                auto fullscreen = find("stageFullscreenButton");
                int operatorIndex = -1;
                const auto displays = outputs.displays();
                for (int i = 0; i < displays.size(); ++i) {
                    if (displays[i].toMap()["operator"].toBool()) {
                        operatorIndex = i;
                    }
                }
                if (!combo || !fullscreen || operatorIndex < 0) {
                    fail("Operator display missing");
                    return;
                }
                const auto previous = combo->property("currentIndex");
                combo->setProperty("currentIndex", operatorIndex);
                if (fullscreen->property("enabled").toBool()) {
                    fail("Operator fullscreen must be disabled");
                    return;
                }
                combo->setProperty("currentIndex", previous);
                capture("displays");
            } else if (state->phase == 1) {
                if (!click("audienceBlackoutButton")) {
                    return;
                }
                if (!outputs.audienceBlack() || outputs.stageBlack()) {
                    fail("Blackout must be role-specific");
                    return;
                }
                if (!click("audienceBlackoutButton") || !click("outputTab_speaker")) {
                    return;
                }
            } else if (state->phase == 2) {
                auto transport = find("presentTransport");
                if (find("outputTab_live") || find("outputLiveControls") || !transport) {
                    fail("Live controls must have one home in the pinned toolbar");
                    return;
                }
                for (const auto &name : { "outputPreviousButton", "outputNextButton",
                         "outputPlayButton", "outputAudioButton", "outputClearTextButton" }) {
                    if (!VisualLookup::find(transport, name)) {
                        fail(QString("Missing pinned control: ") + name);
                        return;
                    }
                }
                capture("transport-controls");
                if (!click("outputAudienceBlackout") || !outputs.audienceBlack()
                    || outputs.stageBlack() || !click("outputAudienceBlackout")) {
                    fail("Persistent audience blackout must work outside Displays");
                    return;
                }
            } else if (state->phase == 3) {
                if (outputs.liveCount() > 1) {
                    if (!click("outputNextButton") || outputs.liveIndex() != 1
                        || !click("outputPreviousButton")) {
                        fail("Live navigation not wired");
                        return;
                    }
                }
                if (!click("outputClearTextButton") || !outputs.textCleared()) {
                    fail("Text control not wired");
                    return;
                }
                if (!click("outputClearTextButton")) {
                    return;
                }
                const bool playing = outputs.playing();
                if (!click("outputPlayButton") || outputs.playing() == playing
                    || !click("outputPlayButton")) {
                    fail("Pinned media playback control not wired");
                    return;
                }
                const bool audio = outputs.audioEnabled();
                if (!click("outputAudioButton") || outputs.audioEnabled() == audio) {
                    fail("Audio control not wired");
                    return;
                }
                if (!click("outputAudioButton") || !click("outputTab_speaker")) {
                    return;
                }
            } else if (state->phase == 4) {
                if (panel->property("section") != "speaker"
                    || !find("outputSpeakerControls")->isVisible()) {
                    fail("Speaker tab did not open");
                    return;
                }
                find("outputSpeakerMessage")->setProperty("text", "Two minutes remaining");
                find("outputCountdownMinutes")->setProperty("value", 2);
                if (!click("outputSendMessage") || outputs.stageMessage() != "Two minutes remaining"
                    || !click("outputStartCountdown") || !click("outputPauseCountdown")) {
                    fail("Speaker controls not wired");
                    return;
                }
            } else if (state->phase == 5) {
                if (outputs.countdown() != "02:00") {
                    fail("Countdown duration not applied");
                    return;
                }
                capture("speaker-tools");
            } else if (state->phase == 6) {
                if (!click("outputClearMessage") || !outputs.stageMessage().isEmpty()) {
                    fail("Message clear not wired");
                    return;
                }

                QMetaObject::invokeMethod(
                    window, "showOutputs", Q_ARG(QVariant, QVariant("displays")));
            } else if (state->phase == 7) {
                if (panel->property("section") != "displays") {
                    fail("Reopening must start at display setup");
                    return;
                }
                if (state->captures > 0) {
                    return;
                }
                qInfo() << "Output settings checks passed: grouped tabs, operator protection, role "
                           "controls, "
                           "live navigation, speaker tools and unchanged routing/snapshots";
                timer->stop();
                qApp->exit(0);
                return;
            }
            ++state->phase;
        });
    timer->start();
    QTimer::singleShot(20000, &engine, [] { qApp->exit(272); });
}
