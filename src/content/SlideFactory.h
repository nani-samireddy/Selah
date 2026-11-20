#pragma once
#include <QFont>
#include <QVariantList>

namespace SlideFactory {
QString fontFor(const QString &text);
QVariantList songSlides(
    const QVariantMap &song, int maxLines, QString *error, const QVariantMap &definition = {});
QVariantList scriptureSlides(
    const QVariantList &verses, int maxLines, QString *error, const QVariantMap &definition = {});
int selfTest();
}
