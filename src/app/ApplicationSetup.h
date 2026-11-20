#pragma once
class QGuiApplication;

namespace ApplicationSetup {
void prepareGraphicsEnvironment();
void configure(QGuiApplication &app);
void registerBundledFonts();
}
