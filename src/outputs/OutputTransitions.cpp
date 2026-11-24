#include "OutputManager.h"
#include "SlideTransition.h"
#include <algorithm>

void OutputManager::initializeTransitions() {
    m_transitionTimer.setTimerType(Qt::PreciseTimer);
    m_transitionTimer.setInterval(16);
    connect(&m_transitionTimer, &QTimer::timeout, this, [this] {
        m_transitionProgress
            = std::clamp(double(m_transitionClock.elapsed()) / m_transitionDuration, 0.0, 1.0);
        if (m_transitionProgress >= 1) {
            m_transitionTimer.stop();
        }
        emit transitionChanged();
    });
}

void OutputManager::finishTransition() {
    m_transitionTimer.stop();
    if (m_transitionProgress == 1) {
        return;
    }
    m_transitionProgress = 1;
    emit transitionChanged();
}

void OutputManager::prepareTransition(const QVariantMap &target) {
    const bool interrupted = transitioning();
    finishTransition();
    const auto transition = SlideTransition::resolve(target);
    // Rapid navigation takes the latest requested slide immediately. It must not
    // queue fades or revive an older frozen frame. Safety/standby actions also cut.
    if (interrupted || !hasAudienceContent() || m_audienceBlack || defaultImageShowing()
        || transition["type"] == "cut") {
        return;
    }
    m_transitionDuration = transition["durationMs"].toInt();
    m_transitionEffect = SlideTransition::effects().indexOf(transition["type"].toString());
    emit transitionStarting(); // Freeze each renderer's last GPU frame before changing content.
    m_transitionProgress = 0;
    m_transitionClock.start();
    m_transitionTimer.start();
    emit transitionChanged();
}
