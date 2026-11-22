#include "DiagnosticRunner.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "MediaLibrary.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "ProgramChecks.h"
#include "ProgramStore.h"
#include "PrototypeChecks.h"
#include "RemoteChecks.h"
#include "RemoteController.h"
#include "SlideFactory.h"
#include "TemplateChecks.h"
#include "TemplatePresetChecks.h"
#include "TemplateStore.h"
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
    if (app.arguments().contains("--compact") && !engine.rootObjects().isEmpty()) {
        auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (window) {
            window->setWidth(1120);
            window->setHeight(760);
        }
    }
    int workspaceIndex = app.arguments().indexOf("--workspace-smoke");
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
