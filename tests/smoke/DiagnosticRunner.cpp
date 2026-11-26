#include "DiagnosticRunner.h"
#include "AiChecks.h"
#include "BackgroundChecks.h"
#include "BibleImportChecks.h"
#include "ContentLibrary.h"
#include "DualScriptureChecks.h"
#include "EditorDocument.h"
#include "FontLibrary.h"
#include "FontPreviewChecks.h"
#include "MediaLibrary.h"
#include "NativeMenuChecks.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "OutputPanelChecks.h"
#include "PresentChecks.h"
#include "ProgramBrowserChecks.h"
#include "ProgramChecks.h"
#include "ProgramStore.h"
#include "PrototypeChecks.h"
#include "QuickAddChecks.h"
#include "RemoteChecks.h"
#include "RemoteController.h"
#include "ResizeChecks.h"
#include "SlideFactory.h"
#include "StandbyChecks.h"
#include "TemplateChecks.h"
#include "TemplatePresetChecks.h"
#include "TemplateStore.h"
#include "TextFitChecks.h"
#include "TransitionChecks.h"
#include "UpdateSetupChecks.h"
#include "WorkspaceChecks.h"
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

int runRequestedDiagnostics(QGuiApplication &app, EditorDocument &document, MediaLibrary &library,
    ContentLibrary &content, OutputManager &outputs, NdiController &ndi, ProgramStore &programs,
    QQmlApplicationEngine &engine) {
    const int standbyIndex = app.arguments().indexOf("--standby-smoke");
    if (standbyIndex >= 0) {
        if (standbyIndex + 2 >= app.arguments().size()) {
            return 870;
        }
        runStandbyChecks(
            outputs, engine, app.arguments()[standbyIndex + 1], app.arguments()[standbyIndex + 2]);
    }
    const int setupIndex = app.arguments().indexOf("--update-setup-smoke");
    if (setupIndex >= 0) {
        if (setupIndex + 1 >= app.arguments().size()) {
            return 580;
        }
        runUpdateSetupChecks(engine, app.arguments()[setupIndex + 1]);
    }
    const int fitIndex = app.arguments().indexOf("--text-fit-smoke");
    if (fitIndex >= 0) {
        if (fitIndex + 1 >= app.arguments().size()) {
            return 500;
        }
        runTextFitChecks(document, outputs, engine, app.arguments()[fitIndex + 1]);
    }
    const int quickIndex = app.arguments().indexOf("--quick-add-smoke");
    if (quickIndex >= 0) {
        if (quickIndex + 1 >= app.arguments().size()) {
            return 470;
        }
        runQuickAddUiChecks(engine, app.arguments()[quickIndex + 1]);
    }
    if (app.arguments().contains("--compact") && !engine.rootObjects().isEmpty()) {
        auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (window) {
            window->setWidth(1120);
            window->setHeight(760);
        }
    }
#ifdef Q_OS_MACOS
    const int menuIndex = app.arguments().indexOf("--menu-smoke");
    if (menuIndex >= 0) {
        if (menuIndex + 1 >= app.arguments().size()) {
            return 250;
        }
        runNativeMenuChecks(engine, app.arguments()[menuIndex + 1]);
    }
#endif
    const int backgroundIndex = app.arguments().indexOf("--background-smoke");
    if (backgroundIndex >= 0) {
        if (backgroundIndex + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 320;
        }
        auto templates = qobject_cast<TemplateStore *>(
            engine.rootObjects().first()->property("templates").value<QObject *>());
        if (!templates) {
            return 320;
        }
        runBackgroundChecks(
            document, *templates, outputs, ndi, engine, app.arguments()[backgroundIndex + 1]);
    }
    const int aiIndex = app.arguments().indexOf("--ai-smoke");
    if (aiIndex >= 0) {
        const auto args = app.arguments();
        const auto runtimeIndex = args.indexOf("--ai-runtime");
        const auto modelIndex = args.indexOf("--ai-model");
        if (aiIndex + 1 >= args.size() || runtimeIndex < 0 || modelIndex < 0) {
            return 420;
        }
        runAiChecks(document, content, outputs, engine, args[aiIndex + 1],
            args.value(runtimeIndex + 1), args.value(modelIndex + 1));
    }
    const int resizeIndex = app.arguments().indexOf("--resize-smoke");
    if (resizeIndex >= 0) {
        if (resizeIndex + 1 >= app.arguments().size()) {
            return 310;
        }
        runResizeChecks(document, outputs, engine, app.arguments()[resizeIndex + 1]);
    }
    const int fontPreviewIndex = app.arguments().indexOf("--font-preview-smoke");
    if (fontPreviewIndex >= 0) {
        if (fontPreviewIndex + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 300;
        }
        auto fonts = qobject_cast<FontLibrary *>(
            engine.rootObjects().first()->property("fonts").value<QObject *>());
        if (!fonts) {
            return 300;
        }
        runFontPreviewChecks(
            *fonts, document, outputs, engine, app.arguments()[fontPreviewIndex + 1]);
    }
    const int transitionIndex = app.arguments().indexOf("--transition-smoke");
    if (transitionIndex >= 0) {
        if (transitionIndex + 1 >= app.arguments().size()) {
            return 290;
        }
        runTransitionOutputChecks(outputs, ndi, engine, app.arguments()[transitionIndex + 1]);
    }
    const int outputUxIndex = app.arguments().indexOf("--output-ux-smoke");
    if (outputUxIndex >= 0) {
        if (outputUxIndex + 1 >= app.arguments().size()) {
            return 270;
        }
        runOutputPanelChecks(document, outputs, engine, app.arguments()[outputUxIndex + 1]);
    }
    const int browserIndex = app.arguments().indexOf("--program-browser-smoke");
    if (browserIndex >= 0) {
        if (browserIndex + 1 >= app.arguments().size()) {
            return 260;
        }
        runProgramBrowserChecks(
            content, document, outputs, programs, engine, app.arguments()[browserIndex + 1]);
    }
    const int dualIndex = app.arguments().indexOf("--dual-scripture-smoke");
    if (dualIndex >= 0) {
        if (dualIndex + 1 >= app.arguments().size()) {
            return 240;
        }
        runDualScriptureUiChecks(content, programs, engine, app.arguments()[dualIndex + 1]);
    }
    int workspaceIndex = app.arguments().indexOf("--workspace-smoke");
    const int bibleImportIndex = app.arguments().indexOf("--bible-import-smoke");
    if (bibleImportIndex >= 0) {
        if (bibleImportIndex + 1 >= app.arguments().size()) {
            return 230;
        }
        runBibleImportChecks(
            content, document, outputs, engine, app.arguments()[bibleImportIndex + 1]);
    }
    const int presentIndex = app.arguments().indexOf("--present-smoke");
    if (presentIndex >= 0) {
        if (presentIndex + 1 >= app.arguments().size()) {
            return 220;
        }
        runPresentChecks(
            outputs, programs, document, ndi, engine, app.arguments()[presentIndex + 1]);
    }
    const bool workspaceDemo = app.arguments().contains("--workspace-demo");
    if (workspaceDemo) {
        workspaceIndex = app.arguments().indexOf("--workspace-demo");
    }
    if (workspaceIndex >= 0) {
        if (workspaceIndex + 1 >= app.arguments().size()) {
            return 209;
        }
        runWorkspaceChecks(content, document, outputs, programs, engine,
            app.arguments()[workspaceIndex + 1], workspaceDemo);
    }
    const int remoteIndex = app.arguments().indexOf("--remote-demo");
    if (remoteIndex >= 0) {
        if (remoteIndex + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 159;
        }
        auto remote = qobject_cast<RemoteController *>(
            engine.rootObjects()[0]->property("remote").value<QObject *>());
        if (!remote) {
            return 159;
        }
        startRemoteDemo(*remote, outputs, engine, app.arguments()[remoteIndex + 1]);
    }
    const int templateIndex = app.arguments().indexOf("--template-smoke");
    const int presetsIndex = app.arguments().indexOf("--template-presets-smoke");
    if (presetsIndex >= 0) {
        if (presetsIndex + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 270;
        }
        runTemplatePresetChecks(engine, app.arguments()[presetsIndex + 1]);
    }
    if (templateIndex >= 0) {
        if (templateIndex + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 149;
        }
        auto templates = qobject_cast<TemplateStore *>(
            engine.rootObjects()[0]->property("templates").value<QObject *>());
        if (!templates) {
            return 149;
        }
        runTemplateChecks(*templates, programs, document, outputs, engine,
            app.arguments()[templateIndex + 1], app.arguments().contains("--compact"));
    }
    const int programIndex = app.arguments().indexOf("--program-smoke");
    if (programIndex >= 0) {
        if (programIndex + 1 >= app.arguments().size()) {
            return 119;
        }
        runProgramChecks(programs, content, document, outputs, engine,
            app.arguments()[programIndex + 1], app.arguments().contains("--compact"));
    }
    if (const int result
        = startContentChecks(app, document, library, content, outputs, ndi, programs, engine)) {
        return result;
    }
    const int physicalIndex = app.arguments().indexOf("--physical-smoke");
    if (physicalIndex >= 0) {
        if (physicalIndex + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 50;
        }
        runPhysicalDisplayChecks(
            ndi, outputs, document, engine, app.arguments()[physicalIndex + 1]);
    }
    const int ndiIndex = app.arguments().indexOf("--ndi-smoke");
    if (ndiIndex >= 0) {
        if (ndiIndex + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 40;
        }
        runNdiCaptureChecks(ndi, outputs, document, engine, app.arguments()[ndiIndex + 1]);
    }
    if (const int result
        = startRoutingChecks(app, document, library, content, outputs, ndi, programs, engine)) {
        return result;
    }
    if (const int result
        = startMediaChecks(app, document, library, content, outputs, ndi, programs, engine)) {
        return result;
    }
    const int captureIndex = app.arguments().indexOf("--screenshot");
    if (captureIndex >= 0 && captureIndex + 1 < app.arguments().size()) {
        QTimer::singleShot(1500, &app, [&, captureIndex] {
            auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
            const bool saved
                = window && window->grabWindow().save(app.arguments()[captureIndex + 1]);
            app.exit(saved ? 0 : 2);
        });
    }

    return 0;
}
