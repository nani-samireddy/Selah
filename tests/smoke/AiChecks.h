#pragma once
#include <QString>
class EditorDocument;
class ContentLibrary;
class OutputManager;
class QQmlApplicationEngine;
void runAiChecks(EditorDocument &, ContentLibrary &, OutputManager &, QQmlApplicationEngine &,
    const QString &, const QString &, const QString &);
