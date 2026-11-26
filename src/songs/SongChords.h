#pragma once
#include <QVariantList>
#include <QVariantMap>

namespace SongChords {
QVariantMap parse(const QString &source);
QVariantMap importChordPro(const QString &source, const QString &fallbackTitle, QString *error);
QVariantMap cleanSong(const QVariantMap &song);
void annotate(QVariantList &slides, const QVariantMap &song);
}
