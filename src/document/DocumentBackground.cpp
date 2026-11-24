#include "BackgroundFill.h"
#include "EditorDocument.h"
#include <cmath>
#include <QColor>

void EditorDocument::setBackground(const QString &value) {
    const QColor color(value);
    if (!color.isValid() || color == QColor(background())) {
        return;
    }
    checkpoint();
    auto slide = m_slides[m_current].toMap();
    if (color.alpha() == 0 && QColor(background()).alpha() > 0) {
        slide["backgroundRestore"] = background();
    } else if (color.alpha() > 0) {
        slide.remove("backgroundRestore");
    }
    slide["background"] = color.name(QColor::HexArgb);
    m_slides[m_current] = slide;
    commit();
}

void EditorDocument::setBackgroundTransparent(bool transparent) {
    const auto color = QColor(background());
    if (transparent == (color.alpha() == 0)) {
        return;
    }
    if (transparent) {
        setBackground("transparent");
        return;
    }
    const auto saved = m_slides[m_current].toMap().value("backgroundRestore", "#252A3A").toString();
    const QColor restore(saved);
    setBackground(restore.isValid() && restore.alpha() > 0 ? saved : "#252A3A");
}

void EditorDocument::setAmbientOpacity(double opacity) {
    if (!std::isfinite(opacity) || opacity < 0 || opacity > 1 || opacity == ambientOpacity()) {
        return;
    }
    checkpoint();
    auto slide = m_slides[m_current].toMap();
    slide["ambientOpacity"] = opacity;
    m_slides[m_current] = slide;
    commit();
}

bool EditorDocument::setBackgroundFill(const QVariantMap &fill) {
    if (!BackgroundFill::valid(fill)) {
        return false;
    }
    if (backgroundFill() == fill) {
        return true;
    }
    checkpoint();
    auto slide = m_slides[m_current].toMap();
    slide["backgroundFill"] = fill;
    m_slides[m_current] = slide;
    commit();
    return true;
}
