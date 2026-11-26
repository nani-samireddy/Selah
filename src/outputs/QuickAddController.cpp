#include "QuickAddController.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "MediaLibrary.h"
#include "OutputManager.h"
#include "PresentationLibrary.h"
#include "SearchSpelling.h"
#include "SlideFactory.h"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QUuid>

QuickAddController::QuickAddController(ContentLibrary *content, PresentationLibrary *presentations,
    MediaLibrary *media, OutputManager *outputs, QObject *parent, bool preferences)
    : QObject(parent)
    , m_content(content)
    , m_presentations(presentations)
    , m_media(media)
    , m_outputs(outputs)
    , m_preferences(preferences) {
    if (preferences) {
        m_draft = QSettings().value("QuickAdd/draft").toMap();
    }
}

void QuickAddController::setDraft(const QVariantMap &draft) {
    QVariantMap bounded;
    for (const auto &key : { "title", "text", "tags", "language", "translation", "secondary" }) {
        bounded[key] = draft.value(key).toString().left(QString(key) == "text" ? 30000 : 200);
    }
    if (bounded == m_draft) {
        return;
    }
    m_draft = bounded;
    if (m_preferences) {
        QSettings().setValue("QuickAdd/draft", m_draft);
    }
    clearPreview();
    emit draftChanged();
}

void QuickAddController::clearPreview() {
    m_scriptureKey.clear();
    m_slides.clear();
    m_title.clear();
    m_draftPreview = false;
    m_status = "Choose content to preview. The current slide stays live.";
    emit changed();
}

QVariantList QuickAddController::searchSongs(const QString &query) const {
    auto matches = m_content->searchSongs(query);
    if (!matches.isEmpty() || query.trimmed().isEmpty()) {
        return matches;
    }
    QStringList vocabulary;
    for (const auto &song : m_content->songs()) {
        const auto data = song.toMap();
        vocabulary += SearchSpelling::words(data["title"].toString());
        for (const auto &section : data["sections"].toList()) {
            vocabulary += SearchSpelling::words(section.toMap()["primary"].toString());
        }
    }
    return m_content->searchSongs(SearchSpelling::correct(query, vocabulary));
}

bool QuickAddController::stage(
    const QVariantList &slides, const QString &title, const QString &error) {
    m_draftPreview = false;
    m_scriptureKey.clear();
    m_slides = slides;
    m_title = title;
    m_status = slides.isEmpty()
        ? (error.isEmpty() ? "No content found. Check the reference or search." : error)
        : "Preview ready. Choose Present now or Insert next.";
    emit changed();
    return !slides.isEmpty();
}

bool QuickAddController::previewSong(const QString &id) {
    const auto song = m_content->song(id);
    QString error;
    const auto slides = SlideFactory::songSlides(song, 3, &error);
    return stage(slides, song["title"].toString(), error);
}

bool QuickAddController::previewBible(
    const QString &translation, const QString &query, const QString &secondary) {
    auto verses = m_content->searchVerses(translation, query);
    if (verses.isEmpty()) {
        QStringList vocabulary;
        for (const auto &book : m_content->books(translation)) {
            vocabulary += SearchSpelling::words(book.toMap()["name"].toString());
        }
        verses = m_content->searchVerses(translation,
            SearchSpelling::correct(query, vocabulary, SearchSpelling::Vocabulary::BibleBooks));
    }
    if (!secondary.isEmpty() && !verses.isEmpty()) {
        const auto paired = m_content->pairScripture(verses, secondary);
        if (!paired["error"].toString().isEmpty()) {
            return stage({}, {}, paired["error"].toString());
        }
        verses = paired["verses"].toList();
    }
    QString error;
    const auto slides = SlideFactory::scriptureSlides(verses, 4, &error);
    return stage(slides,
        verses.isEmpty() ? "Bible passage" : verses.first().toMap()["reference"].toString(), error);
}

bool QuickAddController::previewScripture(
    const QVariantList &verses, const QVariantMap &definition, int lines) {
    QString error;
    const auto slides = SlideFactory::scriptureSlides(verses, lines, &error, definition);
    if (!stage(slides,
            verses.isEmpty() ? QString() : verses.first().toMap()["reference"].toString(), error)) {
        return false;
    }
    // Identity is based on the passage and layout, never generated slide UUIDs.
    const auto payload = QJsonDocument::fromVariant(
        QVariantMap { { "verses", verses }, { "layout", definition },
            { "lines",
                lines } }).toJson(QJsonDocument::Compact);
    m_scriptureKey = QString::fromLatin1(
        QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
    m_status
        = m_outputs->liveSlides().value(m_outputs->liveIndex()).toMap()["scriptureKey"].toString()
            == m_scriptureKey
        ? "Scripture is live and retained in the live deck."
        : "Preview only. Select Present to take this passage live.";
    emit changed();
    return true;
}

bool QuickAddController::presentScripture() {
    if (m_scriptureKey.isEmpty()
        || !m_outputs->addQuickSlides(m_slides, "Scripture reading", true, m_scriptureKey)) {
        m_status = "Select and preview a passage first.";
        emit changed();
        return false;
    }
    m_presentedScriptureKey = m_scriptureKey;
    m_status = "Scripture is live and retained in the live deck.";
    emit changed();
    return true;
}

bool QuickAddController::advanceScripture(int delta) {
    const int next = m_outputs->liveIndex() + delta;
    const auto deck = m_outputs->liveSlides();
    if ((delta != -1 && delta != 1) || next < 0 || next >= deck.size()
        || m_presentedScriptureKey.isEmpty()
        || deck[m_outputs->liveIndex()].toMap()["scriptureKey"].toString()
            != m_presentedScriptureKey
        || deck[next].toMap()["scriptureKey"].toString() != m_presentedScriptureKey) {
        return false;
    }
    m_outputs->advance(delta);
    return true;
}

bool QuickAddController::previewDraft(bool song) {
    const auto title = m_draft["title"].toString().trimmed();
    auto text = m_draft["text"].toString();
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    if (title.isEmpty() || text.trimmed().isEmpty()) {
        return stage({}, {}, "Enter a name and text first.");
    }
    QVariantList sections, order;
    for (const auto &part :
        text.trimmed().split(QRegularExpression("\\n\\s*\\n"), Qt::SkipEmptyParts)) {
        const auto key = "s" + QString::number(sections.size() + 1);
        sections.append(QVariantMap { { "key", key },
            { "label", "Section " + QString::number(sections.size() + 1) },
            { "primary", part.trimmed() } });
        order.append(key);
    }
    if (sections.size() > 100) {
        return stage({}, {}, "Use at most 100 sections.");
    }
    QString error;
    auto slides = SlideFactory::songSlides(
        { { "title", title }, { "sections", sections }, { "arrangement", order } }, 4, &error);
    if (!song) {
        for (int i = 0; i < slides.size(); ++i) {
            auto slide = slides[i].toMap();
            slide["name"] = title + " · " + QString::number(i + 1);
            slide["sourceKind"] = "presentation";
            slides[i] = slide;
        }
    }
    const bool ready = stage(slides, title, error);
    m_draftPreview = ready;
    m_songDraft = song;
    emit changed();
    return ready;
}

bool QuickAddController::previewMedia(const QVariantMap &asset) {
    const auto source = QUrl(asset["source"].toString());
    const auto kind = asset["kind"].toString();
    if (!source.isLocalFile() || !QFileInfo(source.toLocalFile()).isFile()
        || (kind != "image" && kind != "video" && kind != "audio")) {
        return stage({}, {}, "Choose an available image, video or audio file.");
    }
    QVariantMap element { { "uid", QUuid::createUuid().toString(QUuid::WithoutBraces) },
        { "kind", kind }, { "name", asset["name"] }, { "source", source.toString() }, { "x", 0 },
        { "y", 0 }, { "width", 1920 }, { "height", 1080 }, { "fit", "contain" },
        { "loop", kind != "audio" }, { "muted", kind != "audio" }, { "visible", true },
        { "opacity", 1.0 }, { "fontSize", 36 }, { "color", "#FFFFFF" }, { "rotation", 0 },
        { "locked", false } };
    if (kind == "audio") {
        element["x"] = 80;
        element["y"] = 900;
        element["width"] = 400;
        element["height"] = 80;
    }
    return stage({ QVariantMap { { "name", asset["name"] }, { "background", "#000000" },
                     { "ambientOpacity", 0.0 }, { "elements", QVariantList { element } } } },
        asset["name"].toString());
}

bool QuickAddController::previewPresentation(const QString &id) {
    const auto data = m_presentations->presentation(id);
    return stage(data["slides"].toList(), data["title"].toString());
}

bool QuickAddController::importFile(const QUrl &source, bool presentation) {
    const QFileInfo info(source.toLocalFile());
    if (!source.isLocalFile() || !info.isFile()) {
        return stage({}, {}, "Choose a local file.");
    }
    if (presentation) {
        QFile file(source.toLocalFile());
        if (info.size() > 50 * 1024 * 1024 || !file.open(QIODevice::ReadOnly)) {
            return stage({}, {}, "Choose a Selah project under 50 MB.");
        }
        const auto root = QJsonDocument::fromJson(file.readAll()).object();
        if (root["format"].toInt() != 1) {
            return stage({}, {}, "Choose a valid Selah presentation file.");
        }
        return stage(EditorDocument::validatedSlides(root["slides"].toArray().toVariantList()),
            info.completeBaseName(), "The file does not contain valid presentation slides.");
    }
    const auto extension = info.suffix().toLower();
    const bool video = QStringList { "mp4", "mov", "webm", "mkv", "m4v" }.contains(extension);
    const bool audio
        = QStringList { "mp3", "wav", "m4a", "aac", "flac", "ogg", "aiff", "aif", "opus" }.contains(
            extension);
    if (!video && !audio
        && !QStringList { "png", "jpg", "jpeg", "webp", "gif", "bmp", "svg" }.contains(extension)) {
        return stage({}, {}, "Choose a supported image, video or audio file.");
    }
    QVariantMap asset { { "source", source.toString() }, { "name", info.completeBaseName() },
        { "kind",
            video       ? "video"
                : audio ? "audio"
                        : "image" } };
    if (!previewMedia(asset)) {
        return false;
    }
    m_media->registerAsset(m_slides.first().toMap()["elements"].toList().first().toMap());
    return true;
}

bool QuickAddController::add(bool now) {
    if (!m_outputs->addQuickSlides(m_slides, m_title, now)) {
        m_status = "Preview content first; load a program before using Insert next.";
        emit changed();
        return false;
    }
    m_status = now ? "Quick addition is live. Resume program returns to your previous slide."
                   : "Queued after the current service item. The live slide is unchanged.";
    emit changed();
    return true;
}

bool QuickAddController::saveDraft() {
    if (!m_draftPreview || m_slides.isEmpty()) {
        return false;
    }
    const auto id = m_songDraft
        ? m_content->createQuickSong(m_draft["title"].toString(), m_draft["text"].toString(),
              m_draft["tags"], m_draft.value("language", "English").toString())
        : m_presentations->save(m_title, m_slides);
    m_status = id.isEmpty() ? "Could not save this draft to the library." : "Saved to library.";
    if (!id.isEmpty()) {
        m_draftPreview = false;
    }
    emit changed();
    return !id.isEmpty();
}
