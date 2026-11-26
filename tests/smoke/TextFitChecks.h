#pragma once
#include <QString>
class QQmlApplicationEngine;
class EditorDocument;
class OutputManager;
void runTextFitChecks(EditorDocument &document, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder);
