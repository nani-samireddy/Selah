#pragma once
#include <QByteArray>
#include <QString>
#include <QUrl>

// Each import owns its SQLite connection on the worker thread.
namespace ScriptureImporter {
QString importFile(const QString &databasePath, const QUrl &source);
QString importData(const QString &databasePath, const QByteArray &data, const QUrl &source);
}
