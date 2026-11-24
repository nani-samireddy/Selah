#pragma once
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

// Renders downloaded preview bytes without registering a family application-wide.
class FontPreviewItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QByteArray fontBytes READ fontBytes WRITE setFontBytes NOTIFY changed)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY changed)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY changed)
    Q_PROPERTY(bool missingCharacters READ missingCharacters NOTIFY changed)
    Q_PROPERTY(bool truncated READ truncated NOTIFY changed)
public:
    explicit FontPreviewItem(QQuickItem *parent = nullptr);

    QByteArray fontBytes() const { return m_bytes; }

    QString text() const { return m_text; }

    QColor color() const { return m_color; }

    bool missingCharacters() const { return m_missing; }

    bool truncated() const { return m_truncated; }

    void setFontBytes(const QByteArray &bytes);
    void setText(const QString &text);
    void setColor(const QColor &color);
    void paint(QPainter *painter) override;
signals:
    void changed();

private:
    void relayout();
    QByteArray m_bytes;
    QString m_text;
    QColor m_color = Qt::white;
    bool m_missing = false, m_truncated = false;
};
