#pragma once
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>

// One asynchronous native CLI invocation per proposal; inference stays out of the UI process.
class CactusBackend final : public QObject {
    Q_OBJECT
public:
    explicit CactusBackend(QObject *parent = nullptr, int timeoutMs = 30000);
    ~CactusBackend() override;

    bool busy() const { return m_busy; }

    void request(const QString &runtime, const QString &model, const QString &prompt,
        const QByteArray &context, const QString &scope = "all");
    void cancel();
signals:
    void completed(const QByteArray &response, const QString &error);

private:
    void readOutput();
    void finish(const QString &error = QString());
    QProcess m_process;
    QTimer m_deadline;
    QTemporaryDir m_workspace;
    QByteArray m_stdout, m_stderr;
    QString m_failure;
    bool m_busy = false;
};
