#pragma once
#include <QVariantList>

namespace AiRequest {
struct Parsed {
    bool handled = false;
    QString query;
    QString error;
};

// Ground explicit requests in installed book identities, not model-generated numbers.
Parsed scripture(const QString &prompt, const QVariantList &books);
QString teluguSongTerms(const QString &prompt);
}
