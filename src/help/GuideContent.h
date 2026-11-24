#pragma once
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

// Only bundled guide topics are readable; QML does not need filesystem access.
class GuideContent : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit GuideContent(QObject *parent = nullptr)
        : QObject(parent) { }

    Q_INVOKABLE QString read(const QString &topic) const;
};
