#include "LiveSessionStore.h"
#include "OutputManager.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

LiveSessionStore::LiveSessionStore(
    OutputManager *outputs, const QString &directory, QObject *parent)
    : QObject(parent)
    , m_outputs(outputs) {
    const auto folder = directory.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : directory;
    QDir().mkpath(folder);
    m_path = folder + "/live-session.json";
    QFile file(m_path);
    if (file.open(QIODevice::ReadOnly) && file.size() <= 64 * 1024 * 1024) {
        const auto data = QJsonDocument::fromJson(file.readAll()).object().toVariantMap();
        if (data["format"].toInt() == 1 && !data["cleanExit"].toBool()
            && !data["session"].toMap()["slides"].toList().isEmpty()) {
            m_pending = data;
        }
    }
    connect(outputs, &OutputManager::snapshotChanged, this, [this] {
        m_armed = true;
        checkpoint();
    });
    connect(outputs, &OutputManager::liveChanged, this, &LiveSessionStore::checkpoint);
    connect(outputs, &OutputManager::stateChanged, this, &LiveSessionStore::checkpoint);
    connect(outputs, &OutputManager::sessionEnded, this, [this] {
        m_armed = false;
        if (!available()) {
            QFile::remove(m_path);
        }
    });
}

QString LiveSessionStore::summary() const {
    const auto session = m_pending["session"].toMap();
    const auto slides = session["slides"].toList();
    const int index = session["index"].toInt();
    QSet<QString> additions;
    for (const auto &value : slides) {
        const auto slide = value.toMap();
        if (slide["quickAddition"].toBool()) {
            additions.insert(slide.value("scriptureKey", slide["programItemId"]).toString());
        }
    }
    return QString("%1 · slide %2 of %3 · %4 spontaneous additions")
        .arg(slides.value(index).toMap().value("programTitle", "Live session").toString())
        .arg(index + 1)
        .arg(slides.size())
        .arg(additions.size());
}

void LiveSessionStore::checkpoint() {
    if (!m_armed || available() || m_restoring || m_outputs->liveCount() < 1) {
        return;
    }
    const QVariantMap root { { "format", 1 }, { "cleanExit", false },
        { "savedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate) },
        { "session", m_outputs->sessionSnapshot() } };
    const auto bytes
        = QJsonDocument(QJsonObject::fromVariantMap(root)).toJson(QJsonDocument::Compact);
    if (bytes.size() > 64 * 1024 * 1024) {
        m_status = "Live session is too large to checkpoint";
        emit changed();
        return;
    }
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        m_status = "Could not save live-session recovery";
        emit changed();
    }
}

bool LiveSessionStore::recover() {
    if (!available()) {
        return false;
    }
    emit restoring();
    m_restoring = true;
    const bool ok = m_outputs->restoreSession(m_pending["session"].toMap());
    m_restoring = false;
    if (!ok) {
        m_status = "Recovery file is invalid. The current session was kept.";
        emit changed();
        return false;
    }
    m_pending.clear();
    m_armed = true;
    m_status = "Session restored. Outputs are off; media and countdown are paused.";
    checkpoint();
    emit changed();
    return true;
}

void LiveSessionStore::discard() {
    m_pending.clear();
    QFile::remove(m_path);
    checkpoint();
    emit changed();
}

void LiveSessionStore::finishCleanly() {
    if (available()) {
        return; // Keep an unanswered recovery offer for the next launch.
    }
    m_armed = false;
    QFile::remove(m_path);
}
