#pragma once
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QScreen>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QWindow>

// Owns display routing and the immutable live snapshot, independent of editor drafts.
class OutputManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool transitioning READ transitioning NOTIFY transitionChanged)
    Q_PROPERTY(int transitionEffect READ transitionEffect NOTIFY transitionChanged)
    Q_PROPERTY(QVariantMap audienceFill READ audienceFill NOTIFY liveChanged)
    Q_PROPERTY(QVariantMap nextFill READ nextFill NOTIFY liveChanged)
    Q_PROPERTY(double transitionProgress READ transitionProgress NOTIFY transitionChanged)
    Q_PROPERTY(QVariantList liveOrder READ liveOrder NOTIFY snapshotChanged)
    Q_PROPERTY(QString snapshotId READ snapshotId NOTIFY snapshotChanged)
    Q_PROPERTY(bool canResumeProgram READ canResumeProgram NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantList displays READ displays NOTIFY displaysChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(bool audiencePreviewOpen READ audiencePreviewOpen NOTIFY stateChanged)
    Q_PROPERTY(bool stagePreviewOpen READ stagePreviewOpen NOTIFY stateChanged)
    Q_PROPERTY(bool audienceFullscreen READ audienceFullscreen NOTIFY stateChanged)
    Q_PROPERTY(bool stageFullscreen READ stageFullscreen NOTIFY stateChanged)
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
    Q_PROPERTY(QString liveProgramTitle READ liveProgramTitle NOTIFY snapshotChanged)
    Q_PROPERTY(QString nextTitle READ nextTitle NOTIFY liveChanged)
    Q_PROPERTY(bool musicianView READ musicianView WRITE setMusicianView NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap stageCurrent READ stageCurrent NOTIFY liveChanged)
    Q_PROPERTY(QVariantMap stageNext READ stageNext NOTIFY liveChanged)
    Q_PROPERTY(QString stageCue READ stageCue NOTIFY liveChanged)
    Q_PROPERTY(QString stageSection READ stageSection NOTIFY liveChanged)
    Q_PROPERTY(QString songKey READ songKey NOTIFY liveChanged)
    Q_PROPERTY(QString liveNotes READ liveNotes NOTIFY liveChanged)
    Q_PROPERTY(QString liveText READ liveText NOTIFY liveChanged)
    Q_PROPERTY(QString nextText READ nextText NOTIFY liveChanged)
    Q_PROPERTY(QString liveBackground READ liveBackground NOTIFY liveChanged)
    Q_PROPERTY(int liveIndex READ liveIndex NOTIFY liveChanged)
    Q_PROPERTY(int liveCount READ liveCount NOTIFY liveChanged)
    Q_PROPERTY(QString stageMessage READ stageMessage WRITE setStageMessage NOTIFY stateChanged)
    Q_PROPERTY(QString countdown READ countdown NOTIFY stateChanged)
    Q_PROPERTY(QString standbyMediaKind READ standbyMediaKind NOTIFY stateChanged)
    Q_PROPERTY(bool importingStandbyMedia READ importingStandbyMedia NOTIFY stateChanged)
    Q_PROPERTY(QUrl defaultImageSource READ defaultImageSource NOTIFY stateChanged)
    Q_PROPERTY(QString defaultImageName READ defaultImageName NOTIFY stateChanged)
    Q_PROPERTY(
        QString defaultImageFit READ defaultImageFit WRITE setDefaultImageFit NOTIFY stateChanged)
    Q_PROPERTY(bool defaultImageAvailable READ defaultImageAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool defaultImageShowing READ defaultImageShowing NOTIFY stateChanged)
    Q_PROPERTY(bool hasAudienceContent READ hasAudienceContent NOTIFY liveChanged)
    Q_PROPERTY(QVariantList audienceElements READ audienceElements NOTIFY liveChanged)
    Q_PROPERTY(QString audienceBackground READ audienceBackground NOTIFY liveChanged)
    Q_PROPERTY(double audienceAmbientOpacity READ audienceAmbientOpacity NOTIFY liveChanged)
    Q_PROPERTY(QString audienceTitle READ audienceTitle NOTIFY liveChanged)
public:
    explicit OutputManager(QObject *parent = nullptr);
    QVariantList displays() const;
    QVariantList liveOrder() const;

    int transitionEffect() const { return m_transitionEffect; }

    QVariantMap audienceFill() const {
        return defaultImageShowing() ? QVariantMap {} : liveSlide().value("backgroundFill").toMap();
    }

    QVariantMap nextFill() const { return liveSlide(1).value("backgroundFill").toMap(); }

    bool transitioning() const { return m_transitionTimer.isActive(); }

    double transitionProgress() const { return m_transitionProgress; }

    QString status() const { return m_status; }

    bool audienceFullscreen() const {
        return m_audienceEnabled && m_fullscreenRoles.contains("audience");
    }

    bool stageFullscreen() const { return m_stageEnabled && m_fullscreenRoles.contains("stage"); }

    bool audiencePreviewOpen() const { return m_audienceEnabled && !audienceFullscreen(); }

    bool stagePreviewOpen() const { return m_stageEnabled && !stageFullscreen(); }

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

    bool canResumeProgram() const { return m_resumeIndex >= 0; }

    QVariantList liveSlides() const { return m_liveSlides; }

    QString liveSource() const { return m_liveSource; }

    QString liveProgramItemId() const { return liveSlide()["programItemId"].toString(); }

    QString liveTitle() const;

    QString liveProgramTitle() const {
        return m_liveSlides.value(0).toMap().value("programTitle", "Live presentation").toString();
    }

    QString nextTitle() const;

    bool musicianView() const { return m_musicianView; }

    void setMusicianView(bool value);
    QVariantMap stageCurrent() const;
    QVariantMap stageNext() const;

    QString stageCue() const { return liveSlide().value("stageCue").toString(); }

    QString stageSection() const { return liveSlide().value("stageSection").toString(); }

    QString songKey() const { return liveSlide().value("songKey").toString(); }

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

    QString standbyMediaKind() const;

    bool importingStandbyMedia() const { return m_importingStandbyMedia; }

    Q_INVOKABLE void importStandbyMedia(const QUrl &source);

    QUrl defaultImageSource() const { return QUrl(m_defaultImageSource); }

    QString defaultImageName() const { return m_defaultImageName; }

    QString defaultImageFit() const { return m_defaultImageFit; }

    bool defaultImageAvailable() const;

    bool defaultImageShowing() const { return !m_defaultImageLive.isEmpty(); }

    bool hasAudienceContent() const { return defaultImageShowing() || liveCount() > 0; }

    QVariantList audienceElements() const;
    QString audienceBackground() const;
    double audienceAmbientOpacity() const;
    QString audienceTitle() const;
    void setDefaultImageFit(const QString &fit);
    Q_INVOKABLE bool setDefaultImage(const QUrl &source);
    Q_INVOKABLE void removeDefaultImage();
    Q_INVOKABLE bool showDefaultImage(bool showing);
    Q_INVOKABLE void bindWindows(QWindow *audience, QWindow *stage, QWindow *editor);
    Q_INVOKABLE bool route(const QString &role, const QString &displayId);
    Q_INVOKABLE void preview(const QString &role);
    Q_INVOKABLE void stop(const QString &role);
    Q_INVOKABLE void stopPresentation();
    Q_INVOKABLE void blackout(const QString &role, bool value);
    Q_INVOKABLE void clearText(bool value);
    Q_INVOKABLE void takeLive(
        const QVariantList &slides, int index, const QString &source = QString());
    Q_INVOKABLE void advance(int delta);
    Q_INVOKABLE bool addQuickSlides(const QVariantList &slides, const QString &title, bool now,
        const QString &scriptureKey = QString());
    Q_INVOKABLE bool resumeProgram();
    Q_INVOKABLE void startCountdown(int minutes);
    Q_INVOKABLE void stopCountdown();

    bool cueFullscreen(const QString &role) const { return m_fullscreenRoles.contains(role); }

    bool hasCueWindow(const QString &role) const {
        return role == "audience" ? !m_audience.isNull() : !m_stage.isNull();
    }

    void setCueTransition(const QString &effect, int durationMs);
    bool setCueOverlay(const QUrl &source);
    void clearCueOverlay();
    QVariantMap sessionSnapshot() const;
    bool restoreSession(const QVariantMap &snapshot);
    static int selfTest();
signals:
    void sessionEnded();
    void transitionStarting();
    void transitionChanged();
    void displaysChanged();
    void stateChanged();
    void liveChanged();
    void snapshotChanged();

private:
    void initializeTransitions();
    void prepareTransition(const QVariantMap &target);
    void finishTransition();
    static QString screenId(QScreen *screen);
    static QString primaryText(const QVariantMap &slide);
    QVariantMap liveSlide(int offset = 0) const;
    void refreshDisplays();
    void screenRemoved(QScreen *screen);
    QPointer<QWindow> m_audience, m_stage, m_editor;
    QString m_audienceDisplay, m_stageDisplay, m_status = "Outputs stopped";
    bool m_musicianView = false;
    bool m_audienceEnabled = false, m_stageEnabled = false, m_audienceBlack = false,
         m_stageBlack = false, m_textCleared = false, m_playing = true, m_audioEnabled = false;
    QSet<QString> m_reconnect;
    QSet<QScreen *> m_observed;
    QHash<QString, quint64> m_routeTokens;
    QSet<QString> m_fullscreenRoles;
    QVariantMap m_cueTransition, m_cueOverlay;
    QVariantList m_liveSlides;
    int m_index = -1, m_remaining = 0;
    int m_resumeIndex = -1;
    QString m_stageMessage, m_liveSource, m_snapshotId;
    bool m_importingStandbyMedia = false;
    int m_standbyGeneration = 0;
    QString m_defaultImageSource, m_defaultImageName, m_defaultImageFit = "contain";
    QVariantList m_defaultImageLive;
    QTimer m_timer, m_transitionTimer;
    QElapsedTimer m_transitionClock;
    double m_transitionProgress = 1;
    int m_transitionDuration = 300;
    int m_transitionEffect = 0;
};
