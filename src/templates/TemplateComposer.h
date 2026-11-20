#pragma once
#include <QVariantList>

namespace TemplateComposer {
QVariantList songSlides(
    const QVariantMap &song, int maximumLines, const QVariantMap &definition, QString *error);
QVariantList scriptureSlides(
    const QVariantList &verses, int maximumLines, const QVariantMap &definition, QString *error);
}
