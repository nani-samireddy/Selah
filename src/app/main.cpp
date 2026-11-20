#include "ApplicationSetup.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "MediaLibrary.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include "RemoteController.h"
#include "TemplateStore.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#if CHURCHDECK_ENABLE_DIAGNOSTICS
#include "DiagnosticRunner.h"
#include "ModelChecks.h"
#endif

int main(int argc, char *argv[]) {
    ApplicationSetup::prepareGraphicsEnvironment();
    QGuiApplication app(argc, argv);
    ApplicationSetup::configure(app);
    ApplicationSetup::registerBundledFonts();
#if CHURCHDECK_ENABLE_DIAGNOSTICS
    if (app.arguments().contains("--self-test")) {
        return runModelChecks();
    }
#endif
    QQuickStyle::setStyle("Basic");
    EditorDocument document(nullptr, !app.arguments().contains("--fresh"));
    MediaLibrary library;
    ContentLibrary content;
    TemplateStore templates;
    OutputManager outputs;
    ProgramStore programs(&outputs);
    NdiController ndi(&outputs);
    RemoteController remote(&outputs, &library);
    // These controllers live on the C++ stack, including when wrapped by diagnostics.
    for (QObject *controller : QList<QObject *> {
             &document, &library, &content, &outputs, &programs, &ndi, &templates, &remote }) {
        QQmlEngine::setObjectOwnership(controller, QQmlEngine::CppOwnership);
    }
    QObject::connect(
        &document, &EditorDocument::mediaImported, &library, &MediaLibrary::registerAsset);
    for (const auto &slide : document.slides()) {
        for (const auto &element : slide.toMap()["elements"].toList()) {
            library.registerAsset(element.toMap());
        }
    }
    QQmlApplicationEngine engine;
    // Required QML properties make the composition root's dependencies explicit.
    engine.setInitialProperties({ { "doc", QVariant::fromValue<QObject *>(&document) },
        { "library", QVariant::fromValue<QObject *>(&library) },
        { "content", QVariant::fromValue<QObject *>(&content) },
        { "programs", QVariant::fromValue<QObject *>(&programs) },
        { "outputs", QVariant::fromValue<QObject *>(&outputs) },
        { "ndi", QVariant::fromValue<QObject *>(&ndi) },
        { "remote", QVariant::fromValue<QObject *>(&remote) },
        { "templates", QVariant::fromValue<QObject *>(&templates) } });
    ndi.setEngine(&engine);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, &remote, &RemoteController::stop);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, &ndi, &NdiController::stop);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("ChurchDeck", "Main");
#if CHURCHDECK_ENABLE_DIAGNOSTICS
    if (const int result = runRequestedDiagnostics(
            app, document, library, content, outputs, ndi, programs, engine)) {
        return result;
    }
#endif
    return app.exec();
}
