#include "ModelInstaller.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>

namespace {
struct Asset {
    QString file, url;
    QByteArray sha;
};

QString folder() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ai/cactus-4.0.0";
}

QList<Asset> assets() {
#ifdef Q_OS_WIN
    const QString platform = "windows-x86_64", binary = "cactus.exe";
    const QByteArray sha = "3727bc70f4ad4f5dcf3719b90ec01a3a5cbf7a79da9db8424b8ea64c7c4db1b2";
#else
    const QString platform = "macos-arm64", binary = "cactus";
    const QByteArray sha = "39bd925a3759ad3dd18056918feffc82be43a92d4eb2ef5f8baf73a707ee3f3b";
#endif
    const QString base = "https://huggingface.co/Cactus-Compute/needle3/resolve/"
                         "c95d25b4efa1e9601e785d92438e4ade0dbddbf8/";
    return { { binary,
                 "https://huggingface.co/Cactus-Compute/cactus/resolve/"
                 "1a028f5a3972b67dd8d8752f882113d475e7ec4c/releases/4.0.0/"
                     + platform + "/" + binary,
                 sha },
        { "needle3.cact", base + "needle3.cact",
            "c9d915eca282ed42d1a09b143b592adb4cc6744ffe2d294adf5cfc5548170c38" },
        { "LICENSE-Needle.txt", base + "LICENSE",
            "cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30" } };
}
}

bool ModelInstaller::supported() {
#if defined(Q_OS_MACOS)
    return QSysInfo::buildCpuArchitecture() == "arm64";
#elif defined(Q_OS_WIN)
    return QSysInfo::buildCpuArchitecture() == "x86_64";
#else
    return false;
#endif
}

bool ModelInstaller::managedReady() {
    if (!supported()) {
        return false;
    }
    for (const auto &asset : assets()) {
        if (!VerifiedDownload::matches(folder() + "/" + asset.file, asset.sha)) {
            return false;
        }
    }
    return true;
}

ModelInstaller::ModelInstaller(QObject *parent)
    : QObject(parent) {
    connect(&m_download, &VerifiedDownload::progress, this, [this](qint64 received, qint64 total) {
        m_received = received;
        m_total = total;
        emit changed();
    });
    connect(&m_download, &VerifiedDownload::finished, this,
        [this](const QString &, const QString &error) {
            if (!error.isEmpty()) {
                m_busy = false;
                m_status = error;
                emit changed();
                return;
            }
            ++m_index;
            next();
        });
}

void ModelInstaller::install() {
    if (m_busy || m_download.busy()) {
        return;
    }
    if (!supported()) {
        m_status = "Managed download supports Apple Silicon Macs and Windows x64. You can still "
                   "choose compatible local files.";
        emit changed();
        return;
    }
    m_busy = true;
    m_index = 0;
    next();
}

void ModelInstaller::next() {
    if (!m_busy) {
        return;
    }
    const auto files = assets();
    if (m_index == files.size()) {
#ifndef Q_OS_WIN
        if (!QFile::setPermissions(folder() + "/cactus",
                QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner)) {
            m_busy = false;
            m_status = "Could not make the downloaded runtime executable.";
            emit changed();
            return;
        }
#endif
        m_busy = false;
        m_status = "Luma downloaded and verified. Select Enable Luma when you're ready.";
        emit changed();
        emit installed();
        return;
    }
    const auto asset = files[m_index];
    m_received = m_total = 0;
    m_status = QString("Preparing %1 of %2: %3").arg(m_index + 1).arg(files.size()).arg(asset.file);
    emit changed();
    if (VerifiedDownload::matches(folder() + "/" + asset.file, asset.sha)) {
        ++m_index;
        QTimer::singleShot(0, this, &ModelInstaller::next);
        return;
    }
    m_download.start(QUrl(asset.url), folder() + "/" + asset.file, asset.sha, 100 * 1024 * 1024);
}

void ModelInstaller::cancel() {
    if (!m_busy) {
        return;
    }
    m_busy = false;
    m_status = "Model download cancelled. Verified files are kept for the next attempt.";
    m_download.cancel();
    emit changed();
}
