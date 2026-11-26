#include "ServiceCues.h"
#include "OutputManager.h"
#include "ServiceCueActions.h"
#include "SlideTransition.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

ServiceCues::ServiceCues(OutputManager *outputs, const QString &directory, QObject *parent)
    : QObject(parent)
    , m_outputs(outputs) {
    const auto folder = directory.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : directory;
    QDir().mkpath(folder);
    m_path = folder + "/service-cues.json";
    QFile file(m_path);
    bool loaded = false;
    if (file.open(QIODevice::ReadOnly) && file.size() < 1024 * 1024) {
        const auto doc = QJsonDocument::fromJson(file.readAll());
        const auto values = doc.array().toVariantList();
        loaded = doc.isArray() && values.size() <= 32;
        for (const auto &value : values) {
            loaded &= ServiceCueActions::valid(value.toMap());
        }
        if (loaded) {
            for (const auto &value : values) {
                m_cues.append(ServiceCueActions::normalize(value.toMap()));
            }
        }
    }
    if (!loaded) {
        m_cues = { QVariantMap { { "name", "Start worship" }, { "stage", "musician" },
                       { "audience", "lyrics" }, { "audio", "off" }, { "media", "keep" },
                       { "timer", "pause" }, { "message", "" }, { "minutes", 20 } },
            QVariantMap { { "name", "Sermon" }, { "stage", "speaker" }, { "audience", "keep" },
                { "audio", "off" }, { "media", "pause" }, { "timer", "start" }, { "minutes", 30 },
                { "message", "" } },
            QVariantMap { { "name", "Closing" }, { "stage", "speaker" }, { "audience", "default" },
                { "audio", "off" }, { "media", "pause" }, { "timer", "pause" }, { "minutes", 20 },
                { "message", "Closing" } } };
    }
    for (auto &value : m_cues) {
        value = ServiceCueActions::normalize(value.toMap());
    }
    m_wait.setSingleShot(true);
    connect(&m_wait, &QTimer::timeout, this, &ServiceCues::next);
    m_player.setAudioOutput(&m_clipAudio);
    m_clipAudio.setVolume(.8);
    connect(&m_player, &QMediaPlayer::errorOccurred, this,
        [this](QMediaPlayer::Error, const QString &) {
            fail("The cue audio clip could not be played");
        });
    connect(outputs, &OutputManager::sessionEnded, this, &ServiceCues::cancel);
    connect(outputs, &OutputManager::liveChanged, this, [this] {
        if (m_running && !m_executing) {
            cancel();
        }
        emit changed();
    });
}

bool ServiceCues::persist(const QVariantList &cues) {
    QSaveFile file(m_path);
    const auto bytes = QJsonDocument(QJsonArray::fromVariantList(cues)).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        m_status = "Could not save service cues";
        emit changed();
        return false;
    }
    m_cues = cues;
    emit changed();
    return true;
}

bool ServiceCues::saveCue(int index, const QVariantMap &cue) {
    if (m_running || !ServiceCueActions::valid(cue) || index < -1 || index >= m_cues.size()
        || (index == -1 && m_cues.size() >= 32)) {
        m_status = "Choose a name, valid actions and a countdown from 1 to 180 minutes";
        emit changed();
        return false;
    }
    auto next = m_cues;
    if (index == -1) {
        next.append(ServiceCueActions::normalize(cue));
    } else {
        next[index] = ServiceCueActions::normalize(cue);
    }
    return persist(next);
}

bool ServiceCues::removeCue(int index) {
    if (m_running || index < 0 || index >= m_cues.size()) {
        return false;
    }
    auto next = m_cues;
    next.removeAt(index);
    return persist(next);
}

bool ServiceCues::trigger(int index) {
    return index >= 0 && index < m_cues.size() && runCue(m_cues[index].toMap());
}

QVariantList ServiceCues::actionTypes() const {
    return ServiceCueActions::catalog();
}

QStringList ServiceCues::transitionEffects() const {
    return QStringList { "authored", "cut" } + SlideTransition::effects();
}

QVariantMap ServiceCues::newAction(const QString &type) const {
    return ServiceCueActions::defaults(type);
}

QVariantList ServiceCues::serviceItems() const {
    QVariantList result;
    QSet<QString> seen;
    for (const auto &value : m_outputs->liveOrder()) {
        const auto row = value.toMap();
        const auto id = row["itemId"].toString();
        if (id.isEmpty() || seen.contains(id)) {
            continue;
        }
        seen.insert(id);
        result.append(QVariantMap { { "id", id }, { "title", row["itemTitle"] } });
    }
    return result;
}
