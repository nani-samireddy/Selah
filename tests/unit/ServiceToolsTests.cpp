#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "LiveSessionStore.h"
#include "OutputManager.h"
#include "PortableBundle.h"
#include "ProgramStore.h"
#include "ServiceCues.h"
#include "SlideFactory.h"
#include "SongChords.h"
#include "TemplateDefinition.h"
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QThread>

int runServiceToolsChecks() {
    auto check = [](bool ok, const char *name) {
        if (!ok) {
            qCritical() << "Service tools:" << name;
        }
        return ok;
    };
    QString error;
    const auto song = SongChords::importChordPro(
        "{title: Test Song}\n{key: C}\n{start_of_chorus: Chorus}\n{comment: Repeat "
        "twice}\n[C]Amazing [G]grace\n[Am]How sweet the [F]sound\n{end_of_chorus}\n",
        "Untitled", &error);
    if (!check(error.isEmpty() && song["title"] == "Test Song" && song["songKey"] == "C",
            "ChordPro metadata")) {
        return 1;
    }
    auto repeated = song;
    repeated["arrangement"] = QVariantList { "s1", "s1" };
    const auto slides = SlideFactory::songSlides(repeated, 1, &error);
    if (!check(error.isEmpty() && slides.size() == 4, "Repeated chorus pagination")) {
        return 2;
    }
    for (const auto &value : slides) {
        const auto slide = value.toMap();
        if (!check(slide["stageChords"].toList().size() == 2 && slide["stageCue"] == "Repeat twice",
                "Chords and cues follow each split")) {
            return 3;
        }
        for (const auto &element : slide["elements"].toList()) {
            if (!check(!element.toMap()["text"].toString().contains("[C]"),
                    "Audience has no chord tokens")) {
                return 4;
            }
        }
    }
    const auto telugu = SongChords::parse("[D]నీ [A/C#]వెలుగులో [Chorus]");
    if (!check(telugu["text"] == "నీ వెలుగులో [Chorus]"
                && telugu["chords"].toList()[1].toMap()["offset"].toInt() == 3,
            "Unicode anchors and literal brackets")) {
        return 5;
    }
    auto definition = TemplateDefinition::starter("bilingual");
    auto paired = song;
    auto section = song["sections"].toList()[0].toMap();
    section["secondary"] = "నీ వెలుగులో\nస్తుతి పాడెదము";
    paired["sections"] = QVariantList { section };
    const auto templateSlides = SlideFactory::songSlides(paired, 1, &error, definition);
    int chordCount = 0;
    for (const auto &slide : templateSlides) {
        chordCount += slide.toMap()["stageChords"].toList().size();
    }
    if (!check(error.isEmpty() && chordCount == 4, "Template and bilingual chord mapping")) {
        return 6;
    }
    QTemporaryDir dir;
    { // Simulate an unclean process exit: checkpoint persisted, finishCleanly never called.
        OutputManager outputs;
        LiveSessionStore recovery(&outputs, dir.path());
        outputs.takeLive(slides, 1, "program:test");
        if (!outputs.addQuickSlides(templateSlides, "Spontaneous", true, "GEN:1:1")) {
            return 7;
        }
        outputs.setStageMessage("Repeat");
        outputs.startCountdown(12);
    }
    OutputManager restored;
    LiveSessionStore recovery(&restored, dir.path());
    if (!check(recovery.available() && restored.liveCount() == 0,
            "Recovery requires explicit confirmation")) {
        return 8;
    }
    if (!check(recovery.recover() && restored.liveCount() == slides.size() + templateSlides.size()
                && restored.canResumeProgram() && !restored.audienceEnabled()
                && !restored.stageEnabled() && !restored.playing() && !restored.audioEnabled()
                && restored.stageMessage() == "Repeat" && restored.countdown() == "12:00",
            "Recovery restores deck/history/resume point, outputs stay off")) {
        return 9;
    }
    if (!check(restored.resumeProgram() && restored.liveIndex() == 1,
            "Original program resume point")) {
        return 10;
    }
    auto invalid = restored.sessionSnapshot();
    invalid["index"] = 999999;
    if (!check(!restored.restoreSession(invalid) && restored.liveIndex() == 1,
            "Invalid recovery leaves session intact")) {
        return 11;
    }
    recovery.finishCleanly();
    LiveSessionStore clean(&restored, dir.path());
    if (!check(!clean.available(), "Clean shutdown clears recovery")) {
        return 12;
    }
    ServiceCues cues(&restored, dir.path());
    auto cue = QVariantMap { { "name", "Worship" }, { "stage", "musician" },
        { "audience", "lyrics" }, { "audio", "off" }, { "media", "keep" }, { "timer", "pause" },
        { "minutes", 20 }, { "message", "" } };
    cue["name"] = "Test cue";
    cue["timer"] = "start";
    cue["minutes"] = 7;
    cue["message"] = "Instrumental";
    if (!check(cues.saveCue(-1, cue) && cues.trigger(3) && restored.musicianView()
                && restored.countdown() == "07:00" && restored.stageMessage() == "Instrumental"
                && !restored.audienceEnabled(),
            "Cue applies grouped settings without starting output")) {
        return 13;
    }
    ServiceCues loaded(&restored, dir.path());
    if (!check(loaded.cues().size() == 4 && loaded.cues()[3].toMap()["name"] == "Test cue",
            "Cue persistence")) {
        return 14;
    }
    restored.removeDefaultImage();
    if (!check(!cues.trigger(2) && restored.stageMessage() == "Instrumental",
            "Cue preflight prevents partial action")) {
        return 15;
    }
    QTemporaryDir sourceDir, destDir;
    ProgramStore program(&restored, nullptr, sourceDir.path());
    QImage image(32, 32, QImage::Format_RGB32);
    image.fill(Qt::blue);
    const auto media = sourceDir.filePath("blue.png");
    if (!image.save(media) || !program.createProgram("Portable", "2026-11-25")
        || !program.addMedia({ { "kind", "image" },
            { "source", QUrl::fromLocalFile(media).toString() }, { "name", "Blue" } })
        || !program.addSong(paired, 2, definition) || !program.save()) {
        return 16;
    }
    const auto bundlePath = sourceDir.filePath("test.selahbundle");
    auto wait = [&] {
        QElapsedTimer elapsed;
        elapsed.start();
        while (program.bundleBusy() && elapsed.elapsed() < 10000) {
            QCoreApplication::processEvents();
            QThread::msleep(5);
        }
        QCoreApplication::processEvents();
        return !program.bundleBusy();
    };
    if (!check(program.exportBundle(QUrl::fromLocalFile(bundlePath)) && wait()
                && QFile::exists(bundlePath),
            "Async portable export")) {
        return 17;
    }
    QFile bundle(bundlePath);
    if (!bundle.open(QIODevice::ReadOnly)) {
        return 18;
    }
    const auto bytes = bundle.readAll();
    if (!check(!bytes.contains(QUrl::fromLocalFile(media).toString().toUtf8()),
            "Bundle has no original media path")) {
        return 19;
    }
    QFile::remove(media);
    auto progress = std::make_shared<BundleProgress>();
    auto imported = PortableBundle::read(bundlePath, destDir.path(), progress);
    if (!check(imported.error.isEmpty(), qPrintable(imported.error))) {
        return 20;
    }
    const auto items = imported.program["items"].toList();
    const auto newSource = items[0]
                               .toMap()["slides"]
                               .toList()[0]
                               .toMap()["elements"]
                               .toList()[0]
                               .toMap()["source"]
                               .toString();
    if (!check(QFile::exists(QUrl(newSource).toLocalFile())
                && QImage(QUrl(newSource).toLocalFile()).pixelColor(0, 0) == Qt::blue
                && !items[1].toMap()["layoutTemplate"].toMap().isEmpty(),
            "Offline media and template snapshot")) {
        return 21;
    }
    ProgramStore destination(&restored, nullptr, destDir.path());
    if (!check(
            destination.importBundle(QUrl::fromLocalFile(bundlePath)), "Portable import starts")) {
        return 22;
    }
    QElapsedTimer elapsed;
    elapsed.start();
    while (destination.bundleBusy() && elapsed.elapsed() < 10000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    QCoreApplication::processEvents();
    if (!check(destination.title() == "Portable" && destination.items().size() == 2
                && destination.id() != program.id(),
            "Portable import commits independent service")) {
        return 23;
    }
    const auto damagedPath = sourceDir.filePath("damaged.selahbundle");
    QFile damaged(damagedPath);
    if (!damaged.open(QIODevice::WriteOnly)) {
        return 24;
    }
    auto corrupt = bytes;
    corrupt[corrupt.size() - 1] = char(corrupt.back() ^ 0xFF);
    damaged.write(corrupt);
    damaged.close();
    const auto bad
        = PortableBundle::read(damagedPath, destDir.path(), std::make_shared<BundleProgress>());
    if (!check(!bad.error.isEmpty() && bad.program.isEmpty(), "Corrupt bundle rejected")) {
        return 25;
    }
    auto cancelled = std::make_shared<BundleProgress>();
    cancelled->cancelled = true;
    if (!check(!PortableBundle::read(bundlePath, destDir.path(), cancelled).error.isEmpty(),
            "Cancelled import leaves no program")) {
        return 26;
    }
    qInfo() << "Service tools checks passed: chords, Unicode, bilingual pagination, confirmed "
               "recovery, cues and portable bundles.";
    return 0;
}
