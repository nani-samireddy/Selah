#pragma once
#include <QString>
class ProgramStore;
class TemplateStore;
class EditorDocument;
class OutputManager;
class QQmlApplicationEngine;
void runTemplateChecks(TemplateStore &templates, ProgramStore &programs, EditorDocument &editor,
    OutputManager &outputs, QQmlApplicationEngine &engine, const QString &folder, bool compact);
