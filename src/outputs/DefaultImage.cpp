#include "DefaultImage.h"
#include "DocumentElements.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QSaveFile>
#include <QStandardPaths>

QString DefaultImage::prepare(const QUrl &source, QString *error) {
    const QFileInfo info(source.toLocalFile());
    if (!source.isLocalFile() || !info.isReadable() || !info.isFile()
        || info.size() > 50 * 1024 * 1024) {
        *error = "Choose a readable local image under 50 MB";
        return {};
    }
    QImageReader reader(info.absoluteFilePath());
    reader.setAutoTransform(true);
    const auto size = reader.size();
    if (!reader.canRead() || !size.isValid() || qint64(size.width()) * size.height() > 50000000) {
        *error = "Choose a supported image of up to 50 megapixels";
        return {};
    }
    reader.setScaledSize(size.scaled(1920, 1080, Qt::KeepAspectRatio));
    if (reader.read().isNull()) {
        *error = "This image could not be decoded";
        return {};
    }
    QFile input(info.absoluteFilePath());
    if (!input.open(QIODevice::ReadOnly)) {
        *error = "Could not read the default image";
        return {};
    }
    const auto bytes = input.readAll();
    const auto directory
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/default-images";
    const auto path = directory + "/"
        + QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())
        + "." + QString::fromLatin1(reader.format()).toLower();
    if (!QDir().mkpath(directory)) {
        *error = "Could not create default image storage";
        return {};
    }
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size()
        || !output.commit()) {
        *error = "Could not save the default image";
        return {};
    }
    return QUrl::fromLocalFile(path).toString();
}

QVariantMap DefaultImage::layer(const QString &source, const QString &fit) {
    auto image = DocumentElements::text("Default image", "", 0, 0, 1920, 1080, 36, "Arial");
    image["uid"] = "default-image";
    image["kind"] = "image";
    image["source"] = source;
    image["fit"] = fit;
    return image;
}
