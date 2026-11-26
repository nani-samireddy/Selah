#pragma once
#include "VerifiedDownload.h"

// Optional, pinned Cactus/Needle assets. Downloads never enable inference automatically.
class ModelInstaller final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool supported READ supported CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(qint64 received READ received NOTIFY changed)
    Q_PROPERTY(qint64 total READ total NOTIFY changed)
public:
    explicit ModelInstaller(QObject *parent = nullptr);

    bool busy() const { return m_busy; }

    static bool supported();
    static bool managedReady();

    QString status() const { return m_status; }

    qint64 received() const { return m_received; }

    qint64 total() const { return m_total; }

    Q_INVOKABLE void install();
    Q_INVOKABLE void cancel();
signals:
    void changed();
    void installed();

private:
    void next();
    VerifiedDownload m_download;
    QString m_status = "Optional local model · about 37 MB · usable offline after download";
    qint64 m_received = 0, m_total = 0;
    int m_index = 0;
    bool m_busy = false;
};
