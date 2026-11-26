#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramSlides.h"
#include "SlideTransition.h"
#include <QUuid>

QVariantMap OutputManager::sessionSnapshot() const {
    return { { "slides", m_liveSlides }, { "index", m_index }, { "source", m_liveSource },
        { "resumeIndex", m_resumeIndex }, { "message", m_stageMessage },
        { "remaining", m_remaining }, { "musician", m_musicianView },
        { "cueOverlay", m_cueOverlay }, { "cueTransition", m_cueTransition } };
}

bool OutputManager::restoreSession(const QVariantMap &snapshot) {
    const auto slides = snapshot["slides"].toList();
    const int index = snapshot.value("index", -1).toInt(),
              resume = snapshot.value("resumeIndex", -1).toInt();
    if (slides.isEmpty() || slides.size() > 5000 || index < 0 || index >= slides.size()
        || resume < -1 || resume >= slides.size() || snapshot["message"].toString().size() > 10000
        || snapshot["source"].toString().size() > 1000) {
        return false;
    }
    const auto overlay = snapshot["cueOverlay"].toMap();
    const auto transition = snapshot["cueTransition"].toMap();
    if (!transition.isEmpty() && !SlideTransition::valid(transition)) {
        return false;
    }
    if (!overlay.isEmpty()
        && EditorDocument::validatedSlides({ QVariantMap { { "background", "#000000" },
                                               { "elements", QVariantList { overlay } } } })
            .isEmpty()) {
        return false;
    }
    QVariantList validated;
    for (int i = 0; i < slides.size(); i += 500) {
        const auto chunk
            = EditorDocument::validatedSlides(ProgramSlides::cloneWithFreshIds(slides.mid(i, 500)));
        if (chunk.isEmpty()) {
            return false;
        }
        validated += chunk;
    }
    stop("audience");
    stop("stage");
    finishTransition();
    m_liveSlides = validated;
    m_cueOverlay = overlay;
    m_cueTransition = transition;
    m_index = index;
    m_resumeIndex = resume;
    m_liveSource = snapshot["source"].toString();
    m_stageMessage = snapshot["message"].toString();
    m_remaining = qBound(0, snapshot["remaining"].toInt(), 180 * 60);
    m_musicianView = snapshot["musician"].toBool();
    m_playing = false;
    m_audioEnabled = false;
    m_timer.stop();
    m_defaultImageLive.clear();
    m_textCleared = false;
    m_audienceBlack = false;
    m_stageBlack = false;
    m_snapshotId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_status = "Session recovered · outputs off";
    emit snapshotChanged();
    emit liveChanged();
    emit stateChanged();
    return true;
}
