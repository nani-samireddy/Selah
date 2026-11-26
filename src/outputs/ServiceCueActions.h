#pragma once
#include <QVariantMap>

namespace ServiceCueActions {
QVariantList catalog();
QVariantMap defaults(const QString &type);
QVariantMap normalize(const QVariantMap &cue);
bool valid(const QVariantMap &cue, QString *error = nullptr);
QByteArray oscPacket(const QVariantMap &action);
}
