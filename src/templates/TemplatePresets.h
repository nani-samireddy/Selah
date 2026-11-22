#pragma once
#include <QString>

class MediaLibrary;
class TemplateStore;

// Installs bundled layouts and durable local media without touching documents or programs.
namespace TemplatePresets {
QString install(
    TemplateStore &store, MediaLibrary *media = nullptr, const QString &directory = QString());
int selfTest();
}
