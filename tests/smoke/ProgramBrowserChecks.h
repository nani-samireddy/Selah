#pragma once
#include <QString>
class ContentLibrary;
class EditorDocument;
class OutputManager;
class ProgramStore;
class QQmlApplicationEngine;
void runProgramBrowserChecks(ContentLibrary &content, EditorDocument &document,
    OutputManager &outputs, ProgramStore &programs, QQmlApplicationEngine &engine,
    const QString &folder);
