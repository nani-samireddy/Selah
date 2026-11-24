#include "FontLibrary.h"
#include <limits>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRawFont>

namespace {
constexpr qint64 maximumPreview = 64 * 1024 * 1024;
constexpr qint64 previewBudget = 32 * 1024 * 1024;
}

void FontLibrary::cancelPreview() {
    ++m_previewGeneration;
    if (m_previewReply) {
        m_previewReply->abort();
        m_previewReply = nullptr;
    }
    m_previewBusy = false;
    m_previewBytes.clear();
    m_previewFamily.clear();
    m_previewStatus.clear();
    emit previewChanged();
}

void FontLibrary::requestPreview(const QString &family) {
    if (family == m_previewFamily && (m_previewBusy || !m_previewBytes.isEmpty())) {
        return;
    }
    cancelPreview();
    m_previewFamily = family;
    if (family.isEmpty() || available(family)) {
        emit previewChanged();
        return;
    }
    if (m_previewCache.contains(family)) {
        m_previewBytes = m_previewCache.value(family);
        m_previewOrder.removeAll(family);
        m_previewOrder.append(family);
        m_previewStatus = "Preview only · Download & use to keep the family offline.";
        emit previewChanged();
        return;
    }
    if (!m_byName.contains(family)) {
        m_previewStatus = "Choose a catalog font to preview.";
        emit previewChanged();
        return;
    }
    m_previewBusy = true;
    m_previewStatus = "Loading " + family + " preview…";
    emit previewChanged();
    const auto record = m_byName.value(family);
    File selected;
    int best = std::numeric_limits<int>::max();
    // Prefer a regular/variable face; preview one native face, not every weight.
    for (const auto &entry : record["files"].toList()) {
        const auto file = entry.toMap();
        if (!file["font"].toBool()) {
            continue;
        }
        const auto path = file["path"].toString();
        const auto name = QFileInfo(path).fileName();
        int score = name.contains("Regular", Qt::CaseInsensitive) ? 0 : name.contains('[') ? 1 : 10;
        if (name.contains("Italic", Qt::CaseInsensitive) != record["italic"].toBool()) {
            score += 100;
        }
        if (score < best) {
            best = score;
            selected
                = { QUrl("https://raw.githubusercontent.com/google/fonts/" + m_commit + "/" + path),
                      name, file["sha"].toString(), true };
        }
    }
    if (selected.url.isEmpty()) {
        previewCss();
    } else {
        fetchPreview(selected, false);
    }
}

void FontLibrary::previewCss() {
    const auto record = m_byName.value(m_previewFamily);
    const QString axis = record["italic"].toBool() ? ":ital,wght@1," : ":wght@";
    // No preview text is sent to the provider. Fetch a normal face with all its glyphs.
    const auto url = QUrl("https://fonts.googleapis.com/css2?family="
        + QString::fromLatin1(QUrl::toPercentEncoding(m_previewFamily)) + axis
        + record["weight"].toString());
    fetchPreview({ url, "font.css", {}, false }, true);
}

void FontLibrary::fetchPreview(const File &file, bool cssFallback) {
    QNetworkRequest request(file.url);
    request.setRawHeader("User-Agent", "ChurchDeckNative/0.1");
    request.setTransferTimeout(20000);
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    auto reply = m_network.get(request);
    m_previewReply = reply;
    const auto generation = m_previewGeneration;
    connect(reply, &QNetworkReply::downloadProgress, this, [reply](qint64 received, qint64 total) {
        if (received > maximumPreview || total > maximumPreview) {
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation, file, cssFallback] {
        if (generation != m_previewGeneration) {
            reply->deleteLater();
            return;
        }
        const auto bytes = reply->isOpen() ? reply->readAll() : QByteArray();
        const auto error = reply->error();
        reply->deleteLater();
        if (generation != m_previewGeneration) {
            return;
        }
        m_previewReply = nullptr;
        auto fail = [this] {
            m_previewBusy = false;
            m_previewStatus = "Preview unavailable. Check your connection or try again.";
            emit previewChanged();
        };
        if (error != QNetworkReply::NoError || bytes.isEmpty() || bytes.size() > maximumPreview) {
            if (!cssFallback && file.font) {
                previewCss();
            } else {
                fail();
            }
            return;
        }
        if (!file.sha.isEmpty()) {
            QByteArray blob = "blob " + QByteArray::number(bytes.size());
            blob.append('\0');
            blob += bytes;
            if (QString::fromLatin1(
                    QCryptographicHash::hash(blob, QCryptographicHash::Sha1).toHex())
                != file.sha) {
                fail();
                return;
            }
        }
        if (!file.font) {
            const auto urls = cssFontUrls(bytes);
            if (urls.isEmpty()) {
                fail();
                return;
            }
            fetchPreview({ urls.first(), "preview.ttf", {}, true }, true);
            return;
        }
        // A raw font is private to the preview: no QFontDatabase registration, no live changes.
        const QRawFont raw(bytes, 30);
        if (!raw.isValid()) {
            fail();
            return;
        }
        m_previewBytes = bytes;
        m_previewBusy = false;
        m_previewStatus = "Preview only · Download & use to keep the family offline.";
        m_previewCache.insert(m_previewFamily, bytes);
        m_previewOrder.append(m_previewFamily);
        qint64 total = 0;
        for (const auto &value : m_previewCache) {
            total += value.size();
        }
        while (m_previewOrder.size() > 1 && (total > previewBudget || m_previewOrder.size() > 4)) {
            const auto oldest = m_previewOrder.takeFirst();
            total -= m_previewCache.take(oldest).size();
        }
        emit previewChanged();
    });
}
