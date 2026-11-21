#include "FontLibrary.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

FontLibrary::FontLibrary(QObject *parent, const QString &directory)
    : QObject(parent) {
    m_directory = directory.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/fonts/google"
        : directory;
    QFile source(":/google-fonts/catalog.json");
    if (source.open(QIODevice::ReadOnly)) {
        const auto catalog = QJsonDocument::fromJson(source.readAll()).object();
        m_commit = catalog.value("commit").toString();
        m_records = catalog.value("families").toArray().toVariantList();
        for (const auto &value : m_records) {
            const auto record = value.toMap();
            m_byName.insert(record["family"].toString(), record);
        }
    } else {
        m_status = "The bundled font catalog is unavailable";
    }
    loadCache();
}

FontLibrary::~FontLibrary() {
    cancel();
    for (int id : m_fontIds) {
        QFontDatabase::removeApplicationFont(id);
    }
}

QString FontLibrary::directoryFor(const QString &family) const {
    return QDir(m_directory)
        .filePath(QString::fromLatin1(
            QCryptographicHash::hash(family.toUtf8(), QCryptographicHash::Sha256)
                .toHex()
                .left(24)));
}

void FontLibrary::loadCache() {
    QFile index(QDir(m_directory).filePath("index.json"));
    if (!index.open(QIODevice::ReadOnly)) {
        return;
    }
    const auto records
        = QJsonDocument::fromJson(index.readAll()).object().value("families").toObject();
    for (auto it = records.begin(); it != records.end(); ++it) {
        const auto family = it.key();
        const auto record = it.value().toObject().toVariantMap();
        QStringList names;
        QList<int> loaded;
        bool valid = true;
        for (const auto &file : record.value("files").toStringList()) {
            if (QFileInfo(file).fileName() != file) {
                valid = false;
                break;
            }
            const int id
                = QFontDatabase::addApplicationFont(QDir(directoryFor(family)).filePath(file));
            if (id < 0) {
                valid = false;
                break;
            }
            loaded.append(id);
            names += QFontDatabase::applicationFontFamilies(id);
        }
        if (valid && !names.isEmpty()) {
            m_fontIds += loaded;
            m_cache.insert(family, record);
            m_resolved.insert(family, names.contains(family) ? family : names.first());
        } else {
            for (int id : loaded) {
                QFontDatabase::removeApplicationFont(id);
            }
        }
    }
}

void FontLibrary::saveIndex() {
    QVariantMap families;
    for (auto it = m_cache.cbegin(); it != m_cache.cend(); ++it) {
        families.insert(it.key(), it.value());
    }
    const auto bytes
        = QJsonDocument::fromVariant(QVariantMap { { "format", 1 }, { "families", families } })
              .toJson();
    QSaveFile file(QDir(m_directory).filePath("index.json"));
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        m_status = "Font is ready for this session, but its cache index could not be saved";
    }
}

bool FontLibrary::available(const QString &family) const {
    return m_resolved.contains(family)
        || QFontDatabase::families().contains(family, Qt::CaseInsensitive);
}

QString FontLibrary::resolvedFamily(const QString &family) const {
    return m_resolved.value(family, family);
}

QVariantList FontLibrary::search(
    const QString &query, const QString &scope, const QString &script) const {
    QVariantList result;
    const auto needle = query.trimmed();
    const auto local = QFontDatabase::families();
    if (scope == "system") {
        for (const auto &family : local) {
            if (family.contains(needle, Qt::CaseInsensitive)) {
                result.append(QVariantMap { { "family", family },
                    { "category", "On this computer" }, { "available", true } });
            }
        }
        return result;
    }
    for (const auto &value : m_records) {
        auto record = value.toMap();
        const auto family = record["family"].toString();
        if (!family.contains(needle, Qt::CaseInsensitive)
            || (script != "all" && !record["subsets"].toStringList().contains(script))) {
            continue;
        }
        const bool ready
            = m_resolved.contains(family) || local.contains(family, Qt::CaseInsensitive);
        if (scope == "offline" && !ready) {
            continue;
        }
        result.append(QVariantMap {
            { "family", family }, { "category", record["category"] }, { "available", ready } });
    }
    return result;
}

QString FontLibrary::licenseText(const QString &family) const {
    QString source = family;
    if (!m_cache.contains(source)) {
        for (auto it = m_resolved.cbegin(); it != m_resolved.cend(); ++it) {
            if (it.value() == family) {
                source = it.key();
                break;
            }
        }
    }
    QFile notice(QDir(directoryFor(source)).filePath("NOTICE.txt"));
    if (notice.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(notice.readAll());
    }
    return "Font notices are stored with each downloaded family. System fonts retain the license "
           "supplied with the operating system or font installation.";
}
