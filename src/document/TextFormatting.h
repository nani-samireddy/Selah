#pragma once
#include <QObject>
#include <QQuickTextDocument>
#include <QtQml/qqmlregistration.h>
#include <QVariantMap>

// A plain-text source plus sanitized Qt rich text keeps formatting portable across outputs.
// Qt registration may instantiate a derived constructor wrapper (including on MSVC).
class TextFormatting : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit TextFormatting(QObject *parent = nullptr)
        : QObject(parent) { }

    Q_INVOKABLE QVariantMap display(const QVariantMap &element) const;
    Q_INVOKABLE QString html(const QVariantMap &element) const;
    Q_INVOKABLE QVariantMap apply(
        const QVariantMap &element, const QVariantMap &patch, int start = -1, int end = -1) const;
    Q_INVOKABLE QVariantMap capture(const QVariantMap &element, const QString &html) const;
    Q_INVOKABLE QVariantMap styleAt(const QVariantMap &element, int start, int end) const;
    Q_INVOKABLE void hydrate(QQuickTextDocument *document, const QVariantMap &element) const;
    Q_INVOKABLE QVariantMap captureDocument(
        const QVariantMap &element, QQuickTextDocument *document) const;
    static QVariantMap defaults();
    static QVariantMap normalize(const QVariantMap &element);
    static bool isStyle(const QString &key);
    static int selfTest();
};
