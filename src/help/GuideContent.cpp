#include "GuideContent.h"
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

QString GuideContent::read(const QString &topic) const {
    static const QStringList topics { "getting-started.md", "songs.md", "bible.md", "design.md",
        "presenting.md", "remote-and-data.md" };
    if (!topics.contains(topic)) {
        return {};
    }
    QFile file(":/guides/" + topic);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

QString GuideContent::formatted(const QString &topic) const {
    auto markdown = read(topic);
    if (markdown.isEmpty()) {
        return {};
    }
    // The reader provides a topic heading; avoid repeating the article's H1.
    markdown.remove(QRegularExpression("^# [^\\n]*\\n+"));
    QTextDocument document;
    QFont font("Inter");
    font.setPixelSize(13);
    document.setDefaultFont(font);
    document.setMarkdown(markdown);
    for (auto block = document.begin(); block.isValid(); block = block.next()) {
        QTextCursor cursor(block);
        auto format = block.blockFormat();
        const int heading = format.headingLevel();
        format.setLineHeight(150, QTextBlockFormat::ProportionalHeight);
        format.setTopMargin(heading ? 20 : 0);
        format.setBottomMargin(heading ? 8 : block.textList() ? 6 : 12);
        cursor.setBlockFormat(format);
        if (heading) {
            cursor.select(QTextCursor::BlockUnderCursor);
            QTextCharFormat text;
            QFont headingFont("Inter");
            headingFont.setPixelSize(heading <= 2 ? 16 : 14);
            headingFont.setWeight(QFont::Medium);
            text.setFont(headingFont);
            cursor.mergeCharFormat(text);
        }
    }
    return document.toHtml();
}
