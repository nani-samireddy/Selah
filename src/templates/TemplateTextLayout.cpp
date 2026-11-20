#include "TemplateTextLayout.h"
#include <QFont>
#include <QTextBoundaryFinder>
#include <QTextLayout>

bool TemplateTextLayout::fits(const QString &text, const QVariantMap &box, int maximumLines) {
    if (text.isEmpty()) {
        return true;
    }
    QFont font(box.value("fontFamily", "Arial").toString());
    font.setPixelSize(box["fontSize"].toInt());
    const double width = box["width"].toDouble() - 8;
    const double availableHeight = box["height"].toDouble() - 8;
    double height = 0;
    int lines = 0;
    for (const auto &paragraph : text.split('\n')) {
        // An empty paragraph still consumes a rendered line.
        QTextLayout layout(paragraph.isEmpty() ? QStringLiteral(" ") : paragraph, font);
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(option);
        layout.beginLayout();
        bool fit = true;
        while (true) {
            auto line = layout.createLine();
            if (!line.isValid()) {
                break;
            }
            line.setLineWidth(qMax(1.0, width));
            height += line.height();
            if (++lines > maximumLines || height > availableHeight
                || line.naturalTextWidth() > width + 0.5) {
                fit = false;
                break;
            }
        }
        layout.endLayout();
        if (!fit) {
            return false;
        }
    }
    return true;
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
