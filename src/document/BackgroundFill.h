#pragma once
#include <cmath>
#include <QColor>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

// Optional canvas fill metadata. Legacy slides continue using their background color.
namespace BackgroundFill {
inline bool valid(const QVariantMap &fill) {
    if (fill.isEmpty()) {
        return true;
    }
    const auto type = fill.value("type").toString();
    if (!QStringList { "color", "linear", "radial", "image" }.contains(type)) {
        return false;
    }
    if (type == "linear" || type == "radial") {
        const auto stops = fill.value("stops").toList();
        if (stops.size() < 2 || stops.size() > 8) {
            return false;
        }
        double previous = -1;
        for (const auto &entry : stops) {
            const auto stop = entry.toMap();
            bool ok = false;
            const double position = stop.value("position").toDouble(&ok);
            if (!ok || !std::isfinite(position) || position < 0 || position > 1
                || position < previous || !QColor(stop.value("color").toString()).isValid()) {
                return false;
            }
            previous = position;
        }
        bool ok = false;
        const auto angle = fill.value("angle", 0).toDouble(&ok);
        if (!ok || !std::isfinite(angle) || angle < 0 || angle > 360) {
            return false;
        }
    }
    if (type == "image") {
        if (!QUrl(fill.value("source").toString()).isLocalFile()
            || !QStringList { "contain", "cover", "stretch" }.contains(
                fill.value("fit").toString())) {
            return false;
        }
    }
    return true;
}
}
