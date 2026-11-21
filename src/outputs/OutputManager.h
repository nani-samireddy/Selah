#pragma once
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QScreen>
#include <QSet>
#include <QTimer>
#include <QVariantList>
#include <QWindow>

// Owns display routing and the immutable live snapshot, independent of editor drafts.
class OutputManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList liveOrder READ liveOrder NOTIFY snapshotChanged)
    Q_PROPERTY(QString snapshotId READ snapshotId NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantList displays READ displays NOTIFY displaysChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(bool audienceEnabled READ audienceEnabled NOTIFY stateChanged)
    Q_PROPERTY(bool stageEnabled READ stageEnabled NOTIFY stateChanged)
    Q_PROPERTY(QString audienceDisplay READ audienceDisplay NOTIFY stateChanged)
    Q_PROPERTY(QString stageDisplay READ stageDisplay NOTIFY stateChanged)
    Q_PROPERTY(bool audienceBlack READ audienceBlack NOTIFY stateChanged)
    Q_PROPERTY(bool stageBlack READ stageBlack NOTIFY stateChanged)
    Q_PROPERTY(bool textCleared READ textCleared NOTIFY stateChanged)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY stateChanged)
    Q_PROPERTY(bool audioEnabled READ audioEnabled WRITE setAudioEnabled NOTIFY stateChanged)
    Q_PROPERTY(QVariantList liveElements READ liveElements NOTIFY liveChanged)
    Q_PROPERTY(QVariantList nextElements READ nextElements NOTIFY liveChanged)
    Q_PROPERTY(QString nextBackground READ nextBackground NOTIFY liveChanged)
    Q_PROPERTY(double liveAmbientOpacity READ liveAmbientOpacity NOTIFY liveChanged)
    Q_PROPERTY(double nextAmbientOpacity READ nextAmbientOpacity NOTIFY liveChanged)
    Q_PROPERTY(QString liveSource READ liveSource NOTIFY liveChanged)
    Q_PROPERTY(QString liveProgramItemId READ liveProgramItemId NOTIFY liveChanged)
    Q_PROPERTY(QString liveTitle READ liveTitle NOTIFY liveChanged)
    Q_PROPERTY(QString nextTitle READ nextTitle NOTIFY liveChanged)
    Q_PROPERTY(QString liveNotes READ liveNotes NOTIFY liveChanged)
    Q_PROPERTY(QString liveText READ liveText NOTIFY liveChanged)
    Q_PROPERTY(QString nextText READ nextText NOTIFY liveChanged)
    Q_PROPERTY(QString liveBackground READ liveBackground NOTIFY liveChanged)
    Q_PROPERTY(int liveIndex READ liveIndex NOTIFY liveChanged)
    Q_PROPERTY(int liveCount READ liveCount NOTIFY liveChanged)
    Q_PROPERTY(QString stageMessage READ stageMessage WRITE setStageMessage NOTIFY stateChanged)
    Q_PROPERTY(QString countdown READ countdown NOTIFY stateChanged)
public:
    explicit OutputManager(QObject *parent = nullptr);
    QVariantList displays() const;
    QVariantList liveOrder() const;

    QString status() const { return m_status; }

    bool audienceEnabled() const { return m_audienceEnabled; }

    bool stageEnabled() const { return m_stageEnabled; }

    QString audienceDisplay() const { return m_audienceDisplay; }

    QString stageDisplay() const { return m_stageDisplay; }

    bool audienceBlack() const { return m_audienceBlack; }

    bool stageBlack() const { return m_stageBlack; }

    bool textCleared() const { return m_textCleared; }

    bool playing() const { return m_playing; }

    void setPlaying(bool value) {
        m_playing = value;
        emit stateChanged();
    }

    bool audioEnabled() const { return m_audioEnabled; }

    void setAudioEnabled(bool value) {
        m_audioEnabled = value;
        emit stateChanged();
    }

    QVariantList liveElements() const;
    QVariantList nextElements() const;

    QString snapshotId() const { return m_snapshotId; }

    QVariantList liveSlides() const { return m_liveSlides; }

    QString liveSource() const { return m_liveSource; }

    QString liveProgramItemId() const { return liveSlide()["programItemId"].toString(); }

    QString liveTitle() const;
    QString nextTitle() const;
    QString liveNotes() const;
    QString liveText() const;
    QString nextText() const;
    QString liveBackground() const;

    QString nextBackground() const {
        return liveSlide(1).value("background", "#000000").toString();
    }

    double liveAmbientOpacity() const {
        return m_index < 0
            ? 0
            : liveSlide().value("ambientOpacity", m_index == 0 ? 0.85 : 0.5).toDouble();
    }

    double nextAmbientOpacity() const {
        return m_index + 1 >= m_liveSlides.size()
            ? 0
            : liveSlide(1).value("ambientOpacity", 0.5).toDouble();
    }

    int liveIndex() const { return m_index; }

    int liveCount() const { return m_liveSlides.size(); }

    QString stageMessage() const { return m_stageMessage; }

    void setStageMessage(const QString &value) {
        m_stageMessage = value;
        emit stateChanged();
    }

    QString countdown() const;
    Q_INVOKABLE void bindWindows(QWindow *audience, QWindow *stage, QWindow *editor);
    Q_INVOKABLE bool route(const QString &role, const QString &displayId);
    Q_INVOKABLE void preview(const QString &role);
    Q_INVOKABLE void stop(const QString &role);
    Q_INVOKABLE void blackout(const QString &role, bool value);
    Q_INVOKABLE void clearText(bool value);
    Q_INVOKABLE void takeLive(
        const QVariantList &slides, int index, const QString &source = QString());
    Q_INVOKABLE void advance(int delta);
    Q_INVOKABLE void startCountdown(int minutes);
    Q_INVOKABLE void stopCountdown();
    static int selfTest();
signals:
    void displaysChanged();
    void stateChanged();
    void liveChanged();
    void snapshotChanged();

private:
    static QString screenId(QScreen *screen);
    static QString primaryText(const QVariantMap &slide);
    QVariantMap liveSlide(int offset = 0) const;
    void refreshDisplays();
    void screenRemoved(QScreen *screen);
    QPointer<QWindow> m_audience, m_stage, m_editor;
    QString m_audienceDisplay, m_stageDisplay, m_status = "Outputs stopped";
    bool m_audienceEnabled = false, m_stageEnabled = false, m_audienceBlack = false,
         m_stageBlack = false, m_textCleared = false, m_playing = true, m_audioEnabled = false;
    QSet<QString> m_reconnect;
    QSet<QScreen *> m_observed;
    QHash<QString, quint64> m_routeTokens;
    QSet<QString> m_fullscreenRoles;
    QVariantList m_liveSlides;
    int m_index = -1, m_remaining = 0;
    QString m_stageMessage, m_liveSource, m_snapshotId;
    QTimer m_timer;
};
