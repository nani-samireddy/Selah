#pragma once
#include <QString>
#include <QUrl>
#include <QVariantMap>

// Copies selected standby media into profile storage; changing it never takes it live.
namespace DefaultImage {
QString kind(const QUrl &source);
QString prepare(const QUrl &source, QString *error);
QVariantMap layer(const QString &source, const QString &fit);
}
