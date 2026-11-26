#pragma once
#include "VerifiedDownload.h"
#include <QJsonObject>
#include <QVariantMap>

// v1 uses explicit verified downloads and manual replacement of the packaged app.
class UpdateManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString installedVersion READ installedVersion CONSTANT)
    Q_PROPERTY(QString platform READ platform CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantMap release READ release NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(qint64 received READ received NOTIFY changed)
    Q_PROPERTY(qint64 total READ total NOTIFY changed)
    Q_PROPERTY(QString downloadedPath READ downloadedPath NOTIFY changed)
public:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager() override;
    static QString installedVersion();
    static QString platform();
    static QVariantMap selectRelease(
        const QJsonObject &release, const QString &installed, const QString &target);

    QString status() const { return m_status; }

    QVariantMap release() const { return m_release; }

    bool busy() const { return m_check || m_download.busy(); }

    qint64 received() const { return m_received; }

    qint64 total() const { return m_total; }

    QString downloadedPath() const { return m_path; }

    Q_INVOKABLE void check();
    Q_INVOKABLE void download();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void showDownload();
signals:
    void changed();

private:
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_check;
    VerifiedDownload m_download;
    QVariantMap m_release;
    QString m_status = "Check for a new stable Selah release.", m_path;
    qint64 m_received = 0, m_total = 0;
};
