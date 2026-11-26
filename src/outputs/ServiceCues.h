#pragma once
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
class OutputManager;
class QNetworkReply;

class ServiceCues : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList cues READ cues NOTIFY changed)
    Q_PROPERTY(QVariantList actionTypes READ actionTypes CONSTANT)
    Q_PROPERTY(QStringList transitionEffects READ transitionEffects CONSTANT)
    Q_PROPERTY(QVariantList serviceItems READ serviceItems NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY changed)
public:
    explicit ServiceCues(
        OutputManager *outputs, const QString &directory = {}, QObject *parent = nullptr);

    QVariantList cues() const { return m_cues; }

    QVariantList actionTypes() const;
    QStringList transitionEffects() const;
    QVariantList serviceItems() const;

    QString status() const { return m_status; }

    bool running() const { return m_running; }

    Q_INVOKABLE QVariantMap newAction(const QString &type) const;
    Q_INVOKABLE bool saveCue(int index, const QVariantMap &cue);
    Q_INVOKABLE bool removeCue(int index);
    Q_INVOKABLE bool trigger(int index);
    Q_INVOKABLE bool runCue(const QVariantMap &cue);
    Q_INVOKABLE bool testCue(const QVariantMap &cue);
    Q_INVOKABLE void cancel();
signals:
    void changed();

private:
    bool preflight(const QVariantMap &cue, QString *error) const;
    void next();
    void fail(const QString &error);
    bool persist(const QVariantList &cues);
    OutputManager *m_outputs;
    QString m_path, m_status, m_runningName, m_snapshot;
    QVariantList m_cues, m_actions;
    int m_step = 0;
    bool m_running = false, m_executing = false;
    QTimer m_wait;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QMediaPlayer m_player;
    QAudioOutput m_clipAudio;
};
