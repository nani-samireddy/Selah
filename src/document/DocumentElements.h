#pragma once
#include <QString>
#include <QVariantMap>

namespace DocumentElements {
// Every editable/rendered element receives an independent stable identity.
QVariantMap text(const QString &name, const QString &content, double x, double y, double width,
    double height, double size, const QString &family, const QString &color = "#F5EFE7");
}
