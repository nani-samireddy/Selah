#pragma once
#include <QVariantList>

namespace ProgramSources {
// Older services retain prepared text and attribution; preserve their explicit line breaks.
QVariantList scripture(const QVariantMap &item, QString *error);
}
