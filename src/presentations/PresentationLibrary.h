#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>

// Saved custom presentations are independent of editor recovery and program snapshots.
class PresentationLibrary final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList presentations READ presentations NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit PresentationLibrary(QObject *parent = nullptr, const QString &directory = QString());
    ~PresentationLibrary() override;
    QVariantList presentations() const;

    QString status() const { return m_status; }

    Q_INVOKABLE QVariantMap presentation(const QString &id) const;
    Q_INVOKABLE QString save(
        const QString &title, const QVariantList &slides, const QString &id = QString());
    static int selfTest();
signals:
    void changed();

private:
    void setStatus(const QString &message);
    QSqlDatabase m_db;
    QString m_status = "Custom slides ready";
};
