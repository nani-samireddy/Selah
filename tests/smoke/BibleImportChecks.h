#pragma once
#include <QString>
class ContentLibrary;
class EditorDocument;
class OutputManager;
class QQmlApplicationEngine;
void runBibleImportChecks(ContentLibrary &content, EditorDocument &document, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder);
