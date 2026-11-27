#include "TemplatePresets.h"
#include "MediaLibrary.h"
#include "TemplateStore.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

// Referencing the generated initializer keeps resources linked into static-core test runners.
static void initializePresetResources() {
    Q_INIT_RESOURCE(template_presets);
}

namespace {
QString localAsset(const QString &pack, const QString &name, const QString &folder) {
    if (name.isEmpty() || QFileInfo(name).fileName() != name) {
        return {};
    }
    QFile source(":/template-presets/" + pack + "/media/" + name);
    if (!source.open(QIODevice::ReadOnly)) {
        return {};
    }
    const auto bytes = source.readAll();
    const auto hash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
    const auto destination
        = folder + "/" + pack + "/" + QString::fromLatin1(hash.left(16)) + "-" + name;
    QFile existing(destination);
    if (existing.open(QIODevice::ReadOnly)
        && QCryptographicHash::hash(existing.readAll(), QCryptographicHash::Sha256).toHex()
            == hash) {
        return QUrl::fromLocalFile(destination).toString();
    }
    existing.close();
    if (!QDir().mkpath(QFileInfo(destination).absolutePath())) {
        return {};
    }
    QSaveFile output(destination);
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size()
        || !output.commit()) {
        return {};
    }
    return QUrl::fromLocalFile(destination).toString();
}
}

QString TemplatePresets::install(
    TemplateStore &store, MediaLibrary *media, const QString &directory) {
    initializePresetResources();
    const auto folder
        = (directory.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                               : directory)
        + "/template-assets";
    for (const auto &pack : { QString("selected") }) {
        QFile manifest(":/template-presets/" + pack + "/templates.json");
        if (!manifest.open(QIODevice::ReadOnly)) {
            return "Could not load bundled templates: " + pack;
        }
        const auto root = QJsonDocument::fromJson(manifest.readAll()).object();
        const auto definitions = root["templates"].toArray();
        if (root["pack"] != pack || root["version"].toInt() != 1 || definitions.isEmpty()) {
            return "Invalid bundled template manifest: " + pack;
        }
        QMap<QString, QString> assets;
        const auto resolve = [&](const QString &name) {
            if (!assets.contains(name)) {
                assets[name] = localAsset(pack, name, folder);
            }
            return assets[name];
        };
        for (const auto &value : definitions) {
            auto definition = value.toObject().toVariantMap();
            auto slide = definition["slide"].toMap();
            auto elements = slide["elements"].toList();
            for (auto &element : elements) {
                auto layer = element.toMap();
                if (layer["kind"] != "image") {
                    continue;
                }
                const auto prefix = "asset:" + pack + "/media/";
                const auto source = layer["source"].toString();
                const auto resolved
                    = source.startsWith(prefix) ? resolve(source.mid(prefix.size())) : QString();
                if (resolved.isEmpty()) {
                    return "Could not install bundled template media: " + source;
                }
                layer["source"] = resolved;
                element = layer;
            }
            slide["elements"] = elements;
            definition["slide"] = slide;
            if (!store.installPreset(definition)) {
                return "Could not install bundled template " + definition["name"].toString()
                    + ": " + store.status();
            }
        }
        // Photos are available for reuse in Media; decorative rules stay out of that catalog.
        for (const auto &value : root["media"].toArray()) {
            const auto name = value.toString();
            const auto source = resolve(name);
            if (source.isEmpty()) {
                return "Could not install bundled background: " + name;
            }
            if (media) {
                media->registerAsset({ { "source", source }, { "kind", "image" },
                    { "name", pack + " · " + QFileInfo(name).completeBaseName() } });
            }
        }
    }
    return {};
}
