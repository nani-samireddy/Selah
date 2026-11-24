#pragma once
#include <QString>
class OutputManager;
class NdiController;
class QQmlApplicationEngine;
void runTransitionOutputChecks(OutputManager &outputs, NdiController &ndi,
    QQmlApplicationEngine &engine, const QString &folder);
