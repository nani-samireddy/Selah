#include "TextFormatting.h"
#include "TextLayoutMetrics.h"
#include <algorithm>
#include <cmath>
#include <QColor>
#include <QFont>
#include <QQuickTextDocument>
#include <QTextBlock>
#include <QTextBoundaryFinder>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextLayout>
#include <QTextList>
#include <QUrl>

namespace {
// Rich text is a local formatting representation, never a source of remote resources.
class LocalDocument : public QTextDocument {
    QVariant loadResource(int, const QUrl &) override { return {}; }
};

QFont baseFont(const QVariantMap &e) {
    QFont font(e.value("fontFamily", "Arial").toString());
    const double size = e.value("fontSize", 36).toDouble();
    font.setPixelSize(std::isfinite(size) ? qRound(std::clamp(size, 8.0, 400.0)) : 36);
    font.setWeight(QFont::Weight(e.value("fontWeight", 400).toInt()));
    font.setItalic(e.value("italic", false).toBool());
    font.setUnderline(e.value("underline", false).toBool());
    font.setStrikeOut(e.value("strikethrough", false).toBool());
    font.setLetterSpacing(QFont::AbsoluteSpacing, e.value("letterSpacing", 0).toDouble());
    const auto features = e.value("fontFeatures").toMap();
    for (auto i = features.begin(); i != features.end(); ++i) {
        if (auto tag = QFont::Tag::fromString(i.key().toLatin1())) {
            font.setFeature(*tag, i.value().toUInt());
        }
    }
    const auto axes = e.value("fontAxes").toMap();
    for (auto i = axes.begin(); i != axes.end(); ++i) {
        if (auto tag = QFont::Tag::fromString(i.key().toLatin1())) {
            font.setVariableAxis(*tag, i.value().toFloat());
        }
    }
    const auto casing = e.value("textCase", "original").toString();
    if (casing == "upper") {
        font.setCapitalization(QFont::AllUppercase);
    } else if (casing == "lower") {
        font.setCapitalization(QFont::AllLowercase);
    } else if (casing == "title") {
        font.setCapitalization(QFont::Capitalize);
    } else if (casing == "smallCaps") {
        font.setCapitalization(QFont::SmallCaps);
    }
    return font;
}

Qt::Alignment alignment(const QString &value) {
    if (value == "center") {
        return Qt::AlignHCenter;
    }
    if (value == "right") {
        return Qt::AlignRight;
    }
    if (value == "justify") {
        return Qt::AlignJustify;
    }
    return Qt::AlignLeft;
}

QTextCharFormat safeCharacter(const QTextCharFormat &source) {
    QTextCharFormat safe;
    // Deliberately omit image, link, object, and resource properties.
    const auto properties = source.properties();
    for (auto i = properties.begin(); i != properties.end(); ++i) {
        if ((i.key() >= QTextFormat::FirstFontProperty && i.key() <= QTextFormat::LastFontProperty)
            || i.key() == QTextFormat::ForegroundBrush || i.key() == QTextFormat::TextUnderlineStyle
            || i.key() == QTextFormat::TextVerticalAlignment) {
            safe.setProperty(i.key(), i.value());
        }
    }
    return safe;
}

void paragraphDefaults(QTextCursor &cursor, const QVariantMap &e) {
    QTextBlockFormat format;
    format.setAlignment(alignment(e.value("alignment", "left").toString()));
    format.setBottomMargin(e.value("paragraphSpacing", 0).toDouble());
    format.setTextIndent(e.value("paragraphIndent", 0).toDouble());
    const auto height = e.value("lineHeight", 1.0).toDouble();
    format.setLineHeight(e.value("lineHeightUnit", "percent") == "px" ? height : height * 100,
        e.value("lineHeightUnit", "percent") == "px" ? QTextBlockFormat::FixedHeight
                                                     : QTextBlockFormat::ProportionalHeight);
    cursor.mergeBlockFormat(format);
}

void restoreRuns(QTextDocument &document, const QVariantMap &element);

void load(LocalDocument &target, const QVariantMap &e) {
    target.setDocumentMargin(0);
    target.setDefaultFont(baseFont(e));
    const auto plain = e.value("text").toString();
    const auto rich = e.value("richText").toString();
    LocalDocument source;
    source.setDefaultFont(baseFont(e));
    if (!rich.isEmpty() && rich.size() <= 2000000) {
        source.setHtml(rich);
    }
    if (rich.isEmpty() || source.toPlainText() != plain) {
        target.setPlainText(plain);
        QTextCursor cursor(&target);
        cursor.select(QTextCursor::Document);
        QTextCharFormat format;
        format.setForeground(QColor(e.value("color", "#FFFFFF").toString()));
        cursor.mergeCharFormat(format);
        paragraphDefaults(cursor, e);
        return;
    }
    QTextCursor cursor(&target);
    QHash<QTextList *, QTextList *> lists;
    for (auto block = source.begin(); block.isValid(); block = block.next()) {
        if (block != source.begin()) {
            cursor.insertBlock();
        }
        const auto blockSource = block.blockFormat();
        QTextBlockFormat blockSafe;
        blockSafe.setAlignment(blockSource.alignment());
        blockSafe.setBottomMargin(blockSource.bottomMargin());
        blockSafe.setTextIndent(blockSource.textIndent());
        blockSafe.setLineHeight(blockSource.lineHeight(), blockSource.lineHeightType());
        cursor.setBlockFormat(blockSafe);
        if (auto list = block.textList()) {
            QTextListFormat listSafe;
            listSafe.setStyle(list->format().style());
            listSafe.setIndent(std::clamp(list->format().indent(), 1, 8));
            if (lists.contains(list)) {
                lists[list]->add(cursor.block());
            } else {
                lists[list] = cursor.createList(listSafe);
            }
        }
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const auto fragment = it.fragment();
            if (fragment.isValid()) {
                cursor.insertText(fragment.text(), safeCharacter(fragment.charFormat()));
            }
        }
    }
    restoreRuns(target, e);
}

QVariantList exportRuns(const QTextDocument &document) {
    QVariantList runs;
    for (auto block = document.begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const auto fragment = it.fragment();
            const auto format = fragment.charFormat();
            QVariantMap axes, features;
            const auto fontAxes = format.fontVariableAxes();
            const auto fontFeatures = format.fontFeatures();
            for (auto i = fontAxes.begin(); i != fontAxes.end(); ++i) {
                axes[QString::fromLatin1(i.key().toString())] = i.value();
            }
            for (auto i = fontFeatures.begin(); i != fontFeatures.end(); ++i) {
                features[QString::fromLatin1(i.key().toString())] = i.value();
            }
            if (!axes.isEmpty() || !features.isEmpty()
                || format.fontCapitalization() != QFont::MixedCase) {
                runs.append(QVariantMap { { "start", fragment.position() },
                    { "length", fragment.length() }, { "axes", axes }, { "features", features },
                    { "capitalization", int(format.fontCapitalization()) } });
            }
        }
    }
    return runs;
}

void restoreRuns(QTextDocument &document, const QVariantMap &element) {
    document.setDefaultFont(baseFont(element));
    for (const auto &value : element.value("textRuns").toList()) {
        const auto run = value.toMap();
        const int from = std::clamp(run.value("start").toInt(), 0, document.characterCount() - 1);
        const int length
            = std::clamp(run.value("length").toInt(), 0, document.characterCount() - 1 - from);
        QTextCursor cursor(&document);
        cursor.setPosition(from);
        cursor.setPosition(from + length, QTextCursor::KeepAnchor);
        QTextCharFormat format;
        QMap<QFont::Tag, float> axes;
        QMap<QFont::Tag, quint32> features;
        const auto sourceAxes = run.value("axes").toMap(),
                   sourceFeatures = run.value("features").toMap();
        for (auto i = sourceAxes.begin(); i != sourceAxes.end(); ++i) {
            if (auto tag = QFont::Tag::fromString(i.key())) {
                axes[*tag] = i.value().toFloat();
            }
        }
        for (auto i = sourceFeatures.begin(); i != sourceFeatures.end(); ++i) {
            if (auto tag = QFont::Tag::fromString(i.key())) {
                features[*tag] = i.value().toUInt();
            }
        }
        format.setFontVariableAxes(axes);
        format.setFontFeatures(features);
        format.setFontCapitalization(
            QFont::Capitalization(std::clamp(run.value("capitalization").toInt(), 0, 4)));
        cursor.mergeCharFormat(format);
    }
}

void resizeText(QVariantMap &e, LocalDocument &document) {
    const auto mode = e.value("textSizing", "fixed").toString();
    if (mode == "fixed" || mode == "shrink") {
        return;
    }
    document.setTextWidth(mode == "autoWidth" ? -1 : e.value("width", 400).toDouble());
    const auto size = document.size();
    if (mode == "autoWidth") {
        e["width"] = std::clamp(std::ceil(size.width()) + 2, 40.0, 1920.0);
    }
    e["height"] = std::clamp(std::ceil(size.height()) + 2, 30.0, 1080.0);
}
}

QVariantMap TextFormatting::defaults() {
    return { { "richText", "" }, { "textRuns", QVariantList() }, { "fontWeight", 400 },
        { "italic", false }, { "underline", false }, { "strikethrough", false },
        { "letterSpacing", 0.0 }, { "lineHeight", 1.0 }, { "lineHeightUnit", "percent" },
        { "paragraphSpacing", 0.0 }, { "paragraphIndent", 0.0 }, { "textCase", "original" },
        { "verticalAlignment", "top" }, { "textSizing", "fixed" }, { "minFontSize", 32.0 },
        { "wrap", true }, { "clipText", true }, { "maxLines", 0 }, { "ellipsis", true },
        { "baseline", "normal" }, { "listStyle", "none" }, { "listIndent", 1 },
        { "fontFeatures", QVariantMap() }, { "fontAxes", QVariantMap() } };
}

bool TextFormatting::isStyle(const QString &key) {
    return defaults().contains(key)
        || QStringList { "fontFamily", "fontSize", "color", "alignment" }.contains(key);
}

QVariantMap TextFormatting::normalize(const QVariantMap &input) {
    auto e = defaults();
    for (auto i = input.begin(); i != input.end(); ++i) {
        e[i.key()] = i.value();
    }
    const QVariantMap bounds { { "fontWeight", QVariantList { 1.0, 1000.0 } },
        { "minFontSize", QVariantList { 8.0, 400.0 } },
        { "letterSpacing", QVariantList { -100.0, 500.0 } },
        { "lineHeight", QVariantList { 0.1, 1000.0 } },
        { "paragraphSpacing", QVariantList { 0.0, 1080.0 } },
        { "paragraphIndent", QVariantList { -1920.0, 1920.0 } },
        { "maxLines", QVariantList { 0.0, 1000.0 } }, { "listIndent", QVariantList { 1.0, 8.0 } } };
    const auto fallback = defaults();
    for (auto i = bounds.begin(); i != bounds.end(); ++i) {
        bool ok = false;
        const double n = e[i.key()].toDouble(&ok);
        const auto limits = i.value().toList();
        e[i.key()] = ok && std::isfinite(n)
            ? std::clamp(n, limits[0].toDouble(), limits[1].toDouble())
            : fallback[i.key()];
    }
    for (const auto &key :
        { "italic", "underline", "strikethrough", "wrap", "clipText", "ellipsis" }) {
        e[key] = e[key].toBool();
    }
    const QMap<QString, QStringList> choices {
        { "textCase", { "original", "upper", "lower", "title", "smallCaps" } },
        { "verticalAlignment", { "top", "center", "bottom" } },
        { "textSizing", { "fixed", "autoWidth", "autoHeight", "shrink" } },
        { "baseline", { "normal", "super", "sub" } },
        { "listStyle", { "none", "bullet", "number" } }, { "lineHeightUnit", { "percent", "px" } }
    };
    for (auto i = choices.begin(); i != choices.end(); ++i) {
        if (!i.value().contains(e[i.key()].toString())) {
            e[i.key()] = fallback[i.key()];
        }
    }
    for (const auto &key : { "fontAxes", "fontFeatures" }) {
        QVariantMap safe;
        const auto values = e[key].toMap();
        for (auto i = values.begin(); i != values.end(); ++i) {
            bool ok = false;
            const double value = i.value().toDouble(&ok);
            if (QFont::Tag::fromString(i.key()) && ok && std::isfinite(value)) {
                safe[i.key()] = std::clamp(value, -100000.0, 100000.0);
            }
        }
        e[key] = safe;
    }
    return e;
}

QString TextFormatting::html(const QVariantMap &element) const {
    LocalDocument document;
    load(document, normalize(element));
    return document.toHtml();
}

QVariantMap TextFormatting::capture(const QVariantMap &element, const QString &html) const {
    auto e = normalize(element);
    LocalDocument input;
    input.setHtml(html.left(2000000));
    e["text"] = input.toPlainText();
    e["richText"] = html.left(2000000);
    LocalDocument safe;
    load(safe, e);
    e["richText"] = safe.toHtml();
    e["textRuns"] = exportRuns(safe);
    resizeText(e, safe);
    return e;
}

QVariantMap TextFormatting::apply(
    const QVariantMap &element, const QVariantMap &patch, int start, int end) const {
    auto e = normalize(element);
    LocalDocument document;
    load(document, e);
    const bool whole = start < 0 || end < 0 || start == end
        || (std::min(start, end) == 0 && std::max(start, end) >= document.characterCount() - 1);
    if (whole) {
        for (auto i = patch.begin(); i != patch.end(); ++i) {
            if (isStyle(i.key())) {
                e[i.key()] = i.value();
            }
        }
        e = normalize(e);
    }
    // Box layout stays editable when the user has selected a word or paragraph.
    for (const auto &key : { "verticalAlignment", "textSizing", "minFontSize", "wrap", "clipText",
             "maxLines", "ellipsis" }) {
        if (patch.contains(key)) {
            e[key] = patch[key];
        }
    }
    e = normalize(e);
    QTextCursor cursor(&document);
    cursor.setPosition(
        whole ? 0 : std::clamp(std::min(start, end), 0, document.characterCount() - 1));
    cursor.setPosition(whole ? document.characterCount() - 1
                             : std::clamp(std::max(start, end), 0, document.characterCount() - 1),
        QTextCursor::KeepAnchor);
    QTextCharFormat character;
    if (patch.contains("fontFamily")) {
        character.setFontFamilies({ patch["fontFamily"].toString() });
    }
    if (patch.contains("fontSize")) {
        character.setProperty(
            QTextFormat::FontPixelSize, std::clamp(patch["fontSize"].toDouble(), 8.0, 400.0));
    }
    if (patch.contains("fontWeight")) {
        character.setFontWeight(std::clamp(patch["fontWeight"].toInt(), 1, 1000));
    }
    if (patch.contains("italic")) {
        character.setFontItalic(patch["italic"].toBool());
    }
    if (patch.contains("underline")) {
        character.setFontUnderline(patch["underline"].toBool());
    }
    if (patch.contains("strikethrough")) {
        character.setFontStrikeOut(patch["strikethrough"].toBool());
    }
    if (patch.contains("color") && QColor(patch["color"].toString()).isValid()) {
        character.setForeground(QColor(patch["color"].toString()));
    }
    if (patch.contains("letterSpacing")) {
        character.setFontLetterSpacingType(QFont::AbsoluteSpacing);
        character.setFontLetterSpacing(
            std::clamp(patch["letterSpacing"].toDouble(), -100.0, 500.0));
    }
    if (patch.contains("textCase")) {
        character.setFontCapitalization(
            baseFont({ { "textCase", patch["textCase"] } }).capitalization());
    }
    if (patch.contains("baseline")) {
        character.setVerticalAlignment(patch["baseline"] == "super"
                ? QTextCharFormat::AlignSuperScript
                : patch["baseline"] == "sub" ? QTextCharFormat::AlignSubScript
                                             : QTextCharFormat::AlignNormal);
    }
    if (patch.contains("fontAxes") || patch.contains("fontFeatures")) {
        const auto configured = baseFont(normalize(patch));
        if (patch.contains("fontAxes")) {
            QMap<QFont::Tag, float> axes;
            for (const auto &tag : configured.variableAxisTags()) {
                axes[tag] = configured.variableAxisValue(tag);
            }
            character.setFontVariableAxes(axes);
        }
        if (patch.contains("fontFeatures")) {
            QMap<QFont::Tag, quint32> features;
            for (const auto &tag : configured.featureTags()) {
                features[tag] = configured.featureValue(tag);
            }
            character.setFontFeatures(features);
        }
    }
    cursor.mergeCharFormat(character);
    auto block = cursor.blockFormat();
    if (patch.contains("alignment")) {
        block.setAlignment(alignment(patch["alignment"].toString()));
    }
    if (patch.contains("paragraphSpacing")) {
        block.setBottomMargin(std::clamp(patch["paragraphSpacing"].toDouble(), 0.0, 1080.0));
    }
    if (patch.contains("paragraphIndent")) {
        block.setTextIndent(std::clamp(patch["paragraphIndent"].toDouble(), -1920.0, 1920.0));
    }
    if (patch.contains("lineHeight") || patch.contains("lineHeightUnit")) {
        const bool px = patch.value("lineHeightUnit", e["lineHeightUnit"]) == "px";
        const double height = patch.value("lineHeight", e["lineHeight"]).toDouble();
        block.setLineHeight(px ? height : height * 100,
            px ? QTextBlockFormat::FixedHeight : QTextBlockFormat::ProportionalHeight);
    }
    cursor.mergeBlockFormat(block);
    if (patch.contains("listStyle") || patch.contains("listIndent")) {
        const auto style = patch.value("listStyle", e["listStyle"]).toString();
        if (style == "none") {
            for (auto b = document.findBlock(cursor.selectionStart());
                b.isValid() && b.position() <= cursor.selectionEnd(); b = b.next()) {
                if (auto list = b.textList()) {
                    list->remove(b);
                }
                QTextCursor c(b);
                auto format = b.blockFormat();
                format.setIndent(0);
                c.setBlockFormat(format);
            }
        } else {
            QTextListFormat format;
            format.setStyle(
                style == "number" ? QTextListFormat::ListDecimal : QTextListFormat::ListDisc);
            format.setIndent(std::clamp(patch.value("listIndent", e["listIndent"]).toInt(), 1, 8));
            cursor.createList(format);
        }
    }
    e["text"] = document.toPlainText();
    e["richText"] = document.toHtml();
    e["textRuns"] = exportRuns(document);
    resizeText(e, document);
    return e;
}

QVariantMap TextFormatting::styleAt(const QVariantMap &element, int start, int end) const {
    auto style = normalize(element);
    LocalDocument document;
    load(document, style);
    QTextCursor cursor(&document);
    cursor.setPosition(std::clamp(std::min(start, end), 0, document.characterCount() - 1));
    if (cursor.position() < document.characterCount() - 1) {
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }
    const auto format = cursor.charFormat();
    const auto font = format.font();
    style["fontFamily"] = font.family().isEmpty() ? style["fontFamily"] : QVariant(font.family());
    const double px = format.doubleProperty(QTextFormat::FontPixelSize);
    style["fontSize"] = px > 0  ? px
        : font.pointSizeF() > 0 ? font.pointSizeF() * 96 / 72
                                : style["fontSize"];
    QVariantMap axes, features;
    const auto currentAxes = format.fontVariableAxes();
    const auto currentFeatures = format.fontFeatures();
    for (auto i = currentAxes.begin(); i != currentAxes.end(); ++i) {
        axes[QString::fromLatin1(i.key().toString())] = i.value();
    }
    for (auto i = currentFeatures.begin(); i != currentFeatures.end(); ++i) {
        features[QString::fromLatin1(i.key().toString())] = i.value();
    }
    if (!axes.isEmpty()) {
        style["fontAxes"] = axes;
    }
    if (!features.isEmpty()) {
        style["fontFeatures"] = features;
    }
    const auto block = cursor.blockFormat();
    const auto horizontal = block.alignment();
    style["alignment"] = horizontal.testFlag(Qt::AlignJustify) ? "justify"
        : horizontal.testFlag(Qt::AlignHCenter)                ? "center"
        : horizontal.testFlag(Qt::AlignRight)                  ? "right"
                                                               : "left";
    style["paragraphSpacing"] = block.bottomMargin();
    style["paragraphIndent"] = block.textIndent();
    if (block.lineHeight() > 0) {
        const bool fixed = block.lineHeightType() == QTextBlockFormat::FixedHeight;
        style["lineHeightUnit"] = fixed ? "px" : "percent";
        style["lineHeight"] = fixed ? block.lineHeight() : block.lineHeight() / 100;
    }
    style["textCase"] = format.fontCapitalization() == QFont::AllUppercase ? "upper"
        : format.fontCapitalization() == QFont::AllLowercase               ? "lower"
        : format.fontCapitalization() == QFont::Capitalize                 ? "title"
        : format.fontCapitalization() == QFont::SmallCaps                  ? "smallCaps"
                                                                           : "original";
    if (auto list = cursor.block().textList()) {
        style["listStyle"]
            = list->format().style() == QTextListFormat::ListDecimal ? "number" : "bullet";
        style["listIndent"] = list->format().indent();
    } else {
        style["listStyle"] = "none";
    }
    style["fontWeight"] = font.weight();
    style["letterSpacing"] = font.letterSpacing();
    style["baseline"] = format.verticalAlignment() == QTextCharFormat::AlignSuperScript ? "super"
        : format.verticalAlignment() == QTextCharFormat::AlignSubScript                 ? "sub"
                                                                                        : "normal";
    style["italic"] = font.italic();
    style["underline"] = font.underline();
    style["strikethrough"] = font.strikeOut();
    if (format.foreground().style() != Qt::NoBrush) {
        style["color"] = format.foreground().color().name(QColor::HexArgb);
    }
    return style;
}

void TextFormatting::hydrate(QQuickTextDocument *document, const QVariantMap &element) const {
    if (!document || !document->textDocument()
        || document->textDocument()->toPlainText() != element.value("text").toString()) {
        return;
    }
    restoreRuns(*document->textDocument(), normalize(element));
}

QVariantMap TextFormatting::captureDocument(
    const QVariantMap &element, QQuickTextDocument *document) const {
    if (!document || !document->textDocument()) {
        return element;
    }
    auto e = element;
    e["textRuns"] = exportRuns(*document->textDocument());
    return capture(e, document->textDocument()->toHtml());
}

QVariantMap TextFormatting::fitted(const QVariantMap &element, int maximumLines) {
    const auto source = normalize(element);
    if (source["textSizing"] != "shrink" || source["text"].toString().isEmpty()) {
        return source;
    }
    const double authored = std::max(8.0, source.value("fontSize", 36).toDouble());
    const int maximum = int(std::floor(authored));
    const int minimum = int(std::ceil(qMin(authored, source["minFontSize"].toDouble())));
    const double width = std::max(1.0, source["width"].toDouble() - 8);
    const double height = std::max(1.0, source["height"].toDouble() - 8);
    const int limit = source["maxLines"].toInt();
    if (limit > 0) {
        maximumLines = qMin(maximumLines, limit);
    }
    auto fits = [&](const QVariantMap &data) {
        const auto size = TextLayoutMetrics::measure(data["text"].toString(), data, width);
        return size.width <= width + 0.5 && size.height <= height && size.lines <= maximumLines;
    };
    if (fits(source)) {
        return source;
    }
    if (minimum > maximum) {
        auto overflow = source;
        overflow["_fitOverflow"] = true;
        return overflow;
    }
    auto scaled = [&](int pixels) {
        auto data = source;
        const double factor = pixels / authored;
        data["fontSize"] = pixels;
        for (const auto &key : { "letterSpacing", "paragraphSpacing", "paragraphIndent" }) {
            data[key] = source[key].toDouble() * factor;
        }
        if (source["lineHeightUnit"] == "px") {
            data["lineHeight"] = source["lineHeight"].toDouble() * factor;
        }
        if (!source["richText"].toString().isEmpty()) {
            LocalDocument document;
            load(document, source);
            // Scale each run independently so selected-word sizes, weight and color survive.
            struct Run {
                int start, length;
                QTextCharFormat format;
            };
            QList<Run> runs;
            for (auto block = document.begin(); block.isValid(); block = block.next()) {
                for (auto it = block.begin(); !it.atEnd(); ++it) {
                    const auto fragment = it.fragment();
                    auto format = fragment.charFormat();
                    auto font = format.font();
                    if (font.pixelSize() > 0) {
                        font.setPixelSize(qMax(1, qRound(font.pixelSize() * factor)));
                    } else if (font.pointSizeF() > 0) {
                        font.setPointSizeF(font.pointSizeF() * factor);
                    }
                    if (font.letterSpacingType() == QFont::AbsoluteSpacing) {
                        font.setLetterSpacing(
                            QFont::AbsoluteSpacing, font.letterSpacing() * factor);
                    }
                    format.setFont(font);
                    runs.append({ fragment.position(), fragment.length(), format });
                }
            }
            for (const auto &run : runs) {
                QTextCursor cursor(&document);
                cursor.setPosition(run.start);
                cursor.setPosition(run.start + run.length, QTextCursor::KeepAnchor);
                cursor.mergeCharFormat(run.format);
            }
            for (auto block = document.begin(); block.isValid(); block = block.next()) {
                QTextCursor cursor(block);
                auto format = block.blockFormat();
                format.setBottomMargin(format.bottomMargin() * factor);
                format.setTextIndent(format.textIndent() * factor);
                if (format.lineHeightType() == QTextBlockFormat::FixedHeight) {
                    format.setLineHeight(
                        format.lineHeight() * factor, QTextBlockFormat::FixedHeight);
                }
                cursor.setBlockFormat(format);
            }
            data["richText"] = document.toHtml();
        }
        return data;
    };
    auto best = scaled(minimum);
    int low = minimum, high = maximum;
    while (low <= high) {
        const int middle = (low + high) / 2;
        const auto candidate = scaled(middle);
        if (fits(candidate)) {
            best = candidate;
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    best["_fitOverflow"] = !fits(best);
    return best;
}

QVariantMap TextFormatting::display(const QVariantMap &element) const {
    auto e = normalize(element);
    if (e["textSizing"] == "shrink") {
        return fitted(e);
    }
    const int limit = e.value("maxLines").toInt();
    if (limit <= 0) {
        return e;
    }
    LocalDocument document;
    load(document, e);
    document.setTextWidth(e.value("width", 400).toDouble());
    document.size();
    int lines = 0, end = -1;
    for (auto block = document.begin(); block.isValid() && end < 0; block = block.next()) {
        const auto layout = block.layout();
        for (int i = 0; i < layout->lineCount(); ++i) {
            const auto line = layout->lineAt(i);
            if (++lines == limit) {
                end = block.position() + line.textStart() + line.textLength();
                break;
            }
        }
    }
    if (end < 0 || end >= document.characterCount() - 1) {
        return e;
    }
    QTextCursor cursor(&document);
    cursor.setPosition(end);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    if (e.value("ellipsis", true).toBool()) {
        while (cursor.position() > 0 && document.toPlainText().right(1).trimmed().isEmpty()) {
            cursor.deletePreviousChar();
        }
        cursor.insertText(QString(QChar(0x2026)));
        document.size();
        // Make space for the ellipsis at a grapheme boundary, including Indic conjuncts.
        for (int attempts = 0;
            attempts < 100 && document.lineCount() > limit && cursor.position() > 1; ++attempts) {
            cursor.deletePreviousChar();
            const auto text = document.toPlainText();
            QTextBoundaryFinder boundary(QTextBoundaryFinder::Grapheme, text);
            boundary.toEnd();
            const int previous = boundary.toPreviousBoundary();
            cursor.setPosition(std::max(0, previous));
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.insertText(QString(QChar(0x2026)));
            document.size();
        }
    }
    e["text"] = document.toPlainText();
    e["richText"] = document.toHtml();
    e["textRuns"] = exportRuns(document);
    return e;
}

TextLayoutMetrics::Result TextLayoutMetrics::measure(
    const QString &text, const QVariantMap &element, double width) {
    auto data = TextFormatting::normalize(element);
    data["text"] = text;
    LocalDocument document;
    load(document, data);
    auto option = document.defaultTextOption();
    option.setWrapMode(data.value("wrap", true).toBool() ? QTextOption::WrapAtWordBoundaryOrAnywhere
                                                         : QTextOption::NoWrap);
    document.setDefaultTextOption(option);
    document.setTextWidth(std::max(1.0, width));
    Result result;
    result.height = document.size().height();
    for (auto block = document.begin(); block.isValid(); block = block.next()) {
        const auto layout = block.layout();
        result.lines += layout->lineCount();
        for (int i = 0; i < layout->lineCount(); ++i) {
            const auto line = layout->lineAt(i);
            result.width = std::max(result.width, line.x() + line.naturalTextWidth());
        }
    }
    return result;
}
