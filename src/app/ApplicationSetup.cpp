#include "ApplicationSetup.h"
#include <QFontDatabase>
#include <QGuiApplication>
#include <QStandardPaths>

void ApplicationSetup::prepareGraphicsEnvironment() {
    // Grayscale glyph filtering keeps transparent NDI text free of LCD color fringes.
    qputenv("QSG_DISTANCEFIELD_ANTIALIASING", "gray");
}

void ApplicationSetup::configure(QGuiApplication &app) {
    app.setOrganizationName("ChurchDeck");
    // Keep the established storage/settings namespace across the Selah rename.
    app.setApplicationName("Church Deck");
    app.setApplicationDisplayName("Selah");
    if (app.arguments().contains("--screenshot")) {
        app.setApplicationName("Church Deck Capture");
    }
    if (app.arguments().contains("--remote-demo") || app.arguments().contains("--template-smoke")
        || app.arguments().contains("--program-smoke")
        || app.arguments().contains("--content-smoke") || app.arguments().contains("--media-smoke")
        || app.arguments().contains("--routing-smoke") || app.arguments().contains("--ndi-smoke")
        || app.arguments().contains("--physical-smoke")
        || app.arguments().contains("--self-test")) {
        app.setApplicationName("Church Deck Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--workspace-demo")
        || app.arguments().contains("--workspace-smoke")) {
        app.setApplicationName("Church Deck UX Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--self-test")) {
        app.setApplicationName("Church Deck Unit Verification");
    }
}

void ApplicationSetup::registerBundledFonts() {
    QFontDatabase::addApplicationFont(":/qt/qml/ChurchDeck/assets/fonts/Inter.ttf");
    QFontDatabase::addApplicationFont(":/qt/qml/ChurchDeck/assets/fonts/NotoSansTelugu.ttf");
    QFontDatabase::addApplicationFont(":/qt/qml/ChurchDeck/assets/fonts/NotoSansDevanagari.ttf");
    QFontDatabase::addApplicationFallbackFontFamily(QChar::Script_Telugu, "Noto Sans Telugu");
    QFontDatabase::addApplicationFallbackFontFamily(
        QChar::Script_Devanagari, "Noto Sans Devanagari");
}
