#pragma once
#include <QString>
class OutputManager;
class ProgramStore;
class EditorDocument;
class NdiController;
class QQmlApplicationEngine;
void runPresentChecks(OutputManager &outputs, ProgramStore &programs, EditorDocument &document,
    NdiController &ndi, QQmlApplicationEngine &engine, const QString &folder);
