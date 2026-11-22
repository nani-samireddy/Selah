#pragma once
#include <functional>
#include <QByteArray>
#include <QUrl>
#include <QVariantList>
class QObject;

namespace BibleRepository {
QVariantList catalog();
QUrl rawUrl(const QUrl &source);
void download(const QUrl &source, QObject *owner,
    std::function<void(const QByteArray &, const QUrl &, const QString &)> finished,
    std::function<void(qint64, qint64)> progress = {});
}
