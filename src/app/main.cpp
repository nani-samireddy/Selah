#include "ApplicationSetup.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "FontLibrary.h"
#include "MediaLibrary.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "PresentationLibrary.h"
#include "ProgramStore.h"
#include "RemoteController.h"
#include "TemplatePresets.h"
#include "TemplateStore.h"
#include "TextFormatting.h"
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtQml/QQmlExtensionPlugin>
Q_IMPORT_QML_PLUGIN(ChurchDeckFormattingPlugin)
#include <QWindow>
#ifdef Q_OS_MACOS
#include "WindowChrome.h"
#endif
#if CHURCHDECK_ENABLE_DIAGNOSTICS
#include "DiagnosticRunner.h"
#include "ModelChecks.h"
#endif

int main(int argc, char *argv[]) {
    ApplicationSetup::prepareGraphicsEnvironment();
    QGuiApplication app(argc, argv);
    ApplicationSetup::configure(app);
    ApplicationSetup::registerBundledFonts();
    QFont interfaceFont("Inter");
    interfaceFont.setPixelSize(12);
    app.setFont(interfaceFont);
    app.setWindowIcon(QIcon(":/qt/qml/ChurchDeck/assets/branding/app-icon.png"));
#if CHURCHDECK_ENABLE_DIAGNOSTICS
    if (app.arguments().contains("--self-test")) {
        return runModelChecks();
    }
#endif
    QQuickStyle::setStyle("Basic");
    FontLibrary fonts;
    EditorDocument document(nullptr, !app.arguments().contains("--fresh"));
    MediaLibrary library;
    ContentLibrary content;
    TemplateStore templates;
    if (const auto error = TemplatePresets::install(templates, &library); !error.isEmpty()) {
        qWarning().noquote() << error;
    }
    PresentationLibrary presentations;
    OutputManager outputs;
    ProgramStore programs(&outputs);
    NdiController ndi(&outputs);
    RemoteController remote(&outputs, &library);
    // These controllers live on the C++ stack, including when wrapped by diagnostics.
    for (QObject *controller : QList<QObject *> { &document, &library, &content, &outputs,
             &programs, &ndi, &templates, &presentations, &remote, &fonts }) {
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
    QString startupWorkspace = "programs";
#if CHURCHDECK_ENABLE_DIAGNOSTICS
    // Existing visual fixtures start at the canvas; normal launches start with service preparation.
    for (const auto &argument : app.arguments()) {
        if (argument.endsWith("-smoke") || argument == "--screenshot") {
            startupWorkspace = "design";
        }
    }
#endif
    // Required QML properties make the composition root's dependencies explicit.
    engine.setInitialProperties(
        { { "presentations", QVariant::fromValue<QObject *>(&presentations) },
            { "startupWorkspace", startupWorkspace },
            { "doc", QVariant::fromValue<QObject *>(&document) },
            { "library", QVariant::fromValue<QObject *>(&library) },
            { "content", QVariant::fromValue<QObject *>(&content) },
            { "programs", QVariant::fromValue<QObject *>(&programs) },
            { "outputs", QVariant::fromValue<QObject *>(&outputs) },
            { "ndi", QVariant::fromValue<QObject *>(&ndi) },
            { "remote", QVariant::fromValue<QObject *>(&remote) },
            { "fonts", QVariant::fromValue<QObject *>(&fonts) },
            { "templates", QVariant::fromValue<QObject *>(&templates) } });
    ndi.setEngine(&engine);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, &remote, &RemoteController::stop);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, &ndi, &NdiController::stop);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("ChurchDeck", "Main");
#ifdef Q_OS_MACOS
    if (!engine.rootObjects().isEmpty()) {
        WindowChrome::configure(qobject_cast<QWindow *>(engine.rootObjects().first()));
    }
#endif
#if CHURCHDECK_ENABLE_DIAGNOSTICS
    if (const int result = runRequestedDiagnostics(
            app, document, library, content, outputs, ndi, programs, engine)) {
        return result;
    }
#endif
    return app.exec();
}
