#include "AiRequest.h"
#include "BookMatcher.h"
#include <algorithm>
#include <QMap>
#include <QRegularExpression>
#include <QSet>

AiRequest::Parsed AiRequest::scripture(const QString &prompt, const QVariantList &books) {
    const auto result = BookMatcher::resolve(prompt, books, books);
    return { result.handled, result.query, result.error };
}

QString AiRequest::teluguSongTerms(const QString &prompt) {
    if (!QRegularExpression("[\\x{0C00}-\\x{0C7F}]").match(prompt).hasMatch()) {
        return {};
    }
    auto terms = prompt.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    const QSet<QString> commands { "find", "search", "show", "me", "please", "songs", "song",
        "about", "for", "వెతుకు", "వెతకండి", "చూపించు", "చూపించండి", "గురించి", "పాటలు", "పాటలను" };
    bool stripped = false;
    while (!terms.isEmpty() && commands.contains(terms.first().toCaseFolded())) {
        terms.removeFirst();
        stripped = true;
    }
    while (!terms.isEmpty() && commands.contains(terms.last().toCaseFolded())) {
        terms.removeLast();
        stripped = true;
    }
    return stripped ? terms.join(' ') : QString();
}
