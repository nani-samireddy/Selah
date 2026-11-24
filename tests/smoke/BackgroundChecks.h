#pragma once
#include <QString>
class EditorDocument;
class TemplateStore;
class OutputManager;
class NdiController;
class QQmlApplicationEngine;
void runBackgroundChecks(EditorDocument &document, TemplateStore &templates, OutputManager &outputs,
    NdiController &ndi, QQmlApplicationEngine &engine, const QString &folder);
