#include "UpdateManager.h"
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSysInfo>
#include <QVersionNumber>

namespace {
QVersionNumber version(QString text) {
    if (text.startsWith('v')) {
        text.remove(0, 1);
    }
    if (!QRegularExpression("^[0-9]+\\.[0-9]+\\.[0-9]+$").match(text).hasMatch()) {
        return {};
    }
    return QVersionNumber::fromString(text);
}
}

QString UpdateManager::installedVersion() {
    return SELAH_VERSION;
}

QString UpdateManager::platform() {
    const auto cpu = QSysInfo::buildCpuArchitecture();
#ifdef Q_OS_MACOS
    return cpu == "arm64" ? "macOS-arm64" : cpu == "x86_64" ? "macOS-x86_64" : QString();
#elif defined(Q_OS_WIN)
    return cpu == "x86_64" ? "Windows-x64" : QString();
#else
    return cpu == "x86_64" ? "Linux-x64" : QString();
#endif
}

QVariantMap UpdateManager::selectRelease(
    const QJsonObject &release, const QString &installed, const QString &target) {
    const auto tag = release["tag_name"].toString();
    const auto candidate = version(tag), current = version(installed);
    if (release["draft"].toBool() || release["prerelease"].toBool() || candidate.isNull()
        || current.isNull() || QVersionNumber::compare(candidate, current) <= 0
        || target.isEmpty()) {
        return {};
    }
    QVariantMap result { { "version", candidate.toString() },
        { "notes", release["body"].toString().left(50000) } };
    QStringList suffixes;
    if (target.startsWith("macOS")) {
        suffixes = { ".dmg", ".zip" };
    } else if (target.startsWith("Windows")) {
        suffixes = { ".zip" };
    } else {
        suffixes = { ".tar.gz" };
    }
    for (const auto &suffix : suffixes) {
        const auto name = "Selah-" + candidate.toString() + "-" + target + suffix;
        for (const auto &item : release["assets"].toArray()) {
            const auto asset = item.toObject();
            const QUrl url(asset["browser_download_url"].toString());
            const auto digest = asset["digest"].toString();
            const auto size = asset["size"].toVariant().toLongLong();
            if (asset["name"] != name || asset["state"] != "uploaded" || size <= 0
                || size > 2LL * 1024 * 1024 * 1024 || url.scheme() != "https"
                || url.host() != "github.com" || !url.userInfo().isEmpty() || url.hasQuery()
                || url.hasFragment()
                || url.path() != "/nani-samireddy/Selah/releases/download/" + tag + "/" + name
                || !QRegularExpression("^sha256:[a-f0-9]{64}$").match(digest).hasMatch()) {
                continue;
            }
            result["url"] = url;
            result["name"] = name;
            result["sha256"] = digest.mid(7);
            result["size"] = size;
            return result;
        }
    }
    return result;
}

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent) {
    connect(&m_download, &VerifiedDownload::progress, this, [this](qint64 received, qint64 total) {
        m_received = received;
        m_total = total;
        emit changed();
    });
    connect(&m_download, &VerifiedDownload::finished, this,
        [this](const QString &path, const QString &error) {
            m_path = path;
            m_status = error.isEmpty()
                ? "Download verified. Stop presenting, then open the download folder to install."
                : error;
            emit changed();
        });
}

UpdateManager::~UpdateManager() {
    if (m_check) {
        disconnect(m_check, nullptr, this, nullptr);
        m_check->abort();
    }
}

void UpdateManager::check() {
    if (busy()) {
        return;
    }
    m_release.clear();
    m_path.clear();
    m_received = m_total = 0;
    m_status = "Checking GitHub Releases…";
    QNetworkRequest request(
        QUrl("https://api.github.com/repos/nani-samireddy/Selah/releases/latest"));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "Selah/" + installedVersion().toUtf8());
    request.setTransferTimeout(20000);
    auto reply = m_network.get(request);
    m_check = reply;
    connect(reply, &QNetworkReply::readyRead, this, [reply] {
        if (reply->bytesAvailable() > 2 * 1024 * 1024) {
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_check != reply) {
            reply->deleteLater();
            return;
        }
        m_check = nullptr;
        const auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == 404) {
            m_status = "No stable release has been published yet.";
        } else if (code == 403 || code == 429) {
            m_status = "GitHub is limiting update checks. Please retry later.";
        } else if (reply->error() != QNetworkReply::NoError) {
            m_status = "Could not check for updates. Check your connection and retry.";
        } else {
            const auto data = reply->readAll();
            QJsonParseError error;
            const auto json = QJsonDocument::fromJson(data, &error);
            if (data.size() > 2 * 1024 * 1024 || error.error != QJsonParseError::NoError
                || !json.isObject() || version(json.object()["tag_name"].toString()).isNull()) {
                m_status = "The release information was invalid. Please retry later.";
            } else {
                m_release = selectRelease(json.object(), installedVersion(), platform());
                m_status = m_release.isEmpty() ? "No newer stable version is available."
                    : !m_release.contains("url")
                    ? "A new version exists, but a verified package for this platform is not "
                      "available yet."
                    : "Selah " + m_release["version"].toString() + " is available.";
            }
        }
        reply->deleteLater();
        emit changed();
    });
    emit changed();
}

void UpdateManager::download() {
    if (busy() || !m_release.contains("url")) {
        return;
    }
    const auto folder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (folder.isEmpty()) {
        m_status = "No Downloads folder is available.";
        emit changed();
        return;
    }
    m_path.clear();
    m_received = 0;
    m_total = m_release["size"].toLongLong();
    m_status = "Downloading update…";
    m_download.start(m_release["url"].toUrl(), QDir(folder).filePath(m_release["name"].toString()),
        m_release["sha256"].toByteArray(), m_total);
    emit changed();
}

void UpdateManager::cancel() {
    if (m_check) {
        auto reply = m_check.data();
        m_check = nullptr;
        reply->abort();
        m_status = "Update check cancelled.";
        emit changed();
    }
    m_download.cancel();
}

void UpdateManager::showDownload() {
    if (m_path.isEmpty() || !VerifiedDownload::matches(m_path, m_release["sha256"].toByteArray())) {
        m_path.clear();
        m_status = "The download is missing or has changed. Download it again.";
        emit changed();
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_path).absolutePath()))) {
        m_status = "Could not open the download folder: " + QFileInfo(m_path).absolutePath();
        emit changed();
    }
}
