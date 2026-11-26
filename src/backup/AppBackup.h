#pragma once
#include "PortableBundle.h"
#include <QFutureWatcher>
#include <QObject>
#include <QTimer>
#include <QUrl>

class AppBackup : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool restoreReady READ restoreReady NOTIFY changed)
public:
    explicit AppBackup(const QString &directory = {}, QObject *parent = nullptr);
    ~AppBackup() override;

    bool busy() const { return m_watcher.isRunning(); }

    int progress() const { return m_progress ? m_progress->percent.load() : 0; }

    QString status() const { return m_status; }

    bool restoreReady() const;
    Q_INVOKABLE bool exportBackup(const QUrl &url);
    Q_INVOKABLE bool importBackup(const QUrl &url);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void cancelRestore();
    // Called before any controllers open databases. Keeps a rollback copy of the old profile.
    static QString applyPendingRestore(const QString &root, bool applySettings = true);
    static BundleResult write(const QString &root, const QVariantMap &settings, const QString &path,
        const std::shared_ptr<BundleProgress> &progress);
    static BundleResult prepare(
        const QString &path, const QString &root, const std::shared_ptr<BundleProgress> &progress);
signals:
    void changed();

private:
    bool start(const QUrl &url, bool importing);
    QString m_root, m_status;
    QFutureWatcher<BundleResult> m_watcher;
    QTimer m_timer;
    std::shared_ptr<BundleProgress> m_progress;
};
