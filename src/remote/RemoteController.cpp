#include "RemoteController.h"
#include "OutputManager.h"
#include "RemotePreview.h"
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QUuid>

namespace {
bool privateAddress(const QHostAddress &ip) {
    bool ok = false;
    const auto value = ip.toIPv4Address(&ok);
    return ok
        && ((value >> 24) == 10 || (value >> 20) == 0xac1 || (value >> 16) == 0xc0a8
            || (value >> 16) == 0xa9fe);
}

}

RemoteController::RemoteController(OutputManager *outputs, MediaLibrary *media, QObject *parent)
    : QObject(parent)
    , m_outputs(outputs)
    , m_media(media) {
    m_clock.start();
    m_server.setHandler([this](const RemoteRequest &request) { return handle(request); });
    connect(outputs, &OutputManager::liveChanged, this, [this] { ++m_revision; });
    connect(outputs, &OutputManager::stateChanged, this, [this] { ++m_revision; });
    m_expiry.setInterval(30000);
    connect(&m_expiry, &QTimer::timeout, this, &RemoteController::expire);
}

QVariantList RemoteController::interfaces() const {
    QVariantList result;
    QSet<QString> seen;
    for (const auto &interface : QNetworkInterface::allInterfaces()) {
        if (!(interface.flags() & QNetworkInterface::IsUp)
            || !(interface.flags() & QNetworkInterface::IsRunning)) {
            continue;
        }
        for (const auto &entry : interface.addressEntries()) {
            const auto ip = entry.ip();
            if (!privateAddress(ip) || seen.contains(ip.toString())) {
                continue;
            }
            seen.insert(ip.toString());
            result.append(QVariantMap { { "address", ip.toString() },
                { "label", interface.humanReadableName() + " · " + ip.toString() } });
        }
    }
    result.append(
        QVariantMap { { "address", "127.0.0.1" }, { "label", "This computer only · 127.0.0.1" } });
    return result;
}

bool RemoteController::start(const QString &ip) {
    if (running()) {
        return true;
    }
    bool offered = false;
    for (const auto &entry : interfaces()) {
        offered |= entry.toMap()["address"].toString() == ip;
    }
    if (!offered || !m_server.listen(QHostAddress(ip), 0)) {
        m_status = "Cannot start remote control. Refresh the network list and try again.";
        emit changed();
        return false;
    }
    m_host = ip + ":" + QString::number(m_server.port());
    m_address = "http://" + m_host;
    m_running = true;
    m_status = ip == "127.0.0.1" ? "Running on this computer only"
                                 : "Ready to pair on your local network";
    m_attempts.clear();
    m_globalAttempts = {};
    rotateCodes();
    m_expiry.start();
    return true;
}

void RemoteController::stop() {
    m_server.stop();
    m_expiry.stop();
    m_sessions.clear();
    m_attempts.clear();
    m_running = false;
    m_address.clear();
    m_host.clear();
    m_controlCode.clear();
    m_speakerCode.clear();
    m_controlQr.clear();
    m_speakerQr.clear();
    m_previews.clear();
    m_status = "Remote control is off";
    emit changed();
}

void RemoteController::rotateCodes() {
    if (!running()) {
        return;
    }
    auto code = [] {
        return QString::number(QRandomGenerator::system()->bounded(1000000)).rightJustified(6, '0');
    };
    m_controlCode = code();
    do {
        m_speakerCode = code();
    } while (m_speakerCode == m_controlCode);
    m_sessions.clear();
    m_controlQr = RemotePreview::qrDataUrl(m_address + "/#control=" + m_controlCode);
    m_speakerQr = RemotePreview::qrDataUrl(m_address + "/#speaker=" + m_speakerCode);
    emit changed();
}

QString RemoteController::randomToken() {
    QByteArray bytes;
    for (int i = 0; i < 8; ++i) {
        const quint32 value = QRandomGenerator::system()->generate();
        bytes.append(reinterpret_cast<const char *>(&value), sizeof(value));
    }
    return QString::fromLatin1(bytes.toHex());
}

void RemoteController::revoke(const QString &id) {
    for (auto it = m_sessions.begin(); it != m_sessions.end();) {
        if (it->id == id) {
            it = m_sessions.erase(it);
        } else {
            ++it;
        }
    }
    emit changed();
}

void RemoteController::expire() {
    const auto now = m_clock.elapsed();
    bool removed = false;
    for (auto it = m_sessions.begin(); it != m_sessions.end();) {
        if (now - it->touched > 30 * 60 * 1000 || now - it->created > 12 * 60 * 60 * 1000) {
            it = m_sessions.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    if (removed) {
        emit changed();
    }
}

QVariantList RemoteController::clients() const {
    QVariantList result;
    for (const auto &session : m_sessions) {
        result.append(QVariantMap {
            { "id", session.id }, { "name", session.name }, { "role", session.role } });
    }
    return result;
}
