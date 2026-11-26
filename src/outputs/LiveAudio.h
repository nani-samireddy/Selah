#pragma once
#include <QHash>
#include <QObject>
#include <QVariantList>
class LiveAudioTrack;

// One audio player set per live session, independent of screen/NDI renderer instances.
class LiveAudio : public QObject {
    Q_OBJECT
public:
    explicit LiveAudio(QObject *parent = nullptr)
        : QObject(parent) { }

    void sync(const QVariantList &elements, bool playing, bool audible);
signals:
    void problem(const QString &message);

private:
    QHash<QString, LiveAudioTrack *> m_tracks;
};
