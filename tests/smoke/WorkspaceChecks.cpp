#include "WorkspaceChecks.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "PresentationLibrary.h"
#include "ProgramStore.h"
#include "TextFormatting.h"
#include "VisualLookup.h"
#include <functional>
#include <memory>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>

void runWorkspaceChecks(ContentLibrary &content, EditorDocument &document, OutputManager &outputs,
    ProgramStore &programs, QQmlApplicationEngine &engine, const QString &folder, bool stayOpen) {
    QDir().mkpath(folder);
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(210);
        return;
    }
    auto feedback = engine.singletonInstance<QObject *>("ChurchDeck", "InteractionFeedback");
    const bool originalSounds = feedback && feedback->property("soundEnabled").toBool();
    const bool originalMotion = feedback && feedback->property("reducedMotion").toBool();
    if (feedback) {
        QMetaObject::invokeMethod(feedback, "setSounds", Q_ARG(QVariant, QVariant(false)));
    }
    window->setTitle("Selah — UX verification");
    const QVariantMap song { { "title", "Together in Your light" }, { "primaryLanguage", "Telugu" },
        { "secondaryLanguage", "English" },
        { "sections",
            QVariantList {
                QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                    { "primary", "నీ కృప నన్ను నడిపించును\nప్రతి రోజూ నీతో సాగెదను" },
                    { "secondary", "Your grace will guide me\nEvery day I walk with You" } },
                QVariantMap { { "key", "c1" }, { "label", "Chorus" },
                    { "primary", "నీ వెలుగులో కూడుదము\nస్తుతితో హృదయాలను పైకెత్తుదము" },
                    { "secondary", "We gather in Your light\nWe lift our hearts in praise" } } } },
        { "arrangement", QVariantList { "v1", "c1" } },
        { "credits", "Original UX verification lyrics" } };
    const auto songId = content.saveSong(song);
    if (!programs.createProgram("Midweek UX destination", "2026-11-25")) {
        qApp->exit(211);
        return;
    }
    const auto alternateProgramId = programs.id();
    if (songId.isEmpty() || !programs.createProgram("Sunday Worship", "2026-11-22")
        || !programs.addPresentation(document.slides(), "Welcome & announcements")
        || !programs.addSong(content.song(songId), 3)
        || !programs.addScripture(content.passage("engwebp", "GEN", 1, 1, 2), 4)) {
        qWarning() << "Workspace fixture" << programs.status() << content.status();
        qApp->exit(211);
        return;
    }
    const auto mediaFixture = QDir(folder).filePath("Ambient background.svg");
    if (!QFile::exists(mediaFixture)) {
        QFile::copy(":/qt/qml/ChurchDeck/assets/ambient.svg", mediaFixture);
    }
    document.importMedia(QUrl::fromLocalFile(mediaFixture), false);
    programs.setSelected(1);
    outputs.takeLive(document.slides(), 0, "workspace-baseline");
    const auto baseline = outputs.snapshotId();
    auto switchTo = [window](const QString &name) {
        QMetaObject::invokeMethod(window, "switchWorkspace", Q_ARG(QVariant, QVariant(name)));
    };
    switchTo("programs");

    auto findVisual = VisualLookup::find;

    struct State {
        int phase = 0;
        QString snapshot, title, customId, quickId;
        QVariantList quickDraftBefore;
        bool passed = true;
        QVariantList checks;
    };

    auto state = std::make_shared<State>();
    auto timer = new QTimer(&engine);
    timer->setInterval(1000);
    QObject::connect(timer, &QTimer::timeout, &engine,
        [&, window, state, timer, switchTo, findVisual, folder, baseline, songId,
            alternateProgramId, stayOpen, feedback, originalSounds, originalMotion] {
            auto check = [&](bool condition, const QString &label) {
                state->checks.append(QVariantMap { { "check", label }, { "passed", condition } });
                state->passed &= condition;
                if (!condition) {
                    qWarning() << "Workspace check failed:" << label;
                }
            };
            auto capture = [&](const QString &name) {
                check(window->grabWindow().save(QDir(folder).filePath(name + ".png")),
                    "Capture " + name);
            };
            auto library = [window](const QString &section) {
                QMetaObject::invokeMethod(
                    window, "showLibrarySection", Q_ARG(QVariant, QVariant(section)));
            };
            if (state->phase == 0) {
                check(feedback && findVisual(window->contentItem(), "feedbackSettings"),
                    "Interface feedback preferences are available");
                auto panel = window->findChild<QObject *>("programPanel");
                check(panel && panel->property("opened").toBool()
                        && !panel->property("modal").toBool(),
                    "Programs is a persistent workspace");
                auto navigation = findVisual(window->contentItem(), "workspaceNav_programs");

                check(navigation && navigation->property("selected").toBool(),
                    "Navigation marks the current workspace");
                capture("programs");
                auto drag = findVisual(window->contentItem(), "programOrderDrag");
                if (drag) {
                    const auto before = programs.items();
                    const auto selected = programs.selectedItem()["id"];
                    QMetaObject::invokeMethod(drag, "arm", Q_ARG(QVariant, QVariant(0)),
                        Q_ARG(QVariant, QVariant(QPointF(12, 12))));
                    QMetaObject::invokeMethod(
                        drag, "update", Q_ARG(QVariant, QVariant(QPointF(12, drag->height() - 4))));
                    QMetaObject::invokeMethod(drag, "finish");
                    check(programs.items().last().toMap()["id"] == before.first().toMap()["id"]
                            && programs.selectedItem()["id"] == selected,
                        "Drag controller commits the intended insertion");
                    programs.undo();
                    check(programs.items() == before, "Drag controller makes one undoable move");
                    QMetaObject::invokeMethod(drag, "arm", Q_ARG(QVariant, QVariant(0)),
                        Q_ARG(QVariant, QVariant(QPointF(12, 12))));
                    QMetaObject::invokeMethod(
                        drag, "update", Q_ARG(QVariant, QVariant(QPointF(-20, 120))));
                    QMetaObject::invokeMethod(drag, "finish");
                    check(programs.items() == before, "Dropping outside the service list cancels");
                } else {
                    check(false, "Drag controller is available");
                }
                switchTo("libraries");
            } else if (state->phase == 1) {
                auto panel = window->findChild<QObject *>("songsPanel");
                check(panel && panel->property("opened").toBool()
                        && !panel->property("modal").toBool(),
                    "Library navigation stays outside modal pickers");
                auto primaryLanguage
                    = panel ? panel->findChild<QObject *>("songPrimaryLanguage") : nullptr;
                auto secondaryLanguage
                    = panel ? panel->findChild<QObject *>("songSecondaryLanguage") : nullptr;
                check(
                    primaryLanguage && secondaryLanguage, "Song language dropdowns are available");
                if (primaryLanguage && secondaryLanguage) {
                    const auto original = panel->property("draft");
                    primaryLanguage->setProperty("value", "Telugu");
                    secondaryLanguage->setProperty("value", "English");
                    QMetaObject::invokeMethod(secondaryLanguage, "chosen");
                    QMetaObject::invokeMethod(panel, "create");
                    check(primaryLanguage->property("value") == "Telugu"
                            && secondaryLanguage->property("value") == "English",
                        "New song inherits the most recently chosen languages");
                    secondaryLanguage->setProperty("value", "");
                    QMetaObject::invokeMethod(secondaryLanguage, "chosen");
                    QMetaObject::invokeMethod(panel, "create");
                    check(secondaryLanguage->property("value").toString().isEmpty(),
                        "Single-language preference remains optional");
                    QMetaObject::invokeMethod(panel, "load", Q_ARG(QVariant, original));
                }
                const auto songObject = engine.newQObject(panel);
                const int count = songObject.property("sections").property("length").toInt();
                bool allCards = count > 0;
                for (int i = 0; i < count; ++i) {
                    allCards
                        &= findVisual(window->contentItem(),
                               i == 0 ? "primaryLyrics" : "primaryLyrics_" + QString::number(i))
                        != nullptr;
                }
                check(allCards, "All song sections have visible card editors at once");
                if (auto tags = window->findChild<QObject *>("songTagEditor")) {
                    QMetaObject::invokeMethod(
                        panel, "load", Q_ARG(QVariant, QVariant(content.song(songId))));
                    const QString uniqueTag = "Sunday Worship " + songId.left(6);
                    QMetaObject::invokeMethod(
                        tags, "reset", Q_ARG(QVariant, QVariant("Worship, ఆదరణ")));
                    if (auto input = findVisual(window->contentItem(), "songTagInput")) {
                        input->setProperty("text", uniqueTag + ", worship");
                        QMetaObject::invokeMethod(tags, "commit");
                        QMetaObject::invokeMethod(panel, "save");
                        check(content.searchSongs("", uniqueTag).size() == 1
                                && content.parseSongTags(content.song(songId)["tags"]).size() == 3,
                            "Song tag chips save multi-word and multilingual labels without "
                            "duplicates");
                        if (auto chip = findVisual(window->contentItem(), "songTagChip_0")) {
                            QMetaObject::invokeMethod(chip, "clicked");
                            QMetaObject::invokeMethod(panel, "save");
                            check(!content.parseSongTags(content.song(songId)["tags"])
                                      .contains("Worship"),
                                "Removing a tag chip persists on Save song");
                        } else {
                            check(false, "Removable song tag chips are available");
                        }
                        panel->setProperty("selectedTag", uniqueTag);
                        QMetaObject::invokeMethod(panel, "refresh");
                        check(
                            engine.newQObject(panel).property("results").property("length").toInt()
                                == 1,
                            "Song library filters by exact tag");
                        panel->setProperty("selectedTag", "");
                        QMetaObject::invokeMethod(panel, "refresh");
                    } else {
                        check(false, "Song tag input is available");
                    }
                } else {
                    check(false, "Song tag editor is available");
                }
                if (auto destination
                    = findVisual(window->contentItem(), "songProgramDestination")) {
                    check(destination->property("targetId").toString() == programs.id(),
                        "Add-to-program button defaults to the currently open program");
                    const auto before = programs.items();
                    if (auto arrow
                        = findVisual(window->contentItem(), "songProgramDestination_menu")) {
                        QMetaObject::invokeMethod(arrow, "clicked");
                        auto menu
                            = destination->findChild<QObject *>("songProgramDestination_choices");
                        check(menu && menu->property("y").toDouble() < 0
                                && menu->property("focus").toBool(),
                            "Program menu opens above a footer button and accepts keyboard focus");
                        capture("program-destinations");
                    }
                    QMetaObject::invokeMethod(destination, "chooseProgram",
                        Q_ARG(QVariant, QVariant(alternateProgramId)));
                    check(destination->property("targetId").toString() == alternateProgramId
                            && programs.items() == before && outputs.snapshotId() == baseline
                            && panel->property("opened").toBool()
                            && panel->property("message").toString().startsWith("Added to "),
                        "Dropdown adds to another program and keeps the open workspace and live "
                        "snapshot");
                } else {
                    check(false, "Song program destination split button is available");
                }
                capture("songs");
                library("bible");
            } else if (state->phase == 2) {
                auto panel = window->findChild<QObject *>("biblePanel");
                auto bible = engine.newQObject(panel);
                check(findVisual(window->contentItem(), "bibleChapterGrid")
                        && findVisual(window->contentItem(), "bibleVerseGrid"),
                    "Bible sidebar has chapter and verse grids");
                panel->setProperty("chapterIndex", 2);
                if (auto books = findVisual(window->contentItem(), "bibleBookPicker")) {
                    books->setProperty("currentIndex", 1);
                    QMetaObject::invokeMethod(books, "activated", Q_ARG(int, 1));
                }
                check(bible.property("chapter").property("chapter").toInt() == 1,
                    "Choosing a book defaults to chapter one");
                QMetaObject::invokeMethod(panel, "jumpToVerse", Q_ARG(QVariant, QVariant(10)));
                auto list = findVisual(window->contentItem(), "scriptureVerseList");
                check(panel->property("highlightedVerse").toInt() == 10 && list
                        && list->property("currentIndex").toInt() == 9,
                    "Verse grid locates and highlights a verse in the full chapter");
                check(bible.property("verses").property("length").toInt() > 10,
                    "Verse navigation preserves the complete chapter");
                if (auto grid = findVisual(window->contentItem(), "bibleVerseGrid")) {
                    check(
                        grid->height() >= 64, "Verse grid remains usable at compact window sizes");
                }
                check(list && list->property("contentY").toDouble() > 0,
                    "Verse navigation actually scrolls the chapter");
                capture("bible");
                library("media");
            } else if (state->phase == 3) {
                capture("media");
                library("templates");
            } else if (state->phase == 4) {
                capture("templates");
                switchTo("design");
            } else if (state->phase == 5) {
                check(outputs.snapshotId() == baseline,
                    "Workspace browsing preserves the live snapshot");
                capture("design");
                if (auto drag = findVisual(window->contentItem(), "layerOrderDrag")) {
                    const auto before = document.elements();
                    const auto topId = before.last().toMap()["uid"];
                    QMetaObject::invokeMethod(drag, "arm", Q_ARG(QVariant, QVariant(0)),
                        Q_ARG(QVariant, QVariant(QPointF(12, 12))));
                    QMetaObject::invokeMethod(
                        drag, "update", Q_ARG(QVariant, QVariant(QPointF(12, drag->height() - 4))));
                    QMetaObject::invokeMethod(drag, "finish");
                    check(document.elements().first().toMap()["uid"] == topId
                            && document.selectedElement()["uid"] == topId,
                        "Layer drag maps the frontmost row to the correct stacking position");
                    document.undo();
                    check(
                        document.elements() == before, "Layer drag is one undoable document edit");
                    QMetaObject::invokeMethod(drag, "arm", Q_ARG(QVariant, QVariant(0)),
                        Q_ARG(QVariant, QVariant(QPointF(12, 12))));
                    QMetaObject::invokeMethod(
                        drag, "update", Q_ARG(QVariant, QVariant(QPointF(-30, 90))));
                    QMetaObject::invokeMethod(drag, "finish");
                    check(document.elements() == before, "Layer drop outside the list cancels");
                    check(outputs.snapshotId() == baseline, "Layer editing preserves live output");
                } else {
                    check(false, "Layer drag controller is available");
                }
                check(programs.present(0), "Explicit presentation starts the prepared program");
                state->snapshot = outputs.snapshotId();
                state->title = outputs.liveTitle();
                switchTo("present");
            } else if (state->phase == 6) {
                check(feedback && feedback->property("presentationMode").toBool(),
                    "Present workspace silences interface cues");
                if (feedback) {
                    const auto previous = feedback->property("lastPlayed");
                    QMetaObject::invokeMethod(
                        feedback, "setSounds", Q_ARG(QVariant, QVariant(true)));
                    QMetaObject::invokeMethod(feedback, "play", Q_ARG(QVariant, QVariant("click")));
                    check(feedback->property("lastPlayed") == previous,
                        "Presentation mode blocks sound playback even when sounds are enabled");
                    QMetaObject::invokeMethod(
                        feedback, "setSounds", Q_ARG(QVariant, QVariant(false)));
                }
                auto order = window->findChild<QObject *>("liveServiceOrder");
                check(order && order->property("count").toInt() == outputs.liveCount(),
                    "Present lists the immutable live service");
                capture("present");
                switchTo("programs");
                programs.setItemDetails(
                    "Revised preparation draft", "These changes must stay off live output");
            } else if (state->phase == 7) {
                check(
                    outputs.snapshotId() == state->snapshot && outputs.liveTitle() == state->title,
                    "Preparing another draft leaves the running service intact");
                switchTo("present");
                outputs.advance(1);
            } else if (state->phase == 8) {
                auto order = window->findChild<QObject *>("liveServiceOrder");
                check(order && order->property("currentIndex").toInt() == outputs.liveIndex(),
                    "Present follows desktop and remote slide navigation");
                switchTo("design");
                document.setSelected(0);
                auto picker = window->findChild<QObject *>("fontPicker");
                check(picker != nullptr, "Font picker is available in Design");
                if (picker) {
                    picker->setProperty("currentFamily", "Noto Sans Telugu");
                    QMetaObject::invokeMethod(picker, "open");
                }
            } else if (state->phase == 9) {
                capture("fonts");
                if (auto picker = window->findChild<QObject *>("fontPicker")) {
                    QMetaObject::invokeMethod(picker, "close");
                }
                if (auto appearance = findVisual(window->contentItem(), "appearancePicker")) {
                    QMetaObject::invokeMethod(
                        appearance, "setMode", Q_ARG(QVariant, QVariant("light")));
                    check(appearance->property("currentIcon") == "moon",
                        "Appearance button changes to moon in light mode");
                }
                switchTo("libraries");
                library("songs");
            } else if (state->phase == 10) {
                capture("songs-light");
                library("bible");
            } else if (state->phase == 11) {
                capture("bible-light");
                switchTo("design");
                document.setSelected(1);
                auto editor = findVisual(window->contentItem(), "richTextEditor");
                check(editor != nullptr, "Rich text editing is available");
                if (editor) {
                    QMetaObject::invokeMethod(editor, "begin", Q_ARG(QVariant, QVariant(1)));
                    QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 0), Q_ARG(int, 7));
                    QMetaObject::invokeMethod(editor, "applyFormat",
                        Q_ARG(QVariant,
                            QVariant(
                                QVariantMap { { "fontWeight", 700 }, { "color", "#CC7744" } })));
                    TextFormatting formatter;
                    const auto styled = formatter.styleAt(document.selectedElement(), 0, 7);
                    check(styled["fontWeight"].toInt() == 700
                            && editor->property("sessionOpen").toBool(),
                        "Selected-word formatting commits without losing the editor");
                    QMetaObject::invokeMethod(editor, "finish");
                }
            } else if (state->phase == 12) {
                capture("design-light");
                check(outputs.snapshotId() == state->snapshot,
                    "Appearance and text editing leave live output untouched");
                if (auto appearance = findVisual(window->contentItem(), "appearancePicker")) {
                    QMetaObject::invokeMethod(
                        appearance, "setMode", Q_ARG(QVariant, QVariant("dark")));
                    check(appearance->property("currentIcon") == "sun",
                        "Appearance button changes to sun in dark mode");
                }
            } else if (state->phase == 13) {
                auto saved = qobject_cast<PresentationLibrary *>(
                    window->property("presentations").value<QObject *>());
                check(saved != nullptr, "Custom presentation library is available");
                if (saved) {
                    state->customId = saved->save("Custom UX presentation", document.slides());
                    check(!state->customId.isEmpty(),
                        "Custom presentation persists from editor slides");
                    QMetaObject::invokeMethod(window, "editCustomPresentation",
                        Q_ARG(QVariant, QVariant(saved->presentation(state->customId))));
                    check(!window->property("templateMode").toBool()
                            && window->property("editingPresentationId").toString()
                                == state->customId,
                        "Opening custom slides selects a single presentation editing context");
                    switchTo("libraries");
                    library("custom");
                }
            } else if (state->phase == 14) {
                auto panel = window->findChild<QObject *>("customSlidesPanel");
                check(panel && panel->property("opened").toBool()
                        && findVisual(window->contentItem(), "customPresentationsGrid"),
                    "Custom slides has its own library workspace");
                capture("custom-slides");
                auto saved = qobject_cast<PresentationLibrary *>(
                    window->property("presentations").value<QObject *>());
                if (saved && panel) {
                    const auto entry = saved->presentation(state->customId);
                    const auto beforeCount = programs.items().size();
                    QMetaObject::invokeMethod(
                        panel, "addToProgram", Q_ARG(QVariant, QVariant(entry)));
                    check(programs.items().size() == beforeCount + 1,
                        "Custom library adds a presentation to the program");
                    const auto programCopy = programs.items().last().toMap();
                    auto changed = entry["slides"].toList();
                    auto slide = changed.first().toMap();
                    slide["name"] = "Library source revised";
                    changed[0] = slide;
                    saved->save(entry["title"].toString(), changed, state->customId);
                    check(programs.items().last().toMap() == programCopy
                            && outputs.snapshotId() == state->snapshot,
                        "Editing a saved source preserves program copies and live output");
                    auto templates = window->property("templates").value<QObject *>();
                    const auto definitions
                        = templates ? templates->property("layouts").toList() : QVariantList();
                    if (!definitions.isEmpty()) {
                        QMetaObject::invokeMethod(
                            window, "editTemplate", Q_ARG(QVariant, definitions.first()));
                        check(window->property("templateMode").toBool()
                                && window->property("editingPresentationId").toString().isEmpty(),
                            "Opening a template replaces custom editing context");
                    }
                    QMetaObject::invokeMethod(window, "editCustomPresentation",
                        Q_ARG(QVariant, QVariant(saved->presentation(state->customId))));
                    check(!window->property("templateMode").toBool(),
                        "Returning to custom slides removes template editing controls");
                }
            } else if (state->phase == 15) {
                if (auto panel = window->findChild<QObject *>("customSlidesPanel")) {
                    QMetaObject::invokeMethod(panel, "createRequested");
                }
            } else if (state->phase == 16) {
                auto dialog = window->findChild<QObject *>("savePresentationDialog");
                auto name = window->findChild<QObject *>("customPresentationName");
                check(dialog && name && dialog->property("opened").toBool(),
                    "New custom presentation opens its naming dialog");
                if (dialog && name) {
                    name->setProperty("text", "New UX custom presentation");
                    QMetaObject::invokeMethod(dialog, "save", Q_ARG(QVariant, QVariant(false)));
                    state->customId = window->property("editingPresentationId").toString();
                    check(!state->customId.isEmpty() && document.slides().size() == 1
                            && document.elements().isEmpty()
                            && !window->property("templateMode").toBool(),
                        "Create opens one blank custom presentation in Design");
                    document.addText();
                    QMetaObject::invokeMethod(window, "saveCustomPresentation");
                }
            } else if (state->phase == 17) {
                auto dialog = window->findChild<QObject *>("savePresentationDialog");
                auto saved = qobject_cast<PresentationLibrary *>(
                    window->property("presentations").value<QObject *>());
                if (dialog && saved) {
                    QMetaObject::invokeMethod(dialog, "save", Q_ARG(QVariant, QVariant(false)));
                    check(saved->presentation(state->customId)["slides"]
                                    .toList()
                                    .first()
                                    .toMap()["elements"]
                                    .toList()
                                    .size()
                                == 1
                            && outputs.snapshotId() == state->snapshot,
                        "Save presentation updates the current library entry without changing live "
                        "output");
                }
            } else if (state->phase == 18) {
                state->quickDraftBefore = document.slides();
                switchTo("libraries");
                library("songs");
                if (auto action = findVisual(window->contentItem(), "quickSongAction")) {
                    QMetaObject::invokeMethod(action, "clicked");
                }
            } else if (state->phase == 19) {
                auto dialog = window->findChild<QObject *>("quickSongDialog");
                auto name = window->findChild<QObject *>("quickSongName");
                auto lyrics = findVisual(window->contentItem(), "quickSongLyrics");
                auto tags = findVisual(window->contentItem(), "quickSongTagInput");
                check(dialog && dialog->property("opened").toBool() && name && lyrics && tags,
                    "Quick Song opens a modal with name, lyrics and tags");
                if (dialog && name && lyrics && tags) {
                    name->setProperty("text", "Quick UX song " + songId.left(6));
                    lyrics->setProperty(
                        "text", "పంక్తి ఒకటి\nపంక్తి రెండు\n\nChorus line one\nChorus line two");
                    tags->setProperty("text", "Sunday, Telugu");
                    capture("quick-song");
                    QMetaObject::invokeMethod(dialog, "saveSong");
                    auto panel = window->findChild<QObject *>("songsPanel");
                    const auto draft
                        = engine.newQObject(panel).property("draft").toVariant().toMap();
                    state->quickId = draft["id"].toString();
                    const auto saved = content.song(state->quickId);
                    check(saved["sections"].toList().size() == 2
                            && saved["sections"].toList().first().toMap()["primary"]
                                == "పంక్తి ఒకటి\nపంక్తి రెండు"
                            && content.parseSongTags(saved["tags"]).size() == 2,
                        "Quick Song saves blank-line sections, preserves lyric lines and commits "
                        "pending tags");
                    check(document.slides() == state->quickDraftBefore
                            && outputs.snapshotId() == state->snapshot,
                        "Quick Song leaves the design draft and live output untouched");
                }
            } else if (state->phase == 20) {
                capture("quick-song-saved");
                if (auto action = findVisual(window->contentItem(), "quickSongAction")) {
                    QMetaObject::invokeMethod(action, "clicked");
                }
            } else if (state->phase == 21) {
                auto dialog = window->findChild<QObject *>("quickSongDialog");
                auto name = window->findChild<QObject *>("quickSongName");
                if (dialog && name) {
                    const auto count = content.songs().size();
                    name->setProperty("text", "Cancelled Quick Song");
                    QMetaObject::invokeMethod(dialog, "reject");
                    check(content.songs().size() == count && !state->quickId.isEmpty(),
                        "Cancelling Quick Song creates no library entry");
                }
                switchTo("design");
            } else {
                capture("design-dark");
                if (auto picker = window->findChild<QObject *>("fontPicker")) {
                    QMetaObject::invokeMethod(picker, "close");
                }
                if (feedback) {
                    check(feedback->property("soundReady").toBool(),
                        "Interface feedback sound is preloaded");
                    QMetaObject::invokeMethod(
                        feedback, "setReducedMotion", Q_ARG(QVariant, QVariant(true)));
                    check(feedback->property("duration").toInt() == 0,
                        "Reduced motion removes transition duration");
                    QMetaObject::invokeMethod(
                        feedback, "setReducedMotion", Q_ARG(QVariant, QVariant(originalMotion)));
                    QMetaObject::invokeMethod(
                        feedback, "setSounds", Q_ARG(QVariant, QVariant(originalSounds)));
                }
                QFile report(QDir(folder).filePath("report.json"));
                if (report.open(QIODevice::WriteOnly)) {
                    report.write(QJsonDocument::fromVariant(
                        QVariantMap { { "passed", state->passed }, { "checks", state->checks } })
                            .toJson());
                }
                timer->stop();
                qInfo() << "Workspace checks" << (state->passed ? "passed" : "failed");
                if (stayOpen && state->passed) {
                    switchTo("programs");
                } else {
                    qApp->exit(state->passed ? 0 : 212);
                }
            }
            ++state->phase;
        });
    timer->start();
}
