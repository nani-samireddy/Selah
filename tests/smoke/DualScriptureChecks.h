#pragma once
#include <QString>
class ContentLibrary;
class ProgramStore;
class QQmlApplicationEngine;
void runDualScriptureUiChecks(ContentLibrary &content, ProgramStore &programs,
    QQmlApplicationEngine &engine, const QString &folder);
