#include "OutputManager.h"
#include "ServiceCueActions.h"
#include "ServiceCues.h"
#include <QFileInfo>
#include <QHostAddress>
#include <QImageReader>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopedValueRollback>
#include <QUdpSocket>

bool ServiceCues::preflight(const QVariantMap &cue, QString *error) const {
    if (!ServiceCueActions::valid(cue, error)) {
        return false;
    }
    auto fail = [&](int index, const QString &message) {
        *error = QString("Action %1: %2").arg(index + 1).arg(message);
        return false;
    };
    bool audienceActive = m_outputs->cueFullscreen("audience"),
         stageActive = m_outputs->cueFullscreen("stage");
    for (int index = 0; index < cue["actions"].toList().size(); ++index) {
        const auto action = cue["actions"].toList()[index].toMap();
        const auto type = action["type"].toString();
        if (type == "audience" && action["value"] == "default"
            && !m_outputs->defaultImageAvailable()) {
            return fail(index, "Choose a standby media first");
        }
        if (type == "audience" && action["value"] == "lyrics" && !m_outputs->liveCount()) {
            return fail(index, "Load a presentation first");
        }
        if (type == "navigate" && action["slide"].toInt() > m_outputs->liveCount()) {
            return fail(index, "That slide is not in the loaded live deck");
        }
        if (type == "item") {
            bool found = false;
            for (const auto &row : m_outputs->liveOrder()) {
                found |= row.toMap()["itemId"] == action["itemId"];
            }
            if (!found) {
                return fail(index, "That service item is not in the loaded live deck");
            }
        }
        if (type == "resume" && !m_outputs->canResumeProgram()) {
            return fail(index, "There is no saved program resume point");
        }
        if (type == "overlay" && !m_outputs->hasAudienceContent()) {
            return fail(index, "Load content before showing an overlay");
        }
        if (type == "overlay" || type == "clip") {
            const auto path = QUrl(action["source"].toString()).toLocalFile();
            const QFileInfo file(path);
            if (!file.isFile() || !file.isReadable() || file.size() == 0) {
                return fail(index, "Choose an available local file");
            }
            if (type == "overlay") {
                QImageReader reader(path);
                const auto size = reader.size();
                if (!reader.canRead() || size.width() <= 0 || size.height() <= 0
                    || qint64(size.width()) * size.height() > 64000000) {
                    return fail(index, "Choose a readable image under 64 megapixels");
                }
            } else if (!QStringList {
                           "wav", "mp3", "ogg", "m4a", "aac", "flac", "aiff", "aif", "opus" }
                           .contains(file.suffix().toLower())) {
                return fail(index, "Choose a supported audio file");
            }
        }
        if (type == "osc" && QHostAddress(action["host"].toString()).isNull()) {
            return fail(index, "Use a numeric IPv4 or IPv6 address for OSC");
        }
        if (type == "output") {
            const auto role = action["role"].toString(), value = action["value"].toString();
            if (value != "stop" && !m_outputs->hasCueWindow(role)) {
                return fail(index, "The output window is unavailable");
            }
            if (value == "start") {
                const auto display
                    = role == "audience" ? m_outputs->audienceDisplay() : m_outputs->stageDisplay();
                bool available = false;
                for (const auto &row : m_outputs->displays()) {
                    const auto screen = row.toMap();
                    available |= screen["id"] == display && !screen["operator"].toBool();
                }
                if (!available) {
                    return fail(index, "Assign an available external display in Displays first");
                }
                const bool otherActive = role == "audience" ? stageActive : audienceActive;
                const auto otherDisplay
                    = role == "audience" ? m_outputs->stageDisplay() : m_outputs->audienceDisplay();
                if (otherActive && display == otherDisplay) {
                    return fail(index, "Both outputs cannot use the same fullscreen display");
                }
            }
            if (role == "audience") {
                audienceActive = value == "start";
            } else {
                stageActive = value == "start";
            }
        }
    }
    return true;
}

bool ServiceCues::testCue(const QVariantMap &input) {
    QString error;
    const auto cue = ServiceCueActions::normalize(input);
    const bool ok = preflight(cue, &error);
    m_status = ok ? QString("Test passed · %1 actions validated. Nothing was executed or sent.")
                        .arg(cue["actions"].toList().size())
                  : error;
    emit changed();
    return ok;
}

bool ServiceCues::runCue(const QVariantMap &input) {
    if (m_running) {
        return false;
    }
    const auto cue = ServiceCueActions::normalize(input);
    QString error;
    if (!preflight(cue, &error)) {
        m_status = error;
        emit changed();
        return false;
    }
    m_runningName = cue["name"].toString();
    m_actions = cue["actions"].toList();
    m_step = 0;
    m_running = true;
    m_snapshot = m_outputs->snapshotId();
    next();
    return true;
}

void ServiceCues::fail(const QString &error) {
    const auto name = m_runningName;
    const int step = m_step;
    cancel();
    m_status = (name.isEmpty() ? QString() : name + " · ")
        + QString("stopped at action %1: ").arg(step) + error;
    emit changed();
}

void ServiceCues::cancel() {
    const bool wasRunning = m_running;
    m_running = false;
    m_wait.stop();
    m_player.stop();
    if (m_reply) {
        auto reply = m_reply;
        m_reply = nullptr;
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    if (wasRunning) {
        m_status = m_runningName + " cancelled · completed actions are kept";
        emit changed();
    }
}

void ServiceCues::next() {
    if (!m_running) {
        return;
    }
    if (m_outputs->snapshotId() != m_snapshot) {
        cancel();
        return;
    }
    while (m_running && m_step < m_actions.size()) {
        const auto action = m_actions[m_step++].toMap();
        const auto type = action["type"].toString(), value = action["value"].toString();
        m_status
            = QString("%1 · action %2 of %3").arg(m_runningName).arg(m_step).arg(m_actions.size());
        emit changed();
        QScopedValueRollback<bool> executing(m_executing, true);
        if (type == "delay") {
            m_wait.start(action["durationMs"].toInt());
            return;
        }
        if (type == "stage") {
            m_outputs->setMusicianView(value == "musician");
        } else if (type == "audio") {
            m_outputs->setAudioEnabled(value == "on");
        } else if (type == "media") {
            m_outputs->setPlaying(value == "play");
        } else if (type == "message") {
            m_outputs->setStageMessage(action["text"].toString());
        } else if (type == "timer") {
            if (value == "start") {
                m_outputs->startCountdown(action["minutes"].toInt());
            } else {
                m_outputs->stopCountdown();
            }
        } else if (type == "audience") {
            if (value == "default") {
                if (!m_outputs->showDefaultImage(true)) {
                    fail("Standby media is unavailable");
                    return;
                }
                m_outputs->blackout("audience", false);
            } else if (value == "lyrics") {
                m_outputs->showDefaultImage(false);
                m_outputs->clearText(false);
                m_outputs->blackout("audience", false);
            } else if (value == "clear") {
                m_outputs->clearText(true);
            } else if (value == "restore") {
                m_outputs->clearText(false);
                m_outputs->blackout("audience", false);
            } else {
                m_outputs->blackout("audience", true);
            }
        } else if (type == "navigate" || type == "item" || type == "resume") {
            int target = action["slide"].toInt() - 1;
            if (type == "item") {
                target = -1;
                for (const auto &row : m_outputs->liveOrder()) {
                    if (row.toMap()["itemId"] == action["itemId"]) {
                        target = row.toMap()["index"].toInt();
                        break;
                    }
                }
            }
            if (type == "resume") {
                if (!m_outputs->resumeProgram()) {
                    fail("Program resume point is unavailable");
                    return;
                }
            } else {
                if (target < 0 || target >= m_outputs->liveCount()) {
                    fail("Target slide is unavailable");
                    return;
                }
                m_outputs->advance(target - m_outputs->liveIndex());
            }
            m_snapshot = m_outputs->snapshotId();
        } else if (type == "transition") {
            m_outputs->setCueTransition(action["effect"].toString(), action["durationMs"].toInt());
        } else if (type == "overlay") {
            if (!m_outputs->setCueOverlay(QUrl(action["source"].toString()))) {
                fail("Overlay is unavailable");
                return;
            }
        } else if (type == "clearOverlay") {
            m_outputs->clearCueOverlay();
        } else if (type == "clip") {
            m_player.setSource(QUrl(action["source"].toString()));
            m_player.play();
        } else if (type == "stopClip") {
            m_player.stop();
        } else if (type == "output") {
            const auto role = action["role"].toString();
            if (value == "stop") {
                m_outputs->stop(role);
            } else if (value == "preview") {
                m_outputs->preview(role);
            } else if (!m_outputs->route(role,
                           role == "audience" ? m_outputs->audienceDisplay()
                                              : m_outputs->stageDisplay())) {
                fail(m_outputs->status());
                return;
            }
        } else if (type == "osc") {
            QUdpSocket socket;
            const auto packet = ServiceCueActions::oscPacket(action);
            if (socket.writeDatagram(packet, QHostAddress(action["host"].toString()),
                    quint16(action["port"].toInt()))
                != packet.size()) {
                fail("OSC datagram could not be sent");
                return;
            }
        } else if (type == "http") {
            QNetworkRequest request(QUrl(action["url"].toString()));
            request.setAttribute(
                QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
            request.setTransferTimeout(5000);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            auto reply = action["method"] == "POST"
                ? m_network.post(request, action["body"].toString().toUtf8())
                : m_network.get(request);
            m_reply = reply;
            connect(reply, &QNetworkReply::readyRead, this, [reply] { reply->readAll(); });
            connect(reply, &QNetworkReply::finished, this, [this, reply] {
                if (m_reply != reply) {
                    return;
                }
                m_reply = nullptr;
                const auto code
                    = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                const bool ok
                    = reply->error() == QNetworkReply::NoError && code >= 200 && code < 300;
                reply->deleteLater();
                if (!m_running) {
                    return;
                }
                if (!ok) {
                    fail("HTTP request failed or redirected");
                    return;
                }
                next();
            });
            return;
        }
    }
    if (m_running) {
        m_running = false;
        m_status = m_runningName + " applied";
        emit changed();
    }
}
