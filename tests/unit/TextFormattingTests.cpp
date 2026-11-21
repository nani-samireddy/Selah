#include "DocumentElements.h"
#include "EditorDocument.h"
#include "TextFormatting.h"
#include "TextLayoutMetrics.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextList>

int TextFormatting::selfTest() {
    auto check = [](bool ok, const char *label) {
        if (!ok) {
            qWarning() << "Text formatting:" << label;
        }
        return ok;
    };
    TextFormatting formatter;
    auto element = DocumentElements::text(
        "Test", "Hello world\nనా తల్లియు నీవే", 100, 100, 600, 200, 48, "Arial");
    auto result = formatter.apply(
        element, { { "fontWeight", 700 }, { "italic", true }, { "color", "#FF3344" } }, 6, 11);
    if (!check(result["text"] == element["text"], "Formatting preserves multilingual source")) {
        return 1;
    }
    auto selected = formatter.styleAt(result, 6, 11), untouched = formatter.styleAt(result, 0, 5);
    if (!check(selected["fontWeight"].toInt() == 700 && selected["italic"].toBool()
                && untouched["fontWeight"].toInt() == 400 && !untouched["italic"].toBool(),
            "Selected words have independent formatting")) {
        return 2;
    }
    result = formatter.apply(result, { { "underline", true }, { "letterSpacing", 2.5 } });
    if (!check(formatter.styleAt(result, 6, 11)["fontWeight"].toInt() == 700
                && formatter.styleAt(result, 0, 5)["underline"].toBool(),
            "Layer formatting preserves mixed weights")) {
        return 3;
    }
    const QVariantMap axes { { "wght", 650.0 }, { "wdth", 90.0 } },
        features { { "tnum", 1 }, { "liga", 0 } };
    result = formatter.apply(result,
        { { "fontAxes", axes }, { "fontFeatures", features }, { "textCase", "upper" } }, 6, 11);
    auto json = QJsonDocument::fromVariant(result).toJson();
    auto reopened = QJsonDocument::fromJson(json).toVariant().toMap();
    selected = formatter.styleAt(reopened, 6, 11);
    if (!check(selected["fontAxes"].toMap()["wght"].toInt() == 650
                && selected["fontFeatures"].toMap()["tnum"].toInt() == 1
                && selected["textCase"] == "upper",
            "Variable axes and OpenType survive JSON and rich text")) {
        return 4;
    }
    const auto allSelected = formatter.apply(element, { { "fontSize", 60 }, { "fontWeight", 600 } },
        0, element["text"].toString().size());
    if (!check(allSelected["fontSize"].toInt() == 60 && allSelected["fontWeight"].toInt() == 600,
            "Select-all updates inheritable template styles")) {
        return 15;
    }
    const auto regularMetrics = TextLayoutMetrics::measure("First line\nSecond line", element, 600);
    auto spaced = element;
    spaced["lineHeight"] = 2.0;
    const auto spacedMetrics = TextLayoutMetrics::measure("First line\nSecond line", spaced, 600);
    if (!check(spacedMetrics.height > regularMetrics.height && spacedMetrics.lines == 2,
            "Pagination measures authored line spacing")) {
        return 16;
    }
    auto bullets = formatter.apply(element,
        { { "listStyle", "number" }, { "paragraphSpacing", 10 }, { "paragraphIndent", 12 } });
    QTextDocument document;
    document.setHtml(formatter.html(bullets));
    if (!check(document.firstBlock().textList() && document.lastBlock().textList()
                && document.firstBlock().textList() == document.lastBlock().textList(),
            "Numbered paragraphs remain one list")) {
        return 5;
    }
    const auto positioned = formatter.apply(
        element, { { "verticalAlignment", "center" }, { "textSizing", "autoHeight" } }, 0, 5);
    if (!check(
            positioned["verticalAlignment"] == "center" && positioned["textSizing"] == "autoHeight",
            "Layer sizing remains available during range editing")) {
        return 14;
    }
    auto autoHeight = formatter.apply(element, { { "textSizing", "autoHeight" } });
    if (!check(autoHeight["height"].toDouble() >= 30 && autoHeight["height"].toDouble() <= 1080,
            "Auto height is bounded")) {
        return 6;
    }
    auto untrusted = element;
    untrusted["richText"] = "<p><a href='https://example.com'>Hello world</a><img "
                            "src='https://example.com/image.png'></p>";
    auto safe = formatter.html(untrusted);
    if (!check(!safe.contains("https://example.com") && !safe.contains("<img"),
            "Rich text cannot introduce resource or link loads")) {
        return 7;
    }
    auto substituted = result;
    substituted["text"] = "A newly generated lyric";
    QTextDocument replacement;
    replacement.setHtml(formatter.html(substituted));
    if (!check(replacement.toPlainText() == "A newly generated lyric",
            "Template replacements discard stale character ranges")) {
        return 8;
    }
    auto truncated = element;
    truncated["text"] = "One two three four five six seven eight nine ten";
    truncated["width"] = 180;
    truncated["maxLines"] = 1;
    const auto display = formatter.display(truncated);
    if (!check(display["text"].toString().size() < truncated["text"].toString().size()
                && truncated["text"] != display["text"],
            "Truncation does not modify authored text")) {
        return 9;
    }
    EditorDocument editor(nullptr, false);
    editor.addText();
    const auto original = editor.selectedElement();
    editor.formatText(
        { { "fontWeight", 700 }, { "fontSize", 64 }, { "verticalAlignment", "center" } }, 0, 4);
    const auto formatted = editor.slides();
    editor.undo();
    if (!check(editor.selectedElement().isEmpty()
                || editor.elements().last().toMap()["richText"] == original["richText"],
            "Formatting is undoable")) {
        return 10;
    }
    editor.redo();
    if (!check(editor.slides() == formatted, "Formatting is redoable")) {
        return 11;
    }
    QTemporaryDir directory;
    const auto path = QUrl::fromLocalFile(directory.filePath("rich.churchdeck"));
    if (!editor.saveProject(path)) {
        return 12;
    }
    EditorDocument restored(nullptr, false);
    if (!check(restored.openProject(path) && restored.slides() == editor.slides(),
            "Rich styles round-trip in projects")) {
        return 13;
    }
    qInfo() << "Rich text formatting checks passed";
    return 0;
}
