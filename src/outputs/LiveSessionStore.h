#pragma once
#include <QObject>
#include <QVariantMap>
class OutputManager;

// Recovery never starts screens, audio, media playback, or countdowns.
class LiveSessionStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit LiveSessionStore(
        OutputManager *outputs, const QString &directory = {}, QObject *parent = nullptr);

    bool available() const { return !m_pending.isEmpty(); }

    QString summary() const;

    QString status() const { return m_status; }

    Q_INVOKABLE bool recover();
    Q_INVOKABLE void discard();
    void finishCleanly();
signals:
    void restoring();
    void changed();

private:
    void checkpoint();
    OutputManager *m_outputs;
    QString m_path, m_status;
    QVariantMap m_pending;
    bool m_armed = false, m_restoring = false;
};
