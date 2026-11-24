#pragma once
#include <QString>
class FontLibrary;
class EditorDocument;
class OutputManager;
class QQmlApplicationEngine;
void runFontPreviewChecks(FontLibrary &fonts, EditorDocument &document, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder);
