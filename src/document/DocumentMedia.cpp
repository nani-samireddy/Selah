#include "DocumentElements.h"
#include "EditorDocument.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImageReader>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QtConcurrent>

QVariantMap EditorDocument::prepareMedia(const QUrl &url) {
    if (!url.isLocalFile()) {
        return { { "importError", "Choose an image or video on this computer" } };
    }
    const QFileInfo info(url.toLocalFile());
    if (!info.isFile() || !info.isReadable()) {
        return { { "importError", "Media file is missing or unreadable" } };
    }
    QImageReader reader(info.absoluteFilePath());
    const bool image = reader.canRead();
    const auto mime = QMimeDatabase().mimeTypeForFile(info).name();
    if (!image && !mime.startsWith("video/")) {
        return { { "importError", "Choose a supported image or video file" } };
    }
    QFile input(info.absoluteFilePath());
    if (!input.open(QIODevice::ReadOnly)) {
        return { { "importError", "Could not read media file" } };
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&input)) {
        return { { "importError", "Could not import media" } };
    }
    const auto folder
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/media";
    if (!QDir().mkpath(folder)) {
        return { { "importError", "Could not create media folder" } };
    }
    const auto destination
        = folder + "/" + QString::fromLatin1(hash.result().toHex()) + "." + info.suffix().toLower();
    if (!QFile::exists(destination) && !QFile::copy(info.absoluteFilePath(), destination)) {
        return { { "importError", "Could not copy media into the library" } };
    }
    auto el = DocumentElements::text(info.fileName(), "", 0, 0, 1920, 1080, 36, "Arial");
    el["kind"] = image ? "image" : "video";
    el["source"] = QUrl::fromLocalFile(destination).toString();
    el["fit"] = "contain";
    el["loop"] = true;
    el["muted"] = true;
    return el;
}

bool EditorDocument::insertMedia(const QVariantMap &result, int slideIndex) {
    if (result.contains("importError")) {
        setStatus(result["importError"].toString());
        return false;
    }
    emit mediaImported(result);
    checkpoint();
    auto s = m_slides[slideIndex].toMap();
    auto list = s["elements"].toList();
    list.prepend(result);
    s["elements"] = list;
    m_slides[slideIndex] = s;
    if (m_current == slideIndex) {
        m_selected = 0;
    }
    commit();
    return true;
}

bool EditorDocument::addMedia(const QUrl &url) {
    return insertMedia(prepareMedia(url), m_current);
}

void EditorDocument::importMedia(const QUrl &url, bool insertOnSlide) {
    if (m_importing) {
        return;
    }
    // Navigation may change while copying; project replacement must cancel insertion.
    const int targetSlide = m_current, generation = m_generation;
    m_importing = true;
    emit importingChanged();
    setStatus("Importing media…");
    auto watcher = new QFutureWatcher<QVariantMap>(this);
    connect(watcher, &QFutureWatcher<QVariantMap>::finished, this,
        [this, watcher, targetSlide, generation, insertOnSlide] {
            const auto result = watcher->result();
            watcher->deleteLater();
            m_importing = false;
            emit importingChanged();
            if (!result.contains("importError") && (!insertOnSlide || generation != m_generation)) {
                emit mediaImported(result);
            }
            if (!insertOnSlide) {
                setStatus(result.contains("importError") ? result["importError"].toString()
                                                         : "Added to media library");
                if (result.contains("importError")) {
                    emit importFailed(status());
                }
                return;
            }
            if (generation != m_generation) {
                setStatus("Project changed during import — media kept in library");
                emit importFailed(status());
                return;
            }
            if (!insertMedia(result, targetSlide)) {
                emit importFailed(status());
            }
        });
    watcher->setFuture(QtConcurrent::run([url] { return prepareMedia(url); }));
}

bool EditorDocument::useMedia(const QVariantMap &asset) {
    const auto source = QUrl(asset["source"].toString());
    if (!source.isLocalFile() || !QFileInfo::exists(source.toLocalFile())) {
        setStatus("This library file is missing");
        return false;
    }
    auto el = DocumentElements::text(asset["name"].toString(), "", 0, 0, 1920, 1080, 36, "Arial");
    el["kind"] = asset["kind"];
    el["source"] = source.toString();
    el["fit"] = "contain";
    el["loop"] = true;
    el["muted"] = true;
    if (el["kind"] != "image" && el["kind"] != "video") {
        return false;
    }
    return insertMedia(el, m_current);
}

void EditorDocument::importBackground(const QUrl &url) {
    if (m_importing) {
        return;
    }
    const int targetSlide = m_current, generation = m_generation;
    m_importing = true;
    emit importingChanged();
    setStatus("Importing background…");
    auto watcher = new QFutureWatcher<QVariantMap>(this);
    connect(watcher, &QFutureWatcher<QVariantMap>::finished, this,
        [this, watcher, targetSlide, generation] {
            const auto result = watcher->result();
            watcher->deleteLater();
            m_importing = false;
            emit importingChanged();
            if (result.contains("importError") || result.value("kind") != "image") {
                setStatus(result.value("importError", "Choose a supported image for the background")
                        .toString());
                emit importFailed(status());
                return;
            }
            emit mediaImported(result);
            if (generation != m_generation) {
                setStatus("Project changed during import — image kept in library");
                return;
            }
            checkpoint();
            auto slide = m_slides[targetSlide].toMap();
            slide["backgroundFill"] = QVariantMap { { "type", "image" },
                { "source", result.value("source") }, { "fit", "cover" } };
            slide["ambientOpacity"] = 0.0;
            const auto color = QColor(slide.value("background").toString());
            if (color.alpha() == 0) {
                const QColor previous(slide.value("backgroundRestore").toString());
                slide["background"] = previous.isValid() && previous.alpha() > 0
                    ? previous.name(QColor::HexArgb)
                    : QString("#252A3A");
                slide.remove("backgroundRestore");
            }
            m_slides[targetSlide] = slide;
            commit();
        });
    watcher->setFuture(QtConcurrent::run([url] { return prepareMedia(url); }));
}
