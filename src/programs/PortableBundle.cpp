#include "PortableBundle.h"
#include <functional>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScopeGuard>
#include <QUrl>
#include <QUuid>

namespace {
constexpr qint64 manifestLimit = 32 * 1024 * 1024;
constexpr qint64 totalLimit = 50LL * 1024 * 1024 * 1024;
const QByteArray magic("SELAHBUNDLE1\n");
using Rewrite = std::function<QVariant(const QString &, bool)>;

QVariant visit(const QVariant &value, const Rewrite &rewrite) {
    if (value.metaType().id() == QMetaType::QVariantMap) {
        auto map = value.toMap();
        const bool media = map["kind"] == "image" || map["kind"] == "video"
            || map["kind"] == "audio" || map["type"] == "image";
        for (auto it = map.begin(); it != map.end(); ++it) {
            if (it.key() == "source"
                && (media || QUrl(it->toString()).isLocalFile()
                    || it->toString().startsWith("selah-asset:"))) {
                it.value() = rewrite(it->toString(), media);
            } else {
                it.value() = visit(it.value(), rewrite);
            }
        }
        return map;
    }
    if (value.metaType().id() == QMetaType::QVariantList) {
        auto list = value.toList();
        for (auto &item : list) {
            item = visit(item, rewrite);
        }
        return list;
    }
    return value;
}

bool transfer(QIODevice &from, QIODevice *to, qint64 size, QCryptographicHash &hash,
    const std::shared_ptr<BundleProgress> &progress, qint64 &done, qint64 total) {
    while (size > 0) {
        if (progress->cancelled) {
            return false;
        }
        const auto bytes = from.read(qMin(size, qint64(1024 * 1024)));
        if (bytes.isEmpty() || (to && to->write(bytes) != bytes.size())) {
            return false;
        }
        hash.addData(bytes);
        size -= bytes.size();
        done += bytes.size();
        progress->percent = total > 0 ? qMin(99LL, done * 100 / total) : 0;
    }
    return true;
}
}

BundleResult PortableBundle::write(const QVariantMap &program, const QString &path,
    const std::shared_ptr<BundleProgress> &progress) {
    QString error;
    QVariantList assets;
    QMap<QString, QString> tokens;
    QStringList sources;
    qint64 total = 0, scanned = 0;
    const auto portable = visit(program, [&](const QString &source, bool) -> QVariant {
        if (tokens.contains(source)) {
            return tokens[source];
        }
        const QUrl url(source);
        const auto local = url.isLocalFile() ? url.toLocalFile()
            : source.startsWith("qrc:/")     ? ":" + source.mid(4)
                                             : QString();
        QFile file(local);
        if (local.isEmpty() || !file.open(QIODevice::ReadOnly) || file.size() > totalLimit
            || assets.size() >= 20000) {
            error = "A media file is missing or unsupported: " + source;
            return source;
        }
        if (QFileInfo(local).absoluteFilePath() == QFileInfo(path).absoluteFilePath()) {
            error = "Choose a bundle destination outside the source media";
            return source;
        }
        total += file.size();
        if (total > totalLimit) {
            error = "Bundle media exceeds the 50 GB limit";
            return source;
        }
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!transfer(file, nullptr, file.size(), hash, progress, scanned, 0)) {
            error = "Bundle cancelled or media could not be read";
            return source;
        }
        const auto digest = QString::fromLatin1(hash.result().toHex());
        const auto suffix = QFileInfo(local).suffix().toLower();
        const auto safeSuffix
            = QRegularExpression("^[a-z0-9]{1,10}$").match(suffix).hasMatch() ? suffix : "bin";
        const auto name = QString::number(assets.size()) + "-" + digest + "." + safeSuffix;
        const auto token = "selah-asset:" + name;
        tokens[source] = token;
        sources.append(local);
        assets.append(
            QVariantMap { { "name", name }, { "size", file.size() }, { "sha256", digest } });
        return token;
    }).toMap();
    if (!error.isEmpty() || progress->cancelled) {
        return { {}, {}, error.isEmpty() ? "Bundle cancelled" : error };
    }
    const auto manifest = QJsonDocument(
        QJsonObject::fromVariantMap({ { "format", "selah-portable-1" }, { "program", portable },
            { "assets",
                assets } })).toJson(QJsonDocument::Compact);
    if (manifest.size() > manifestLimit) {
        return { {}, {}, "Program manifest is too large" };
    }
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(magic) != magic.size()) {
        return { {}, {}, "Could not create bundle" };
    }
    QDataStream stream(&output);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << quint64(manifest.size());
    if (output.write(manifest) != manifest.size()) {
        return { {}, {}, "Could not write bundle manifest" };
    }
    qint64 done = 0;
    for (int i = 0; i < sources.size(); ++i) {
        QFile input(sources[i]);
        QCryptographicHash hash(QCryptographicHash::Sha256);
        const auto asset = assets[i].toMap();
        if (!input.open(QIODevice::ReadOnly) || input.size() != asset["size"].toLongLong()
            || !transfer(input, &output, input.size(), hash, progress, done, total)
            || hash.result().toHex() != asset["sha256"].toByteArray()) {
            return { {}, {}, "Bundle cancelled or source media changed; export again" };
        }
    }
    if (progress->cancelled || stream.status() != QDataStream::Ok || !output.commit()) {
        return { {}, {}, "Bundle cancelled or could not be saved" };
    }
    progress->percent = 100;
    return {};
}

BundleResult PortableBundle::read(const QString &path, const QString &directory,
    const std::shared_ptr<BundleProgress> &progress) {
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly) || input.read(magic.size()) != magic) {
        return { {}, {}, "Invalid Selah portable bundle" };
    }
    QDataStream stream(&input);
    stream.setByteOrder(QDataStream::BigEndian);
    quint64 length = 0;
    stream >> length;
    if (stream.status() != QDataStream::Ok || length > manifestLimit
        || length > quint64(input.bytesAvailable())) {
        return { {}, {}, "Invalid bundle manifest length" };
    }
    const auto doc = QJsonDocument::fromJson(input.read(length));
    const auto root = doc.object().toVariantMap();
    const auto assets = root["assets"].toList();
    if (root["format"] != "selah-portable-1" || assets.size() > 20000
        || root["assets"].metaType().id() != QMetaType::QVariantList
        || root["program"].toMap().isEmpty()) {
        return { {}, {}, "Invalid bundle manifest" };
    }
    qint64 total = 0;
    QSet<QString> names;
    static const QRegularExpression safeName("^[0-9]{1,5}-[0-9a-f]{64}\\.[a-z0-9]{1,10}$");
    static const QRegularExpression digest("^[0-9a-f]{64}$");
    for (const auto &value : assets) {
        const auto asset = value.toMap();
        const auto name = asset["name"].toString();
        const auto size = asset["size"].toLongLong();
        if (!safeName.match(name).hasMatch() || names.contains(name) || size < 0
            || size > totalLimit || !digest.match(asset["sha256"].toString()).hasMatch()) {
            return { {}, {}, "Invalid bundle asset entry" };
        }
        names.insert(name);
        total += size;
        if (total > totalLimit) {
            return { {}, {}, "Bundle exceeds the 50 GB limit" };
        }
    }
    if (input.bytesAvailable() != total) {
        return { {}, {}, "Bundle media is truncated or has unexpected data" };
    }
    const auto folder
        = directory + "/bundle-media/" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!QDir().mkpath(folder)) {
        return { {}, {}, "Could not create bundle media folder" };
    }
    bool keep = false;
    const auto cleanup = qScopeGuard([&] {
        if (!keep) {
            QDir(folder).removeRecursively();
        }
    });
    QMap<QString, QString> paths;
    qint64 done = 0;
    for (const auto &value : assets) {
        const auto asset = value.toMap();
        const auto name = asset["name"].toString();
        const auto dest = folder + "/" + name;
        QSaveFile output(dest);
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!output.open(QIODevice::WriteOnly)
            || !transfer(input, &output, asset["size"].toLongLong(), hash, progress, done, total)
            || hash.result().toHex() != asset["sha256"].toByteArray() || !output.commit()) {
            return { {}, {}, "Bundle cancelled, damaged or could not be extracted" };
        }
        paths["selah-asset:" + name] = QUrl::fromLocalFile(dest).toString();
    }
    QString error;
    const auto program = visit(root["program"], [&](const QString &source, bool) -> QVariant {
        if (!paths.contains(source)) {
            error = "Bundle contains an undeclared media reference";
            return source;
        }
        return paths[source];
    }).toMap();
    if (!error.isEmpty() || progress->cancelled) {
        return { {}, {}, error.isEmpty() ? "Bundle cancelled" : error };
    }
    keep = true;
    progress->percent = 100;
    return { program, folder, {} };
}
