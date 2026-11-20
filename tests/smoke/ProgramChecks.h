#pragma once
#include <QString>
class ProgramStore;
class ContentLibrary;
class EditorDocument;
class OutputManager;
class QQmlApplicationEngine;
void runProgramChecks(ProgramStore &programs, ContentLibrary &library, EditorDocument &editor,
    OutputManager &outputs, QQmlApplicationEngine &engine, const QString &folder, bool compact);
