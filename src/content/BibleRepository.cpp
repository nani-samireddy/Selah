#include "BibleRepository.h"
#include <memory>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

QVariantList BibleRepository::catalog() {
    // QML reference-sequence iteration can call this getter per element. Read and resolve the
    // immutable resource once, rather than reparsing 1,000+ records for every property read.
    static const auto records = []() -> QVariantList {
        QFile file(":/qt/qml/ChurchDeck/assets/scriptures/bible-repository.json");
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        auto rows
            = QJsonDocument::fromJson(file.readAll()).object()["bibles"].toArray().toVariantList();
        for (auto &value : rows) {
            auto row = value.toMap();
            QUrl url;
            url.setScheme("https");
            url.setHost("raw.githubusercontent.com");
            url.setPath(
                "/nani-samireddy/Holy-Bible-XML-Format/master/" + row["filename"].toString());
            row["url"] = url;
            value = row;
        }
        return rows;
    }();
    return records;
}

QUrl BibleRepository::rawUrl(const QUrl &source) {
    if (source.scheme() != "https" || source.hasQuery() || source.hasFragment()
        || !source.userInfo().isEmpty() || source.port(-1) != -1) {
        return {};
    }
    const auto path = source.path();
    const auto prefix = source.host() == "github.com"
        ? QString("/nani-samireddy/Holy-Bible-XML-Format/blob/master/")
        : source.host() == "raw.githubusercontent.com"
        ? QString("/nani-samireddy/Holy-Bible-XML-Format/master/")
        : QString();
    if (prefix.isEmpty() || !path.startsWith(prefix)) {
        return {};
    }
    const auto file = path.mid(prefix.size());
    if (file.contains('/') || file.contains('\\') || !file.endsWith(".xml", Qt::CaseInsensitive)) {
        return {};
    }
    QUrl raw;
    raw.setScheme("https");
    raw.setHost("raw.githubusercontent.com");
    raw.setPath("/nani-samireddy/Holy-Bible-XML-Format/master/" + file);
    return raw;
}

void BibleRepository::download(const QUrl &source, QObject *owner,
    std::function<void(const QByteArray &, const QUrl &, const QString &)> finished,
    std::function<void(qint64, qint64)> progress) {
    const auto raw = rawUrl(source);
    if (raw.isEmpty()) {
        finished({}, {},
            "Use an XML link from github.com/nani-samireddy/Holy-Bible-XML-Format (master branch)");
        return;
    }
    auto manager = new QNetworkAccessManager(owner);
    QNetworkRequest request(raw);
    request.setTransferTimeout(30000);
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto reply = manager->get(request);
    QObject::connect(
        reply, &QNetworkReply::downloadProgress, owner, [progress](qint64 received, qint64 total) {
            if (progress) {
                progress(received, total);
            }
        });
    auto bytes = std::make_shared<QByteArray>();
    QObject::connect(reply, &QNetworkReply::readyRead, owner, [reply, bytes] {
        bytes->append(reply->readAll());
        if (bytes->size() > 100 * 1024 * 1024) {
            reply->abort();
        }
    });
    QObject::connect(
        reply, &QNetworkReply::finished, owner, [reply, manager, bytes, raw, finished] {
            if (reply->isOpen()) {
                bytes->append(reply->readAll());
            }
            QString error;
            if (bytes->size() > 100 * 1024 * 1024) {
                error = "Bible download exceeds 100 MB";
            } else if (reply->error() != QNetworkReply::NoError) {
                error = "Bible download failed: " + reply->errorString();
            } else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
                error = "Bible download was not successful";
            }
            finished(*bytes, raw, error);
            manager->deleteLater();
        });
}
