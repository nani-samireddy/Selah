#pragma once
#include <QString>
class EditorDocument;
class OutputManager;
class QQmlApplicationEngine;
void runOutputPanelChecks(EditorDocument &document, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder);
