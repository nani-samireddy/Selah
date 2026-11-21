#include "FontLibrary.h"
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTimer>

int FontLibrary::selfTest() {
    QTemporaryDir directory;
    FontLibrary fonts(nullptr, directory.path());
    if (fonts.count() < 1900 || fonts.search("Roboto").isEmpty()
        || fonts.search("Noto Sans Telugu").isEmpty()) {
        return 1;
    }
    const auto telugu = fonts.search("", "google", "telugu");
    if (telugu.size() < 5 || !fonts.search("This family does not exist 999").isEmpty()) {
        return 2;
    }
    const auto urls = cssFontUrls(
        "src:url(https://fonts.gstatic.com/s/family/valid.ttf);src:url(https://evil.example/"
        "font.ttf);src:url(https://fonts.gstatic.com/s/family/file.woff2)");
    if (urls.size() != 1 || urls.first().host() != "fonts.gstatic.com") {
        return 3;
    }
    const QString family = "Font cache verification";
    const auto cache = fonts.directoryFor(family);
    QDir().mkpath(cache);
    if (!QFile::copy(
            ":/qt/qml/ChurchDeck/assets/fonts/Inter.ttf", QDir(cache).filePath("Inter.ttf"))) {
        return 4;
    }
    QFile notice(QDir(cache).filePath("NOTICE.txt"));
    if (!notice.open(QIODevice::WriteOnly) || notice.write("Original font license fixture") < 0) {
        return 5;
    }
    notice.close();
    fonts.m_cache.insert(family, { { "files", QStringList { "Inter.ttf" } } });
    fonts.saveIndex();
    {
        FontLibrary reopened(nullptr, directory.path());
        if (!reopened.available(family) || reopened.resolvedFamily(family) != "Inter"
            || !reopened.licenseText(family).contains("license fixture")) {
            return 6;
        }
    }
    fonts.m_cache[family]["files"] = QStringList { "Inter.ttf", "missing-font.ttf" };
    fonts.saveIndex();
    {
        FontLibrary incomplete(nullptr, directory.path());
        if (incomplete.available(family) || !incomplete.m_fontIds.isEmpty()) {
            return 9;
        }
    }
    if (!fontNotice(QDir(cache).filePath("Inter.ttf")).contains("Inter")) {
        return 7;
    }
    const auto local = QFontDatabase::families();
    if (local.isEmpty() || fonts.search(local.first(), "system").isEmpty()) {
        return 8;
    }
    qInfo() << "Font checks passed: full catalog, script search, trusted native URLs, offline "
               "cache, license notices and system fonts";
    return 0;
}

int FontLibrary::networkSelfTest(const QString &reportDirectory) {
    QTemporaryDir directory;
    bool passed = true;
    QStringList verified;
    {
        FontLibrary fonts(nullptr, directory.path());
        for (const QString &family : QStringList { "Abel", "Edu NSW ACT Hand Pre" }) {
            QEventLoop loop;
            QTimer deadline;
            deadline.setSingleShot(true);
            deadline.start(150000);
            bool ready = false;
            auto success = QObject::connect(&fonts, &FontLibrary::familyReady, &loop,
                [&](const QString &name, const QString &resolved) {
                    ready = name == family && !resolved.isEmpty();
                    loop.quit();
                });
            auto failure = QObject::connect(
                &fonts, &FontLibrary::installFailed, &loop, [&](const QString &message) {
                    qWarning() << message;
                    loop.quit();
                });
            QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
            fonts.install(family);
            loop.exec();
            QObject::disconnect(success);
            QObject::disconnect(failure);
            if (!ready || !fonts.available(family) || fonts.licenseText(family).size() < 100) {
                passed = false;
                break;
            }
            verified.append(family);
            qInfo() << "Downloaded and registered" << family
                    << (fonts.m_css ? "Google CSS / TTF" : "pinned repository");
        }
    }
    {
        FontLibrary offline(nullptr, directory.path());
        for (const auto &family : verified) {
            passed &= offline.available(family) && !offline.licenseText(family).isEmpty();
        }
    }
    if (!reportDirectory.isEmpty()) {
        QDir().mkpath(reportDirectory);
        QFile report(QDir(reportDirectory).filePath("fonts-network.json"));
        if (report.open(QIODevice::WriteOnly)) {
            report.write(QJsonDocument::fromVariant(
                QVariantMap {
                    { "passed", passed }, { "families", verified }, { "offlineReload", passed } })
                    .toJson());
        }
    }
    qInfo() << "Font network / offline reload" << (passed ? "passed" : "failed");
    return passed ? 0 : 1;
}
