#include "OutputManager.h"
#include "DefaultImage.h"
#include "LiveAudio.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QSettings>
#include <QtConcurrentRun>
#include <QUuid>

OutputManager::OutputManager(QObject *parent)
    : QObject(parent) {
    initializeTransitions();
    auto audio = new LiveAudio(this);
    auto syncAudio = [this, audio] {
        audio->sync(
            defaultImageShowing() ? QVariantList {} : liveElements(), m_playing, m_audioEnabled);
    };
    connect(this, &OutputManager::liveChanged, audio, syncAudio);
    connect(this, &OutputManager::stateChanged, audio, syncAudio);
    connect(audio, &LiveAudio::problem, this, [this](const QString &message) {
        m_status = message;
        emit stateChanged();
    });
    QSettings settings;
    m_audienceDisplay = settings.value("outputs/audience").toString();
    m_musicianView = settings.value("outputs/musicianView", false).toBool();
    m_stageDisplay = settings.value("outputs/stage").toString();
    m_defaultImageSource = settings.value("outputs/defaultImage").toString();
    m_defaultImageName = settings.value("outputs/defaultImageName").toString();
    m_defaultImageFit = settings.value("outputs/defaultImageFit", "contain").toString();
    if (m_defaultImageFit != "contain" && m_defaultImageFit != "cover") {
        m_defaultImageFit = "contain";
    }
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *) { refreshDisplays(); });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &OutputManager::screenRemoved,
        Qt::DirectConnection);
    connect(
        qGuiApp, &QGuiApplication::primaryScreenChanged, this, [this] { emit displaysChanged(); });
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (m_remaining > 0) {
            --m_remaining;
        } else {
            m_timer.stop();
        }
        emit stateChanged();
    });
    refreshDisplays();
}

QString OutputManager::screenId(QScreen *s) {
    return QString::fromLatin1(QCryptographicHash::hash(
        (s->manufacturer() + "|" + s->model() + "|" + s->serialNumber() + "|" + s->name()).toUtf8(),
        QCryptographicHash::Sha256)
            .toHex()
            .left(24));
}

QVariantList OutputManager::displays() const {
    QVariantList list;
    int index = 0;
    for (auto screen : QGuiApplication::screens()) {
        auto geometry = screen->geometry();
        list.append(QVariantMap { { "id", screenId(screen) },
            { "name",
                QString("Display %1 · %2 · %3 × %4")
                    .arg(++index)
                    .arg(screen->name())
                    .arg(geometry.width())
                    .arg(geometry.height()) },
            { "operator", m_editor && m_editor->screen() == screen }, { "width", geometry.width() },
            { "height", geometry.height() } });
    }
    return list;
}

void OutputManager::bindWindows(QWindow *a, QWindow *s, QWindow *editor) {
    m_audience = a;
    m_stage = s;
    m_editor = editor;
    if (a) {
        a->setTransientParent(nullptr);
    }
    if (s) {
        s->setTransientParent(nullptr);
    }
    for (const auto &role : QStringList { "audience", "stage" }) {
        auto window = role == "audience" ? a : s;
        if (!window) {
            continue;
        }
        connect(window, &QWindow::visibilityChanged, this,
            [this, window, role](QWindow::Visibility visibility) {
                if (window != (role == "audience" ? m_audience : m_stage)) {
                    return;
                }
                const bool enabled = role == "audience" ? m_audienceEnabled : m_stageEnabled;
                if (enabled && visibility == QWindow::FullScreen) {
                    m_fullscreenRoles.insert(role);
                } else if (enabled && visibility == QWindow::Windowed) {
                    m_fullscreenRoles.remove(role);
                }
                emit stateChanged();
            });
        connect(window, &QObject::destroyed, this, [this, role] {
            if (role == "audience" && m_audience.isNull()) {
                m_audienceEnabled = false;
            }
            if (role == "stage" && m_stage.isNull()) {
                m_stageEnabled = false;
            }
            emit stateChanged();
        });
    }
    if (editor) {
        connect(editor, &QWindow::screenChanged, this, [this] { emit displaysChanged(); });
    }
    emit displaysChanged();
}

void OutputManager::refreshDisplays() {
    for (auto screen : QGuiApplication::screens()) {
        if (m_observed.contains(screen)) {
            continue;
        }
        m_observed.insert(screen);
        connect(screen, &QScreen::geometryChanged, this, [this, screen] {
            for (auto window : { m_audience, m_stage }) {
                if (window && window->isVisible() && window->visibility() == QWindow::FullScreen
                    && window->screen() == screen) {
                    window->setGeometry(screen->geometry());
                }
            }
            emit displaysChanged();
        });
    }
    emit displaysChanged();
    for (const auto &role : QStringList { "audience", "stage" }) {
        if (m_reconnect.contains(role)) {
            const auto desired = role == "audience" ? m_audienceDisplay : m_stageDisplay;
            bool found = false;
            for (auto screen : QGuiApplication::screens()) {
                found = found || screenId(screen) == desired;
            }
            if (found && route(role, desired)) {
                m_reconnect.remove(role);
            }
        }
    }
}

void OutputManager::screenRemoved(QScreen *screen) {
    m_observed.remove(screen);
    for (const auto &role : QStringList { "audience", "stage" }) {
        auto window = role == "audience" ? m_audience : m_stage;
        if (window && window->isVisible() && window->screen() == screen) {
            const bool fullscreen
                = window->visibility() == QWindow::FullScreen || m_fullscreenRoles.contains(role);
            ++m_routeTokens[role];
            m_fullscreenRoles.remove(role);
            window->hide();
            if (role == "audience") {
                m_audienceEnabled = false;
            } else {
                m_stageEnabled = false;
            }
            if (fullscreen) {
                m_reconnect.insert(role);
            }
            m_status = "Display disconnected — output hidden";
        }
    }
    emit stateChanged();
    QTimer::singleShot(0, this, [this] { emit displaysChanged(); });
}

bool OutputManager::route(const QString &role, const QString &id) {
    if (role != "audience" && role != "stage") {
        return false;
    }
    QScreen *target = nullptr;
    for (auto s : QGuiApplication::screens()) {
        if (screenId(s) == id) {
            target = s;
        }
    }
    auto window = role == "audience" ? m_audience : m_stage;
    auto other = role == "audience" ? m_stage : m_audience;
    if (!target || !window) {
        m_status = "Assigned display is unavailable";
        emit stateChanged();
        return false;
    }
    if (m_editor && m_editor->screen() == target) {
        m_status = "Use another display for fullscreen output; use Preview on this screen";
        emit stateChanged();
        return false;
    }
    if (other && other->isVisible() && other->visibility() == QWindow::FullScreen
        && other->screen() == target) {
        m_status = "That display already has another output";
        emit stateChanged();
        return false;
    }
    const auto otherRole = role == "audience" ? QString("stage") : QString("audience");
    const auto otherDisplay = role == "audience" ? m_stageDisplay : m_audienceDisplay;
    if (m_fullscreenRoles.contains(otherRole) && otherDisplay == id) {
        m_status = "That display already has another output";
        emit stateChanged();
        return false;
    }
    const auto token = ++m_routeTokens[role];
    m_fullscreenRoles.insert(role);
    if (role == "audience") {
        m_audienceEnabled = false;
    } else {
        m_stageEnabled = false;
    }
    window->hide();
    window->setScreen(target);
    window->setGeometry(target->geometry());
    window->showNormal();
    // macOS needs a native window positioned on the chosen screen before entering
    // its fullscreen Space. Starting fullscreen on an unrealized window can use
    // the primary screen even after setScreen(). The token cancels stale requests.
    window->setScreen(target);
    window->setGeometry(target->geometry());
    if (role == "audience") {
        m_audienceEnabled = true;
        m_audienceDisplay = id;
    } else {
        m_stageEnabled = true;
        m_stageDisplay = id;
    }
    QSettings().setValue("outputs/" + role, id);
    m_status = "Positioning " + role + " output…";
    emit stateChanged();
    QPointer<QScreen> screenGuard = target;
    QTimer::singleShot(150, this, [this, window, screenGuard, role, token] {
        if (m_routeTokens.value(role) != token || !window || !screenGuard) {
            return;
        }
        if (m_editor && m_editor->screen() == screenGuard) {
            stop(role);
            m_status = "Output stopped: that display is now the operator screen";
            emit stateChanged();
            return;
        }
        window->setScreen(screenGuard);
        window->setGeometry(screenGuard->geometry());
        window->showFullScreen();
        m_status = role + " fullscreen enabled";
        emit stateChanged();
        if (m_editor) {
            m_editor->requestActivate();
        }
    });
    return true;
}

void OutputManager::preview(const QString &role) {
    if (role != "audience" && role != "stage") {
        return;
    }
    auto window = role == "audience" ? m_audience : m_stage;
    if (!window) {
        return;
    }
    ++m_routeTokens[role];
    m_fullscreenRoles.remove(role);
    m_reconnect.remove(role);
    window->hide();
    window->setScreen(m_editor ? m_editor->screen() : QGuiApplication::primaryScreen());
    auto area = window->screen()->availableGeometry();
    window->setGeometry(area.x() + 50, area.y() + 60, qMin(960, area.width() - 100),
        qMin(540, area.height() - 120));
    window->showNormal();
    if (role == "audience") {
        m_audienceEnabled = true;
    } else {
        m_stageEnabled = true;
    }
    m_status = role + " windowed preview";
    emit stateChanged();
}

void OutputManager::stop(const QString &role) {
    if (role != "audience" && role != "stage") {
        return;
    }
    ++m_routeTokens[role];
    m_fullscreenRoles.remove(role);
    m_reconnect.remove(role);
    auto window = role == "audience" ? m_audience : m_stage;
    if (window) {
        window->hide();
    }
    if (role == "audience") {
        m_audienceEnabled = false;
    } else {
        m_stageEnabled = false;
    }
    m_status = role + " output stopped";
    emit stateChanged();
}

void OutputManager::stopPresentation() {
    finishTransition();
    stop("audience");
    stop("stage");
    m_playing = false;
    m_timer.stop();
    clearCueOverlay();
    m_status = "Presentation stopped";
    emit sessionEnded();
    // Keep the prepared snapshot and position available for the next session.
    emit stateChanged();
}

void OutputManager::blackout(const QString &role, bool value) {
    if (role == "audience") {
        finishTransition();
        m_audienceBlack = value;
    } else if (role == "stage") {
        m_stageBlack = value;
    } else {
        return;
    }
    emit stateChanged();
}

void OutputManager::clearText(bool value) {
    finishTransition();
    m_textCleared = value;
    emit stateChanged();
}

void OutputManager::takeLive(const QVariantList &slides, int index, const QString &source) {
    if (index < 0 || index >= slides.size()) {
        return;
    }
    prepareTransition(slides[index].toMap());
    m_resumeIndex = -1;
    m_cueTransition.clear();
    m_cueOverlay.clear();
    m_snapshotId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_liveSource = source;
    m_liveSlides = slides;
    m_index = index;
    m_defaultImageLive.clear();
    m_playing = true;
    emit snapshotChanged();
    emit liveChanged();
    emit stateChanged();
}

bool OutputManager::addQuickSlides(
    const QVariantList &slides, const QString &title, bool now, const QString &scriptureKey) {
    if (!scriptureKey.isEmpty()) {
        for (int i = 0; i < m_liveSlides.size(); ++i) {
            if (m_liveSlides[i].toMap()["scriptureKey"].toString() == scriptureKey) {
                if (m_resumeIndex < 0 && m_index >= 0 && !liveSlide()["quickAddition"].toBool()) {
                    m_resumeIndex = m_index;
                    emit snapshotChanged();
                }
                advance(i - m_index);
                return true;
            }
        }
    }
    if ((!now && m_index < 0) || slides.isEmpty() || slides.size() > 500
        || m_liveSlides.size() + slides.size() > 5000) {
        return false;
    }
    for (const auto &value : slides) {
        if (!value.toMap().contains("elements")) {
            return false;
        }
    }
    int insertion = m_index + 1;
    // Queue after the complete service item, not between its lyric slides.
    const auto item = liveProgramItemId();
    if (!item.isEmpty()) {
        while (insertion < m_liveSlides.size()
            && m_liveSlides[insertion].toMap()["programItemId"].toString() == item) {
            ++insertion;
        }
    }
    if (!now) {
        while (insertion < m_liveSlides.size()
            && m_liveSlides[insertion].toMap()["quickAddition"].toBool()) {
            ++insertion;
        }
    }
    if (now && m_resumeIndex < 0 && m_index >= 0 && !liveSlide()["quickAddition"].toBool()) {
        m_resumeIndex = m_index;
    }
    if (m_resumeIndex >= insertion) {
        m_resumeIndex += slides.size();
    }
    const auto group = scriptureKey.isEmpty()
        ? "quick-" + QUuid::createUuid().toString(QUuid::WithoutBraces)
        : QStringLiteral("scripture-session");
    const auto programTitle = liveProgramTitle();
    for (int i = 0; i < slides.size(); ++i) {
        auto slide = slides[i].toMap();
        slide["programItemId"] = group;
        slide["programItemTitle"] = title;
        slide["programTitle"] = programTitle;
        slide["quickAddition"] = true;
        if (!scriptureKey.isEmpty()) {
            slide["scriptureKey"] = scriptureKey;
        }
        m_liveSlides.insert(insertion + i, slide);
    }
    m_snapshotId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    emit snapshotChanged();
    if (now || m_index < 0) {
        advance(insertion - m_index);
    } else {
        emit liveChanged();
    }
    m_status = now ? "Quick addition is live" : "Quick addition queued after this item";
    emit stateChanged();
    return true;
}

bool OutputManager::resumeProgram() {
    if (m_resumeIndex < 0 || m_resumeIndex >= m_liveSlides.size()) {
        return false;
    }
    const int index = m_resumeIndex;
    m_resumeIndex = -1;
    advance(index - m_index);
    emit snapshotChanged();
    return true;
}

void OutputManager::advance(int delta) {
    int next = m_index + delta;
    if (next < 0 || next >= m_liveSlides.size()) {
        return;
    }
    if (next == m_index && !defaultImageShowing()) {
        return;
    }
    if (m_liveSlides[next].toMap()["quickAddition"].toBool()) {
        if (m_resumeIndex < 0 && m_index >= 0) {
            int resume = next + 1;
            while (resume < m_liveSlides.size()
                && m_liveSlides[resume].toMap()["quickAddition"].toBool()) {
                ++resume;
            }
            if (resume < m_liveSlides.size()
                || !m_liveSlides[m_index].toMap()["quickAddition"].toBool()) {
                m_resumeIndex = resume < m_liveSlides.size() ? resume : m_index;
                emit snapshotChanged();
            }
        }
    } else if (m_resumeIndex >= 0) {
        m_resumeIndex = -1;
        emit snapshotChanged();
    }
    prepareTransition(m_liveSlides[next].toMap());
    m_index = next;
    m_defaultImageLive.clear();
    emit liveChanged();
    emit stateChanged();
}

bool OutputManager::defaultImageAvailable() const {
    return QFileInfo(QUrl(m_defaultImageSource).toLocalFile()).isFile();
}

bool OutputManager::setDefaultImage(const QUrl &source) {
    QString error;
    const auto imported = DefaultImage::prepare(source, &error);
    if (imported.isEmpty()) {
        m_status = error;
        emit stateChanged();
        return false;
    }
    ++m_standbyGeneration;
    m_defaultImageSource = imported;
    m_defaultImageName = QFileInfo(source.toLocalFile()).fileName();
    QSettings settings;
    settings.setValue("outputs/defaultImage", imported);
    settings.setValue("outputs/defaultImageName", m_defaultImageName);
    m_status = "Standby media saved · choose Show standby media to display it";
    emit stateChanged();
    return true;
}

void OutputManager::setDefaultImageFit(const QString &fit) {
    if (fit != "contain" && fit != "cover") {
        return;
    }
    m_defaultImageFit = fit;
    QSettings().setValue("outputs/defaultImageFit", fit);
    emit stateChanged();
}

void OutputManager::removeDefaultImage() {
    ++m_standbyGeneration;
    m_defaultImageSource.clear();
    m_defaultImageName.clear();
    QSettings settings;
    settings.remove("outputs/defaultImage");
    settings.remove("outputs/defaultImageName");
    m_status = "Standby media removed";
    emit stateChanged();
}

bool OutputManager::showDefaultImage(bool showing) {
    if (showing && !defaultImageAvailable()) {
        m_status = "Choose standby media first; the saved file may be missing";
        emit stateChanged();
        return false;
    }
    finishTransition();
    // Keep the service snapshot, notes, index and transport state intact behind standby.
    m_defaultImageLive = showing
        ? QVariantList { DefaultImage::layer(m_defaultImageSource, m_defaultImageFit) }
        : QVariantList();
    m_status = showing ? (m_audienceBlack ? "Standby media selected · audience is blacked out"
                                          : "Standby media is showing")
                       : "Presentation restored";
    emit liveChanged();
    emit stateChanged();
    return true;
}

QVariantList OutputManager::audienceElements() const {
    auto elements = defaultImageShowing() ? m_defaultImageLive : liveElements();
    bool audioOnly = !elements.isEmpty();
    for (const auto &value : elements) {
        audioOnly &= value.toMap()["kind"] == "audio";
    }
    if (audioOnly && defaultImageAvailable()) {
        elements.prepend(DefaultImage::layer(m_defaultImageSource, m_defaultImageFit));
    }
    if (!m_cueOverlay.isEmpty()) {
        elements.append(m_cueOverlay);
    }
    return elements;
}

QString OutputManager::audienceBackground() const {
    return defaultImageShowing() ? "#000000" : liveBackground();
}

double OutputManager::audienceAmbientOpacity() const {
    return defaultImageShowing() ? 0 : liveAmbientOpacity();
}

QString OutputManager::audienceTitle() const {
    return defaultImageShowing() ? "Standby media" : liveTitle();
}

QVariantMap OutputManager::liveSlide(int offset) const {
    const int i = m_index + offset;
    return i >= 0 && i < m_liveSlides.size() ? m_liveSlides[i].toMap() : QVariantMap();
}

QVariantList OutputManager::liveElements() const {
    return liveSlide()["elements"].toList();
}

QVariantList OutputManager::nextElements() const {
    return liveSlide(1)["elements"].toList();
}

QString OutputManager::liveTitle() const {
    return liveSlide()["name"].toString();
}

QString OutputManager::nextTitle() const {
    return liveSlide(1)["name"].toString();
}

QString OutputManager::liveNotes() const {
    return liveSlide()["notes"].toString();
}

QString OutputManager::liveBackground() const {
    return liveSlide().value("background", "#000000").toString();
}

QString OutputManager::primaryText(const QVariantMap &slide) {
    QString scripture, secondary;
    bool hasScripture = false;
    for (const auto &value : slide["elements"].toList()) {
        const auto element = value.toMap();
        if (!element.value("visible", true).toBool()) {
            continue;
        }
        const auto binding = element["binding"].toString();
        if (binding == "Primary lyrics") {
            return element["text"].toString();
        }
        if (binding == "Scripture text" && !hasScripture) {
            scripture = element["text"].toString();
            hasScripture = true;
        }
        if (binding == "Secondary scripture text" && secondary.isEmpty()) {
            secondary = element["text"].toString();
        }
    }
    // Speaker displays and remote text views must retain both Bible translations too.
    if (hasScripture) {
        return scripture + (secondary.isEmpty() ? QString() : "\n\n" + secondary);
    }
    double largest = 0;
    QString result;
    for (const auto &v : slide["elements"].toList()) {
        auto e = v.toMap();
        if (e.value("kind", "text") == "text" && e.value("visible", true).toBool()
            && e["fontSize"].toDouble() > largest) {
            largest = e["fontSize"].toDouble();
            result = e["text"].toString();
        }
    }
    return result;
}

QString OutputManager::liveText() const {
    return primaryText(liveSlide());
}

QString OutputManager::nextText() const {
    return primaryText(liveSlide(1));
}

QString OutputManager::countdown() const {
    return QString("%1:%2")
        .arg(m_remaining / 60, 2, 10, QChar('0'))
        .arg(m_remaining % 60, 2, 10, QChar('0'));
}

void OutputManager::startCountdown(int minutes) {
    m_remaining = qBound(1, minutes, 180) * 60;
    m_timer.start();
    emit stateChanged();
}

void OutputManager::stopCountdown() {
    m_timer.stop();
    emit stateChanged();
}

QVariantList OutputManager::liveOrder() const {
    QVariantList result;
    for (int index = 0; index < m_liveSlides.size(); ++index) {
        const auto slide = m_liveSlides[index].toMap();
        result.append(QVariantMap { { "index", index }, { "title", slide.value("name") },
            { "quickAddition", slide.value("quickAddition", false) },
            { "itemId", slide.value("programItemId") },
            { "itemTitle", slide.value("programItemTitle") },
            { "section", slide.value("programSection") } });
    }
    return result;
}

void OutputManager::setMusicianView(bool value) {
    m_musicianView = value;
    QSettings().setValue("outputs/musicianView", value);
    emit stateChanged();
}

QVariantMap OutputManager::stageCurrent() const {
    return { { "text", liveText() },
        { "chords",
            liveSlide().value("stageLyrics").toString() == liveText()
                ? liveSlide().value("stageChords")
                : QVariant() } };
}

QVariantMap OutputManager::stageNext() const {
    return { { "text", nextText() },
        { "chords",
            liveSlide(1).value("stageLyrics").toString() == nextText()
                ? liveSlide(1).value("stageChords")
                : QVariant() } };
}

bool OutputManager::setCueOverlay(const QUrl &source) {
    if (!source.isLocalFile() || !QFileInfo(source.toLocalFile()).isFile()) {
        return false;
    }
    finishTransition();
    m_cueOverlay = DefaultImage::layer(source.toString(), "contain");
    m_cueOverlay["uid"] = "cue-overlay";
    m_cueOverlay["name"] = "Cue overlay";
    emit liveChanged();
    return true;
}

void OutputManager::clearCueOverlay() {
    if (m_cueOverlay.isEmpty()) {
        return;
    }
    finishTransition();
    m_cueOverlay.clear();
    emit liveChanged();
}

QString OutputManager::standbyMediaKind() const {
    return DefaultImage::kind(defaultImageSource());
}

void OutputManager::importStandbyMedia(const QUrl &source) {
    if (m_importingStandbyMedia) {
        return;
    }
    m_importingStandbyMedia = true;
    const int generation = ++m_standbyGeneration;
    m_status = "Importing standby media…";
    emit stateChanged();
    auto watcher = new QFutureWatcher<QVariantMap>(this);
    connect(
        watcher, &QFutureWatcher<QVariantMap>::finished, this, [this, watcher, source, generation] {
            const auto result = watcher->result();
            watcher->deleteLater();
            m_importingStandbyMedia = false;
            if (generation != m_standbyGeneration) {
                emit stateChanged();
                return;
            }
            if (!result["error"].toString().isEmpty()) {
                m_status = result["error"].toString();
                emit stateChanged();
                return;
            }
            m_defaultImageSource = result["source"].toString();
            m_defaultImageName = QFileInfo(source.toLocalFile()).fileName();
            QSettings settings;
            settings.setValue("outputs/defaultImage", m_defaultImageSource);
            settings.setValue("outputs/defaultImageName", m_defaultImageName);
            m_status = "Standby media saved · choose Show standby media to display it";
            emit stateChanged();
        });
    watcher->setFuture(QtConcurrent::run([source] {
        QString error;
        const auto imported = DefaultImage::prepare(source, &error);
        return QVariantMap { { "source", imported }, { "error", error } };
    }));
}
