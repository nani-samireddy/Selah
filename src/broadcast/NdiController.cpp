#include "NdiController.h"
#include "OutputManager.h"
#include <QJSValue>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QRegularExpression>
#include <rhi/qrhi.h>

NdiController::NdiController(OutputManager *outputs, QObject *parent)
    : QObject(parent)
    , m_outputs(outputs) {
    m_runtimePath = NdiWorker::discoverRuntime();
    if (!sdkEnabled()) {
        m_status = "NDI SDK headers missing — capture diagnostics are available";
    }
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &NdiController::renderFrame);
}

NdiController::~NdiController() {
    m_timer.stop();
    if (m_worker) {
        m_worker->disconnect(this);
        m_worker->requestStop();
        m_worker->wait();
        delete m_worker;
        m_worker = nullptr;
    }
    releaseScene();
}

QVariantList NdiController::rendererProbe() const {
    if (!m_scene || !m_engine) {
        return {};
    }
    auto object = m_engine->newQObject(m_scene.get());
    return object.property("probe").callWithInstance(object).toVariant().toList();
}

bool NdiController::offscreenHidden() const {
    return m_window && !m_window->isVisible() && !m_window->handle();
}

void NdiController::setVerificationPattern(bool value) {
    if (m_scene) {
        m_scene->setProperty("verificationPattern", value);
    }
}

void NdiController::setBlackout(bool value) {
    m_blackout = value;
    if (m_scene) {
        m_scene->setProperty("blackout", value);
    }
    emit changed();
}

bool NdiController::createScene(int width, int height, bool overlayOnly) {
    if (!m_engine) {
        m_status = "Broadcast scene engine is unavailable";
        return false;
    }
    m_size = QSize(width, height);
    m_control = std::make_unique<QQuickRenderControl>();
    m_window = std::make_unique<QQuickWindow>(m_control.get());
    m_window->setColor(Qt::transparent);
    m_window->setGeometry(0, 0, width, height);
    QQmlComponent component(m_engine);
    component.loadFromModule("ChurchDeck", "BroadcastScene");
    if (component.isError()) {
        m_status = component.errorString();
        releaseScene();
        return false;
    }
    QVariantMap initial { { "controller", QVariant::fromValue(m_outputs) },
        { "overlayOnly", overlayOnly }, { "blackout", m_blackout } };
    m_scene.reset(qobject_cast<QQuickItem *>(component.createWithInitialProperties(initial)));
    if (!m_scene) {
        m_status = component.errorString();
        releaseScene();
        return false;
    }
    m_scene->setParentItem(m_window->contentItem());
    m_scene->setScale(width / 1920.0);
    m_scene->setTransformOrigin(QQuickItem::TopLeft);
    // No native window is created or shown: this works without assigning a display.
    if (!m_control->initialize()) {
        m_status = "Offscreen graphics initialization failed";
        releaseScene();
        return false;
    }
    auto rhi = m_control->rhi();
    if (!rhi) {
        m_status = "Offscreen GPU capture requires the Qt RHI renderer";
        releaseScene();
        return false;
    }
    m_backend = QString::fromLatin1(rhi->backendName());
    m_texture.reset(rhi->newTexture(QRhiTexture::RGBA8, m_size, 1,
        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    m_depth.reset(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_size));
    if (!m_texture->create() || !m_depth->create()) {
        m_status = "Could not allocate broadcast render textures";
        releaseScene();
        return false;
    }
    QRhiTextureRenderTargetDescription description(QRhiColorAttachment(m_texture.get()));
    description.setDepthStencilBuffer(m_depth.get());
    m_target.reset(rhi->newTextureRenderTarget(description));
    m_pass.reset(m_target->newCompatibleRenderPassDescriptor());
    m_target->setRenderPassDescriptor(m_pass.get());
    if (!m_target->create()) {
        m_status = "Could not create broadcast render target";
        releaseScene();
        return false;
    }
    m_window->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(m_target.get()));
    return true;
}

void NdiController::releaseScene() {
    if (m_control && m_control->rhi()) {
        m_control->rhi()->finish();
    }
    m_scene.reset();
    if (m_control) {
        m_control->invalidate();
    }
    if (m_window) {
        m_window->setRenderTarget(QQuickRenderTarget());
    }
    m_target.reset();
    m_pass.reset();
    m_depth.reset();
    m_texture.reset();
    m_window.reset();
    m_control.reset();
}

bool NdiController::start(
    const QString &name, int width, int height, int fps, bool overlayOnly, bool diagnostic) {
    if (busy()) {
        m_status = "Stop the current broadcast session first";
        emit changed();
        return false;
    }
    if ((QSize(width, height) != QSize(1280, 720) && QSize(width, height) != QSize(1920, 1080))
        || (fps != 30 && fps != 60)) {
        m_status = "Choose 720p or 1080p at 30 or 60 fps";
        emit changed();
        return false;
    }
    if (!diagnostic && !sdkEnabled()) {
        m_status = "Install the official NDI SDK and rebuild to enable network sending";
        emit changed();
        return false;
    }
    if (!diagnostic && NdiWorker::discoverRuntime(m_runtimePath).isEmpty()) {
        m_status = "NDI runtime not found — choose the installed runtime library";
        emit changed();
        return false;
    }
    auto safeName = name.trimmed();
    safeName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), " ");
    safeName = safeName.left(160);
    if (safeName.isEmpty()) {
        safeName = "Selah Broadcast";
    }
    while (safeName.toUtf8().size() > 180) {
        safeName.chop(1);
    }
    if (!safeName.isEmpty() && safeName.back().isHighSurrogate()) {
        safeName.chop(1);
    }
    m_diagnostic = diagnostic;
    m_overlayOnly = overlayOnly;
    m_targetFps = fps;
    m_durationMs = 0;
    m_frames = 0;
    m_processed = 0;
    m_dropped = 0;
    m_totalRenderMs = 0;
    m_lastFrame = QImage();
    if (!createScene(width, height, overlayOnly)) {
        emit changed();
        return false;
    }
    m_worker = new NdiWorker(safeName, m_runtimePath, fps, diagnostic);
    connect(m_worker, &NdiWorker::senderReady, this, [this, fps] {
        m_active = true;
        m_elapsed.start();
        m_timer.start(qMax(1, qRound(1000.0 / fps)));
        m_status = m_diagnostic ? "Capture-only diagnostic · no NDI network transmission"
                                : "NDI video sender active";
        emit changed();
    });
    connect(m_worker, &NdiWorker::failed, this, [this](const QString &error) {
        m_status = error;
        if (m_active) {
            m_durationMs = m_elapsed.elapsed();
        }
        m_active = false;
        m_timer.stop();
        releaseScene();
        emit changed();
    });
    connect(m_worker, &QThread::finished, this, [this] {
        m_processed = m_worker->processed();
        m_dropped = m_worker->dropped();
        m_worker->deleteLater();
        m_worker = nullptr;
        m_active = false;
        m_timer.stop();
        releaseScene();
        emit changed();
        if (m_status == "Stopping broadcast…") {
            m_status = "Broadcast stopped";
            emit changed();
        }
    });
    m_status = diagnostic ? "Starting offscreen capture…" : "Starting NDI sender…";
    m_worker->start();
    emit changed();
    return true;
}

void NdiController::stop() {
    if (m_active) {
        m_durationMs = m_elapsed.elapsed();
    }
    m_timer.stop();
    m_active = false;
    if (m_worker) {
        m_worker->requestStop();
        m_status = "Stopping broadcast…";
    } else {
        releaseScene();
        m_status = "Broadcast stopped";
    }
    emit changed();
}

void NdiController::renderFrame() {
    if (!m_active || !m_control || !m_worker) {
        return;
    }
    QElapsedTimer duration;
    duration.start();
    auto rhi = m_control->rhi();
    m_control->polishItems();
    m_control->beginFrame();
    m_control->sync();
    m_control->render();
    QRhiReadbackResult result;
    auto updates = rhi->nextResourceUpdateBatch();
    updates->readBackTexture(QRhiReadbackDescription(m_texture.get()), &result);
    m_control->commandBuffer()->resourceUpdate(updates);
    // endOffscreenFrame completes pending readbacks before returning.
    m_control->endFrame();
    if (result.data.isEmpty()) {
        stop();
        m_status = "GPU readback returned no video frame";
        emit changed();
        return;
    }
    const auto format = result.format == QRhiTexture::BGRA8 ? QImage::Format_ARGB32_Premultiplied
                                                            : QImage::Format_RGBA8888_Premultiplied;
    QImage frame(reinterpret_cast<const uchar *>(result.data.constData()), result.pixelSize.width(),
        result.pixelSize.height(), result.pixelSize.width() * 4, format);
    if (rhi->isYUpInFramebuffer()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        frame = frame.flipped(Qt::Vertical);
#else
        frame = frame.mirrored(false, true);
#endif
    }
    m_lastFrame = frame.convertToFormat(QImage::Format_RGBA8888);
    // convertToFormat may share an already straight buffer; own it before result is destroyed.
    if (m_lastFrame.constBits() == reinterpret_cast<const uchar *>(result.data.constData())) {
        m_lastFrame = m_lastFrame.copy();
    }
    m_worker->submit(m_lastFrame);
    ++m_frames;
    m_totalRenderMs += duration.nsecsElapsed() / 1000000.0;
    if (m_frames % 10 == 0) {
        emit changed();
    }
}
