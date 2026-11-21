#include "FontLibrary.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRawFont>
#include <QRegularExpression>
#include <QSaveFile>
#include <QtEndian>
#include <QTimer>

namespace {
constexpr qint64 maximumFile = 64 * 1024 * 1024;

bool writeFile(const QString &path, const QByteArray &bytes) {
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}
}

void FontLibrary::cancel() {
    ++m_generation;
    if (m_busy) {
        m_status = "Font download cancelled";
    }
    if (m_reply) {
        m_reply->abort();
        m_reply = nullptr;
    }
    m_queue.clear();
    m_busy = false;
    m_progress = 0;
    emit stateChanged();
}

void FontLibrary::fail(const QString &message) {
    cancel();
    m_status = message;
    emit stateChanged();
    emit installFailed(message);
}

void FontLibrary::install(const QString &family) {
    if (m_busy) {
        return;
    }
    if (available(family)) {
        QTimer::singleShot(
            0, this, [this, family] { emit familyReady(family, resolvedFamily(family)); });
        return;
    }
    if (!m_byName.contains(family)) {
        fail("Choose a font from the catalog");
        return;
    }
    m_family = family;
    m_busy = true;
    m_progress = 0;
    m_downloadedFonts.clear();
    m_downloadedLicenses.clear();
    m_queue.clear();
    ++m_generation;
    if (!QDir().mkpath(directoryFor(family))) {
        fail("Cannot create the font cache. Check available disk space.");
        return;
    }
    const auto record = m_byName[family];
    m_css = record.value("css", false).toBool();
    if (m_css) {
        useGoogleCss();
        return;
    } else {
        for (const auto &value : record["files"].toList()) {
            const auto file = value.toMap();
            const auto path = file["path"].toString();
            m_queue.append(
                { QUrl("https://raw.githubusercontent.com/google/fonts/" + m_commit + "/" + path),
                    QFileInfo(path).fileName(), file["sha"].toString(), file["font"].toBool() });
        }
    }
    m_total = m_queue.size();
    m_status = "Downloading " + family + "…";
    emit stateChanged();
    next();
}

void FontLibrary::useGoogleCss() {
    m_css = true;
    m_queue.clear();
    m_downloadedFonts.clear();
    const auto record = m_byName[m_family];
    const QString axis = record["italic"].toBool() ? ":ital,wght@1," : ":wght@";
    const auto url = QUrl("https://fonts.googleapis.com/css2?family="
        + QString::fromLatin1(QUrl::toPercentEncoding(m_family)) + axis
        + record["weight"].toString());
    m_queue.append({ url, "font.css", {}, false });
    m_total = 1;
    m_progress = 0;
    m_status = "Downloading " + m_family + " from Google Fonts…";
    emit stateChanged();
    next();
}

QList<QUrl> FontLibrary::cssFontUrls(const QByteArray &css) {
    QList<QUrl> result;
    const QRegularExpression pattern("url\\((https://fonts\\.gstatic\\.com/[^)]+\\.ttf)\\)");
    auto matches = pattern.globalMatch(QString::fromUtf8(css));
    while (matches.hasNext()) {
        const QUrl url(matches.next().captured(1));
        if (url.scheme() == "https" && url.host() == "fonts.gstatic.com" && !result.contains(url)) {
            result.append(url);
        }
    }
    return result;
}

void FontLibrary::next() {
    if (!m_busy) {
        return;
    }
    if (m_queue.isEmpty()) {
        finish();
        return;
    }
    const auto task = m_queue.takeFirst();
    QNetworkRequest request(task.url);
    request.setRawHeader("User-Agent", "ChurchDeckNative/0.1");
    request.setTransferTimeout(45000);
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    auto reply = m_network.get(request);
    m_reply = reply;
    const auto generation = m_generation;
    connect(reply, &QNetworkReply::downloadProgress, this,
        [this, reply, generation](qint64 received, qint64 total) {
            if (generation != m_generation) {
                return;
            }
            if (received > maximumFile || total > maximumFile) {
                reply->abort();
                return;
            }
            const double fraction = total > 0 ? double(received) / total : 0;
            m_progress = qBound(
                0, int(100.0 * (m_total - m_queue.size() - 1 + fraction) / qMax(1, m_total)), 99);
            emit stateChanged();
        });
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation, task] {
        const auto bytes = reply->readAll();
        const auto error = reply->error();
        reply->deleteLater();
        if (generation != m_generation) {
            return;
        }
        m_reply = nullptr;
        if (error != QNetworkReply::NoError || bytes.isEmpty() || bytes.size() > maximumFile) {
            if (!m_css && error != QNetworkReply::NoError) {
                useGoogleCss();
                return;
            }
            fail("Could not download " + m_family
                + ". Check your internet connection and try again.");
            return;
        }
        if (!task.sha.isEmpty()) {
            QByteArray blob = "blob " + QByteArray::number(bytes.size());
            blob.append('\0');
            blob += bytes;
            if (QString::fromLatin1(
                    QCryptographicHash::hash(blob, QCryptographicHash::Sha1).toHex())
                != task.sha) {
                fail("Font download did not match the catalog. Try again.");
                return;
            }
        }
        if (task.name == "font.css") {
            const auto urls = cssFontUrls(bytes);
            if (urls.isEmpty()) {
                fail("Google Fonts did not return a native font file for this family.");
                return;
            }
            for (int i = 0; i < urls.size(); ++i) {
                m_queue.append({ urls[i], QString("face-%1.ttf").arg(i), {}, true });
            }
            m_total += urls.size();
            next();
            return;
        }
        const auto path = QDir(directoryFor(m_family)).filePath(task.name);
        if (!writeFile(path, bytes)) {
            fail("Cannot save the font. Check available disk space.");
            return;
        }
        if (task.font) {
            m_downloadedFonts.append(task.name);
        } else {
            m_downloadedLicenses.append(task.name);
        }
        next();
    });
}

QString FontLibrary::fontNotice(const QString &path) {
    QRawFont font(path, 16);
    const auto table = font.fontTable("name");
    if (table.size() < 6) {
        return {};
    }
    auto word = [&](int position) {
        return qFromBigEndian<quint16>(
            reinterpret_cast<const uchar *>(table.constData() + position));
    };
    const int count = word(2), base = word(4);
    QStringList result;
    for (int i = 0; i < count && 6 + (i + 1) * 12 <= table.size(); ++i) {
        const int pos = 6 + i * 12, platform = word(pos), id = word(pos + 6),
                  length = word(pos + 8), offset = base + word(pos + 10);
        if ((id != 0 && id != 13 && id != 14) || offset < 0 || offset + length > table.size()) {
            continue;
        }
        QString text;
        if (platform == 0 || platform == 3) {
            for (int at = offset; at + 1 < offset + length; at += 2) {
                text.append(QChar(word(at)));
            }
        } else {
            text = QString::fromLatin1(table.mid(offset, length));
        }
        if (!text.isEmpty() && !result.contains(text)) {
            result.append(text);
        }
    }
    return result.join("\n\n");
}

void FontLibrary::finish() {
    if (m_downloadedFonts.isEmpty()) {
        fail("No usable font files were downloaded");
        return;
    }
    QStringList names;
    QList<int> registered;
    for (const auto &file : m_downloadedFonts) {
        const int id
            = QFontDatabase::addApplicationFont(QDir(directoryFor(m_family)).filePath(file));
        if (id < 0) {
            for (int previous : registered) {
                QFontDatabase::removeApplicationFont(previous);
            }
            fail("This font could not be loaded by the native renderer.");
            return;
        }
        registered.append(id);
        names += QFontDatabase::applicationFontFamilies(id);
    }
    if (names.isEmpty()) {
        for (int id : registered) {
            QFontDatabase::removeApplicationFont(id);
        }
        fail("The downloaded font has no usable family name");
        return;
    }
    const auto resolved = names.contains(m_family) ? m_family : names.first();
    QByteArray notice
        = (m_family + "\nDownloaded from Google Fonts\nhttps://fonts.google.com/specimen/"
            + QString::fromLatin1(QUrl::toPercentEncoding(m_family)) + "\n\n")
              .toUtf8();
    notice += (m_css ? "Provider: fonts.gstatic.com\n\n" : "Source commit: " + m_commit + "\n\n")
                  .toUtf8();
    for (const auto &file : m_downloadedLicenses) {
        QFile license(QDir(directoryFor(m_family)).filePath(file));
        if (license.open(QIODevice::ReadOnly)) {
            notice += license.readAll() + "\n\n";
        }
    }
    for (const auto &file : m_downloadedFonts) {
        notice += fontNotice(QDir(directoryFor(m_family)).filePath(file)).toUtf8() + "\n\n";
    }
    if (m_css
        && (notice.toLower().contains("open font license")
            || notice.contains("scripts.sil.org/OFL"))) {
        QFile license(":/google-fonts/OFL.txt");
        if (license.open(QIODevice::ReadOnly)) {
            notice += license.readAll();
        }
    }
    if (!writeFile(QDir(directoryFor(m_family)).filePath("NOTICE.txt"), notice)) {
        for (int id : registered) {
            QFontDatabase::removeApplicationFont(id);
        }
        fail("Cannot save the font's license notice");
        return;
    }
    m_fontIds += registered;
    m_resolved.insert(m_family, resolved);
    m_cache.insert(m_family,
        { { "files", m_downloadedFonts }, { "resolved", resolved },
            { "provider", m_css ? "google-css" : "google-repository" },
            { "commit", m_css ? QString() : m_commit } });
    m_busy = false;
    m_progress = 100;
    m_status = m_family + " is ready offline";
    saveIndex();
    emit changed();
    emit stateChanged();
    emit familyReady(m_family, resolved);
}
