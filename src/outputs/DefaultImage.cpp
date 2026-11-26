#include "DefaultImage.h"
#include "DocumentElements.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QSaveFile>
#include <QStandardPaths>

QString DefaultImage::kind(const QUrl &source) {
    return QStringList { "mp4", "mov", "m4v", "webm", "mkv", "avi" }.contains(
               QFileInfo(source.toLocalFile()).suffix().toLower())
        ? "video"
        : "image";
}

QString DefaultImage::prepare(const QUrl &source, QString *error) {
    const QFileInfo info(source.toLocalFile());
    if (!source.isLocalFile() || !info.isReadable() || !info.isFile() || info.size() <= 0) {
        *error = "Choose a readable image or video on this computer";
        return {};
    }
    const bool video = kind(source) == "video";
    QString suffix;
    if (video) {
        if (info.size() > 2LL * 1024 * 1024 * 1024
            || !QMimeDatabase()
                .mimeTypeForFile(info, QMimeDatabase::MatchContent)
                .name()
                .startsWith("video/")) {
            *error = "Choose a supported video under 2 GB";
            return {};
        }
        suffix = info.suffix().toLower();
    } else {
        if (info.size() > 50 * 1024 * 1024) {
            *error = "Choose an image under 50 MB";
            return {};
        }
        QImageReader reader(info.absoluteFilePath());
        reader.setAutoTransform(true);
        const auto size = reader.size();
        if (!reader.canRead() || !size.isValid()
            || qint64(size.width()) * size.height() > 50000000) {
            *error = "Choose a supported image of up to 50 megapixels";
            return {};
        }
        reader.setScaledSize(size.scaled(1920, 1080, Qt::KeepAspectRatio));
        if (reader.read().isNull()) {
            *error = "This image could not be decoded";
            return {};
        }
        suffix = QString::fromLatin1(reader.format()).toLower();
    }
    QFile input(info.absoluteFilePath());
    if (!input.open(QIODevice::ReadOnly)) {
        *error = "Could not read standby media";
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&input) || !input.seek(0)) {
        *error = "Could not read standby media";
        return {};
    }
    // Keep the existing directory so older preferences and full backups continue to work.
    const auto directory
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/default-images";
    const auto path = directory + "/" + QString::fromLatin1(hash.result().toHex()) + "." + suffix;
    if (!QDir().mkpath(directory)) {
        *error = "Could not create standby media storage";
        return {};
    }
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly)) {
        *error = "Could not save standby media";
        return {};
    }
    QCryptographicHash copied(QCryptographicHash::Sha256);
    while (!input.atEnd()) {
        const auto bytes = input.read(1024 * 1024);
        if (bytes.isEmpty() || output.write(bytes) != bytes.size()) {
            *error = "Could not copy standby media";
            return {};
        }
        copied.addData(bytes);
    }
    if (copied.result() != hash.result() || !output.commit()) {
        *error = "Standby media changed or could not be saved. Try again.";
        return {};
    }
    return QUrl::fromLocalFile(path).toString();
}

QVariantMap DefaultImage::layer(const QString &source, const QString &fit) {
    auto media = DocumentElements::text("Standby media", "", 0, 0, 1920, 1080, 36, "Arial");
    media["uid"] = "default-image";
    media["kind"] = kind(QUrl(source));
    media["source"] = source;
    media["fit"] = fit;
    media["loop"] = true;
    media["muted"] = true;
    return media;
}
