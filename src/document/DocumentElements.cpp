#include "DocumentElements.h"
#include <QUuid>

QVariantMap DocumentElements::text(const QString &name, const QString &content, double x, double y,
    double width, double height, double size, const QString &family, const QString &color) {
    return { { "uid", QUuid::createUuid().toString(QUuid::WithoutBraces) }, { "kind", "text" },
        { "name", name }, { "text", content }, { "x", x }, { "y", y }, { "width", width },
        { "height", height }, { "fontSize", size }, { "fontFamily", family }, { "color", color },
        { "opacity", 1.0 }, { "rotation", 0.0 }, { "visible", true }, { "locked", false },
        { "alignment", "left" }, { "binding", "None" } };
}
