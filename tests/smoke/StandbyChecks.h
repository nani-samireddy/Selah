#pragma once
#include <QString>
class OutputManager;
class QQmlApplicationEngine;
void runStandbyChecks(OutputManager &outputs, QQmlApplicationEngine &engine, const QString &video,
    const QString &folder);
