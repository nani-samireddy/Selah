#pragma once
#include <functional>
#include <QHash>
#include <QHostAddress>
#include <QSet>
#include <QTcpServer>

struct RemoteRequest {
    QByteArray method, target;
    QHash<QByteArray, QByteArray> headers;
    QByteArray body;
    QString peer;
};

struct RemoteResponse {
    int status = 200;
    QByteArray type = "application/json; charset=utf-8";
    QByteArray body;
    QHash<QByteArray, QByteArray> headers;
};

// Minimal bounded HTTP/1.1 transport: one request per connection, no upload/file routing.
class RemoteHttpServer final : public QObject {
    Q_OBJECT
public:
    explicit RemoteHttpServer(QObject *parent = nullptr);
    ~RemoteHttpServer() override;
    bool listen(const QHostAddress &address, quint16 port);
    void stop();

    quint16 port() const { return m_server.serverPort(); }

    QString error() const { return m_server.errorString(); }

    void setHandler(std::function<RemoteResponse(const RemoteRequest &)> handler) {
        m_handler = std::move(handler);
    }

private:
    void acceptConnections();
    void receive(class QTcpSocket *socket);
    void respond(class QTcpSocket *socket, const RemoteResponse &response);
    QTcpServer m_server;
    QHash<class QTcpSocket *, QByteArray> m_buffers;
    QSet<class QTcpSocket *> m_sockets;
    std::function<RemoteResponse(const RemoteRequest &)> m_handler;
};
