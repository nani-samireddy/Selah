#include "TemplateTextLayout.h"
#include "TextFormatting.h"
#include "TextLayoutMetrics.h"
#include <QFont>
#include <QRegularExpression>
#include <QTextBoundaryFinder>
#include <QTextLayout>

bool TemplateTextLayout::fits(const QString &text, const QVariantMap &box, int maximumLines) {
    if (text.isEmpty()) {
        return true;
    }
    const double width = qMax(1.0, box["width"].toDouble() - 8);
    const double availableHeight = box["height"].toDouble() - 8;
    auto data = box;
    data["text"] = text;
    data.remove("richText");
    data.remove("textRuns");
    data = TextFormatting::fitted(data, maximumLines);
    const auto layout = TextLayoutMetrics::measure(text, data, width);
    const int displayLimit = box.value("maxLines", 0).toInt();
    const int lines = displayLimit > 0 ? qMin(maximumLines, displayLimit) : maximumLines;
    return layout.lines <= lines && layout.height <= availableHeight && layout.width <= width + 0.5;
}

bool TemplateTextLayout::fitsBinding(
    const QString &text, const QString &binding, const QVariantMap &definition, int maximumLines) {
    for (const auto &value : definition["slide"].toMap()["elements"].toList()) {
        const auto box = value.toMap();
        if (box["binding"] == binding && box.value("visible", true).toBool()
            && box.value("opacity", 1).toDouble() > 0 && !fits(text, box, maximumLines)) {
            return false;
        }
    }
    return true;
}

QString TemplateTextLayout::takePrefix(
    QString &remaining, const QString &binding, const QVariantMap &definition, int maximumLines) {
    if (fitsBinding(remaining, binding, definition, maximumLines)) {
        const auto result = remaining;
        remaining.clear();
        return result;
    }
    // Search only at grapheme boundaries; splitting a UTF-16 code unit can corrupt a conjunct.
    QVector<int> boundaries;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, remaining);
    while (finder.toNextBoundary() >= 0) {
        boundaries << finder.position();
    }
    int low = 0, high = boundaries.size() - 1, best = 0;
    while (low <= high) {
        const int middle = (low + high) / 2;
        const int length = boundaries[middle];
        if (fitsBinding(remaining.left(length), binding, definition, maximumLines)) {
            best = length;
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    if (best == 0) {
        return {};
    }
    if (binding == "Scripture text" || binding == "Secondary scripture text") {
        // Prefer complete thoughts, then clauses. Preserve punctuation and whitespace
        // verbatim so concatenating the chunks reproduces the installed verse.
        int sentenceEnd = 0;
        const QRegularExpression paragraphs(QStringLiteral("\\n\\s*\\n"));
        auto paragraphMatches = paragraphs.globalMatch(remaining);
        while (paragraphMatches.hasNext()) {
            const auto match = paragraphMatches.next();
            if (match.capturedEnd() <= best) {
                sentenceEnd = match.capturedEnd();
            }
        }
        QTextBoundaryFinder sentences(QTextBoundaryFinder::Sentence, remaining);
        if (sentenceEnd == 0) {
            while (sentences.toNextBoundary() >= 0 && sentences.position() <= best) {
                sentenceEnd = sentences.position();
            }
        }
        if (sentenceEnd > 0 && !remaining.left(sentenceEnd).trimmed().isEmpty()) {
            best = sentenceEnd;
        } else {
            int clauseEnd = remaining.lastIndexOf('\n', best - 1) + 1;
            if (remaining.left(clauseEnd).trimmed().isEmpty()) {
                clauseEnd = 0;
            }
            const QRegularExpression punctuation(
                QStringLiteral("[,;:\\x{060C}\\x{061B}\\x{FF0C}\\x{FF1B}\\x{FF1A}][\\s]*|[\\x{0964}"
                               "\\x{0965}][\\s]*|\\s+[—–]\\s+"));
            auto matches = punctuation.globalMatch(remaining);
            while (matches.hasNext()) {
                const auto match = matches.next();
                const int position = match.capturedStart();
                if (position > 0 && position + 1 < remaining.size()
                    && remaining[position - 1].isDigit() && remaining[position + 1].isDigit()) {
                    continue;
                }
                if (match.capturedEnd() <= best) {
                    clauseEnd = match.capturedEnd();
                }
            }
            if (clauseEnd == 0) {
                const QRegularExpression connector(
                    QStringLiteral("\\s+(?=(?:and|but|because|therefore|although|while|for|so|మరియు|"
                                   "అయితే|కాని|కాబట్టి|ఎందుకంటే|और|लेकिन|क्योंकि)(?:\\s|$))"),
                    QRegularExpression::CaseInsensitiveOption
                        | QRegularExpression::UseUnicodePropertiesOption);
                matches = connector.globalMatch(remaining);
                while (matches.hasNext()) {
                    const auto match = matches.next();
                    if (match.capturedEnd() <= best) {
                        clauseEnd = match.capturedEnd();
                    }
                }
            }
            // Do not silently slice an oversized phrase at an arbitrary word.
            // The caller can ask the operator to enlarge the box or allow more shrinking.
            if (clauseEnd == 0) {
                return {};
            }
            best = clauseEnd;
        }
        const auto result = remaining.left(best);
        remaining.remove(0, best);
        return result;
    }
    // Lyric rows retain their authored structure and use word boundaries as a fallback.
    int wordEnd = 0;
    for (int boundary : boundaries) {
        if (boundary > best) {
            break;
        }
        if (remaining[boundary - 1].isSpace()) {
            wordEnd = boundary;
        }
    }
    if (wordEnd > 0) {
        best = wordEnd;
    }
    const auto result = remaining.left(best);
    remaining.remove(0, best);
    return result;
}
