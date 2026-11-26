#include "EditorDocument.h"
#include "OutputManager.h"
#include "ServiceCueActions.h"
#include "ServiceCues.h"
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QUdpSocket>

int runServiceCueChecks() {
    auto check = [](bool ok, const char *name) {
        if (!ok) {
            qCritical() << "Custom cues:" << name;
        }
        return ok;
    };
    auto cue = [](QVariantList actions) {
        return QVariantMap { { "version", 2 }, { "name", "Custom" }, { "actions", actions } };
    };
    auto message = [](const QString &text) {
        return QVariantMap { { "type", "message" }, { "text", text } };
    };
    auto spin = [](const std::function<bool()> &done, int limit = 1000) {
        QElapsedTimer clock;
        clock.start();
        while (!done() && clock.elapsed() < limit) {
            QCoreApplication::processEvents();
            QThread::msleep(2);
        }
        QCoreApplication::processEvents();
        return done();
    };
    QTemporaryDir directory;
    OutputManager outputs;
    EditorDocument document(nullptr, false);
    auto slides = document.slides().mid(0, 2);
    if (slides.size() < 2) {
        auto second = slides[0].toMap();
        second["elements"] = QVariantList {};
        slides.append(second);
    }
    auto page = slides[0].toMap();
    page["programItemId"] = "first";
    page["programItemTitle"] = "First";
    slides[0] = page;
    page = slides[1].toMap();
    page["programItemId"] = "second";
    page["programItemTitle"] = "Second";
    slides[1] = page;
    outputs.takeLive(slides, 0, "cue-test");
    ServiceCues cues(&outputs, directory.path());
    const QVariantMap delay { { "type", "delay" }, { "durationMs", 40 } };
    const auto ordered = cue({ message("before"), delay, message("after"),
        QVariantMap { { "type", "audio" }, { "value", "on" } } });
    if (!check(cues.testCue(ordered) && outputs.stageMessage().isEmpty() && !outputs.audioEnabled(),
            "Test has no side effects")) {
        return 1;
    }
    if (!check(cues.runCue(ordered) && cues.running() && outputs.stageMessage() == "before"
                && !outputs.audioEnabled(),
            "Wait preserves action order")) {
        return 2;
    }
    if (!check(spin([&] { return !cues.running(); }) && outputs.stageMessage() == "after"
                && outputs.audioEnabled(),
            "Delayed steps execute")) {
        return 3;
    }
    if (!check(cues.saveCue(-1, ordered), "Save ordered cue")) {
        return 4;
    }
    ServiceCues reopened(&outputs, directory.path());
    if (!check(reopened.cues().last().toMap()["actions"].toList() == ordered["actions"].toList(),
            "Ordered cue round trip")) {
        return 5;
    }
    outputs.setStageMessage("safe");
    if (!check(!cues.runCue(cue(
                   { message("bad"), QVariantMap { { "type", "navigate" }, { "slide", 999 } } }))
                && outputs.stageMessage() == "safe",
            "Preflight prevents partial action")) {
        return 6;
    }
    cues.runCue(cue({ delay, message("cancel failed") }));
    cues.cancel();
    spin([] { return false; }, 80);
    if (!check(outputs.stageMessage() == "safe" && !cues.running(),
            "Cancellation suppresses delayed actions")) {
        return 7;
    }
    cues.runCue(cue({ delay, message("navigation failed") }));
    outputs.advance(1);
    spin([] { return false; }, 80);
    if (!check(!cues.running() && outputs.stageMessage() == "safe",
            "Operator navigation cancels pending actions")) {
        return 8;
    }
    if (!check(cues.runCue(cue({ QVariantMap { { "type", "item" }, { "itemId", "first" } } }))
                && outputs.liveIndex() == 0,
            "Navigate by stable service identity")) {
        return 9;
    }
    QImage image(16, 16, QImage::Format_ARGB32);
    image.fill(QColor(255, 0, 0, 100));
    const auto path = directory.filePath("overlay.png");
    image.save(path);
    const auto count = outputs.audienceElements().size();
    if (!check(cues.runCue(cue({ QVariantMap {
                   { "type", "overlay" }, { "source", QUrl::fromLocalFile(path).toString() } } }))
                && outputs.audienceElements().size() == count + 1
                && outputs.liveElements().size() == count,
            "Overlay appears only on audience composition")) {
        return 10;
    }
    const auto snapshot = outputs.sessionSnapshot();
    OutputManager recovered;
    if (!check(recovered.restoreSession(snapshot)
                && recovered.audienceElements().size() == count + 1 && !recovered.audienceEnabled(),
            "Recovery retains overlay but never starts outputs")) {
        return 26;
    }
    cues.runCue(cue({ QVariantMap { { "type", "clearOverlay" } } }));
    if (!check(outputs.audienceElements().size() == count, "Clear overlay")) {
        return 11;
    }
    QUdpSocket receiver;
    if (!receiver.bind(QHostAddress::LocalHost, 0)) {
        return 12;
    }
    const QVariantMap osc { { "type", "osc" }, { "host", "127.0.0.1" },
        { "port", receiver.localPort() }, { "address", "/test/go" },
        { "arguments", "[7,0.5,\"hello\",true]" } };
    if (!check(cues.testCue(cue({ osc })) && !receiver.hasPendingDatagrams(),
            "Test never sends OSC")) {
        return 13;
    }
    if (!check(cues.runCue(cue({ osc })) && spin([&] { return receiver.hasPendingDatagrams(); }),
            "OSC sent to configured destination")) {
        return 14;
    }
    QByteArray packet(receiver.pendingDatagramSize(), '\0');
    receiver.readDatagram(packet.data(), packet.size());
    const QByteArray expected = QByteArray::fromHex(
        "2f746573742f676f000000002c69667354000000000000073f00000068656c6c6f000000");
    if (!check(packet == expected, "OSC protocol types, padding and byte order")) {
        qCritical() << packet.toHex();
        return 15;
    }
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        return 16;
    }
    const QVariantMap http { { "type", "http" },
        { "url", QString("http://127.0.0.1:%1/cue").arg(server.serverPort()) },
        { "method", "POST" }, { "body", "{\"go\":true}" } };
    if (!check(cues.testCue(cue({ http })) && !server.hasPendingConnections(),
            "Test never sends HTTP")) {
        return 17;
    }
    if (!cues.runCue(cue({ http, message("http done") }))
        || !spin([&] { return server.hasPendingConnections(); })) {
        return 18;
    }
    auto socket = server.nextPendingConnection();
    if (!spin([&] { return socket->bytesAvailable() > 0; })) {
        return 19;
    }
    const auto request = socket->readAll();
    if (!check(request.startsWith("POST /cue HTTP/1.1") && request.contains("{\"go\":true}")
                && outputs.stageMessage() != "http done",
            "HTTP waits for a successful response")) {
        return 20;
    }
    socket->write("HTTP/1.1 204 No Content\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
    socket->flush();
    if (!check(spin([&] { return !cues.running(); }) && outputs.stageMessage() == "http done",
            "HTTP completion continues sequence")) {
        return 21;
    }
    socket->deleteLater();
    cues.runCue(cue({ http, message("should not run") }));
    if (!spin([&] { return server.hasPendingConnections(); })) {
        return 22;
    }
    socket = server.nextPendingConnection();
    if (!spin([&] { return socket->bytesAvailable() > 0; })) {
        return 23;
    }
    socket->readAll();
    socket->write("HTTP/1.1 500 Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
    socket->flush();
    if (!check(spin([&] { return !cues.running(); }) && outputs.stageMessage() == "http done",
            "HTTP failure stops subsequent actions")) {
        return 24;
    }
    socket->deleteLater();
    cues.runCue(cue({ delay, message("stopped failed") }));
    outputs.stopPresentation();
    spin([] { return false; }, 80);
    if (!check(!cues.running() && outputs.stageMessage() == "http done",
            "Stop presenting cancels cues")) {
        return 25;
    }
    qInfo() << "Custom cue checks passed: ordered actions, migration, dry runs, delays, "
               "cancellation, navigation, overlays, OSC encoding and HTTP success/failure.";
    return 0;
}
