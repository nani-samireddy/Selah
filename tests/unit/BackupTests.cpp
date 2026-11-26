#include "AppBackup.h"
#include "ContentLibrary.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

int runBackupChecks() {
    auto check = [](bool ok, const char *label) {
        if (!ok) {
            qCritical() << "Full backup:" << label;
        }
        return ok;
    };
    QTemporaryDir source, external, destination;
    const auto root = source.path();
    const auto newRoot = destination.filePath("Restored Selah తెలుగు");
    QDir().mkpath(newRoot);
    QFile prior(newRoot + "/prior.txt");
    if (!prior.open(QIODevice::WriteOnly)) {
        return 1;
    }
    prior.write("Keep me");
    prior.close();
    QDir().mkpath(root + "/fonts");
    QFile font(root + "/fonts/test.bin");
    if (!font.open(QIODevice::WriteOnly)) {
        return 2;
    }
    font.write("Font fixture");
    font.close();
    QImage image(16, 16, QImage::Format_RGB32);
    image.fill(Qt::green);
    const auto media = external.filePath("external image.png");
    if (!image.save(media)) {
        return 3;
    }
    const auto url = QUrl::fromLocalFile(media).toString();
    QFile json(root + "/project.json");
    if (!json.open(QIODevice::WriteOnly)) {
        return 4;
    }
    json.write(
        QJsonDocument(QJsonObject { { "font", root + "/fonts/test.bin" }, { "source", url } })
            .toJson());
    json.close();
    OutputManager outputs;
    QFile fixtureDatabase(root + "/content-library.sqlite");
    if (!fixtureDatabase.open(QIODevice::WriteOnly)) {
        return 30;
    }
    fixtureDatabase.close();
    ContentLibrary content(nullptr, root);
    const auto song
        = content.createQuickSong("Backup song", "First line\n\nSecond line", "backup", "Telugu");
    ProgramStore program(&outputs, nullptr, root);
    if (!check(!song.isEmpty() && program.createProgram("Backup service", "2026-11-25")
                && program.addSong(content.song(song))
                && program.addMedia(
                    { { "kind", "image" }, { "source", url }, { "name", "External" } })
                && program.save(),
            "Source profile with open SQLite connections")) {
        return 5;
    }
    const auto path = destination.filePath("all.selahbackup");
    const QVariantMap preferences { { "theme", "light" }, { "bytes", QByteArray("\0\1\2", 3) },
        { "enabled", true }, { "fontRoot", root + "/fonts" } };
    const auto result
        = AppBackup::write(root, preferences, path, std::make_shared<BundleProgress>());
    if (!check(result.error.isEmpty(), qPrintable(result.error))) {
        return 6;
    }
    QFile::remove(media);
    const auto ready = AppBackup::prepare(path, newRoot, std::make_shared<BundleProgress>());
    if (!check(ready.error.isEmpty() && QFile::exists(newRoot + "/prior.txt")
                && QFile::exists(newRoot + ".restore-request"),
            "Restore staged without changing current profile")) {
        qCritical() << ready.error;
        return 7;
    }
    QFile saved(ready.directory + "/restore-preferences.bin");
    if (!saved.open(QIODevice::ReadOnly)) {
        return 8;
    }
    QDataStream stream(&saved);
    stream.setVersion(QDataStream::Qt_6_0);
    QVariantMap restoredPreferences;
    stream >> restoredPreferences;
    saved.close();
    if (!check(restoredPreferences["fontRoot"] == newRoot + "/fonts"
                && restoredPreferences["bytes"] == preferences["bytes"]
                && restoredPreferences["enabled"].toBool(),
            "Typed preferences and relocated roots")) {
        return 9;
    }
    if (!check(AppBackup::applyPendingRestore(newRoot, false).isEmpty()
                && !QFile::exists(newRoot + "/prior.txt")
                && QFile::exists(newRoot + "/fonts/test.bin"),
            "Apply only while databases are closed")) {
        return 10;
    }
    ContentLibrary restored(nullptr, newRoot);
    ProgramStore restoredProgram(&outputs, nullptr, newRoot);
    if (!check(restored.song(song)["title"] == "Backup song"
                && restoredProgram.title() == "Backup service"
                && restoredProgram.items().size() == 2,
            "Songs and programs round trip")) {
        return 11;
    }
    const auto mediaSource = restoredProgram.items()[1]
                                 .toMap()["slides"]
                                 .toList()[0]
                                 .toMap()["elements"]
                                 .toList()[0]
                                 .toMap()["source"]
                                 .toString();
    if (!check(QImage(QUrl(mediaSource).toLocalFile()).pixelColor(0, 0) == Qt::green
                && mediaSource.contains("restored-external"),
            "External media works without original")) {
        return 12;
    }
    QFile restoredJson(newRoot + "/project.json");
    if (!restoredJson.open(QIODevice::ReadOnly)) {
        return 13;
    }
    const auto project = QJsonDocument::fromJson(restoredJson.readAll()).object();
    if (!check(project["font"] == newRoot + "/fonts/test.bin" && project["source"] == mediaSource,
            "Autosave JSON paths relocated")) {
        return 14;
    }
    const auto rollback = QDir(destination.path())
                              .entryList({ "Restored Selah తెలుగు.before-restore-*" }, QDir::Dirs);
    if (!check(
            rollback.size() == 1 && QFile::exists(destination.filePath(rollback[0] + "/prior.txt")),
            "Previous profile preserved")) {
        return 15;
    }
    auto cancelled = std::make_shared<BundleProgress>();
    cancelled->cancelled = true;
    if (!check(!AppBackup::prepare(path, newRoot, cancelled).error.isEmpty()
                && !QFile::exists(newRoot + ".restore-request"),
            "Cancelled restore leaves current data untouched")) {
        return 16;
    }
    qInfo() << "Full backup checks passed: open SQLite snapshots, libraries, external assets, "
               "typed preferences, Unicode relocation, cancellation and rollback profile.";
    return 0;
}
