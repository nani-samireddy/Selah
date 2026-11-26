#include "BookMatcher.h"
#include <algorithm>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QVector>

static void initializeBookResources() {
    Q_INIT_RESOURCE(book_names);
}

namespace {
QJsonObject resource(const QString &name) {
    static const bool initialized = [] {
        initializeBookResources();
        return true;
    }();
    Q_UNUSED(initialized);
    QFile file(":/book-names/" + name);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object()
                                          : QJsonObject();
}

QString latinKey(QString text) {
    text = text.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString result;
    for (const auto ch : text) {
        if (ch.isMark()) {
            continue;
        }
        if (ch.isLetterOrNumber()) {
            // Long vowels have many everyday Roman spellings (aa / ā / a).
            if (QString("aeiou").contains(ch) && !result.isEmpty() && result.back() == ch) {
                continue;
            }
            result += ch;
        }
    }
    return result;
}

int distance(const QString &a, const QString &b) {
    QVector<QVector<int>> d(a.size() + 1, QVector<int>(b.size() + 1));
    for (int i = 0; i <= a.size(); ++i) {
        d[i][0] = i;
    }
    for (int j = 0; j <= b.size(); ++j) {
        d[0][j] = j;
    }
    for (int i = 1; i <= a.size(); ++i) {
        for (int j = 1; j <= b.size(); ++j) {
            d[i][j] = qMin(
                qMin(d[i - 1][j] + 1, d[i][j - 1] + 1), d[i - 1][j - 1] + (a[i - 1] != b[j - 1]));
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
                d[i][j] = qMin(d[i][j], d[i - 2][j - 2] + 1);
            }
        }
    }
    return d.last().last();
}

QMap<QString, QSet<QString>> aliases(const QVariantList &known) {
    static const auto catalog = resource("catalog.json")["books"].toArray();
    QMap<QString, QSet<QString>> result;
    auto add = [&](const QString &code, const QString &name) {
        if (name.isEmpty()) {
            return;
        }
        result[code].insert(BookMatcher::normalize(name));
        auto compact = BookMatcher::normalize(name);
        compact.remove(' ');
        result[code].insert(compact);
        const auto roman = BookMatcher::romanize(name);
        if (!roman.isEmpty()) {
            const auto key = latinKey(roman);
            result[code].insert(key);
            if (!key.isEmpty() && key.front().isDigit()) {
                result[code].insert(key.left(1) + " " + key.mid(1));
            }
            // Optional word-final inherent vowels are common in Romanized Indic names.
            if (key.size() > 5 && (key.endsWith('a') || key.endsWith('u'))) {
                result[code].insert(key.chopped(1));
            }
        }
    };
    for (const auto &row : catalog) {
        const auto book = row.toObject();
        for (const auto &name : book["names"].toArray()) {
            add(book["code"].toString(), name.toString());
        }
    }
    for (const auto &value : known) {
        const auto book = value.toMap();
        add(book["code"].toString(), book["name"].toString());
        add(book["code"].toString(), book["code"].toString());
    }
    return result;
}

bool sameOrdinal(const QString &a, const QString &b) {
    const auto ordinal = [](const QString &s) {
        return !s.isEmpty() && s.front().isDigit() ? s.front() : QChar();
    };
    return ordinal(a) == ordinal(b);
}
}

QString BookMatcher::normalize(const QString &input) {
    auto text = input.normalized(QString::NormalizationForm_C).toCaseFolded();
    for (auto &ch : text) {
        if (ch.digitValue() >= 0) {
            ch = QChar('0' + ch.digitValue());
        }
    }
    return text.simplified();
}

QString BookMatcher::romanize(const QString &text) {
    static const auto characters = resource("indic-romanization.json")["characters"].toObject();
    QString output;
    bool pending = false, converted = false;
    auto flush = [&] {
        if (pending) {
            output += 'a';
        }
        pending = false;
    };
    for (const auto ch : normalize(text)) {
        const auto entry = characters[QString(ch)].toArray();
        if (entry.isEmpty()) {
            flush();
            if (ch.unicode() >= 128 && ch.isLetter()) {
                // Latin diacritics are handled below; unknown scripts stay native-only.
                if (ch.script() != QChar::Script_Latin) {
                    return {};
                }
            }
            output += ch;
            continue;
        }
        converted = true;
        const auto kind = entry[0].toString(), value = entry[1].toString();
        if (kind == "consonant") {
            flush();
            output += value;
            pending = true;
        } else if (kind == "virama") {
            pending = false;
        } else if (kind == "nukta") {
            continue;
        } else if (kind == "vowelSign") {
            pending = false;
            output += value;
        } else {
            flush();
            output += value;
        }
    }
    flush();
    return converted || !output.isEmpty() ? latinKey(output) : QString();
}

BookMatcher::Result BookMatcher::resolve(
    const QString &input, const QVariantList &known, const QVariantList &available) {
    const auto text = normalize(input);
    if (text.isEmpty() || text.size() > 2000) {
        return {};
    }
    const auto index = aliases(known);
    QMap<QString, QString> names;
    for (const auto &v : available) {
        const auto b = v.toMap();
        names[b["code"].toString()] = b["name"].toString();
    }

    struct Match {
        qsizetype start, end;
        QString code;
        int score = 0;
    };

    QList<Match> hits;
    for (auto it = index.cbegin(); it != index.cend(); ++it) {
        for (const auto &alias : it.value()) {
            auto found = QRegularExpression(
                "(?<![\\p{L}\\p{M}0-9])" + QRegularExpression::escape(alias) + "(?![\\p{L}\\p{M}])")
                             .globalMatch(text);
            while (found.hasNext()) {
                const auto m = found.next();
                hits.append({ m.capturedStart(), m.capturedEnd(), it.key(), 0 });
            }
        }
    }
    // Also normalize Latin accents/long vowels in each word before a numeric reference.
    const QRegularExpression reference(
        "(?<![0-9])([0-9]{1,3})(?:\\s*:\\s*([0-9]{1,3})(?:\\s*[-–]\\s*([0-9]{1,3}))?)?(?![0-9])");
    if (hits.isEmpty()) {
        auto refs = reference.globalMatch(text);
        while (refs.hasNext()) {
            const auto ref = refs.next();
            auto prefix = text.left(ref.capturedStart()).trimmed();
            auto words = prefix.split(' ', Qt::SkipEmptyParts);
            for (int count = 1; count <= qMin(5, int(words.size())); ++count) {
                const auto phrase = words.mid(words.size() - count).join(' ');
                const auto key = romanize(phrase);
                if (key.isEmpty() || key.size() > 64) {
                    continue;
                }
                int best = 100;
                QSet<QString> codes;
                for (auto it = index.cbegin(); it != index.cend(); ++it) {
                    for (const auto &alias : it.value()) {
                        if (!sameOrdinal(key, alias) || alias.size() > 64) {
                            continue;
                        }
                        // Do not fuzzy-match short abbreviations or fragments of native names.
                        const bool latin
                            = QRegularExpression("^[a-z0-9]+$").match(alias).hasMatch();
                        if (!latin || (alias.size() < 4 && key != alias)) {
                            continue;
                        }
                        const int limit = key.size() < 4 ? 0
                            : key.size() >= 10           ? 3
                            : key.size() >= 6            ? 2
                                                         : 1;
                        if (qAbs(key.size() - alias.size()) > limit) {
                            continue;
                        }
                        const int score = distance(key, alias);
                        if (score > limit) {
                            continue;
                        }
                        if (score < best) {
                            best = score;
                            codes.clear();
                        }
                        if (score == best) {
                            codes.insert(it.key());
                        }
                    }
                }
                for (const auto &code : codes) {
                    hits.append({ prefix.size() - phrase.size(), prefix.size(), code, best });
                }
            }
        }
    }
    if (hits.isEmpty()) {
        if (!QRegularExpression("[0-9]").match(text).hasMatch()) {
            return {};
        }
        return { true, {}, {}, "Which Bible book do you mean? Include a book name and reference.",
            {} };
    }
    std::sort(hits.begin(), hits.end(), [](const Match &a, const Match &b) {
        if (a.score != b.score) {
            return a.score < b.score;
        }
        return a.end - a.start > b.end - b.start;
    });
    const auto best = hits.first();
    if (!QRegularExpression("[0-9]").match(text).hasMatch()) {
        auto remaining = text.left(best.start) + text.mid(best.end);
        remaining.remove(QRegularExpression("\\b(find|show|search|please|me)\\b"));
        if (!remaining.trimmed().isEmpty()) {
            return {};
        }
    }

    QSet<QString> codes;
    bool multipleReferences = false;
    for (const auto &hit : hits) {
        if (hit.start < best.end && hit.end > best.start) {
            if (hit.score == best.score && hit.start == best.start && hit.end == best.end) {
                codes.insert(hit.code);
            }
        } else {
            multipleReferences = true;
        }
    }
    if (multipleReferences) {
        return { true, {}, {}, "Choose one book and reference at a time.", {} };
    }
    const auto suffix = text.mid(best.end);
    const auto ref = QRegularExpression(
        "^\\s*([0-9]{1,3})(?:\\s*:\\s*([0-9]{1,3})(?:\\s*[-–]\\s*([0-9]{1,3}))?)?")
                         .match(suffix);
    if (!ref.hasMatch()) {
        return { true, {}, {}, "Include the chapter and verse after the book name.", {} };
    }
    const auto remainder = text.left(best.start) + suffix.mid(ref.capturedEnd());
    if (QRegularExpression("[0-9:]").match(remainder).hasMatch()) {
        return { true, {}, {}, "Use one complete reference or verse range, such as John 3:16–18.",
            {} };
    }
    const int chapter = ref.captured(1).toInt(), first = ref.captured(2).toInt(),
              last = ref.captured(3).toInt();
    if (chapter < 1 || (!ref.captured(2).isEmpty() && first < 1)
        || (!ref.captured(3).isEmpty() && last < first)) {
        return { true, {}, {},
            "Check the chapter and verse range; numbers must start at 1 and run forwards.", {} };
    }
    QString numbers = QString::number(chapter);
    if (!ref.captured(2).isEmpty()) {
        numbers += ":" + QString::number(first);
    }
    if (!ref.captured(3).isEmpty()) {
        numbers += "-" + QString::number(last);
    }
    QVariantList choices;
    auto sorted = codes.values();
    std::sort(sorted.begin(), sorted.end());
    for (const auto &code : sorted) {
        if (names.contains(code)) {
            choices.append(QVariantMap {
                { "code", code }, { "name", names[code] }, { "query", code + " " + numbers } });
        }
    }
    if (choices.size() != 1) {
        return { true, {}, {},
            choices.isEmpty()
                ? "That book is not available in the selected Bible."
                : "That book name matches more than one book. Choose the intended book.",
            choices };
    }
    // A known but unavailable exact name must not be silently redirected to another book.
    if (codes.size() != 1) {
        return { true, {}, {}, "That book name is ambiguous. Choose the intended book.", choices };
    }
    const auto code = choices.first().toMap()["code"].toString();
    return { true, code + " " + numbers, code, {}, {} };
}
