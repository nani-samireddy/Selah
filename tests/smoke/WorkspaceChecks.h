#pragma once
#include <QString>
class ContentLibrary;
class EditorDocument;
class OutputManager;
class ProgramStore;
class QQmlApplicationEngine;
void runWorkspaceChecks(ContentLibrary &content, EditorDocument &document, OutputManager &outputs,
    ProgramStore &programs, QQmlApplicationEngine &engine, const QString &folder, bool stayOpen);
