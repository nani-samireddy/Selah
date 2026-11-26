#include "CactusBackend.h"
#include "AiTools.h"
#include <QFile>
#include <QJsonDocument>
#include <QProcessEnvironment>

CactusBackend::CactusBackend(QObject *parent, int timeoutMs)
    : QObject(parent) {
    m_deadline.setSingleShot(true);
    m_deadline.setInterval(qBound(1, timeoutMs, 30000));
    connect(&m_deadline, &QTimer::timeout, this, [this] {
        m_failure = "The assistant timed out. Try a shorter request.";
        m_process.kill();
    });
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &CactusBackend::readOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, &CactusBackend::readOutput);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            finish("Could not start the configured Cactus runtime.");
        }
    });
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus exit) {
        readOutput();
        finish(!m_failure.isEmpty() ? m_failure
                : (code != 0 || exit != QProcess::NormalExit)
                ? "The Cactus runtime stopped without a usable response."
                : QString());
    });
}

CactusBackend::~CactusBackend() {
    disconnect(&m_process, nullptr, this, nullptr);
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

void CactusBackend::request(const QString &runtime, const QString &model, const QString &prompt,
    const QByteArray &context, const QString &scope) {
    if (m_busy) {
        return;
    }
    m_busy = true;
    m_stdout.clear();
    m_stderr.clear();
    m_failure.clear();
    auto write = [&](const QString &name, const QByteArray &data) {
        QFile file(m_workspace.filePath(name));
        return file.open(QIODevice::WriteOnly | QIODevice::Truncate)
            && file.write(data) == data.size();
    };
    if (!m_workspace.isValid()
        || !write(
            "tools.json", QJsonDocument(AiTools::schemas(scope)).toJson(QJsonDocument::Compact))
        || !write("context.txt", context)) {
        finish("Could not prepare the assistant's temporary files.");
        return;
    }
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("CACTUS_TELEMETRY", "0");
    environment.insert("HF_HUB_OFFLINE", "1");
    m_process.setProcessEnvironment(environment);
    m_process.setWorkingDirectory(m_workspace.path());
    m_process.setProgram(runtime);
    // QProcess passes distinct arguments directly. Never route model text through a shell.
    m_process.setArguments({ "--model", model, "--tools", m_workspace.filePath("tools.json"),
        "--system", m_workspace.filePath("context.txt"), "--prompt", prompt });
    m_deadline.start();
    m_process.start();
}

void CactusBackend::readOutput() {
    m_stdout += m_process.readAllStandardOutput();
    m_stderr += m_process.readAllStandardError();
    if (m_stdout.size() > 256 * 1024 || m_stderr.size() > 32 * 1024) {
        m_failure = "The assistant produced too much output.";
        m_stdout.clear();
        m_stderr.clear();
        m_process.kill();
    }
}

void CactusBackend::cancel() {
    if (!m_busy) {
        return;
    }
    m_failure = "Assistant request cancelled.";
    m_process.kill();
}

void CactusBackend::finish(const QString &error) {
    if (!m_busy) {
        return;
    }
    m_deadline.stop();
    m_busy = false;
    emit completed(error.isEmpty() ? m_stdout : QByteArray(), error);
}
