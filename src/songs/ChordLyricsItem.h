#pragma once
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

class ChordLyricsItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantMap content READ content WRITE setContent NOTIFY changed)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY changed)
    Q_PROPERTY(QColor chordColor READ chordColor WRITE setChordColor NOTIFY changed)
    Q_PROPERTY(int pixelSize READ pixelSize WRITE setPixelSize NOTIFY changed)
public:
    explicit ChordLyricsItem(QQuickItem *parent = nullptr);

    QVariantMap content() const { return m_content; }

    QColor color() const { return m_color; }

    QColor chordColor() const { return m_chordColor; }

    int pixelSize() const { return m_size; }

    void setContent(const QVariantMap &v) {
        m_content = v;
        update();
        emit changed();
    }

    void setColor(const QColor &v) {
        m_color = v;
        update();
        emit changed();
    }

    void setChordColor(const QColor &v) {
        m_chordColor = v;
        update();
        emit changed();
    }

    void setPixelSize(int v) {
        m_size = qBound(10, v, 120);
        update();
        emit changed();
    }

    void paint(QPainter *painter) override;
signals:
    void changed();

private:
    qreal layout(QPainter *painter, int size);
    QVariantMap m_content;
    QColor m_color = Qt::white, m_chordColor = QColor("#E8C987");
    int m_size = 48;
};
