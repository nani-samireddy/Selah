#pragma once
#include "NdiWorker.h"
#include <functional>
#include <memory>
#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>

class QQmlEngine;
class QQuickRenderControl;
class QQuickWindow;
class QQuickItem;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;
class OutputManager;

// GUI-thread offscreen rendering; completed straight-alpha frames go to NdiWorker.
class NdiController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool sdkEnabled READ sdkEnabled CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool diagnostic READ diagnostic NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString runtimePath READ runtimePath WRITE setRuntimePath NOTIFY changed)
    Q_PROPERTY(QString backendName READ backendName NOTIFY changed)
    Q_PROPERTY(quint64 framesCaptured READ framesCaptured NOTIFY changed)
    Q_PROPERTY(quint64 framesProcessed READ framesProcessed NOTIFY changed)
    Q_PROPERTY(quint64 droppedFrames READ droppedFrames NOTIFY changed)
    Q_PROPERTY(double captureFps READ captureFps NOTIFY changed)
    Q_PROPERTY(double averageRenderMs READ averageRenderMs NOTIFY changed)
    Q_PROPERTY(int connections READ connections NOTIFY changed)
    Q_PROPERTY(bool blackout READ blackout WRITE setBlackout NOTIFY changed)
    Q_PROPERTY(int frameWidth READ frameWidth NOTIFY changed)
    Q_PROPERTY(int targetFps READ targetFps NOTIFY changed)
    Q_PROPERTY(bool overlayOnly READ overlayOnly NOTIFY changed)
    Q_PROPERTY(quint64 lateCaptureFrames READ lateCaptureFrames NOTIFY changed)
public:
    explicit NdiController(OutputManager *outputs, QObject *parent = nullptr);
    ~NdiController() override;

    void setEngine(QQmlEngine *engine) { m_engine = engine; }

    bool sdkEnabled() const { return NdiWorker::sdkEnabled(); }

    bool active() const { return m_active; }

    bool busy() const { return m_worker; }

    bool diagnostic() const { return m_diagnostic; }

    QString status() const { return m_status; }

    QString runtimePath() const { return m_runtimePath; }

    void setRuntimePath(const QString &path) {
        if (!busy()) {
            m_runtimePath = path;
            emit changed();
        }
    }

    QString backendName() const { return m_backend; }

    quint64 framesCaptured() const { return m_frames; }

    quint64 framesProcessed() const { return m_worker ? m_worker->processed() : m_processed; }

    quint64 droppedFrames() const { return m_worker ? m_worker->dropped() : m_dropped; }

    double captureFps() const {
        const auto ms = m_active && m_elapsed.isValid() ? m_elapsed.elapsed() : m_durationMs;
        return ms > 0 ? 1000.0 * m_frames / ms : 0;
    }

    double averageRenderMs() const { return m_frames ? m_totalRenderMs / m_frames : 0; }

    int connections() const { return m_worker ? m_worker->connections() : 0; }

    bool blackout() const { return m_blackout; }

    int frameWidth() const { return m_size.width(); }

    int targetFps() const { return m_targetFps; }

    bool overlayOnly() const { return m_overlayOnly; }

    quint64 lateCaptureFrames() const {
        const auto ms = m_active && m_elapsed.isValid() ? m_elapsed.elapsed() : m_durationMs;
        const auto expected = quint64(qMax<qint64>(0, ms) * m_targetFps / 1000);
        return expected > m_frames ? expected - m_frames : 0;
    }

    void setBlackout(bool value);

    QImage lastFrame() const { return m_lastFrame; }

    QVariantList rendererProbe() const;
    bool offscreenHidden() const;
    void setVerificationPattern(bool value);
    Q_INVOKABLE bool start(const QString &name, int width, int height, int fps, bool overlayOnly,
        bool diagnostic = false);
    Q_INVOKABLE void stop();

    Q_INVOKABLE void selectRuntime(const QUrl &url) {
        if (url.isLocalFile()) {
            setRuntimePath(url.toLocalFile());
        }
    }

signals:
    void changed();

private:
    bool createScene(int width, int height, bool overlayOnly);
    void releaseScene();
    void renderFrame();
    QQmlEngine *m_engine = nullptr;
    OutputManager *m_outputs;
    std::unique_ptr<QQuickRenderControl> m_control;
    std::unique_ptr<QQuickWindow> m_window;
    std::unique_ptr<QQuickItem> m_scene;
    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<QRhiRenderBuffer> m_depth;
    std::unique_ptr<QRhiTextureRenderTarget> m_target;
    std::unique_ptr<QRhiRenderPassDescriptor> m_pass;
    NdiWorker *m_worker = nullptr;
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    QImage m_lastFrame;
    QSize m_size;
    bool m_stopRequested = false;
    bool m_active = false, m_diagnostic = true, m_blackout = false, m_overlayOnly = false;
    int m_targetFps = 30;
    qint64 m_durationMs = 0;
    QString m_status = "Ready for capture diagnostics", m_runtimePath, m_backend;
    quint64 m_frames = 0, m_processed = 0, m_dropped = 0;
    double m_totalRenderMs = 0;
};
