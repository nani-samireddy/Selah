#pragma once
#include <QString>
#include <QUrl>
#include <QVariantMap>

// Copies a selected image into profile storage; changing it never takes it live.
namespace DefaultImage {
QString prepare(const QUrl &source, QString *error);
QVariantMap layer(const QString &source, const QString &fit);
}
