#include "ApplicationSetup.h"
#include "FontLibrary.h"
#include "ModelChecks.h"
#include <QGuiApplication>
#include <QStandardPaths>
int runBibleXmlChecks(const QString &realFile);

int main(int argc, char **argv) {
    ApplicationSetup::prepareGraphicsEnvironment();
    QGuiApplication app(argc, argv);
    app.setOrganizationName("ChurchDeck");
    // Legacy test-data namespace; the product display name is Selah.
    app.setApplicationName("Church Deck Unit Verification");
    QStandardPaths::setTestModeEnabled(true);
    ApplicationSetup::registerBundledFonts();
    const int bibleIndex = app.arguments().indexOf("--bible-xml-file");
    if (bibleIndex >= 0) {
        return runBibleXmlChecks(app.arguments().value(bibleIndex + 1));
    }
    const int networkIndex = app.arguments().indexOf("--fonts-network");
    if (networkIndex >= 0) {
        return FontLibrary::networkSelfTest(app.arguments().value(networkIndex + 1));
    }
    return runModelChecks();
}
