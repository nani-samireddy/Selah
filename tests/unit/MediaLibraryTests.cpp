#include "MediaLibrary.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImageReader>
#include <QMimeDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QTemporaryDir>
#include <QUuid>
#include <QVideoFrame>

int MediaLibrary::selfTest() {
    QTemporaryDir temp;
    const auto path = temp.filePath("sample.png");
    QImage image(80, 60, QImage::Format_RGB32);
    image.fill(Qt::cyan);
    if (!image.save(path)) {
        return 1;
    }
    const auto source = QUrl::fromLocalFile(path).toString();
    {
        MediaLibrary lib;
        QVariantMap asset { { "source", source }, { "kind", "image" },
            { "name", "Reusable asset" } };
        lib.registerAsset(asset);
        lib.registerAsset(asset);
        int count = 0;
        for (const auto &v : lib.assets()) {
            if (v.toMap()["source"] == source) {
                count++;
            }
        }
        if (count != 1) {
            return 2;
        }
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(6000);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(&lib, &MediaLibrary::changed, &loop, [&] {
            for (const auto &v : lib.assets()) {
                if (v.toMap()["source"] == source && !v.toMap()["thumbnail"].toString().isEmpty()) {
                    loop.quit();
                }
            }
        });
        timeout.start();
        loop.exec();
        bool thumb = false;
        for (const auto &v : lib.assets()) {
            if (v.toMap()["source"] == source) {
                thumb = !v.toMap()["thumbnail"].toString().isEmpty();
            }
        }
        if (!thumb) {
            return 5;
        }
    }
    {
        MediaLibrary lib;
        bool found = false;
        for (const auto &v : lib.assets()) {
            if (v.toMap()["source"] == source) {
                found = true;
            }
        }
        if (!found) {
            return 3;
        }
        QFile::remove(path);
        lib.refresh();
        for (const auto &v : lib.assets()) {
            if (v.toMap()["source"] == source && v.toMap()["available"].toBool()) {
                return 4;
            }
        }
    }
    qInfo()
        << "Library checks passed: deduplication, persistent catalog, and missing-file detection.";
    return 0;
}
