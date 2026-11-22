#include "MediaLibrary.h"
#include "OutputManager.h"
#include "RemoteController.h"
#include "RemotePreview.h"
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace {
RemoteResponse json(int status, const QVariantMap &value) {
    return { status, "application/json; charset=utf-8",
        QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), {} };
}

RemoteResponse error(int status, const QString &message) {
    return json(status, { { "error", message } });
}

QVariantMap body(const RemoteRequest &request) {
    return QJsonDocument::fromJson(request.body).object().toVariantMap();
}

bool number(const QVariant &value) {
    return value.metaType().id() == QMetaType::Double
        || value.metaType().id() == QMetaType::LongLong;
}

QString cookie(const RemoteRequest &request) {
    for (const auto &part : request.headers.value("cookie").split(';')) {
        const auto entry = part.trimmed();
        if (entry.startsWith("churchdeck=")) {
            return QString::fromLatin1(entry.mid(11));
        }
    }
    return {};
}

QString translation(const QVariantMap &slide) {
    QStringList result;
    for (const auto &value : slide.value("elements").toList()) {
        const auto element = value.toMap();
        if (element.value("visible", true).toBool()
            && element.value("binding").toString() == "Translated lyrics") {
            result.append(element.value("text").toString());
        }
    }
    return result.join('\n');
}
}

RemoteResponse RemoteController::pair(const RemoteRequest &request) {
    const auto now = m_clock.elapsed();
    // Retain at most the connection cap's worth of peers; a global limit also bounds distributed
    // guessing.
    if (m_attempts.size() > 64) {
        m_attempts.clear();
    }
    auto &attempt = m_attempts[request.peer];
    if (now - attempt.start >= 60000) {
        attempt = { now, 0 };
    }
    if (now - m_globalAttempts.start >= 60000) {
        m_globalAttempts = { now, 0 };
    }
    if (attempt.count >= 5 || m_globalAttempts.count >= 40) {
        return error(429, "Too many pairing attempts. Wait one minute.");
    }
    ++attempt.count;
    ++m_globalAttempts.count;
    const auto value = body(request);
    const auto code = value.value("code").toString();
    const QString role = code == m_controlCode ? "controller"
        : code == m_speakerCode                ? "speaker"
                                               : "";
    if (role.isEmpty()) {
        return error(401, "Pairing code is incorrect or has changed.");
    }
    expire();
    const auto previous = cookie(request);
    if (!m_sessions.contains(previous) && m_sessions.size() >= 24) {
        return error(
            429, "All 24 remote places are occupied. Ask the operator to remove a device.");
    }
    QString name = value.value("name").toString().simplified().left(60);
    for (auto &character : name) {
        if (character.category() == QChar::Other_Control) {
            character = QChar(' ');
        }
    }
    if (name.isEmpty()) {
        name = "Remote device";
    }
    m_sessions.remove(previous);
    const auto token = randomToken();
    m_sessions.insert(token,
        { QUuid::createUuid().toString(QUuid::WithoutBraces), name, role, now, now, {}, {} });
    auto response = json(200, { { "role", role } });
    response.headers.insert(
        "Set-Cookie", "churchdeck=" + token.toLatin1() + "; HttpOnly; SameSite=Strict; Path=/");
    emit changed();
    return response;
}

QVariantMap RemoteController::state(const Session &session) const {
    const auto slides = m_outputs->liveSlides();
    const auto index = m_outputs->liveIndex();
    return { { "role", session.role }, { "snapshot", m_outputs->snapshotId() }, { "index", index },
        { "count", slides.size() }, { "title", m_outputs->liveTitle() },
        { "nextTitle", m_outputs->nextTitle() }, { "text", m_outputs->liveText() },
        { "nextText", m_outputs->nextText() },
        { "translation", translation(slides.value(index).toMap()) },
        { "nextTranslation", translation(slides.value(index + 1).toMap()) },
        { "notes", m_outputs->liveNotes() }, { "message", m_outputs->stageMessage() },
        { "countdown", m_outputs->countdown() }, { "blackout", m_outputs->audienceBlack() },
        { "textCleared", m_outputs->textCleared() },
        { "defaultImageShowing", m_outputs->defaultImageShowing() },
        { "audienceEnabled", m_outputs->audienceEnabled() },
        { "hostTime", QDateTime::currentMSecsSinceEpoch() } };
}

QVariantMap RemoteController::deck() const {
    QVariantList items;
    QString last;
    const auto slides = m_outputs->liveSlides();
    for (int i = 0; i < slides.size(); ++i) {
        const auto slide = slides[i].toMap();
        const auto id = slide.value("programItemId", QString::number(i)).toString();
        if (items.isEmpty() || id != last) {
            items.append(
                QVariantMap { { "title", slide.value("programItemTitle", slide.value("name")) },
                    { "start", i }, { "slides", QVariantList {} } });
            last = id;
        }
        auto item = items.last().toMap();
        auto pages = item["slides"].toList();
        pages.append(QVariantMap { { "index", i }, { "title", slide.value("name") } });
        item["slides"] = pages;
        items.last() = item;
    }
    return { { "snapshot", m_outputs->snapshotId() },
        { "title", slides.value(0).toMap().value("programTitle", "Live presentation") },
        { "items", items } };
}

RemoteResponse RemoteController::command(const RemoteRequest &request, Session &session) {
    if (session.role != "controller") {
        return error(403, "Speaker view is read-only.");
    }
    const auto value = body(request);
    const auto id = value.value("requestId").toString();
    if (id.size() < 8 || id.size() > 80) {
        return error(400, "A command identifier is required.");
    }
    const auto fingerprint = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
    if (session.commands.contains(id)) {
        if (session.commands[id] != fingerprint) {
            return error(409, "Command identifier already used.");
        }
        return json(200, state(session));
    }
    if (value.value("snapshot").toString() != m_outputs->snapshotId()
        || m_outputs->liveIndex() < 0) {
        return error(409, "The live presentation changed. Refresh before continuing.");
    }
    const auto action = value.value("action").toString();
    if (action == "blackout") {
        if (value.value("value").metaType().id() != QMetaType::Bool) {
            return error(400, "Blackout requires a boolean.");
        }
        m_outputs->blackout("audience", value.value("value").toBool());
    } else if (action == "next" || action == "previous" || action == "jump") {
        if (!number(value.value("expectedIndex"))
            || value.value("expectedIndex").toDouble() != m_outputs->liveIndex()) {
            return error(409, "Another operator moved the live slide. Refresh before continuing.");
        }
        const auto target = action == "next" ? m_outputs->liveIndex() + 1
            : action == "previous"           ? m_outputs->liveIndex() - 1
                                             : value.value("index", -1).toInt();
        if (target < 0 || target >= m_outputs->liveCount()
            || (action == "jump"
                && (!number(value.value("index")) || value.value("index").toDouble() != target))) {
            return error(400, "Slide is outside the live presentation.");
        }
        m_outputs->advance(target - m_outputs->liveIndex());
    } else {
        return error(400, "Unknown command.");
    }
    if (session.commands.size() >= 64) {
        session.commands.remove(session.commandOrder.takeFirst());
    }
    session.commands.insert(id, fingerprint);
    session.commandOrder.append(id);
    return json(200, state(session));
}

RemoteResponse RemoteController::handle(const RemoteRequest &request) {
    if (!running() || request.headers.value("host") != m_host.toLatin1()) {
        return error(403, "Invalid host.");
    }
    if (request.method != "GET" && request.method != "POST") {
        return error(405, "Method not allowed.");
    }
    if (request.method == "POST"
        && (request.headers.value("origin") != m_address.toLatin1()
            || request.headers.value("content-type").split(';').first().trimmed()
                != "application/json")) {
        return error(403, "This action must come from the paired remote page.");
    }
    const auto route = request.target;
    if (request.method == "GET") {
        static const QHash<QByteArray, QPair<QString, QByteArray>> assets {
            { "/", { ":/remote/index.html", "text/html; charset=utf-8" } },
            { "/app.js", { ":/remote/app.js", "application/javascript; charset=utf-8" } },
            { "/style.css", { ":/remote/style.css", "text/css; charset=utf-8" } },
            { "/telugu.ttf",
                { ":/qt/qml/ChurchDeck/assets/fonts/NotoSansTelugu.ttf", "font/ttf" } },
            { "/hindi.ttf",
                { ":/qt/qml/ChurchDeck/assets/fonts/NotoSansDevanagari.ttf", "font/ttf" } }
        };
        if (assets.contains(route)) {
            const auto asset = assets.value(route);
            QFile file(asset.first);
            if (!file.open(QIODevice::ReadOnly)) {
                return error(500, "Bundled asset unavailable.");
            }
            return { 200, asset.second, file.readAll(), {} };
        }
    }
    if (route == "/api/pair" && request.method == "POST") {
        return pair(request);
    }
    expire();
    const auto token = cookie(request);
    auto session = m_sessions.find(token);
    if (session == m_sessions.end()) {
        return error(401, "Pair this device to continue.");
    }
    session->touched = m_clock.elapsed();
    if (route == "/api/state" && request.method == "GET") {
        const auto etag
            = '"' + QByteArray::number(m_revision) + '-' + session->role.toLatin1() + '"';
        RemoteResponse response = request.headers.value("if-none-match") == etag
            ? RemoteResponse { 304, "application/json", {}, {} }
            : json(200, state(*session));
        response.headers.insert("ETag", etag);
        return response;
    }
    if (route == "/api/deck" && request.method == "GET") {
        return json(200, deck());
    }
    if (route == "/api/command" && request.method == "POST") {
        return command(request, *session);
    }
    if (route == "/api/logout" && request.method == "POST") {
        m_sessions.erase(session);
        emit changed();
        auto response = json(200, {});
        response.headers.insert(
            "Set-Cookie", "churchdeck=; Max-Age=0; HttpOnly; SameSite=Strict; Path=/");
        return response;
    }
    if ((route == "/api/preview/current" || route == "/api/preview/next")
        && request.method == "GET") {
        const auto snapshot = m_outputs->snapshotId();
        if (snapshot != m_previewSnapshot) {
            m_previews.clear();
            m_previewSnapshot = snapshot;
        }
        const int index = m_outputs->liveIndex() + (route.endsWith("next") ? 1 : 0);
        const auto slides = m_outputs->liveSlides();
        if (index < 0 || index >= slides.size()) {
            return error(404, "No slide.");
        }
        if (!m_previews.contains(index)) {
            // Two still images only; never decode full-length video or start a hidden output
            // window.
            if (m_previews.size() >= 2) {
                m_previews.clear();
            }
            m_previews.insert(
                index, RemotePreview::render(slides[index].toMap(), m_media->assets()));
        }
        return { 200, "image/jpeg", m_previews[index], {} };
    }
    return error(404, "Unknown remote route.");
}
