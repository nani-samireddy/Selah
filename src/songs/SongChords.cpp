#include "SongChords.h"
#include <algorithm>
#include <QRegularExpression>

QVariantMap SongChords::parse(const QString &source) {
    static const QRegularExpression chord(
        R"(^[A-G](?:#|b)?(?:m|maj|min|dim|aug|sus|add|M)?[0-9]*(?:[#b][0-9]+)?(?:sus[24])?(?:\([^\[\]\n]{1,12}\))?(?:/[A-G][#b]?)?$|^N\.?C\.?$)");
    QString text;
    QVariantList anchors;
    for (qsizetype i = 0; i < source.size(); ++i) {
        if (source[i] == '[') {
            const auto end = source.indexOf(']', i + 1);
            if (end > i && end - i <= 40) {
                const auto label = source.mid(i + 1, end - i - 1);
                if (chord.match(label).hasMatch()) {
                    anchors.append(QVariantMap { { "offset", text.size() }, { "label", label } });
                    i = end;
                    continue;
                }
            }
        }
        text += source[i];
    }
    return { { "text", text }, { "chords", anchors } };
}

QVariantMap SongChords::cleanSong(const QVariantMap &song) {
    auto clean = song;
    QVariantList sections;
    for (const auto &value : song["sections"].toList()) {
        auto section = value.toMap();
        section["primary"] = parse(section["primary"].toString())["text"];
        sections.append(section);
    }
    clean["sections"] = sections;
    return clean;
}

namespace {
QString compact(const QString &text, QList<int> *positions = nullptr) {
    QString result;
    for (int i = 0; i < text.size(); ++i) {
        if (!text[i].isSpace()) {
            result += text[i];
            if (positions) {
                positions->append(i);
            }
        }
    }
    return result;
}
}

void SongChords::annotate(QVariantList &slides, const QVariantMap &song) {
    QMap<QString, QVariantMap> sections;
    for (const auto &value : song["sections"].toList()) {
        const auto section = value.toMap();
        sections[section["key"].toString()] = section;
    }
    QString previous;
    int cursor = 0;
    for (auto &value : slides) {
        auto slide = value.toMap();
        const auto key = slide["sourceSection"].toString();
        const auto section = sections.value(key);
        if (key != previous) {
            cursor = 0;
        }
        previous = key;
        const auto parsed = parse(section["primary"].toString());
        const auto original = parsed["text"].toString();
        QList<int> originalPositions, pagePositions;
        const auto source = compact(original, &originalPositions);
        QString text;
        for (const auto &element : slide["elements"].toList()) {
            const auto box = element.toMap();
            if (box["binding"] == "Primary lyrics" || box["name"] == "Primary lyrics") {
                text = box["text"].toString();
                break;
            }
        }
        const auto page = compact(text, &pagePositions);
        int start = page.isEmpty() ? -1 : source.indexOf(page, cursor);
        if (start < 0 && !page.isEmpty()) {
            start = source.indexOf(page);
        }
        QVariantList anchors;
        if (start >= 0) {
            for (const auto &anchor : parsed["chords"].toList()) {
                const auto item = anchor.toMap();
                const int offset = item["offset"].toInt();
                const auto found = std::lower_bound(
                    originalPositions.cbegin(), originalPositions.cend(), offset);
                const int normalized = int(found - originalPositions.cbegin());
                if (normalized >= start && normalized < start + page.size()) {
                    anchors.append(QVariantMap { { "offset", pagePositions[normalized - start] },
                        { "label", item["label"] } });
                }
            }
            cursor = start + page.size();
        }
        slide["stageLyrics"] = text;
        slide["stageChords"] = anchors;
        slide["stageSection"] = section["label"].toString();
        slide["stageCue"] = section["stageCue"].toString();
        slide["songKey"] = song["songKey"].toString();
        value = slide;
    }
}

QVariantMap SongChords::importChordPro(
    const QString &source, const QString &fallbackTitle, QString *error) {
    QVariantMap song { { "title", fallbackTitle }, { "primaryLanguage", "English" },
        { "secondaryLanguage", "" } };
    QVariantList sections, order;
    QStringList lyrics, cues;
    QString label = "Verse 1";
    auto flush = [&] {
        if (lyrics.join('\n').trimmed().isEmpty()) {
            lyrics.clear();
            return;
        }
        const auto key = "s" + QString::number(sections.size() + 1);
        sections.append(
            QVariantMap { { "key", key }, { "label", label }, { "primary", lyrics.join('\n') },
                { "secondary", "" }, { "stageCue", cues.join(" · ") } });
        order.append(key);
        lyrics.clear();
        cues.clear();
        label = "Section " + QString::number(sections.size() + 1);
    };
    static const QRegularExpression directive(R"(^\{([^}:]+)(?::\s*(.*))?\}$)");
    for (const auto &line : source.split('\n')) {
        const auto trimmed = line.trimmed();
        const auto match = directive.match(trimmed);
        if (match.hasMatch()) {
            const auto name = match.captured(1).toLower();
            const auto value = match.captured(2).trimmed();
            if (name == "title" || name == "t") {
                song["title"] = value;
            } else if (name == "key") {
                song["songKey"] = value;
            } else if (name == "artist" || name == "subtitle" || name == "st") {
                song["credits"] = value;
            } else if (name == "comment" || name == "c" || name == "comment_italic") {
                cues.append(value);
            } else if (name.startsWith("start_of_") || name == "soc" || name == "sov"
                || name == "sob") {
                flush();
                label = !value.isEmpty()
                    ? value
                    : ((name == "soc" || name.endsWith("chorus"))          ? "Chorus"
                              : (name == "sob" || name.endsWith("bridge")) ? "Bridge"
                                                                           : "Verse");
            } else if (name.startsWith("end_of_") || name == "eoc" || name == "eov"
                || name == "eob") {
                flush();
            } else if (name == "new_song" || name == "ns") {
                *error = "Import one ChordPro song at a time";
                return {};
            }
            continue;
        }
        if (trimmed.startsWith('#')) {
            continue;
        }
        if (trimmed.isEmpty()) {
            flush();
        } else {
            lyrics.append(line);
        }
    }
    flush();
    if (sections.isEmpty() || sections.size() > 100) {
        *error = "ChordPro needs 1 to 100 lyric sections";
        return {};
    }
    song["sections"] = sections;
    song["arrangement"] = order;
    return song;
}
