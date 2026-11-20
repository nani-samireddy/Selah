#include "RemoteHttpServer.h"
#include <QTcpSocket>
#include <QTimer>

RemoteHttpServer::RemoteHttpServer(QObject *parent)
    : QObject(parent) {
    m_server.setMaxPendingConnections(24);
    connect(&m_server, &QTcpServer::newConnection, this, &RemoteHttpServer::acceptConnections);
}

RemoteHttpServer::~RemoteHttpServer() {
    stop();
}

bool RemoteHttpServer::listen(const QHostAddress &address, quint16 port) {
    return m_server.listen(address, port);
}

void RemoteHttpServer::stop() {
    m_server.close();
    const auto sockets = m_sockets;
    m_sockets.clear();
    m_buffers.clear();
    for (auto socket : sockets) {
        socket->disconnect(this);
        socket->abort();
        socket->deleteLater();
    }
}

void RemoteHttpServer::acceptConnections() {
    while (m_server.hasPendingConnections()) {
        auto socket = m_server.nextPendingConnection();
        if (m_sockets.size() >= 24) {
            socket->abort();
            socket->deleteLater();
            continue;
        }
        m_sockets.insert(socket);
        m_buffers.insert(socket, {});
        socket->setReadBufferSize(20481);
        auto deadline = new QTimer(socket);
        deadline->setSingleShot(true);
        deadline->start(4000);
        connect(deadline, &QTimer::timeout, socket, &QTcpSocket::abort);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] { receive(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
            m_buffers.remove(socket);
            m_sockets.remove(socket);
            socket->deleteLater();
        });
    }
}

void RemoteHttpServer::receive(QTcpSocket *socket) {
    if (!m_buffers.contains(socket)) {
        return;
    }
    auto &buffer = m_buffers[socket];
    buffer += socket->readAll();
    if (buffer.size() > 20480) {
        respond(socket, { 413, "text/plain", "Request too large" });
        return;
    }
    const int end = buffer.indexOf("\r\n\r\n");
    if (end < 0) {
        if (buffer.size() > 16384) {
            respond(socket, { 431, "text/plain", "Headers too large" });
        }
        return;
    }
    if (end > 16384) {
        respond(socket, { 431, "text/plain", "Headers too large" });
        return;
    }
    const auto lines = buffer.left(end).split('\n');
    const auto start = lines.value(0).trimmed().split(' ');
    if (start.size() != 3 || start[2] != "HTTP/1.1" || !start[1].startsWith('/')
        || start[1].size() > 2048) {
        respond(socket, { 400, "text/plain", "Invalid request" });
        return;
    }
    RemoteRequest request;
    request.method = start[0];
    request.target = start[1];
    request.peer = socket->peerAddress().toString();
    for (int i = 1; i < lines.size(); ++i) {
        const auto line = lines[i];
        const auto colon = line.indexOf(':');
        if (colon < 1 || line.startsWith(' ') || line.startsWith('\t')) {
            respond(socket, { 400, "text/plain", "Invalid header" });
            return;
        }
        const auto key = line.left(colon).toLower(), value = line.mid(colon + 1).trimmed();
        if (request.headers.contains(key)) {
            respond(socket, { 400, "text/plain", "Duplicate header" });
            return;
        }
        request.headers.insert(key, value);
    }
    if (request.headers.contains("transfer-encoding")) {
        respond(socket, { 400, "text/plain", "Chunked requests are not supported" });
        return;
    }
    bool valid = true;
    const auto length = request.headers.contains("content-length")
        ? request.headers["content-length"].toLongLong(&valid)
        : 0;
    if (!valid || length < 0 || length > 4096) {
        respond(socket, { 413, "text/plain", "Invalid body length" });
        return;
    }
    if (buffer.size() < end + 4 + length) {
        return;
    }
    if (buffer.size() != end + 4 + length) {
        respond(socket, { 400, "text/plain", "Pipelining is not supported" });
        return;
    }
    request.body = buffer.mid(end + 4, length);
    m_buffers.remove(socket);
    respond(socket,
        m_handler ? m_handler(request) : RemoteResponse { 503, "text/plain", "Unavailable" });
}

void RemoteHttpServer::respond(QTcpSocket *socket, const RemoteResponse &response) {
    m_buffers.remove(socket);
    static const QHash<int, QByteArray> reasons { { 200, "OK" }, { 204, "No Content" },
        { 304, "Not Modified" }, { 400, "Bad Request" }, { 401, "Unauthorized" },
        { 403, "Forbidden" }, { 404, "Not Found" }, { 405, "Method Not Allowed" },
        { 409, "Conflict" }, { 413, "Content Too Large" }, { 429, "Too Many Requests" },
        { 431, "Headers Too Large" }, { 500, "Internal Server Error" }, { 503, "Unavailable" } };
    QByteArray headers = "HTTP/1.1 " + QByteArray::number(response.status) + " "
        + reasons.value(response.status, "Error") + "\r\n";
    headers += "Content-Type: " + response.type
        + "\r\nContent-Length: " + QByteArray::number(response.body.size())
        + "\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: "
          "nosniff\r\nX-Frame-Options: DENY\r\nReferrer-Policy: "
          "no-referrer\r\nCross-Origin-Resource-Policy: same-origin\r\nPermissions-Policy: "
          "camera=(), microphone=(), geolocation=()\r\n";
    headers += "Content-Security-Policy: default-src 'none'; script-src 'self'; style-src 'self'; "
               "img-src 'self' data:; font-src 'self'; connect-src 'self'; base-uri 'none'; "
               "frame-ancestors 'none'; form-action 'none'\r\n";
    for (auto it = response.headers.cbegin(); it != response.headers.cend(); ++it) {
        headers += it.key() + ": " + it.value() + "\r\n";
    }
    socket->write(headers + "\r\n" + response.body);
    socket->disconnectFromHost();
}
