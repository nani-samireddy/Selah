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

MediaLibrary::MediaLibrary(QObject *parent)
    : QObject(parent) {
    const auto folder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(folder + "/thumbnails");
    m_db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    m_db.setDatabaseName(folder + "/media-library.sqlite");
    if (!m_db.open()) {
        m_error = m_db.lastError().text();
        return;
    }
    QSqlQuery query(m_db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS media(source TEXT PRIMARY KEY,name TEXT NOT "
                    "NULL,kind TEXT NOT NULL,added INTEGER NOT NULL DEFAULT (unixepoch()))")) {
        m_error = query.lastError().text();
    }
    m_player.setVideoSink(&m_sink);
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(6000);
    connect(&m_timeout, &QTimer::timeout, this, &MediaLibrary::finishThumbnail);
    connect(&m_player, &QMediaPlayer::errorOccurred, this, [this] { finishThumbnail(); });
    connect(&m_sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        if (m_thumbnailAsset.isEmpty() || !frame.isValid()) {
            return;
        }
        const auto image = frame.toImage();
        if (image.isNull()) {
            return;
        }
        image.scaled(320, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation)
            .save(thumbnailPath(m_thumbnailAsset["source"].toString()), "JPG", 85);
        finishThumbnail();
    });
    refresh();
    m_bulk = true;
    for (const auto &asset : m_assets) {
        registerAsset(asset.toMap());
    }
    m_bulk = false;
    // Recover imports created before the catalog existed, including unused files.
    QSet<QString> known;
    for (const auto &asset : m_assets) {
        known.insert(asset.toMap()["source"].toString());
    }
    auto discovery = new QFutureWatcher<QVariantList>(this);
    connect(discovery, &QFutureWatcher<QVariantList>::finished, this, [this, discovery] {
        m_bulk = true;
        for (const auto &asset : discovery->result()) {
            registerAsset(asset.toMap());
        }
        m_bulk = false;
        discovery->deleteLater();
        refresh();
    });
    discovery->setFuture(QtConcurrent::run([folder, known] {
        QVariantList found;
        QMimeDatabase mime;
        for (const auto &file : QDir(folder + "/media").entryInfoList(QDir::Files)) {
            const auto source = QUrl::fromLocalFile(file.absoluteFilePath()).toString();
            if (known.contains(source)) {
                continue;
            }
            const auto type = mime.mimeTypeForFile(file).name();
            QString kind = type.startsWith("image/") ? "image"
                : type.startsWith("video/")          ? "video"
                                                     : "";
            if (!kind.isEmpty()) {
                found.append(QVariantMap { { "source", source }, { "kind", kind },
                    { "name",
                        QString("Imported %1 · %2")
                            .arg(kind, file.completeBaseName().left(10)) } });
            }
        }
        return found;
    }));
}

MediaLibrary::~MediaLibrary() {
    const auto connection = m_db.connectionName();
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

QString MediaLibrary::thumbnailPath(const QString &source) const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/thumbnails/"
        + QString::fromLatin1(
            QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha256).toHex())
        + ".jpg";
}

void MediaLibrary::registerAsset(const QVariantMap &element) {
    const auto kind = element["kind"].toString();
    const auto source = element["source"].toString();
    if ((kind != "image" && kind != "video") || !QUrl(source).isLocalFile() || !m_db.isOpen()) {
        return;
    }
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO media(source,name,kind) VALUES(?,?,?) ON CONFLICT(source) DO NOTHING");
    q.addBindValue(source);
    q.addBindValue(element["name"].toString());
    q.addBindValue(kind);
    if (!q.exec()) {
        m_error = q.lastError().text();
        emit changed();
        return;
    }
    const auto thumb = thumbnailPath(source);
    if (!QFileInfo::exists(thumb) && QFileInfo::exists(QUrl(source).toLocalFile())) {
        bool queued = m_thumbnailAsset.value("source") == source;
        for (const auto &asset : m_queue) {
            queued = queued || asset["source"] == source;
        }
        if (!queued) {
            m_queue.enqueue(element);
            QTimer::singleShot(0, this, &MediaLibrary::nextThumbnail);
        }
    }
    if (!m_bulk) {
        refresh();
    }
}

void MediaLibrary::refresh() {
    if (!m_db.isOpen()) {
        return;
    }
    QVariantList result;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT source,name,kind FROM media ORDER BY added DESC,name COLLATE NOCASE")) {
        m_error = q.lastError().text();
        emit changed();
        return;
    }
    while (q.next()) {
        auto source = q.value(0).toString();
        const QFileInfo file(QUrl(source).toLocalFile());
        const auto thumb = thumbnailPath(source);
        result.append(QVariantMap { { "source", source }, { "name", q.value(1) },
            { "kind", q.value(2) }, { "available", file.isFile() }, { "bytes", file.size() },
            { "thumbnail",
                QFileInfo::exists(thumb) ? QUrl::fromLocalFile(thumb).toString() : QString() } });
    }
    m_assets = result;
    emit changed();
}

void MediaLibrary::nextThumbnail() {
    if (!m_thumbnailAsset.isEmpty() || m_queue.isEmpty()) {
        return;
    }
    m_thumbnailAsset = m_queue.dequeue();
    const auto asset = m_thumbnailAsset;
    if (asset["kind"] == "image") {
        auto watcher = new QFutureWatcher<bool>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher] {
            watcher->deleteLater();
            finishThumbnail();
        });
        const auto output = thumbnailPath(asset["source"].toString());
        watcher->setFuture(QtConcurrent::run([asset, output] {
            QImageReader reader(QUrl(asset["source"].toString()).toLocalFile());
            reader.setAutoTransform(true);
            const auto size = reader.size();
            if (size.isValid()) {
                reader.setScaledSize(size.scaled(320, 180, Qt::KeepAspectRatio));
            }
            const auto image = reader.read();
            return !image.isNull() && image.save(output, "JPG", 85);
        }));
    } else {
        m_player.setSource(QUrl(asset["source"].toString()));
        m_timeout.start();
        m_player.play();
    }
}

void MediaLibrary::finishThumbnail() {
    if (m_thumbnailAsset.isEmpty()) {
        return;
    }
    // Clear the asset first: stopping the decoder can emit more signals.
    m_thumbnailAsset.clear();
    m_timeout.stop();
    m_player.stop();
    m_player.setSource(QUrl());
    refresh();
    QTimer::singleShot(0, this, &MediaLibrary::nextThumbnail);
}
