#pragma once
#include <atomic>
#include <QImage>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

// The sender owns at most one pending frame plus the frame currently being sent.
// A slow SDK/network must not create an unbounded queue on the operator thread.
// Sender thread: at most one frame in flight and one replaceable pending frame.
class NdiWorker final : public QThread {
    Q_OBJECT
public:
    NdiWorker(
        QString sourceName, QString runtimePath, int fps, bool diagnostic, int testDelayMs = 0);
    ~NdiWorker() override;
    void submit(QImage frame);
    void requestStop();

    quint64 processed() const { return m_processed.load(); }

    quint64 dropped() const { return m_dropped.load(); }

    int connections() const { return m_connections.load(); }

    static bool sdkEnabled();
    static QString discoverRuntime(const QString &configured = QString());
    static int selfTest();
signals:
    void senderReady();
    void failed(const QString &error);

protected:
    void run() override;

private:
    QString m_name, m_runtime;
    int m_fps, m_testDelay;
    bool m_diagnostic;
    QMutex m_mutex;
    QWaitCondition m_condition;
    QImage m_pending;
    bool m_stopping = false;
    std::atomic<quint64> m_processed { 0 }, m_dropped { 0 };
    std::atomic<int> m_connections { 0 };
};
