#include "SearchSpelling.h"
#include <QRegularExpression>
#include <QSet>
#include <QVector>

namespace {
QString normalized(const QString &word) {
    return word.normalized(QString::NormalizationForm_C).toCaseFolded();
}

int distance(const QString &a, const QString &b) {
    // Optimal string alignment includes adjacent transpositions (grcae → grace).
    QVector<QVector<int>> costs(a.size() + 1, QVector<int>(b.size() + 1));
    for (int i = 0; i <= a.size(); ++i) {
        costs[i][0] = i;
    }
    for (int j = 0; j <= b.size(); ++j) {
        costs[0][j] = j;
    }
    for (int i = 1; i <= a.size(); ++i) {
        for (int j = 1; j <= b.size(); ++j) {
            costs[i][j] = qMin(qMin(costs[i - 1][j] + 1, costs[i][j - 1] + 1),
                costs[i - 1][j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1));
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
                costs[i][j] = qMin(costs[i][j], costs[i - 2][j - 2] + 1);
            }
        }
    }
    return costs.last().last();
}

const QRegularExpression tokens(QStringLiteral("[\\p{L}\\p{M}]+"));
}

QStringList SearchSpelling::words(const QString &text) {
    QStringList result;
    auto matches = tokens.globalMatch(text);
    while (matches.hasNext()) {
        result.append(matches.next().captured());
    }
    return result;
}

QString SearchSpelling::correct(
    const QString &query, const QStringList &vocabulary, Vocabulary kind) {
    const bool books = kind == Vocabulary::BibleBooks;
    const int minimumLength = books ? 3 : 4;
    QSet<QString> dictionary;
    for (const auto &word : vocabulary) {
        if (word.size() >= minimumLength && word.size() <= 64) {
            dictionary.insert(normalized(word));
        }
    }
    QString result;
    qsizetype offset = 0;
    auto matches = tokens.globalMatch(query);
    while (matches.hasNext()) {
        const auto match = matches.next();
        const auto word = normalized(match.captured());
        QString replacement = match.captured();
        if (word.size() >= minimumLength && word.size() <= 64 && !dictionary.contains(word)) {
            int best = 4;
            bool ambiguous = false;
            QString candidate;
            for (const auto &entry : dictionary) {
                // Book names form an explicit vocabulary. Include the candidate's
                // length so omitted letters do not exclude a sensible match.
                const auto length = books ? qMax(entry.size(), word.size()) : word.size();
                const int limit = books && length >= 10 ? 3 : length >= 6 ? 2 : 1;
                if (qAbs(entry.size() - word.size()) > limit) {
                    continue;
                }
                const int score = distance(word, entry);
                if (score > limit || (books && score * 3 > length)) {
                    continue;
                }
                if (score < best) {
                    best = score;
                    candidate = entry;
                    ambiguous = false;
                } else if (score == best) {
                    ambiguous = true;
                }
            }
            if (!candidate.isEmpty() && !ambiguous) {
                replacement = candidate;
            }
        }
        result += query.mid(offset, match.capturedStart() - offset) + replacement;
        offset = match.capturedEnd();
    }
    return result + query.mid(offset);
}
