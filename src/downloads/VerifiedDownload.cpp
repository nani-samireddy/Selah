#include "VerifiedDownload.h"
#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>

VerifiedDownload::VerifiedDownload(QObject *parent)
    : QObject(parent) {
}

VerifiedDownload::~VerifiedDownload() {
    if (m_reply) {
        disconnect(m_reply, nullptr, this, nullptr);
        m_reply->abort();
    }
    m_file.cancelWriting();
}

bool VerifiedDownload::matches(const QString &path, const QByteArray &sha256) {
    QFile file(path);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    return sha256.size() == 64 && file.open(QIODevice::ReadOnly) && hash.addData(&file)
        && hash.result().toHex() == sha256.toLower();
}

bool VerifiedDownload::start(
    const QUrl &url, const QString &destination, const QByteArray &sha256, qint64 maximumBytes) {
    if (busy()) {
        return false;
    }
    if (url.scheme() != "https" || url.host().isEmpty() || !url.userInfo().isEmpty()
        || !QRegularExpression("^[a-fA-F0-9]{64}$").match(QString::fromLatin1(sha256)).hasMatch()
        || maximumBytes <= 0) {
        emit finished({}, "A secure download URL and SHA-256 checksum are required.");
        return false;
    }
    if (!QDir().mkpath(QFileInfo(destination).absolutePath())) {
        emit finished({}, "Could not create the download folder.");
        return false;
    }
    m_file.setFileName(destination);
    m_file.setDirectWriteFallback(false);
    if (!m_file.open(QIODevice::WriteOnly)) {
        emit finished({}, "Could not write to the download folder.");
        return false;
    }
    m_error.clear();
    m_expected = sha256.toLower();
    m_received = 0;
    m_limit = maximumBytes;
    m_hash.reset();
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Selah-desktop");
    request.setTransferTimeout(30000);
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_reply = m_network.get(request);
    m_reply->setReadBufferSize(256 * 1024);
    connect(m_reply, &QNetworkReply::readyRead, this, &VerifiedDownload::consume);
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > m_limit && m_reply) {
            m_error = "The download exceeds the allowed size.";
            m_reply->abort();
            return;
        }
        emit progress(received, total);
    });
    connect(m_reply, &QNetworkReply::finished, this, [this] {
        consume();
        auto reply = m_reply.data();
        if (!reply) {
            return;
        }
        if (m_error.isEmpty() && reply->error() != QNetworkReply::NoError) {
            m_error = "Download failed: " + reply->errorString();
        }
        if (m_error.isEmpty() && m_hash.result().toHex() != m_expected) {
            m_error = "Checksum verification failed. Please retry the download.";
        }
        if (m_error.isEmpty() && !m_file.commit()) {
            m_error = "Could not save the verified download.";
        }
        if (!m_error.isEmpty() && m_file.isOpen()) {
            // cancelWriting marks the temporary file; commit then closes and discards it.
            // Without this, a retry on the same downloader cannot open its next file.
            m_file.cancelWriting();
            m_file.commit();
        }
        const auto path = m_error.isEmpty() ? m_file.fileName() : QString();
        m_reply = nullptr;
        reply->deleteLater();
        emit finished(path, m_error);
    });
    return true;
}

void VerifiedDownload::consume() {
    if (!m_reply || !m_error.isEmpty()) {
        return;
    }
    while (m_reply->bytesAvailable()) {
        const auto data = m_reply->read(128 * 1024);
        m_received += data.size();
        if (m_received > m_limit || m_file.write(data) != data.size()) {
            m_error = m_received > m_limit ? "The download exceeds the allowed size."
                                           : "Could not write the download.";
            m_reply->abort();
            return;
        }
        m_hash.addData(data);
    }
}

void VerifiedDownload::cancel() {
    if (m_reply) {
        m_error = "Download cancelled.";
        m_reply->abort();
    }
}
