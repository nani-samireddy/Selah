#include "ChordLyricsItem.h"
#include "SlideFactory.h"
#include <QFontMetricsF>
#include <QPainter>
#include <QTextLayout>

ChordLyricsItem::ChordLyricsItem(QQuickItem *parent)
    : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setClip(true);
}

qreal ChordLyricsItem::layout(QPainter *painter, int size) {
    const auto text = m_content["text"].toString().left(30000);
    const auto anchors = m_content["chords"].toList();
    QFont font(SlideFactory::fontFor(text));
    font.setPixelSize(size);
    QFont chordFont("Inter");
    chordFont.setPixelSize(qMax(9, qRound(size * .57)));
    chordFont.setWeight(QFont::DemiBold);
    QFontMetricsF chordMetrics(chordFont);
    qreal y = 0;
    int offset = 0;
    for (const auto &paragraph : text.split('\n')) {
        QTextLayout block(paragraph, font);
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        block.setTextOption(option);
        block.beginLayout();
        QList<QTextLine> lines;
        while (true) {
            auto line = block.createLine();
            if (!line.isValid()) {
                break;
            }
            line.setLineWidth(qMax(1., width()));
            lines.append(line);
        }
        block.endLayout();
        for (const auto &line : lines) {
            struct Label {
                QString text;
                qreal x;
                int lane;
            };

            QList<Label> labels;
            QList<qreal> ends;
            for (const auto &value : anchors) {
                const auto anchor = value.toMap();
                const int at = anchor["offset"].toInt() - offset;
                if (at < line.textStart() || at >= line.textStart() + line.textLength()) {
                    continue;
                }
                const auto label = anchor["label"].toString().left(40);
                const qreal labelWidth = chordMetrics.horizontalAdvance(label);
                const qreal x = qBound(0., line.cursorToX(at), qMax(0., width() - labelWidth));
                int lane = 0;
                while (lane < ends.size() && x < ends[lane] + size * .12) {
                    ++lane;
                }
                if (lane == ends.size()) {
                    ends.append(0);
                }
                ends[lane] = x + labelWidth;
                labels.append({ label, x, lane });
            }
            const qreal chordHeight = ends.size() * (chordMetrics.height() + 2);
            if (painter) {
                painter->setPen(m_chordColor);
                painter->setFont(chordFont);
                for (const auto &label : labels) {
                    painter->drawText(
                        QPointF(label.x,
                            y + label.lane * (chordMetrics.height() + 2) + chordMetrics.ascent()),
                        label.text);
                }
                painter->setPen(m_color);
                line.draw(painter, QPointF(0, y + chordHeight));
            }
            y += chordHeight + line.height() + size * .14;
        }
        if (lines.isEmpty()) {
            y += size * .5;
        }
        offset += paragraph.size() + 1;
    }
    return y;
}

void ChordLyricsItem::paint(QPainter *painter) {
    painter->setRenderHint(QPainter::TextAntialiasing);
    int size = m_size;
    while (size > 10 && layout(nullptr, size) > height()) {
        size -= 2;
    }
    layout(painter, size);
}
