#pragma once
#include <QByteArray>
#include <QVariantMap>

namespace RemotePreview {
// Bounded still previews; video uses catalog posters, never a second playback pipeline.
QByteArray render(const QVariantMap &slide, const QVariantList &assets);
QString qrDataUrl(const QString &text);
}
