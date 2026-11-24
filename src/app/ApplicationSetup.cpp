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
        || app.arguments().contains("--template-presets-smoke")
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
    if (app.arguments().contains("--template-presets-smoke")) {
        app.setApplicationName("Church Deck Template Presets Verification");
    }
    if (app.arguments().contains("--present-smoke")) {
        app.setApplicationName("Church Deck Present Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--background-smoke")) {
        app.setApplicationName("Church Deck Background Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--resize-smoke")) {
        app.setApplicationName("Church Deck Resize Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--font-preview-smoke")) {
        app.setApplicationName("Church Deck Font Preview Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--transition-smoke")) {
        app.setApplicationName("Church Deck Transition Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--output-ux-smoke")) {
        app.setApplicationName("Church Deck Output UX Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--program-browser-smoke")) {
        app.setApplicationName("Church Deck Program Browser Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--menu-smoke")) {
        app.setApplicationName("Church Deck Menu Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--dual-scripture-smoke")) {
        app.setApplicationName("Church Deck Dual Scripture Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
    if (app.arguments().contains("--bible-import-smoke")) {
        app.setApplicationName("Church Deck Bible Import Verification");
        QStandardPaths::setTestModeEnabled(true);
    }
}

void ApplicationSetup::registerBundledFonts() {
    for (const auto &font :
        { "cardo/Cardo-Regular.ttf", "cardo/Cardo-Bold.ttf", "prompt/Prompt-Bold.ttf",
            "public-sans/PublicSans[wght].ttf", "freckle-face/FreckleFace-Regular.ttf" }) {
        QFontDatabase::addApplicationFont(":/qt/qml/ChurchDeck/assets/fonts/" + QString(font));
    }
    QFontDatabase::addApplicationFont(":/qt/qml/ChurchDeck/assets/fonts/Inter.ttf");
    QFontDatabase::addApplicationFont(":/qt/qml/ChurchDeck/assets/fonts/NotoSansTelugu.ttf");
    QFontDatabase::addApplicationFont(":/qt/qml/ChurchDeck/assets/fonts/NotoSansDevanagari.ttf");
    QFontDatabase::addApplicationFallbackFontFamily(QChar::Script_Telugu, "Noto Sans Telugu");
    QFontDatabase::addApplicationFallbackFontFamily(
        QChar::Script_Devanagari, "Noto Sans Devanagari");
}
