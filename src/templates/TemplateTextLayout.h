#pragma once
#include <QString>
#include <QVariantMap>

namespace TemplateTextLayout {
// Measurements use shaped QTextLayout lines, including fallback fonts for Indic scripts.
bool fits(const QString &text, const QVariantMap &box, int maximumLines = 10000);
bool fitsBinding(
    const QString &text, const QString &binding, const QVariantMap &definition, int maximumLines);
QString takePrefix(
    QString &remaining, const QString &binding, const QVariantMap &definition, int maximumLines);
}
