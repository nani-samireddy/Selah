#include "EditorDocument.h"
#include "LiveAudio.h"
#include "MediaLibrary.h"
#include "OutputManager.h"
#include "PortableBundle.h"
#include "ProgramStore.h"
#include "QuickAddController.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QMediaPlayer>
#include <QPointer>
#include <QTemporaryDir>
#include <QThread>

int runAudioLibraryChecks() {
    auto check = [](bool ok, const char *message) {
        if (!ok) {
            qCritical() << "Audio library:" << message;
        }
        return ok;
    };
    auto spin = [](const std::function<bool()> &done, int timeout = 3000) {
        QElapsedTimer clock;
        clock.start();
        while (!done() && clock.elapsed() < timeout) {
            QCoreApplication::processEvents();
            QThread::msleep(5);
        }
        QCoreApplication::processEvents();
        return done();
    };
    QTemporaryDir directory;
    const auto path = directory.filePath("Test audio.wav");
    QFile wave(path);
    if (!wave.open(QIODevice::WriteOnly)) {
        return 1;
    }
    QDataStream bytes(&wave);
    bytes.setByteOrder(QDataStream::LittleEndian);
    constexpr quint32 length = 8000 * 2 * 3;
    bytes.writeRawData("RIFF", 4);
    bytes << quint32(36 + length);
    bytes.writeRawData("WAVEfmt ", 8);
    bytes << quint32(16) << quint16(1) << quint16(1) << quint32(8000) << quint32(16000)
          << quint16(2) << quint16(16);
    bytes.writeRawData("data", 4);
    bytes << length;
    for (quint32 i = 0; i < length / 2; ++i) {
        bytes << qint16(0);
    }
    wave.close();
    EditorDocument document(nullptr, false);
    if (!check(document.addMedia(QUrl::fromLocalFile(path)), "Audio import")) {
        return 2;
    }
    const auto element = document.selectedElement();
    if (!check(element["kind"] == "audio" && !element["loop"].toBool() && !element["muted"].toBool()
                && QFile::exists(QUrl(element["source"].toString()).toLocalFile()),
            "Managed copy and single-play defaults")) {
        return 3;
    }
    if (!check(!EditorDocument::validatedSlides(document.slides()).isEmpty(),
            "Audio survives document validation")) {
        return 4;
    }
    MediaLibrary library;
    library.registerAsset(element);
    library.registerAsset(element);
    int count = 0;
    for (const auto &row : library.assets()) {
        if (row.toMap()["source"] == element["source"] && row.toMap()["kind"] == "audio") {
            ++count;
        }
    }
    if (!check(count == 1, "Audio catalog persistence and deduplication")) {
        return 5;
    }
    OutputManager outputs;
    ProgramStore program(&outputs, nullptr, directory.path());
    if (!program.createProgram("Audio program", "2026-11-25") || !program.addMedia(element)
        || !program.save()) {
        return 6;
    }
    {
        ProgramStore reopened(&outputs, nullptr, directory.path());
        if (!check(reopened.items()[0]
                        .toMap()["slides"]
                        .toList()[0]
                        .toMap()["elements"]
                        .toList()[0]
                        .toMap()["kind"]
                    == "audio",
                "Saved audio program reopens")) {
            return 16;
        }
    }
    QuickAddController quick(nullptr, nullptr, &library, &outputs, nullptr, false);
    if (!check(quick.previewMedia(element)
                && !EditorDocument::validatedSlides(quick.slides()).isEmpty(),
            "Quick Add produces recoverable audio slides")) {
        return 17;
    }
    outputs.takeLive(program.selectedSlides(), 0, "audio-test");
    auto players = outputs.findChildren<QMediaPlayer *>("liveAudioPlayer");
    if (!check(players.size() == 1, "Exactly one live audio player")) {
        return 7;
    }
    QPointer<QMediaPlayer> player = players[0];
    if (!check(spin([&] { return player && player->position() > 80; }),
            "Audio decoder starts even without a display window")) {
        return 8;
    }
    outputs.setPlaying(false);
    const auto pausedAt = player->position();
    spin([] { return false; }, 100);
    if (!check(player->playbackState() != QMediaPlayer::PlayingState
                && std::abs(player->position() - pausedAt) <= 100,
            "Pause retains playback position")) {
        return 9;
    }
    outputs.setPlaying(true);
    if (!check(spin([&] { return player->position() > pausedAt; }), "Resume playback")) {
        return 10;
    }
    outputs.setAudioEnabled(false);
    outputs.setStageMessage("Still one player");
    if (!check(outputs.findChildren<QMediaPlayer *>("liveAudioPlayer").size() == 1
                && player == players[0],
            "Unrelated updates do not restart or duplicate audio")) {
        return 11;
    }
    const auto snapshot = outputs.sessionSnapshot();
    OutputManager restored;
    if (!check(restored.restoreSession(snapshot) && !restored.playing() && !restored.audioEnabled(),
            "Recovery never auto-plays audio")) {
        return 12;
    }
    const auto bundle = directory.filePath("audio.selahbundle");
    if (!program.exportBundle(QUrl::fromLocalFile(bundle))
        || !spin([&] { return !program.bundleBusy(); })) {
        return 13;
    }
    const auto imported = PortableBundle::read(
        bundle, directory.filePath("offline"), std::make_shared<BundleProgress>());
    if (!check(imported.error.isEmpty()
                && imported.program["items"]
                        .toList()[0]
                        .toMap()["slides"]
                        .toList()[0]
                        .toMap()["elements"]
                        .toList()[0]
                        .toMap()["kind"]
                    == "audio",
            "Portable bundles include audio")) {
        return 14;
    }
    outputs.stopPresentation();
    if (!check(player && player->playbackState() != QMediaPlayer::PlayingState,
            "Stop presenting pauses audio")) {
        return 15;
    }
    qInfo() << "Audio library checks passed: managed import, catalog, document/program "
               "persistence, one decoder, pause/resume, recovery and portable bundle.";
    return 0;
}
