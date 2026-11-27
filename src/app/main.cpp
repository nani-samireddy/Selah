#include "AiAssistant.h"
#include "AppBackup.h"
#include "ApplicationSetup.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "FontLibrary.h"
#include "LiveSessionStore.h"
#include "MediaLibrary.h"
#include "ModelInstaller.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "PresentationLibrary.h"
#include "ProgramStore.h"
#include "QuickAddController.h"
#include "RemoteController.h"
#include "ServiceCues.h"
#include "TemplatePresets.h"
#include "TemplateStore.h"
#include "TextFormatting.h"
#include "UpdateManager.h"
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtQml/QQmlExtensionPlugin>
Q_IMPORT_QML_PLUGIN(ChurchDeckFormattingPlugin)
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QWindow>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
#ifdef Q_OS_MACOS
#include "WindowChrome.h"
#endif
#if CHURCHDECK_ENABLE_DIAGNOSTICS
#include "DiagnosticRunner.h"
#include "ModelChecks.h"
#endif

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    // Inno Setup checks this marker before install/uninstall. Retain the handle
    // until process exit so setup never automatically closes a live service.
    if (!CreateMutexW(nullptr, FALSE, L"Selah.Desktop.Running")) {
        return 1;
    }
#endif
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
    const auto profileRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(QFileInfo(profileRoot).absolutePath());
    QLockFile profileLock(profileRoot + ".running.lock");
    if (!profileLock.tryLock(10000)) {
        qWarning() << "Selah is already running for this profile";
        return 0;
    }
    if (const auto error = AppBackup::applyPendingRestore(profileRoot); !error.isEmpty()) {
        qCritical().noquote() << error;
        return 1;
    }
    QQuickStyle::setStyle("Basic");
    AppBackup backup;
    FontLibrary fonts;
    EditorDocument document(nullptr, !app.arguments().contains("--fresh"));
    MediaLibrary library;
    // The end-to-end importer check must start with an empty, disposable scripture catalog.
    QTemporaryDir bibleImportDirectory;
    ContentLibrary content(nullptr,
        app.arguments().contains("--bible-import-smoke") ? bibleImportDirectory.path() : QString());
    AiAssistant assistant(&content);
    ModelInstaller modelInstaller;
    UpdateManager updater;
    QObject::connect(&modelInstaller, &ModelInstaller::installed, &assistant, [&] {
        const auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + "/ai/cactus-4.0.0/";
#ifdef Q_OS_WIN
        const auto runtime = path + "cactus.exe";
#else
        const auto runtime = path + "cactus";
#endif
        assistant.configure(
            QUrl::fromLocalFile(runtime), QUrl::fromLocalFile(path + "needle3.cact"));
    });
    if (app.arguments().contains("--enable-ai")) {
        assistant.useManagedModel();
    }
    TemplateStore templates;
    if (!templates.retireLegacyPresets()) {
        qWarning().noquote() << templates.status();
    }
    if (const auto error = TemplatePresets::install(templates, &library); !error.isEmpty()) {
        qWarning().noquote() << error;
    }
    PresentationLibrary presentations;
    OutputManager outputs;
    LiveSessionStore recovery(&outputs);
    ServiceCues serviceCues(&outputs);
    QObject::connect(
        &app, &QGuiApplication::aboutToQuit, &recovery, &LiveSessionStore::finishCleanly);
    QuickAddController quickAdd(&content, &presentations, &library, &outputs);
    QTemporaryDir browserDirectory;
    ProgramStore programs(&outputs, nullptr,
        app.arguments().contains("--program-browser-smoke") ? browserDirectory.path() : QString());
    NdiController ndi(&outputs);
    RemoteController remote(&outputs, &library);
    QObject::connect(&recovery, &LiveSessionStore::restoring, &ndi, &NdiController::stop);
    // These controllers live on the C++ stack, including when wrapped by diagnostics.
    for (QObject *controller : QList<QObject *> { &document, &library, &content, &outputs,
             &programs, &ndi, &templates, &presentations, &remote, &fonts, &assistant, &quickAdd,
             &modelInstaller, &updater, &recovery, &serviceCues, &backup }) {
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
    engine.setInitialProperties({ { "assistant", QVariant::fromValue<QObject *>(&assistant) },
        { "quickAdd", QVariant::fromValue<QObject *>(&quickAdd) },
        { "recovery", QVariant::fromValue<QObject *>(&recovery) },
        { "backup", QVariant::fromValue<QObject *>(&backup) },
        { "serviceCues", QVariant::fromValue<QObject *>(&serviceCues) },
        { "updater", QVariant::fromValue<QObject *>(&updater) },
        { "modelInstaller", QVariant::fromValue<QObject *>(&modelInstaller) },
        { "presentations", QVariant::fromValue<QObject *>(&presentations) },
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
    if (app.arguments().contains("--enable-ai") && !engine.rootObjects().isEmpty()) {
        QMetaObject::invokeMethod(
            engine.rootObjects().first(), "showAiAssistant", Qt::QueuedConnection);
    }
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
