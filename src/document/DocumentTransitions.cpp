#include "EditorDocument.h"
#include "SlideTransition.h"

QVariantMap EditorDocument::defaultTransition() const {
    const auto value = m_slides.first().toMap().value("transitionDefault");
    return SlideTransition::valid(value) ? value.toMap() : SlideTransition::cut();
}

QVariantMap EditorDocument::slideTransition() const {
    return m_slides[m_current].toMap().value("transition").toMap();
}

bool EditorDocument::setDefaultTransition(const QString &type, int durationMs) {
    const QVariantMap transition { { "type", type }, { "durationMs", durationMs } };
    if (!SlideTransition::valid(transition)) {
        return false;
    }
    if (defaultTransition() == transition) {
        return true;
    }
    checkpoint();
    for (auto &entry : m_slides) {
        auto slide = entry.toMap();
        slide["transitionDefault"] = transition;
        entry = slide;
    }
    commit();
    return true;
}

bool EditorDocument::setSlideTransition(const QString &type, int durationMs) {
    const QVariantMap transition { { "type", type }, { "durationMs", durationMs } };
    if (type != "inherit" && !SlideTransition::valid(transition)) {
        return false;
    }
    auto slide = m_slides[m_current].toMap();
    auto updated = slide;
    if (type == "inherit") {
        updated.remove("transition");
    } else {
        updated["transition"] = transition;
    }
    if (updated == slide) {
        return true;
    }
    checkpoint();
    m_slides[m_current] = updated;
    commit();
    return true;
}
