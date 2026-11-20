#include "DocumentElements.h"
#include "MediaLibrary.h"
#include "OutputManager.h"
#include "RemoteController.h"
#include <QBuffer>
#include <QDebug>
#include <QEventLoop>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>

int RemoteController::selfTest() {
    OutputManager outputs;
    MediaLibrary media;
    RemoteController remote(&outputs, &media);
    auto slide = [](const QString &title) {
        auto lyrics = DocumentElements::text(
            "Lyrics", "స్వాగతం\nWelcome", 100, 150, 1720, 500, 90, "Noto Sans Telugu", "#FFFFFF");
        lyrics["binding"] = "Primary lyrics";
        auto translation = DocumentElements::text(
            "Translation", "यीशु", 100, 700, 1720, 200, 60, "Noto Sans Devanagari", "#AABBCC");
        translation["binding"] = "Translated lyrics";
        return QVariantMap { { "name", title }, { "notes", "Private speaker notes" },
            { "elements", QVariantList { lyrics, translation } }, { "programItemId", "song" },
            { "programItemTitle", "Welcome song" }, { "programTitle", "Sunday Worship" } };
    };
    outputs.takeLive({ slide("Verse"), slide("Chorus"), slide("Closing") }, 0, "test-program");
    if (remote.running() || !remote.start("127.0.0.1")) {
        return 1;
    }
    QNetworkAccessManager controller, speaker;

    struct Response {
        int code;
        QByteArray bytes;
        QVariantMap value;
        QByteArray etag;
    };

    auto call = [&](QNetworkAccessManager &client, const QString &path,
                    const QVariantMap &data = QVariantMap(), bool post = false,
                    const QByteArray &origin = QByteArray(), const QByteArray &host = QByteArray(),
                    const QByteArray &etag = QByteArray()) {
        QNetworkRequest request(QUrl(remote.address() + path));
        request.setRawHeader("Origin", origin.isEmpty() ? remote.address().toLatin1() : origin);
        if (!host.isEmpty()) {
            request.setRawHeader("Host", host);
        }
        if (!etag.isEmpty()) {
            request.setRawHeader("If-None-Match", etag);
        }
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        auto reply = post
            ? client.post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact))
            : client.get(request);
        QEventLoop loop;
        QTimer deadline;
        deadline.setSingleShot(true);
        deadline.start(5000);
        QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        const auto bytes = reply->isFinished() ? reply->readAll() : QByteArray();
        Response response { reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
            bytes, QJsonDocument::fromJson(bytes).object().toVariantMap(),
            reply->rawHeader("ETag") };
        reply->abort();
        reply->deleteLater();
        return response;
    };
    auto fail = [](int code, const char *message) {
        qWarning() << "Remote check" << code << message;
        return code;
    };
    if (call(controller, "/").code != 200 || call(controller, "/app.js").code != 200) {
        return fail(2, "offline assets");
    }
    if (call(controller, "/api/state").code != 401) {
        return fail(3, "unauthenticated state");
    }
    if (call(controller, "/api/pair", { { "code", remote.controllerCode() } }, true,
            "http://attacker.test")
            .code
        != 403) {
        return fail(4, "origin");
    }
    if (call(controller, "/", {}, false, {}, "attacker.test").code != 403) {
        return fail(5, "host");
    }
    if (call(controller, "/api/pair",
            { { "code", remote.controllerCode() }, { "name", "Test phone" } }, true)
            .code
        != 200) {
        return fail(6, "controller pairing");
    }
    if (call(speaker, "/api/pair", { { "code", remote.speakerCode() } }, true).code != 200) {
        return fail(7, "speaker pairing");
    }
    auto state = call(controller, "/api/state");
    if (state.value["text"] != "స్వాగతం\nWelcome" || state.value["translation"] != "यीशु"
        || state.value["notes"] != "Private speaker notes") {
        return fail(8, "multilingual speaker state");
    }
    if (call(controller, "/api/state", {}, false, {}, {}, state.etag).code != 304) {
        return fail(9, "conditional polling");
    }
    const auto deck = call(controller, "/api/deck").value;
    if (deck["items"].toList().size() != 1 || deck["title"] != "Sunday Worship") {
        return fail(10, "live program grouping");
    }
    const auto preview = call(controller, "/api/preview/current");
    if (preview.code != 200 || QImage::fromData(preview.bytes).size() != QSize(720, 405)) {
        return fail(11, "still preview");
    }
    QVariantMap command { { "snapshot", outputs.snapshotId() }, { "expectedIndex", 0 },
        { "action", "next" }, { "requestId", "command-0001" } };
    if (call(speaker, "/api/command", command, true).code != 403 || outputs.liveIndex() != 0) {
        return fail(12, "speaker command rejection");
    }
    if (call(controller, "/api/command", command, true).code != 200 || outputs.liveIndex() != 1) {
        return fail(13, "advance");
    }
    if (call(controller, "/api/command", command, true).code != 200 || outputs.liveIndex() != 1) {
        return fail(14, "replay deduplication");
    }
    command["requestId"] = "command-0002";
    if (call(controller, "/api/command", command, true).code != 409 || outputs.liveIndex() != 1) {
        return fail(15, "stale index");
    }
    command["expectedIndex"] = 1;
    command["action"] = "jump";
    command["index"] = 2;
    if (call(controller, "/api/command", command, true).code != 200 || outputs.liveIndex() != 2) {
        return fail(16, "jump");
    }
    command["requestId"] = "command-0003";
    command["action"] = "blackout";
    command["value"] = true;
    if (call(controller, "/api/command", command, true).code != 200 || !outputs.audienceBlack()) {
        return fail(17, "blackout");
    }
    const auto oldSnapshot = outputs.snapshotId();
    outputs.takeLive({ slide("New service") }, 0);
    command["requestId"] = "command-0004";
    if (outputs.snapshotId() == oldSnapshot
        || call(controller, "/api/command", command, true).code != 409) {
        return fail(18, "stale snapshot");
    }
    remote.rotateCodes();
    if (call(controller, "/api/state").code != 401 || call(speaker, "/api/state").code != 401
        || !remote.clients().isEmpty()) {
        return fail(19, "rotate revokes sessions");
    }
    if (call(controller, "/api/pair", { { "code", remote.controllerCode() } }, true).code != 200) {
        return fail(20, "re-pair");
    }
    remote.revoke(remote.clients().first().toMap()["id"].toString());
    if (call(controller, "/api/state").code != 401) {
        return fail(21, "individual revoke");
    }
    for (int i = 0; i < 3; ++i) {
        call(controller, "/api/pair", { { "code", "invalid" } }, true);
    }
    if (call(controller, "/api/pair", { { "code", remote.controllerCode() } }, true).code != 429) {
        return fail(22, "pairing attempt limit");
    }
    // Raw transport checks do not rely on an HTTP client sanitizing hostile framing.
    auto raw = [&](const QByteArray &request) {
        QTcpSocket socket;
        QEventLoop loop;
        QTimer deadline;
        deadline.setSingleShot(true);
        deadline.start(5000);
        QObject::connect(&socket, &QTcpSocket::connected, &loop, [&] { socket.write(request); });
        QByteArray bytes;
        QObject::connect(
            &socket, &QTcpSocket::readyRead, &loop, [&] { bytes += socket.readAll(); });
        QObject::connect(&socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
        QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        socket.connectToHost(QUrl(remote.address()).host(), QUrl(remote.address()).port());
        loop.exec();
        return bytes;
    };
    if (!raw("POST /api/pair HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n").startsWith("HTTP/1.1 400")) {
        return fail(23, "duplicate headers");
    }
    if (!raw("POST /api/pair HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n")
            .startsWith("HTTP/1.1 400")) {
        return fail(24, "chunked framing");
    }
    if (!raw("POST /api/pair HTTP/1.1\r\nContent-Length: 4097\r\n\r\n")
            .startsWith("HTTP/1.1 413")) {
        return fail(25, "body cap");
    }
    if (!raw("GET / HTTP/1.1\r\n" + QByteArray(17000, 'x')).startsWith("HTTP/1.1 431")) {
        return fail(26, "header cap");
    }
    remote.stop();
    if (remote.running() || !remote.clients().isEmpty() || !remote.controllerCode().isEmpty()) {
        return fail(27, "stop clears credentials");
    }
    if (!remote.start("127.0.0.1")) {
        return fail(28, "restart");
    }
    if (call(controller, "/api/pair", { { "code", remote.controllerCode() } }, true).code != 200) {
        return fail(29, "restart pairing");
    }
    auto session = remote.m_sessions.begin();
    session->touched = remote.m_clock.elapsed() - 31 * 60 * 1000;
    if (call(controller, "/api/state").code != 401) {
        return fail(30, "idle expiry");
    }
    call(controller, "/api/pair", { { "code", remote.controllerCode() } }, true);
    session = remote.m_sessions.begin();
    if (session == remote.m_sessions.end()) {
        return fail(31, "expiry re-pair");
    }
    session->created = remote.m_clock.elapsed() - 12 * 60 * 60 * 1000 - 1;
    if (call(controller, "/api/state").code != 401) {
        return fail(32, "absolute expiry");
    }
    call(controller, "/api/pair", { { "code", remote.controllerCode() } }, true);
    if (call(controller, "/api/logout", {}, true).code != 200
        || call(controller, "/api/state").code != 401) {
        return fail(33, "logout");
    }
    for (int i = 0; i < 24; ++i) {
        remote.m_sessions.insert(QString::number(i),
            { QString::number(i), "Fixture", "speaker", remote.m_clock.elapsed(),
                remote.m_clock.elapsed(), {}, {} });
    }
    if (call(controller, "/api/pair", { { "code", remote.controllerCode() } }, true).code != 429) {
        return fail(34, "session cap");
    }
    remote.stop();
    qInfo() << "Remote checks passed: pairing, roles, live navigation, previews, revocation, "
               "limits and HTTP framing";
    return 0;
}
