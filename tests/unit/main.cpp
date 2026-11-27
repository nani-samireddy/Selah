#include "ApplicationSetup.h"
#include "FontLibrary.h"
#include "ModelChecks.h"
#include "TemplatePresets.h"
#include <QGuiApplication>
#include <QStandardPaths>
int exportBundleExchangeFixture(const QString &path);
int importBundleExchangeFixture(const QString &path, const QString &folder);
int runBibleXmlChecks(const QString &realFile);

int main(int argc, char **argv) {
    ApplicationSetup::prepareGraphicsEnvironment();
    QGuiApplication app(argc, argv);
    app.setOrganizationName("ChurchDeck");
    // Legacy test-data namespace; the product display name is Selah.
    app.setApplicationName("Church Deck Unit Verification");
    QStandardPaths::setTestModeEnabled(true);
    ApplicationSetup::registerBundledFonts();
    if (app.arguments().contains("--template-presets")) {
        return TemplatePresets::selfTest();
    }
    const int bibleIndex = app.arguments().indexOf("--bible-xml-file");
    if (bibleIndex >= 0) {
        return runBibleXmlChecks(app.arguments().value(bibleIndex + 1));
    }
    const int networkIndex = app.arguments().indexOf("--fonts-network");
    if (networkIndex >= 0) {
        return FontLibrary::networkSelfTest(app.arguments().value(networkIndex + 1));
    }
    const int exportIndex = app.arguments().indexOf("--bundle-exchange-export");
    if (exportIndex >= 0) {
        return exportBundleExchangeFixture(app.arguments().value(exportIndex + 1));
    }
    const int importIndex = app.arguments().indexOf("--bundle-exchange-import");
    if (importIndex >= 0) {
        return importBundleExchangeFixture(
            app.arguments().value(importIndex + 1), app.arguments().value(importIndex + 2));
    }
    return runModelChecks();
}
