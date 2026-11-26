#include "AiRequest.h"
#include "ContentLibrary.h"
#include "UpdateManager.h"
#include "VerifiedDownload.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QTemporaryDir>

int runUpdateChecks() {
    const QString digest = "sha256:" + QString(64, 'a');
    QJsonObject asset { { "name", "Selah-0.2.0-Windows-x64.zip" }, { "state", "uploaded" },
        { "size", 100 },
        { "browser_download_url",
            "https://github.com/nani-samireddy/Selah/releases/download/v0.2.0/"
            "Selah-0.2.0-Windows-x64.zip" },
        { "digest", digest } };
    QJsonObject release { { "tag_name", "v0.2.0" }, { "body", "Release notes" }, { "draft", false },
        { "prerelease", false }, { "assets", QJsonArray { asset } } };
    if (!UpdateManager::selectRelease(release, "0.1.0", "Windows-x64").contains("url")
        || !UpdateManager::selectRelease(release, "0.2.0", "Windows-x64").isEmpty()
        || !UpdateManager::selectRelease(release, "0.3.0", "Windows-x64").isEmpty()
        || UpdateManager::selectRelease(release, "0.1.0", "macOS-arm64").contains("url")) {
        return 1;
    }
    release["prerelease"] = true;
    if (!UpdateManager::selectRelease(release, "0.1.0", "Windows-x64").isEmpty()) {
        return 2;
    }
    release["prerelease"] = false;
    for (const auto &url : { "http://github.com/nani-samireddy/Selah/releases/download/v0.2.0/"
                             "Selah-0.2.0-Windows-x64.zip",
             "https://example.com/installer.zip",
             "https://github.com/other/repo/releases/download/v0.2.0/"
             "Selah-0.2.0-Windows-x64.zip" }) {
        auto bad = asset;
        bad["browser_download_url"] = url;
        release["assets"] = QJsonArray { bad };
        if (UpdateManager::selectRelease(release, "0.1.0", "Windows-x64").contains("url")) {
            return 3;
        }
    }
    auto bad = asset;
    bad.remove("digest");
    release["assets"] = QJsonArray { bad };
    if (UpdateManager::selectRelease(release, "0.1.0", "Windows-x64").contains("url")) {
        return 4;
    }
    release["tag_name"] = "v0.2.0-preview";
    if (!UpdateManager::selectRelease(release, "0.1.0", "Windows-x64").isEmpty()) {
        return 5;
    }
    QTemporaryDir directory;
    const auto path = directory.filePath("download.zip");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write("verified content") < 0) {
        return 6;
    }
    file.close();
    const auto sha
        = QCryptographicHash::hash("verified content", QCryptographicHash::Sha256).toHex();
    if (!VerifiedDownload::matches(path, sha)
        || VerifiedDownload::matches(path, QByteArray(64, '0'))) {
        return 7;
    }
    VerifiedDownload download;
    if (download.start(QUrl("http://example.com"), path, sha, 1024)
        || !VerifiedDownload::matches(path, sha)) {
        return 8;
    }
    ContentLibrary content(nullptr, directory.filePath("content"));
    const auto books = content.books("engwebp");
    for (const auto &pair : QList<QPair<QString, QString>> { { "యోహాను 3:16 చూపించు", "JHN 3:16" },
             { "show ఆదికాండము 1:1", "GEN 1:1" }, { "John 3:16-18", "JHN 3:16-18" },
             { "John ౩:౧౬", "JHN 3:16" }, { "1 John 3:16", "1JN 3:16" },
             { "John3:16", "JHN 3:16" } }) {
        if (AiRequest::scripture(pair.first, books).query != pair.second) {
            return 9;
        }
    }
    for (const auto &prompt : { "3:16", "John or James 3:16", "John", "John 3:0", "John 3:18-16",
             "John 3:16 and 4:1", "John 3:" }) {
        const auto parsed = AiRequest::scripture(prompt, books);
        if (!parsed.handled || parsed.error.isEmpty() || !parsed.query.isEmpty()) {
            return 10;
        }
    }
    if (AiRequest::scripture("love one another", books).handled
        || AiRequest::teluguSongTerms("కృప గురించి పాటలను వెతుకు") != "కృప"
        || AiRequest::teluguSongTerms("కృప songs చూపించు") != "కృప"
        || !AiRequest::teluguSongTerms("Amazing Grace").isEmpty()) {
        return 11;
    }
    return 0;
}
