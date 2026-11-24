#include "FontPreviewItem.h"
#include <algorithm>
#include <QPainter>
#include <QRawFont>
#include <QTextLayout>
#include <QTextOption>

namespace {
constexpr qreal previewFontSize = 30;
constexpr qreal maximumHeight = 8192;
constexpr int maximumCharacters = 4000;

qreal drawPreview(const QByteArray &bytes, const QString &text, qreal width, QPainter *painter) {
    if (bytes.isEmpty() || width < 1) {
        return 0;
    }
    // QRawFont is thread-local. Construct it on the measuring/painting thread,
    // rather than carrying a GUI-thread font engine into the render thread.
    const QRawFont font(bytes, previewFontSize);
    if (!font.isValid()) {
        return 0;
    }
    qreal y = 0;
    for (const auto &paragraph : text.left(maximumCharacters).split('\n')) {
        if (paragraph.isEmpty()) {
            y += previewFontSize * 1.3;
            continue;
        }
        QTextLayout layout(paragraph);
        layout.setRawFont(font);
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(option);
        layout.beginLayout();
        while (true) {
            auto line = layout.createLine();
            if (!line.isValid()) {
                break;
            }
            line.setLineWidth(width);
            line.setPosition(QPointF(0, y));
            y += line.height();
            if (y >= maximumHeight) {
                break;
            }
        }
        layout.endLayout();
        if (painter) {
            layout.draw(painter, QPointF(0, 0));
        }
        if (y >= maximumHeight) {
            break;
        }
    }
    return y;
}
}

FontPreviewItem::FontPreviewItem(QQuickItem *parent)
    : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    connect(this, &QQuickItem::widthChanged, this, &FontPreviewItem::relayout);
}

void FontPreviewItem::setFontBytes(const QByteArray &bytes) {
    if (m_bytes == bytes) {
        return;
    }
    m_bytes = bytes;
    relayout();
}

void FontPreviewItem::setText(const QString &text) {
    if (m_text == text) {
        return;
    }
    m_text = text;
    relayout();
}

void FontPreviewItem::setColor(const QColor &color) {
    if (m_color == color) {
        return;
    }
    m_color = color;
    emit changed();
    update();
}

void FontPreviewItem::relayout() {
    const auto height = drawPreview(m_bytes, m_text, width(), nullptr);
    setImplicitHeight(std::min(maximumHeight, height));
    m_truncated = m_text.size() > maximumCharacters || height >= maximumHeight;
    m_missing = false;
    if (!m_bytes.isEmpty()) {
        const QRawFont raw(m_bytes, previewFontSize);
        for (char32_t code : m_text.left(maximumCharacters).toUcs4()) {
            if (!QChar::isSpace(code) && code != 0x200C && code != 0x200D
                && !raw.supportsCharacter(code)) {
                m_missing = true;
                break;
            }
        }
    }
    emit changed();
    update();
}

void FontPreviewItem::paint(QPainter *painter) {
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setPen(m_color);
    drawPreview(m_bytes, m_text, width(), painter);
}
