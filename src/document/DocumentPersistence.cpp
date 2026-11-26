#include "BackgroundFill.h"
#include "EditorDocument.h"
#include "SlideTransition.h"
#include <algorithm>
#include <cmath>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QUuid>

QJsonObject EditorDocument::snapshot() const {
    return { { "format", 1 }, { "slides", QJsonArray::fromVariantList(m_slides) },
        { "currentSlide", m_current } };
}

bool EditorDocument::restore(const QJsonObject &obj) {
    // Validate a candidate completely before replacing the draft or invalidating imports.
    if (obj["format"].toInt() != 1 || !obj["slides"].isArray()) {
        return false;
    }
    auto slides = obj["slides"].toArray().toVariantList();
    if (slides.empty() || slides.size() > 500) {
        return false;
    }
    QSet<QString> identifiers;
    for (auto &v : slides) {
        auto map = v.toMap();
        for (const auto &key : { "transition", "transitionDefault" }) {
            if (map.contains(key) && !SlideTransition::valid(map.value(key))) {
                return false;
            }
        }
        if ((map.contains("backgroundFill")
                && (map["backgroundFill"].metaType().id() != QMetaType::QVariantMap
                    || !BackgroundFill::valid(map["backgroundFill"].toMap())))
            || !QColor(map["background"].toString()).isValid() || !map.contains("elements")
            || map["elements"].toList().size() > 1000) {
            return false;
        }
        auto normalized = map["elements"].toList();
        for (auto &e : normalized) {
            auto el = e.toMap();
            const auto kind = el.value("kind", "text").toString();
            if (!QStringList { "text", "image", "video", "audio" }.contains(kind)) {
                return false;
            }
            for (const auto &key :
                { "x", "y", "width", "height", "fontSize", "rotation", "opacity" }) {
                bool ok = false;
                double n = el[key].toDouble(&ok);
                if (!ok || !std::isfinite(n)) {
                    return false;
                }
            }
            if (el["width"].toDouble() < 40 || el["height"].toDouble() < 30
                || el["width"].toDouble() > 1920 || el["height"].toDouble() > 1080
                || el["fontSize"].toDouble() < 8 || el["fontSize"].toDouble() > 400
                || !QColor(el["color"].toString()).isValid()) {
                return false;
            }
            if (kind != "text") {
                if (!QUrl(el["source"].toString()).isLocalFile()
                    || !QStringList { "contain", "cover", "stretch" }.contains(
                        el["fit"].toString())) {
                    return false;
                }
            }
            auto uid = el.value("uid").toString();
            if (uid.isEmpty()) {
                uid = QUuid::createUuid().toString(QUuid::WithoutBraces);
            }
            if (identifiers.contains(uid)) {
                return false;
            }
            identifiers.insert(uid);
            el["uid"] = uid;
            el["kind"] = kind;
            e = el;
        }
        map["elements"] = normalized;
        v = map;
    }
    ++m_generation;
    m_slides = slides;
    m_current = std::clamp(obj["currentSlide"].toInt(), 0, int(slides.size()) - 1);
    m_selected = -1;
    emit changed();
    emit selectionChanged();
    return true;
}

QString EditorDocument::recoveryPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/editor-recovery.churchdeck";
}

bool EditorDocument::saveProject(const QUrl &url) {
    const auto path = url.isEmpty() ? recoveryPath() : url.toLocalFile();
    if (path.isEmpty()) {
        setStatus("Choose a local project file");
        return false;
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    const auto bytes = QJsonDocument(snapshot()).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        setStatus("Could not save project");
        return false;
    }
    setStatus(url.isEmpty() ? "All changes saved" : "Project exported");
    return true;
}

bool EditorDocument::openProject(const QUrl &url) {
    QFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        setStatus("Could not open project");
        return false;
    }
    const auto obj = QJsonDocument::fromJson(file.readAll()).object();
    auto old = snapshot();
    if (!restore(obj)) {
        setStatus("This is not a valid Selah project");
        return false;
    }
    for (const auto &slide : m_slides) {
        for (const auto &element : slide.toMap()["elements"].toList()) {
            if (element.toMap().value("kind", "text") != "text") {
                emit mediaImported(element.toMap());
            }
        }
    }
    m_undo.append(old);
    m_redo.clear();
    commit();
    return true;
}

QVariantList EditorDocument::validatedSlides(const QVariantList &slides) {
    EditorDocument candidate(nullptr, false);
    return candidate.restore({ { "format", 1 }, { "slides", QJsonArray::fromVariantList(slides) } })
        ? candidate.slides()
        : QVariantList();
}
