#include "LiveAudio.h"
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QSet>
#include <QUrl>

class LiveAudioTrack : public QObject {
public:
    explicit LiveAudioTrack(QObject *parent)
        : QObject(parent)
        , audio(this)
        , player(this) {
        player.setObjectName("liveAudioPlayer");
        player.setAudioOutput(&audio);
    }

    QAudioOutput audio;
    QMediaPlayer player;
    QString source;
    bool requested = false, failed = false;
};

void LiveAudio::sync(const QVariantList &elements, bool playing, bool audible) {
    QSet<QString> retained;
    for (const auto &value : elements) {
        const auto element = value.toMap();
        if (element["kind"] != "audio" || !element.value("visible", true).toBool()) {
            continue;
        }
        const auto key = element["uid"].toString(), source = element["source"].toString();
        if (key.isEmpty() || !QUrl(source).isLocalFile()) {
            continue;
        }
        retained.insert(key);
        auto track = m_tracks.value(key);
        if (track && track->source != source) {
            track->player.stop();
            delete track;
            m_tracks.remove(key);
            track = nullptr;
        }
        if (!track) {
            track = new LiveAudioTrack(this);
            m_tracks[key] = track;
            track->source = source;
            connect(&track->player, &QMediaPlayer::errorOccurred, track,
                [this, track](QMediaPlayer::Error, const QString &) {
                    track->failed = true;
                    emit problem("Audio unavailable. Check the file and its format.");
                });
            track->player.setSource(QUrl(source));
        }
        track->player.setLoops(element.value("loop", false).toBool() ? QMediaPlayer::Infinite : 1);
        track->audio.setMuted(!audible || element.value("muted", false).toBool());
        if (playing != track->requested) {
            track->requested = playing;
            if (playing && !track->failed) {
                track->player.play();
            } else {
                track->player.pause();
            }
        }
    }
    const auto keys = m_tracks.keys();
    for (const auto &key : keys) {
        if (!retained.contains(key)) {
            auto track = m_tracks.take(key);
            track->player.stop();
            delete track;
        }
    }
}
