#include "ProgramChecks.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

void runProgramChecks(ProgramStore &programs, ContentLibrary &library, EditorDocument &editor,
    OutputManager &outputs, QQmlApplicationEngine &engine, const QString &folder, bool compact) {
    QDir().mkpath(folder);
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(120);
        return;
    }
    if (compact) {
        window->setWidth(1120);
        window->setHeight(760);
    }
    if (!programs.createProgram("Sunday Worship", "2026-11-22")
        || !programs.addPresentation(editor.slides(), "Welcome & announcements")) {
        qApp->exit(121);
        return;
    }
    QVariantMap song { { "title", "Together in hope / ఆశతో కలిసి" }, { "primaryLanguage", "English" },
        { "secondaryLanguage", "తెలుగు" }, { "credits", "Selah · Original test lyrics" },
        { "sections",
            QVariantList {
                QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                    { "primary",
                        "We gather in Your light\nWe lift our hearts in praise\nWe share a living "
                        "hope" },
                    { "secondary",
                        "నీ వెలుగులో కూడుదము\nస్తుతితో హృదయాలను పైకెత్తుదము\nజీవముగల ఆశను పంచుదము" } },
                QVariantMap { { "key", "c1" }, { "label", "Chorus" },
                    { "primary", "One heart and one song\nTogether we belong" },
                    { "secondary", "ఒకే హృదయం ఒకే గానం\nమనమంతా ఒకే కుటుంబం" } } } },
        { "arrangement", QVariantList { "v1", "c1", "c1" } },
        { "arrangements",
            QVariantList { QVariantMap { { "name", "Full service" },
                               { "order", QVariantList { "v1", "c1", "c1" } } },
                QVariantMap { { "name", "Chorus only" }, { "order", QVariantList { "c1" } } } } },
        { "activeArrangement", 0 } };
    if (!programs.addSong(song, 3) || !programs.setItemSection("Worship")) {
        qApp->exit(122);
        return;
    }
    programs.setItemDetails(song["title"].toString(),
        "Invite the congregation to stand.\nRepeat the chorus softly before the reading.");
    programs.setTemplate("midnight");
    if (!programs.addScripture(library.passage("tel2017", "JHN", 3, 16, 18), 4)
        || !programs.setItemSection("Sermon")) {
        qApp->exit(123);
        return;
    }
    QImage image(1280, 720, QImage::Format_RGB32);
    image.fill(QColor("#3F5960"));
    const auto imagePath = folder + "/offering-test.png";
    image.save(imagePath);
    if (!editor.addMedia(QUrl::fromLocalFile(imagePath))
        || !programs.addMedia(editor.selectedElement()) || !programs.setItemSection("Closing")) {
        qApp->exit(124);
        return;
    }
    programs.setItemDetails("Offering · quiet reflection", "Allow a minute for quiet prayer.");
    programs.moveItem(-1);
    programs.undo();
    programs.setSelected(1);
    programs.save();
    auto panel = engine.newQObject(window->findChild<QObject *>("programPanel"));
    panel.property("open").callWithInstance(panel);
    panel.property("showEditor").callWithInstance(panel);
    QTimer::singleShot(650, qApp, [&, window, folder, panel]() mutable {
        const auto sectionControl = window->findChild<QQuickItem *>("programItemSection");
        if (!sectionControl || !sectionControl->isVisible()) {
            qCritical() << "Service section control missing from program editor";
            qApp->exit(129);
            return;
        }
        if (!window->grabWindow().save(folder + "/program-prepare.png")) {
            qApp->exit(125);
            return;
        }
        if (!programs.present(0)) {
            qApp->exit(126);
            return;
        }
        outputs.preview("audience");
        outputs.preview("stage");
        QTimer::singleShot(350, qApp, [&, window, folder, panel]() mutable {
            window->grabWindow().save(folder + "/program-live.png");
            for (auto output : QGuiApplication::topLevelWindows()) {
                auto w = qobject_cast<QQuickWindow *>(output);
                if (!w) {
                    continue;
                }
                if (output->title().contains("Audience output")) {
                    w->grabWindow().save(folder + "/program-audience.png");
                }
                if (output->title().contains("Stage output")) {
                    w->grabWindow().save(folder + "/program-stage.png");
                }
            }
            const auto liveText = outputs.liveText();
            programs.setItemDetails("Draft title", "Draft notes");
            programs.moveItem(1);
            if (outputs.liveText() != liveText
                || !outputs.liveNotes().contains("Invite the congregation")) {
                qApp->exit(127);
                return;
            }
            // Navigate the existing live snapshot into the next service item.
            outputs.advance(programs.selectedSlides().size());
            if (outputs.liveProgramItemId() == programs.selectedItem()["id"].toString()) {
                qApp->exit(128);
                return;
            }
            outputs.stop("audience");
            outputs.stop("stage");
            // Verify all three native pickers can target the program without changing the editor.
            auto songs = engine.newQObject(window->findChild<QObject *>("songsPanel"));
            songs.setProperty("programTarget", engine.newQObject(&programs));
            songs.property("open").callWithInstance(songs);
            songs.property("load").callWithInstance(
                songs, { engine.toScriptValue(programs.selectedItem()["song"].toMap()) });
            QTimer::singleShot(200, qApp, [&, window, folder, songs, panel]() mutable {
                window->grabWindow().save(folder + "/program-song-picker.png");
                songs.property("close").callWithInstance(songs);
                panel.property("close").callWithInstance(panel);
                qInfo()
                    << "Program smoke passed: mixed service order, bilingual styles, notes, "
                       "audience/stage previews, draft isolation, and cross-item live navigation.";
                qApp->exit(0);
            });
        });
    });
}
