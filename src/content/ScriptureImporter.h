#pragma once
#include <QString>
#include <QUrl>

// Each import owns its SQLite connection on the worker thread.
namespace ScriptureImporter {
QString importFile(const QString &databasePath, const QUrl &source);
}
