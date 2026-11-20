#include "OutputManager.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QGuiApplication>
#include <QSettings>
#include <QUuid>

OutputManager::OutputManager(QObject *parent)
    : QObject(parent) {
    QSettings settings;
    m_audienceDisplay = settings.value("outputs/audience").toString();
    m_stageDisplay = settings.value("outputs/stage").toString();
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

void OutputManager::blackout(const QString &role, bool value) {
    if (role == "audience") {
        m_audienceBlack = value;
    } else if (role == "stage") {
        m_stageBlack = value;
    } else {
        return;
    }
    emit stateChanged();
}

void OutputManager::clearText(bool value) {
    m_textCleared = value;
    emit stateChanged();
}

void OutputManager::takeLive(const QVariantList &slides, int index, const QString &source) {
    if (index < 0 || index >= slides.size()) {
        return;
    }
    m_snapshotId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_liveSource = source;
    m_liveSlides = slides;
    m_index = index;
    m_playing = true;
    emit liveChanged();
    emit stateChanged();
}

void OutputManager::advance(int delta) {
    int next = m_index + delta;
    if (next < 0 || next >= m_liveSlides.size()) {
        return;
    }
    m_index = next;
    emit liveChanged();
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
    for (const auto &value : slide["elements"].toList()) {
        const auto element = value.toMap();
        const auto binding = element["binding"].toString();
        if (element.value("visible", true).toBool()
            && (binding == "Primary lyrics" || binding == "Scripture text")) {
            return element["text"].toString();
        }
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
