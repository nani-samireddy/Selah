#include "ApplicationSetup.h"
#include "ModelChecks.h"
#include <QGuiApplication>
#include <QStandardPaths>

int main(int argc, char **argv) {
    ApplicationSetup::prepareGraphicsEnvironment();
    QGuiApplication app(argc, argv);
    app.setOrganizationName("ChurchDeck");
    app.setApplicationName("Church Deck Unit Verification");
    QStandardPaths::setTestModeEnabled(true);
    ApplicationSetup::registerBundledFonts();
    return runModelChecks();
}
