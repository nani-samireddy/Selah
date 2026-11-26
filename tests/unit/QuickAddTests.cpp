#include "ContentLibrary.h"
#include "MediaLibrary.h"
#include "OutputManager.h"
#include "PresentationLibrary.h"
#include "ProgramStore.h"
#include "QuickAddController.h"
#include "TemplateDefinition.h"
#include <QImage>
#include <QTemporaryDir>

int runQuickAddChecks() {
    QTemporaryDir folder;
    ContentLibrary content(nullptr, folder.filePath("content"));
    PresentationLibrary presentations(nullptr, folder.filePath("presentations"));
    MediaLibrary media;
    OutputManager outputs;
    ProgramStore programs(&outputs, nullptr, folder.filePath("programs"));
    QuickAddController quick(&content, &presentations, &media, &outputs, nullptr, false);
    auto slide = [](const QString &name, const QString &group) {
        return QVariantMap { { "name", name }, { "programItemId", group },
            { "programItemTitle", group }, { "programTitle", "Original program" },
            { "elements",
                QVariantList {
                    QVariantMap { { "kind", "text" }, { "text", name }, { "visible", true } } } } };
    };
    const QVariantList original { slide("Song 1", "song"), slide("Song 2", "song"),
        slide("Reading", "bible") };
    outputs.takeLive(original, 0, "original-source");
    const auto savedProgram = programs.items();
    const auto savedSongs = content.songs();
    quick.setDraft(
        { { "title", "Spontaneous song" }, { "text", "First section\n\nSecond section" } });
    if (!quick.previewDraft(true) || quick.slides().size() != 2 || outputs.liveSlides() != original
        || content.songs() != savedSongs) {
        return 1;
    }
    if (!quick.add(false) || outputs.liveIndex() != 0 || outputs.liveTitle() != "Song 1"
        || outputs.liveCount() != 5 || outputs.liveSlides()[1] != original[1]
        || !outputs.liveOrder()[2].toMap()["quickAddition"].toBool()) {
        return 2;
    }
    quick.setDraft({ { "title", "Announcement" }, { "text", "Welcome, guest speaker" } });
    if (!quick.previewDraft(false) || !quick.add(false)
        || outputs.liveOrder()[4].toMap()["itemTitle"] != "Announcement") {
        return 3;
    }
    outputs.advance(2);
    if (!outputs.canResumeProgram() || !outputs.resumeProgram()
        || outputs.liveTitle() != "Reading") {
        return 4;
    }
    outputs.takeLive(original, 0, "original-source");
    if (!quick.add(true) || !outputs.canResumeProgram() || outputs.liveTitle().isEmpty()
        || !outputs.resumeProgram() || outputs.liveIndex() != 0 || outputs.liveTitle() != "Song 1"
        || outputs.liveSource() != "original-source") {
        return 5;
    }
    outputs.blackout("audience", true);
    const auto snapshot = outputs.liveSlides();
    if (!quick.previewBible("engwebp", "genses 1:1") || outputs.liveSlides() != snapshot
        || !outputs.audienceBlack() || quick.slides().isEmpty() || programs.items() != savedProgram
        || content.songs() != savedSongs) {
        return 6;
    }
    quick.setDraft(
        { { "title", "Spontaneous song" }, { "text", "First section\n\nSecond section" } });
    if (!quick.previewDraft(true) || !quick.saveDraft()
        || quick.searchSongs("spontanous").isEmpty()) {
        return 7;
    }
    quick.setDraft({ { "title", "New text" }, { "text", "A message" } });
    if (!quick.previewDraft(false) || !quick.saveDraft()
        || presentations.presentations().isEmpty()) {
        return 8;
    }
    quick.clearPreview();
    if (quick.add(true) || !quick.slides().isEmpty()) {
        return 9;
    }
    const auto beforeMedia = outputs.liveSlides();
    const auto imagePath = folder.filePath("guest.png");
    QImage image(64, 64, QImage::Format_RGB32);
    image.fill(Qt::blue);
    if (!image.save(imagePath) || !quick.importFile(QUrl::fromLocalFile(imagePath), false)
        || quick.slides().first().toMap()["elements"].toList().first().toMap()["kind"] != "image"
        || outputs.liveSlides() != beforeMedia) {
        return 10;
    }
    const auto presentationId = presentations.presentations().first().toMap()["id"].toString();
    if (!quick.previewPresentation(presentationId) || quick.slides().isEmpty()
        || outputs.liveSlides() != beforeMedia || programs.items() != savedProgram) {
        return 11;
    }
    outputs.takeLive(original, 0, "original-source");
    const auto verses = content.searchVerses("engwebp", "John 3:16-17");
    if (verses.size() != 2 || !quick.previewScripture(verses, {})
        || outputs.liveSlides() != original || !quick.presentScripture()
        || !outputs.canResumeProgram()) {
        return 12;
    }
    const int readingCount = outputs.liveCount();
    const int firstReadingIndex = outputs.liveIndex();
    if (!quick.advanceScripture(1) || outputs.liveIndex() != firstReadingIndex + 1
        || !quick.previewScripture(verses, {}) || !quick.presentScripture()
        || outputs.liveIndex() != firstReadingIndex || outputs.liveCount() != readingCount
        || !outputs.resumeProgram() || outputs.liveIndex() != 0) {
        return 13;
    }
    if (!quick.presentScripture() || !outputs.resumeProgram() || outputs.liveIndex() != 0
        || outputs.liveCount() != readingCount || programs.items() != savedProgram) {
        return 14;
    }
    quick.clearPreview();
    if (quick.presentScripture()) {
        return 15;
    }
    const auto paired = content.pairScripture(verses, "tel2017");
    if (!quick.previewScripture(paired["verses"].toList(), TemplateDefinition::dualScripture())
        || !quick.presentScripture()) {
        return 16;
    }
    bool secondaryVisible = false;
    for (const auto &value : outputs.liveElements()) {
        const auto box = value.toMap();
        secondaryVisible
            |= box["binding"] == "Secondary scripture text" && !box["text"].toString().isEmpty();
    }
    if (!secondaryVisible) {
        return 17;
    }
    auto longVerse = verses.first().toMap();
    longVerse["text"] = QString("God is love. We follow his light. ").repeated(20);
    if (!quick.previewScripture({ longVerse }, {}, 1) || quick.slides().size() < 2
        || !quick.presentScripture()) {
        return 18;
    }
    const int splitStart = outputs.liveIndex();
    // Browsing elsewhere must not redirect the reading controls away from the live passage.
    if (!quick.previewScripture({ verses.last() }, {}) || !quick.advanceScripture(1)
        || outputs.liveIndex() != splitStart + 1) {
        return 19;
    }
    return 0;
}
