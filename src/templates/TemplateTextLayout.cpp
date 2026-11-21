#include "TemplateTextLayout.h"
#include "TextLayoutMetrics.h"
#include <QFont>
#include <QTextBoundaryFinder>
#include <QTextLayout>

bool TemplateTextLayout::fits(const QString &text, const QVariantMap &box, int maximumLines) {
    if (text.isEmpty()) {
        return true;
    }
    const double width = qMax(1.0, box["width"].toDouble() - 8);
    const double availableHeight = box["height"].toDouble() - 8;
    const auto layout = TextLayoutMetrics::measure(text, box, width);
    const int displayLimit = box.value("maxLines", 0).toInt();
    const int lines = displayLimit > 0 ? qMin(maximumLines, displayLimit) : maximumLines;
    return layout.lines <= lines && layout.height <= availableHeight && layout.width <= width + 0.5;
}

bool TemplateTextLayout::fitsBinding(
    const QString &text, const QString &binding, const QVariantMap &definition, int maximumLines) {
    for (const auto &value : definition["slide"].toMap()["elements"].toList()) {
        const auto box = value.toMap();
        if (box["binding"] == binding && box.value("visible", true).toBool()
            && box.value("opacity", 1).toDouble() > 0 && !fits(text, box, maximumLines)) {
            return false;
        }
    }
    return true;
}

QString TemplateTextLayout::takePrefix(
    QString &remaining, const QString &binding, const QVariantMap &definition, int maximumLines) {
    if (fitsBinding(remaining, binding, definition, maximumLines)) {
        const auto result = remaining;
        remaining.clear();
        return result;
    }
    // Search only at grapheme boundaries; splitting a UTF-16 code unit can corrupt a conjunct.
    QVector<int> boundaries;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, remaining);
    while (finder.toNextBoundary() >= 0) {
        boundaries << finder.position();
    }
    int low = 0, high = boundaries.size() - 1, best = 0;
    while (low <= high) {
        const int middle = (low + high) / 2;
        const int length = boundaries[middle];
        if (fitsBinding(remaining.left(length), binding, definition, maximumLines)) {
            best = length;
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    if (best == 0) {
        return {};
    }
    // Prefer a complete word when a page boundary falls within a long lyric/verse line.
    int wordEnd = 0;
    for (int boundary : boundaries) {
        if (boundary > best) {
            break;
        }
        if (remaining[boundary - 1].isSpace()) {
            wordEnd = boundary;
        }
    }
    if (wordEnd > 0) {
        best = wordEnd;
    }
    const auto result = remaining.left(best);
    remaining.remove(0, best);
    return result;
}
