#pragma once
#include <QVariantList>

namespace ProgramSlides {
// Copies a snapshot without sharing renderer identities with its source.
QVariantList cloneWithFreshIds(QVariantList slides);
}
