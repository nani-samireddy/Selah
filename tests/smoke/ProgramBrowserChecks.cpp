#include "ProgramBrowserChecks.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include "VisualLookup.h"
#include <memory>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QTimer>

void runProgramBrowserChecks(ContentLibrary &content, EditorDocument &document,
    OutputManager &outputs, ProgramStore &programs, QQmlApplicationEngine &engine,
    const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window || !programs.createProgram("Sunday Worship", "2026-11-22")
        || !programs.addScripture(content.passage("engwebp", "JHN", 3, 16, 17), 4)) {
        qApp->exit(260);
        return;
    }
    const auto sundayId = programs.id();
    if (!programs.createProgram("Prayer evening", "2026-11-25")
        || !programs.addPresentation(document.slides(), "Welcome") || !programs.save()) {
        qApp->exit(260);
        return;
    }
    const auto prayerId = programs.id();
    outputs.takeLive(document.slides(), 0, "browser-live-baseline");
    const auto baseline = outputs.snapshotId();
    QDir().mkpath(folder);
    window->resize(1280, 900);
    QMetaObject::invokeMethod(window, "switchWorkspace", Q_ARG(QVariant, QVariant("programs")));

    struct State {
        int phase = 0;
        int pendingCaptures = 0;
    };

    auto state = std::make_shared<State>();
    auto timer = new QTimer(&engine);
    timer->setInterval(600);
    QObject::connect(timer, &QTimer::timeout, &engine,
        [&, state, timer, window, sundayId, prayerId, baseline, folder] {
            auto fail = [&](const QString &message) {
                qCritical() << "Program browser check failed:" << message;
                timer->stop();
                qApp->exit(261);
            };
            auto click = [&](const QString &name) {
                auto button = VisualLookup::find(window->contentItem(), name);
                if (!button) {
                    fail("Missing control: " + name);
                    return false;
                }
                return QMetaObject::invokeMethod(button, "clicked");
            };
            auto capture = [&](const QString &name) {
                const auto grab = window->contentItem()->grabToImage();
                if (!grab) {
                    fail("Capture failed");
                    return;
                }
                ++state->pendingCaptures;
                QObject::connect(
                    grab.data(), &QQuickItemGrabResult::ready, &engine,
                    [grab, state, folder, name] {
                        grab->saveToFile(folder + "/" + name + ".png");
                        --state->pendingCaptures;
                    },
                    Qt::SingleShotConnection);
            };
            auto panel = window->findChild<QObject *>("programPanel");
            auto browser = VisualLookup::find(window->contentItem(), "programBrowser");
            if (!panel || !browser || outputs.snapshotId() != baseline) {
                fail("Browser missing or live output changed");
                return;
            }
            if (state->phase == 0) {
                if (panel->property("editorOpen").toBool() || !browser->isVisible()
                    || !VisualLookup::find(window->contentItem(), "programCard_" + sundayId)
                    || !VisualLookup::find(window->contentItem(), "programCard_" + prayerId)) {
                    fail("Programs must start at the card grid");
                    return;
                }
                capture("program-cards");
            } else if (state->phase == 1) {
                if (!click("programCard_" + sundayId)) {
                    return;
                }
            } else if (state->phase == 2) {
                auto heading = VisualLookup::find(window->contentItem(), "programHeading");
                if (!panel->property("editorOpen").toBool() || browser->isVisible()
                    || programs.id() != sundayId || !heading
                    || heading->property("text") != "Sunday Worship"
                    || window->findChild<QObject *>("programTitle")) {
                    fail("Card must open one clean editor heading");
                    return;
                }
                capture("program-editor");
            } else if (state->phase == 3) {
                if (!click("editProgramDetailsButton")) {
                    return;
                }
            } else if (state->phase == 4) {
                auto dialog = window->findChild<QObject *>("editProgramDialog");
                if (!dialog || !dialog->property("opened").toBool()) {
                    fail("Edit details dialog did not open");
                    return;
                }
                dialog->findChild<QObject *>("editProgramTitle")
                    ->setProperty("text", "Sunday Celebration");
                dialog->findChild<QObject *>("programDetailsDate")
                    ->setProperty("text", "2026-11-29");
                if (!click("saveProgramDetailsButton")) {
                    return;
                }
            } else if (state->phase == 5) {
                if (programs.title() != "Sunday Celebration" || programs.date() != "2026-11-29") {
                    fail("Details were not saved");
                    return;
                }
                if (!click("allProgramsButton")) {
                    return;
                }
            } else if (state->phase == 6) {
                if (panel->property("editorOpen").toBool() || !browser->isVisible()) {
                    fail("Back must restore the card grid");
                    return;
                }
                auto search = VisualLookup::find(window->contentItem(), "programSearch");
                search->setProperty("text", "Celebration");
            } else if (state->phase == 7) {
                const auto raw = browser->property("records");
                const auto records = raw.value<QJSValue>().toVariant().toList();
                if (records.size() != 1 || records.first().toMap()["id"] != sundayId) {
                    fail("Search did not reflect the renamed program");
                    return;
                }
                if (!click("createProgramButton")) {
                    return;
                }
            } else if (state->phase == 8) {
                auto dialog = window->findChild<QObject *>("newProgramDialog");
                if (!dialog || !dialog->property("opened").toBool()) {
                    fail("New program dialog did not open");
                    return;
                }
                dialog->findChild<QObject *>("newProgramTitle")
                    ->setProperty("text", "Sunday Evening");
                dialog->findChild<QObject *>("programDetailsDate")
                    ->setProperty("text", "2026-11-29");
                if (!click("createProgramConfirm")) {
                    return;
                }
            } else if (state->phase == 9) {
                if (!panel->property("editorOpen").toBool() || programs.title() != "Sunday Evening"
                    || programs.programs().size() != 3) {
                    fail("Creating a program must open its editor");
                    return;
                }
                QMetaObject::invokeMethod(window, "newProgram");
            } else if (state->phase == 10) {
                auto dialog = window->findChild<QObject *>("newProgramDialog");
                if (!dialog || !dialog->property("opened").toBool()) {
                    fail("File New must open the create dialog");
                    return;
                }
                QMetaObject::invokeMethod(dialog, "close");
            } else if (state->phase == 11) {
                if (!panel->property("editorOpen").toBool()
                    || programs.title() != "Sunday Evening") {
                    fail("Cancelling New must preserve the open editor");
                    return;
                }
                if (!programs.openProgram(sundayId) || programs.title() != "Sunday Celebration") {
                    fail("Renamed details did not persist");
                    return;
                }
                if (state->pendingCaptures > 0) {
                    return;
                }
                qInfo() << "Program browser checks passed: cards, open/back, rename/date, search, "
                           "creation and unchanged live snapshot";
                timer->stop();
                qApp->exit(0);
                return;
            }
            ++state->phase;
        });
    timer->start();
    QTimer::singleShot(20000, &engine, [] {
        qCritical() << "Program browser check timed out";
        qApp->exit(262);
    });
}
