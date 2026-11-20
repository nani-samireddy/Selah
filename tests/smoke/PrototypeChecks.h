#pragma once
#include <QString>
class NdiController;
class OutputManager;
class EditorDocument;
class QQmlApplicationEngine;
void runNdiCaptureChecks(NdiController &ndi, OutputManager &outputs, EditorDocument &document,
    QQmlApplicationEngine &engine, const QString &folder);
void runPhysicalDisplayChecks(NdiController &ndi, OutputManager &outputs, EditorDocument &document,
    QQmlApplicationEngine &engine, const QString &folder);
