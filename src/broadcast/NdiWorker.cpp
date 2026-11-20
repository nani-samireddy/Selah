#include "NdiWorker.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#if CHURCHDECK_HAS_NDI_SDK
#include <Processing.NDI.Lib.h>
#endif

NdiWorker::NdiWorker(QString name, QString path, int fps, bool diagnostic, int delay)
    : m_name(std::move(name))
    , m_runtime(std::move(path))
    , m_fps(fps)
    , m_testDelay(delay)
    , m_diagnostic(diagnostic) {
}

NdiWorker::~NdiWorker() {
    requestStop();
    wait();
}

bool NdiWorker::sdkEnabled() {
    return CHURCHDECK_HAS_NDI_SDK;
}

QString NdiWorker::discoverRuntime(const QString &configured) {
    QStringList candidates;
    if (!configured.isEmpty()) {
        candidates << configured;
    }
    const auto environment = qEnvironmentVariable("NDI_RUNTIME_PATH");
    if (!environment.isEmpty()) {
        candidates << environment;
    }
#ifdef Q_OS_WIN
    for (const auto &directory : { qEnvironmentVariable("NDI_RUNTIME_DIR_V6"),
             qEnvironmentVariable("NDI_RUNTIME_DIR_V5"), QCoreApplication::applicationDirPath() }) {
        if (!directory.isEmpty()) {
            candidates << QDir(directory).filePath("Processing.NDI.Lib.x64.dll");
        }
    }
#elif defined(Q_OS_MACOS)
    candidates << "/usr/local/lib/libndi.dylib"
               << "/Library/NDI SDK for Apple/lib/macOS/libndi.dylib"
               << "/Library/NDI SDK for Apple/lib/macOS/libndi.6.dylib";
#else
    candidates << "/usr/local/lib/libndi.so.6" << "/usr/lib/libndi.so.6";
#endif
    for (const auto &path : candidates) {
        if (QFileInfo(path).isFile()) {
            return QFileInfo(path).absoluteFilePath();
        }
    }
    return {};
}

void NdiWorker::submit(QImage frame) {
    QMutexLocker lock(&m_mutex);
    if (m_stopping) {
        return;
    }
    if (!m_pending.isNull()) {
        ++m_dropped;
    }
    m_pending = std::move(frame);
    m_condition.wakeOne();
}

void NdiWorker::requestStop() {
    QMutexLocker lock(&m_mutex);
    m_stopping = true;
    m_pending = QImage();
    m_condition.wakeAll();
}

void NdiWorker::run() {
#if CHURCHDECK_HAS_NDI_SDK
    QLibrary library;
    decltype(&NDIlib_destroy) destroyRuntime = nullptr;
    decltype(&NDIlib_send_destroy) destroySender = nullptr;
    decltype(&NDIlib_send_send_video_v2) sendVideo = nullptr;
    decltype(&NDIlib_send_get_no_connections) getConnections = nullptr;
    NDIlib_send_instance_t sender = nullptr;
    const auto ndiName = m_name.toUtf8();
    if (!m_diagnostic) {
        const auto path = discoverRuntime(m_runtime);
        if (path.isEmpty()) {
            emit failed(
                "NDI runtime not found. Install the official runtime or choose its library file.");
            return;
        }
        library.setFileName(path);
        if (!library.load()) {
            emit failed("Could not load NDI runtime: " + library.errorString());
            return;
        }
        const auto initialize
            = reinterpret_cast<decltype(&NDIlib_initialize)>(library.resolve("NDIlib_initialize"));
        const auto create = reinterpret_cast<decltype(&NDIlib_send_create)>(
            library.resolve("NDIlib_send_create"));
        destroyRuntime
            = reinterpret_cast<decltype(destroyRuntime)>(library.resolve("NDIlib_destroy"));
        destroySender
            = reinterpret_cast<decltype(destroySender)>(library.resolve("NDIlib_send_destroy"));
        sendVideo
            = reinterpret_cast<decltype(sendVideo)>(library.resolve("NDIlib_send_send_video_v2"));
        getConnections = reinterpret_cast<decltype(getConnections)>(
            library.resolve("NDIlib_send_get_no_connections"));
        if (!initialize || !create || !destroyRuntime || !destroySender || !sendVideo
            || !getConnections) {
            emit failed("The selected library does not provide the required NDI SDK functions.");
            return;
        }
        if (!initialize()) {
            emit failed("NDI initialization failed on this machine.");
            return;
        }
        NDIlib_send_create_t settings;
        settings.p_ndi_name = ndiName.constData();
        settings.p_groups = nullptr;
        settings.clock_video = false;
        settings.clock_audio = false;
        sender = create(&settings);
        if (!sender) {
            destroyRuntime();
            emit failed("Could not create NDI sender.");
            return;
        }
    }
#else
    if (!m_diagnostic) {
        emit failed(
            "This build needs the official NDI SDK headers before network sending can be enabled.");
        return;
    }
#endif
    emit senderReady();
    while (true) {
        QImage frame;
        {
            QMutexLocker lock(&m_mutex);
            while (!m_stopping && m_pending.isNull()) {
                m_condition.wait(&m_mutex);
            }
            if (m_stopping) {
                break;
            }
            frame = std::move(m_pending);
            m_pending = QImage();
        }
        if (m_testDelay > 0) {
            QThread::msleep(m_testDelay);
        }
        // RGBA8888 is straight alpha, unlike Qt Quick's premultiplied render target.
        if (frame.format() != QImage::Format_RGBA8888) {
            frame = frame.convertToFormat(QImage::Format_RGBA8888);
        }
#if CHURCHDECK_HAS_NDI_SDK
        if (sender) {
            NDIlib_video_frame_v2_t video;
            video.xres = frame.width();
            video.yres = frame.height();
            video.FourCC = NDIlib_FourCC_type_RGBA;
            video.frame_rate_N = m_fps;
            video.frame_rate_D = 1;
            video.picture_aspect_ratio = float(frame.width()) / frame.height();
            video.frame_format_type = NDIlib_frame_format_type_progressive;
            video.timecode = NDIlib_send_timecode_synthesize;
            video.p_data = frame.bits();
            video.line_stride_in_bytes = frame.bytesPerLine();
            video.p_metadata = nullptr;
            sendVideo(sender, &video);
            m_connections = getConnections(sender, 0);
        }
#endif
        ++m_processed;
    }
#if CHURCHDECK_HAS_NDI_SDK
    if (sender) {
        destroySender(sender);
        destroyRuntime();
    }
#endif
}
