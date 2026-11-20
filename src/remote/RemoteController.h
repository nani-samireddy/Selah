#pragma once
#include "RemoteHttpServer.h"
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantList>

class OutputManager;
class MediaLibrary;

// Optional LAN service. Sessions are memory-only and read exclusively from the live snapshot.
class RemoteController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString address READ address NOTIFY changed)
    Q_PROPERTY(QString controllerCode READ controllerCode NOTIFY changed)
    Q_PROPERTY(QString speakerCode READ speakerCode NOTIFY changed)
    Q_PROPERTY(QString controllerQr READ controllerQr NOTIFY changed)
    Q_PROPERTY(QString speakerQr READ speakerQr NOTIFY changed)
    Q_PROPERTY(QVariantList interfaces READ interfaces NOTIFY changed)
    Q_PROPERTY(QVariantList clients READ clients NOTIFY changed)
public:
    explicit RemoteController(
        OutputManager *outputs, MediaLibrary *media, QObject *parent = nullptr);

    bool running() const { return m_running; }

    QString status() const { return m_status; }

    QString address() const { return m_address; }

    QString controllerCode() const { return m_controlCode; }

    QString speakerCode() const { return m_speakerCode; }

    QString controllerQr() const { return m_controlQr; }

    QString speakerQr() const { return m_speakerQr; }

    QVariantList interfaces() const;
    QVariantList clients() const;
    Q_INVOKABLE bool start(const QString &ip);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void rotateCodes();
    Q_INVOKABLE void revoke(const QString &id);

    Q_INVOKABLE void refreshInterfaces() { emit changed(); }

    static int selfTest();
signals:
    void changed();

private:
    struct Session {
        QString id, name, role;
        qint64 created = 0, touched = 0;
        QHash<QString, QByteArray> commands;
        QStringList commandOrder;
    };

    struct Attempts {
        qint64 start = 0;
        int count = 0;
    };

    RemoteResponse handle(const RemoteRequest &request);
    RemoteResponse pair(const RemoteRequest &request);
    RemoteResponse command(const RemoteRequest &request, Session &session);
    QVariantMap state(const Session &session) const;
    QVariantMap deck() const;
    void expire();
    static QString randomToken();
    OutputManager *m_outputs;
    MediaLibrary *m_media;
    RemoteHttpServer m_server;
    QElapsedTimer m_clock;
    QTimer m_expiry;
    QHash<QString, Session> m_sessions;
    QHash<QString, Attempts> m_attempts;
    Attempts m_globalAttempts;
    bool m_running = false;
    QString m_status = "Remote control is off", m_address, m_host;
    QString m_controlCode, m_speakerCode, m_controlQr, m_speakerQr;
    quint64 m_revision = 0;
    QString m_previewSnapshot;
    QHash<int, QByteArray> m_previews;
};
