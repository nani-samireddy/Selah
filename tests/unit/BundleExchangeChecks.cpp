#include "OutputManager.h"
#include "ProgramStore.h"
#include "TemplateDefinition.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QThread>

namespace {
bool waitForBundle(ProgramStore &store) {
    QElapsedTimer timer;
    timer.start();
    while (store.bundleBusy() && timer.elapsed() < 15000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    QCoreApplication::processEvents();
    return !store.bundleBusy();
}
}

int exportBundleExchangeFixture(const QString &path) {
    QTemporaryDir directory;
    OutputManager outputs;
    ProgramStore program(&outputs, nullptr, directory.path());
    const auto media = directory.filePath("నీలం blue.png");
    QImage image(32, 32, QImage::Format_RGB32);
    image.fill(Qt::blue);
    const QVariantMap song { { "title", "Offline exchange / వెలుగు" },
        { "primaryLanguage", "English" }, { "secondaryLanguage", "Telugu" },
        { "sections",
            QVariantList { QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                { "primary", "We gather in light" }, { "secondary", "నీ వెలుగులో కూడుదము" } } } },
        { "arrangement", QVariantList { "v1" } } };
    if (!image.save(media) || !program.createProgram("Cross-platform offline service", "2026-11-26")
        || !program.addMedia({ { "kind", "image" },
            { "source", QUrl::fromLocalFile(media).toString() }, { "name", "Blue image" } })
        || !program.setItemSection("Worship")
        || !program.addSong(song, 2, TemplateDefinition::starter("bilingual"))
        || !program.setItemSection("Sermon") || !program.save()
        || !program.exportBundle(QUrl::fromLocalFile(path)) || !waitForBundle(program)
        || !QFile::exists(path)) {
        qCritical() << "Exchange export failed:" << program.status();
        return 1;
    }
    // The temporary source profile and original media are deleted on return.
    qInfo() << "Exported offline fixture on" << QSysInfo::productType()
            << QSysInfo::buildCpuArchitecture();
    return 0;
}

int importBundleExchangeFixture(const QString &path, const QString &folder) {
    QTemporaryDir directory;
    OutputManager outputs;
    ProgramStore program(&outputs, nullptr, directory.path());
    if (!program.importBundle(QUrl::fromLocalFile(path)) || !waitForBundle(program)
        || program.title() != "Cross-platform offline service" || program.items().size() != 2) {
        qCritical() << "Exchange import failed:" << program.status();
        return 2;
    }
    const auto imageItem = program.items()[0].toMap();
    const auto source = imageItem["slides"]
                            .toList()[0]
                            .toMap()["elements"]
                            .toList()[0]
                            .toMap()["source"]
                            .toString();
    const auto songItem = program.items()[1].toMap();
    if (QImage(QUrl(source).toLocalFile()).pixelColor(0, 0) != Qt::blue
        || !QUrl(source).toLocalFile().startsWith(directory.path())
        || imageItem["section"] != "Worship" || songItem["section"] != "Sermon"
        || songItem["layoutTemplate"].toMap().isEmpty()
        || songItem["song"].toMap()["sections"].toList()[0].toMap()["secondary"]
            != "నీ వెలుగులో కూడుదము"
        || outputs.liveCount() != 0) {
        qCritical() << "Exchange content/path/template/section isolation failed";
        return 3;
    }
    QDir().mkpath(folder);
    const auto roundtrip = folder + "/" + QSysInfo::productType() + "-roundtrip.selahbundle";
    if (!program.exportBundle(QUrl::fromLocalFile(roundtrip)) || !waitForBundle(program)
        || !QFile::exists(roundtrip)) {
        return 4;
    }
    QFile report(folder + "/bundle-exchange.txt");
    if (!report.open(QIODevice::WriteOnly)) {
        return 5;
    }
    report.write("PASS: offline import, relocated image pixels, bilingual song, template snapshot, "
                 "service sections, no automatic live output, roundtrip export.\n");
    report.write(
        QSysInfo::productType().toUtf8() + " " + QSysInfo::buildCpuArchitecture().toUtf8() + "\n");
    qInfo() << "Offline cross-platform bundle checks passed on" << QSysInfo::productType();
    return 0;
}
