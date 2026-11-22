#include "SlideFactory.h"
#include "TemplateComposer.h"
#include "TemplateDefinition.h"
#include <QDebug>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QTextLayout>
#include <QUuid>

namespace {
QVariantMap element(const QString &name, const QString &text, int y, int height, int size,
    const QString &binding, const QString &align = "center") {
    return { { "uid", QUuid::createUuid().toString(QUuid::WithoutBraces) }, { "kind", "text" },
        { "name", name }, { "text", text }, { "x", 160 }, { "y", y }, { "width", 1600 },
        { "height", height }, { "fontSize", size }, { "fontFamily", SlideFactory::fontFor(text) },
        { "color", "#F5EFE7" }, { "opacity", 1.0 }, { "rotation", 0 }, { "visible", true },
        { "locked", false }, { "alignment", align }, { "binding", binding } };
}

QStringList wrap(const QString &text, int size) {
    // Shape before splitting so Indic clusters follow the same boundaries as rendering.
    QStringList result;
    QFont font(SlideFactory::fontFor(text));
    font.setPixelSize(size);
    for (const auto &paragraph : text.split('\n')) {
        if (paragraph.isEmpty()) {
            result << "";
            continue;
        }
        QTextLayout layout(paragraph, font);
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(option);
        layout.beginLayout();
        while (true) {
            auto line = layout.createLine();
            if (!line.isValid()) {
                break;
            }
            line.setLineWidth(1590);
            result << paragraph.mid(line.textStart(), line.textLength());
        }
        layout.endLayout();
    }
    return result;
}

int capacity(const QString &text, int size, int height, int maximum) {
    QFont font(SlideFactory::fontFor(text));
    font.setPixelSize(size);
    return qMax(1, qMin(maximum, int(height / QFontMetricsF(font).lineSpacing())));
}

QVariantMap makeSlide(const QString &title, const QString &primary, const QString &secondary,
    const QString &footer, const QString &kind, const QString &sourceId, int primarySize = 96) {
    const bool bilingual = !secondary.isEmpty();
    QVariantList elements;
    elements << element("Content title", title, 65, 70, 28, "Title");
    elements << element("Primary content", primary, bilingual ? 190 : 235, bilingual ? 375 : 650,
        primarySize, kind == "song" ? "Primary lyrics" : "Scripture text");
    if (bilingual) {
        elements << element("Translated content", secondary, 615, 270, 64, "Translated lyrics");
    }
    elements << element("Credits / reference", footer, 990, 60, 24,
        kind == "song" ? "Credits" : "Scripture reference");
    return { { "name", title }, { "background", "#252A3A" }, { "notes", "" },
        { "elements", elements }, { "sourceKind", kind }, { "sourceId", sourceId } };
}
}

QString SlideFactory::fontFor(const QString &text) {
    for (const auto ch : text) {
        if (ch.unicode() >= 0x0C00 && ch.unicode() <= 0x0C7F) {
            return "Noto Sans Telugu";
        }
    }
    for (const auto ch : text) {
        if (ch.unicode() >= 0x0900 && ch.unicode() <= 0x097F) {
            return "Noto Sans Devanagari";
        }
    }
    return "Arial";
}

QVariantList SlideFactory::songSlides(
    const QVariantMap &song, int maxLines, QString *error, const QVariantMap &definition) {
    if (!definition.isEmpty()) {
        return TemplateComposer::songSlides(song, maxLines, definition, error);
    }
    QVariantList result;
    maxLines = qBound(1, maxLines, 6);
    QMap<QString, QVariantMap> sections;
    for (const auto &value : song["sections"].toList()) {
        sections[value.toMap()["key"].toString()] = value.toMap();
    }
    auto order = song["arrangement"].toList();
    if (order.isEmpty()) {
        for (const auto &v : song["sections"].toList()) {
            order << v.toMap()["key"];
        }
    }
    if (order.isEmpty() || order.size() > 300) {
        *error = "The song needs a valid arrangement";
        return {};
    }
    for (const auto &key : order) {
        if (!sections.contains(key.toString())) {
            *error = "Arrangement contains an unknown section";
            return {};
        }
        const auto section = sections[key.toString()];
        auto primary = section["primary"].toString().split('\n');
        auto secondary = section["secondary"].toString().split('\n');
        const bool paired = !section["secondary"].toString().trimmed().isEmpty();
        if (paired && primary.size() != secondary.size()) {
            *error = "Align the two languages to the same number of lines";
            return {};
        }
        const int pcap = capacity(section["primary"].toString(), 96, paired ? 375 : 650, maxLines),
                  scap = capacity(section["secondary"].toString(), 64, 270, maxLines);
        QStringList ppage, spage;
        int part = 0;
        auto flush = [&] {
            if (ppage.isEmpty() && spage.isEmpty()) {
                return;
            }
            auto slide = makeSlide(song["title"].toString() + " · " + section["label"].toString()
                    + " · " + QString::number(++part),
                ppage.join('\n'), spage.join('\n'), song["credits"].toString(), "song",
                song["id"].toString());
            slide["sourceSection"] = key;
            result << slide;
            ppage.clear();
            spage.clear();
        };
        for (int row = 0; row < primary.size(); row++) {
            if (result.size() > 500) {
                *error = "This song would exceed 500 slides";
                return {};
            }
            if (primary[row].trimmed().isEmpty()
                && (!paired || secondary[row].trimmed().isEmpty())) {
                flush();
                continue;
            }
            auto p = wrap(primary[row], 96);
            auto s = paired ? wrap(secondary[row], 64) : QStringList();
            if (ppage.size() + p.size() > pcap || spage.size() + s.size() > scap) {
                flush();
            }
            while (p.size() > pcap || s.size() > scap) {
                ppage = p.mid(0, pcap);
                spage = s.mid(0, scap);
                p = p.mid(pcap);
                s = s.mid(scap);
                flush();
            }
            ppage += p;
            spage += s;
        }
        flush();
        if (result.size() > 500) {
            *error = "This song would exceed 500 slides";
            return {};
        }
    }
    if (result.isEmpty()) {
        *error = "The song has no lyrics to present";
    }
    return result;
}

QVariantList SlideFactory::scriptureSlides(
    const QVariantList &verses, int maxLines, QString *error, const QVariantMap &definition) {
    if (!definition.isEmpty()) {
        return TemplateComposer::scriptureSlides(verses, maxLines, definition, error);
    }
    for (const auto &value : verses) {
        if (!value.toMap()["secondary"].toMap().isEmpty()) {
            return TemplateComposer::scriptureSlides(
                verses, maxLines, TemplateDefinition::dualScripture(), error);
        }
    }
    QVariantList result;
    maxLines = qBound(1, maxLines, 6);
    if (verses.isEmpty() || verses.size() > 200) {
        *error = "Choose between 1 and 200 verses";
        return {};
    }
    for (const auto &value : verses) {
        auto verse = value.toMap();
        auto body = verse["text"].toString();
        if (body.trimmed().isEmpty()) {
            *error = "A selected verse has no text";
            return {};
        }
        const auto lines = wrap(body, 96);
        const auto cap = capacity(body, 96, 650, maxLines);
        const int count = (lines.size() + cap - 1) / cap;
        for (int start = 0; start < lines.size(); start += cap) {
            const auto reference = verse["reference"].toString();
            auto page = makeSlide(reference
                    + (count > 1 ? " · " + QString::number(start / cap + 1) + "/"
                                + QString::number(count)
                                 : ""),
                lines.mid(start, cap).join('\n'), "",
                reference + " · " + verse["abbreviation"].toString(), "scripture",
                verse["translation"].toString());
            page["sourceReference"] = reference;
            page["sourceCopyright"] = verse["copyright"];
            page["sourceLicense"] = verse["license"];
            result << page;
        }
    }
    if (result.size() > 500) {
        *error = "This passage would exceed 500 slides";
        return {};
    }
    return result;
}
