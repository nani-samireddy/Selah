#pragma once
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSaveFile>
#include <QUrl>

// Streams HTTPS downloads into an atomic file; unverified bytes never replace an installed file.
class VerifiedDownload final : public QObject {
    Q_OBJECT
public:
    explicit VerifiedDownload(QObject *parent = nullptr);
    ~VerifiedDownload() override;

    bool busy() const { return !m_reply.isNull(); }

    bool start(
        const QUrl &url, const QString &destination, const QByteArray &sha256, qint64 maximumBytes);
    void cancel();
    static bool matches(const QString &path, const QByteArray &sha256);
signals:
    void progress(qint64 received, qint64 total);
    void finished(const QString &path, const QString &error);

private:
    void consume();
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QSaveFile m_file;
    QCryptographicHash m_hash { QCryptographicHash::Sha256 };
    QByteArray m_expected;
    qint64 m_received = 0, m_limit = 0;
    QString m_error;
};
