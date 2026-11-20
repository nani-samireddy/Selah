#pragma once
class QGuiApplication;
class QQmlApplicationEngine;
class EditorDocument;
class MediaLibrary;
class ContentLibrary;
class OutputManager;
class NdiController;
class ProgramStore;
// Schedules an isolated diagnostic, or returns a command-line validation error.
int runRequestedDiagnostics(QGuiApplication &app, EditorDocument &document, MediaLibrary &library,
    ContentLibrary &content, OutputManager &outputs, NdiController &ndi, ProgramStore &programs,
    QQmlApplicationEngine &engine);

int startContentChecks(QGuiApplication &app, EditorDocument &document, MediaLibrary &library,
    ContentLibrary &content, OutputManager &outputs, NdiController &ndi, ProgramStore &programs,
    QQmlApplicationEngine &engine);

int startRoutingChecks(QGuiApplication &app, EditorDocument &document, MediaLibrary &library,
    ContentLibrary &content, OutputManager &outputs, NdiController &ndi, ProgramStore &programs,
    QQmlApplicationEngine &engine);

int startMediaChecks(QGuiApplication &app, EditorDocument &document, MediaLibrary &library,
    ContentLibrary &content, OutputManager &outputs, NdiController &ndi, ProgramStore &programs,
    QQmlApplicationEngine &engine);
