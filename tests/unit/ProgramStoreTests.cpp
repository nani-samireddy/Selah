#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include "SlideFactory.h"
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>

int ProgramStore::selfTest() {
    QTemporaryDir dir;
    OutputManager outputs;
    ProgramStore store(&outputs, nullptr, dir.path());
    EditorDocument editor(nullptr, false);
    auto check = [](bool ok, const char *name) {
        if (!ok) {
            qCritical() << "Program check failed:" << name;
        }
        return ok;
    };
    if (!check(store.createProgram("Sunday Worship", "2026-11-22"), "Create")) {
        return 1;
    }
    const auto id = store.id();
    QVariantMap song { { "title", "Original bilingual test" }, { "primaryLanguage", "English" },
        { "secondaryLanguage", "తెలుగు" },
        { "sections",
            QVariantList { QVariantMap { { "key", "v1" }, { "label", "Verse" },
                               { "primary", "We sing together\nWe share our hope" },
                               { "secondary", "కలిసి పాడుదము\nఆశను పంచుదము" } },
                QVariantMap { { "key", "c1" }, { "label", "Chorus" },
                    { "primary", "One grateful song" }, { "secondary", "ఒక కృతజ్ఞతా గీతం" } } } },
        { "arrangement", QVariantList { "v1", "c1" } },
        { "arrangements",
            QVariantList {
                QVariantMap { { "name", "Full" }, { "order", QVariantList { "v1", "c1" } } },
                QVariantMap { { "name", "Short" }, { "order", QVariantList { "c1" } } } } },
        { "activeArrangement", 0 } };
    if (!check(store.addPresentation(editor.slides(), "Welcome") && store.addSong(song, 3),
            "Add presentation and song")) {
        return 2;
    }
    store.setItemDetails("Worship song", "Invite everyone to stand.");
    store.setTemplate("paper");
    if (!check(store.selectedSlides()[0].toMap()["background"] == "#EEEAE0"
                && store.selectedSlides()[0].toMap()["ambientOpacity"].toDouble() == 0,
            "Template")) {
        return 3;
    }
    if (!check(store.setArrangement(1) && store.selectedSlides().size() == 1,
            "Alternative arrangement")) {
        return 4;
    }
    const auto songItem = store.selectedItem()["id"].toString();
    if (!check(
            store.present(0) && outputs.liveNotes() == "Invite everyone to stand." && store.live(),
            "Take live with notes")) {
        return 5;
    }
    const auto liveText = outputs.liveText();
    store.setItemDetails("Changed draft title", "Changed notes");
    store.moveItem(-1);
    store.removeSelected();
    if (!check(outputs.liveText() == liveText && outputs.liveNotes() == "Invite everyone to stand."
                && store.liveItemId() == songItem,
            "Live snapshot isolation on edit/reorder/remove")) {
        return 6;
    }
    store.undo();
    if (!check(store.items().size() == 2, "Undo remove")) {
        return 7;
    }
    store.redo();
    if (!check(store.items().size() == 1, "Redo remove")) {
        return 8;
    }
    store.undo();
    if (!check(store.save(), "Save")) {
        return 9;
    }
    {
        ProgramStore reopened(&outputs, nullptr, dir.path());
        if (!check(reopened.id() == id && reopened.items() == store.items(),
                "Reopen last program and snapshots")) {
            return 10;
        }
    }
    const auto exportUrl = QUrl::fromLocalFile(dir.filePath("service.churchprogram"));
    if (!check(store.exportProgram(exportUrl), "Export")) {
        return 11;
    }
    const auto beforeImport = store.items();
    if (!check(store.importProgram(exportUrl) && store.id() != id && store.items().size() == 2
                && store.items()[0].toMap()["id"] != beforeImport[0].toMap()["id"],
            "Import with independent identities")) {
        return 12;
    }
    if (!check(store.openProgram(id), "Open original")) {
        return 13;
    }
    store.setSelected(0);
    if (!check(store.present(0), "Start service")) {
        return 14;
    }
    const auto firstId = outputs.liveProgramItemId();
    outputs.advance(store.selectedSlides().size());
    if (!check(outputs.liveProgramItemId() != firstId, "Cross-item navigation")) {
        return 15;
    }
    outputs.takeLive(editor.slides(), 0);
    if (!check(!store.live(), "Editor output deactivates program tracking")) {
        return 16;
    }
    const auto oldItems = store.items();
    if (!check(!store.addPresentation({ QVariantMap { { "bad", true } } }, "Invalid")
                && store.items() == oldItems,
            "Reject invalid snapshot")) {
        return 17;
    }
    QFile bad(dir.filePath("invalid.churchprogram"));
    if (!bad.open(QIODevice::WriteOnly)) {
        return 18;
    }
    bad.write("{\"format\":\"churchdeck-program-1\",\"program\":{}}");
    bad.close();
    if (!check(
            !store.importProgram(QUrl::fromLocalFile(bad.fileName())) && store.items() == oldItems,
            "Reject invalid import without replacing service")) {
        return 18;
    }
    QImage image(32, 32, QImage::Format_RGB32);
    image.fill(Qt::blue);
    const auto path = dir.filePath("media.png");
    image.save(path);
    if (!check(store.addMedia({ { "name", "Offering background" },
                   { "source", QUrl::fromLocalFile(path).toString() }, { "kind", "image" } }),
            "Media item")) {
        return 19;
    }
    QFile::remove(path);
    if (!check(store.warnings().size() == 1, "Missing media warning")) {
        return 20;
    }
    if (!check(editor.replaceSlides(store.selectedSlides()) && editor.elements().size() == 1,
            "Open item in editor")) {
        return 21;
    }
    editor.addText();
    if (!check(store.updateSelectedSlides(editor.slides())
                && store.selectedSlides()[0].toMap()["elements"].toList().size() == 2,
            "Explicit editor update")) {
        return 22;
    }
    {
        QTemporaryDir reorderDirectory;
        OutputManager live;
        ProgramStore reorder(&live, nullptr, reorderDirectory.path());
        reorder.createProgram("Reorder fixture", "");
        reorder.addPresentation(editor.slides(), "First");
        reorder.addPresentation(editor.slides(), "Middle");
        reorder.addPresentation(editor.slides(), "Last");
        reorder.setSelected(1);
        const auto selection = reorder.selectedItem()["id"];
        const auto original = reorder.items();
        reorder.present(0);
        const auto snapshot = live.snapshotId(), title = live.liveTitle();
        if (!check(reorder.moveItemTo(0, 2) && reorder.items()[2].toMap()["title"] == "First"
                    && reorder.selectedItem()["id"] == selection && reorder.selected() == 0,
                "Drag move preserves selected identity")) {
            return 40;
        }
        if (!check(!reorder.moveItemTo(-1, 0) && !reorder.moveItemTo(0, 3)
                    && !reorder.moveItemTo(0, 0),
                "Reject invalid and unchanged drops")) {
            return 41;
        }
        reorder.undo();
        if (!check(reorder.items() == original && reorder.selectedItem()["id"] == selection
                    && reorder.selected() == 1,
                "One undo restores the entire drop")) {
            return 42;
        }
        reorder.redo();
        if (!check(reorder.items()[2].toMap()["title"] == "First"
                    && reorder.selectedItem()["id"] == selection && live.snapshotId() == snapshot
                    && live.liveTitle() == title,
                "Redo preserves selection and the live snapshot")) {
            return 43;
        }
    }
    {
        QTemporaryDir destinations;
        OutputManager live;
        ProgramStore target(&live, nullptr, destinations.path());
        if (!target.createProgram("Midweek", "2026-11-25")) {
            return 44;
        }
        const auto otherId = target.id();
        if (!target.createProgram("Current Sunday", "2026-11-22")
            || !target.addPresentation(editor.slides(), "Current slides") || !target.present()) {
            return 45;
        }
        target.setItemDetails("Unsaved current title", "Keep this draft");
        const auto current = target.m_program;
        const auto selection = target.m_selected;
        const auto history = target.m_undo;
        const auto snapshot = live.snapshotId();
        if (!check(target.addPresentation(editor.slides(), "Other slides", otherId)
                    && target.addSong(song, 3, {}, otherId)
                    && !target.addPresentation(editor.slides(), "Invalid", "missing-program")
                    && target.addPresentation(editor.slides(), "After rollback", otherId)
                    && target.m_program == current && target.m_selected == selection
                    && target.m_undo == history && target.dirty() && live.snapshotId() == snapshot,
                "Destination adds preserve the open dirty program, history, selection and live "
                "output")) {
            return 46;
        }
        ProgramStore reader(nullptr, nullptr, destinations.path());
        if (!check(reader.id() == target.id() && reader.openProgram(otherId)
                    && reader.items().size() == 3 && reader.items()[1].toMap()["kind"] == "song",
                "Destination adds persist without changing the last-opened program")) {
            return 47;
        }
        if (!check(target.addPresentation(editor.slides(), "Current add", target.id())
                    && target.items().size() == current["items"].toList().size() + 1,
                "Explicit current destination uses normal undoable editing")) {
            return 48;
        }
        target.undo();
        if (!check(target.m_program == current, "Current destination add remains undoable")) {
            return 49;
        }
    }
    qInfo() << "Program checks passed: mixed snapshots, named arrangements, styles, notes, "
               "save/reopen/import, undo/redo, missing media, editor updates, and live isolation "
               "across item navigation.";
    return 0;
}
