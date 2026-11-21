#pragma once
#include <QString>
#include <QVariantMap>

namespace TextLayoutMetrics {
struct Result {
    double width = 0;
    double height = 0;
    int lines = 0;
};

// Measure through the same rich-text layout used by previews and output.
Result measure(const QString &text, const QVariantMap &element, double width);
}
