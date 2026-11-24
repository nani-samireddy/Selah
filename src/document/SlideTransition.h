#pragma once
#include <cmath>
#include <QMetaType>
#include <QStringList>
#include <QVariantMap>

// Shared serialized contract for drafts, saved programs, and live output.
namespace SlideTransition {
inline QVariantMap cut() {
    return { { "type", "cut" }, { "durationMs", 300 } };
}

inline QStringList effects() {
    return { "fade", "wipe-left", "wipe-right", "wipe-up", "wipe-down", "push-left", "push-right",
        "push-up", "push-down", "swipe-left", "swipe-right", "swipe-up", "swipe-down", "fade-black",
        "fade-white", "zoom-in", "zoom-out", "spin", "blur", "iris-in", "iris-out", "brush" };
}

inline bool valid(const QVariant &value) {
    if (value.metaType().id() != QMetaType::QVariantMap) {
        return false;
    }
    const auto transition = value.toMap();
    const auto type = transition.value("type").toString();
    bool ok = false;
    const auto duration = transition.value("durationMs").toDouble(&ok);
    return (type == "cut" || effects().contains(type)) && ok && std::isfinite(duration)
        && duration == std::floor(duration) && duration >= 50 && duration <= 5000;
}

inline QVariantMap resolve(const QVariantMap &slide) {
    if (valid(slide.value("transition"))) {
        return slide["transition"].toMap();
    }
    if (valid(slide.value("transitionDefault"))) {
        return slide["transitionDefault"].toMap();
    }
    // Old projects retain their original immediate slide changes.
    return cut();
}
}
