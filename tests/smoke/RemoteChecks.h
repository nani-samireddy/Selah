#pragma once
#include <QString>
class RemoteController;
class OutputManager;
class QQmlApplicationEngine;
// Persistent, loopback-only fixture for browser integration checks; isolated by ApplicationSetup.
void startRemoteDemo(RemoteController &remote, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder);
