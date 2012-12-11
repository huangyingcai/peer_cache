#include "includes.hh"
#include "constants.hh"

#include "peer_cache_daemon.hh"

PeerCacheDaemon::PeerCacheDaemon()
{
    network_access_manager_ = new QNetworkAccessManager();
    PeerCache* peer_cache = new PeerCache();
    peer_cache->setDirectory("tmp");
    network_access_manager_->setCache(peer_cache);

    proxy_server_ = new QTcpServer():
    proxy_server_->listen(QHostAddress::Any, kDaemonPort);
    connect(proxy_server_, SIGNAL(newConnection()), this,
        SLOT(AcceptIncomingConnection()));

}

PeerCacheDaemon::~PeerCacheDaemon()
{
    delete proxy_server_;
}

PeerCacheDaemon::AcceptIncomingConnection()
{
    QTcpSocket* connection = proxy_server_->nextPendingConnection();
    connect(connection, SIGNAL(readyRead()), this, SLOT(ProcessRequest()));
    connect(connection, SIGNAL(disconnected()), connection,
        SLOT(deleteLater()));
}

/* Implementation from X2 by Ofi Labs (see network/webproxy/webproxy.cpp):
 *    http://gitorious.org/ofi-labs/x2
 */
PeerCacheDaemon::ProcessRequest()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    QByteArray request_data = socket->readAll();

    int pos = request_data.indexOf("\r\n");
    QByteArray request_line = request_data.left(pos);
    request_data.remove(0, pos + 2);

    QList<QByteArray> entries = request_line.split(' ');
    QByteArray method = entries.value(0);
    QByteArray address = entries.value(1);
    QByteArray version = entries.value(2);

    QUrl url = QUrl::fromEncoded(address);
    if (!url.isValid()) {
        qWarning() << "Invalid URL:" << url;
        socket->disconnectFromHost();
        return;
    }

    QString host = url.host();
    int port = (url.port() < 0) ? 80 : url.port();
    QByteArray request = url.encodedPath();
    if (url.hasQuery()) {
        request.append('?').append(url.encodedQuery());
    }
    request_line = method + " " + request + " " + version + "\r\n";
    qDebug() << "Request received: " << request_line;
    request_data.prepend(request_line);
    qDebug() << "Full request: " << request_data;



    QString key = host + ':' + QString::number(port);
    QTcpSocket* proxy_socket = socket->findChild<QTcpSocket*>(key);
    if (proxy_socket) {
        proxy_socket->setObjectName(key);
        proxy_socket->setProperty("url", url);
        proxy_socket->setProperty("request_data", request_data);
        proxy_socket->write(request_data);
    } else {
        proxy_socket = new QTcpSocket(socket);
        proxy_socket->setObjectName(key);
        proxy_socket->setProperty("url", url);
        proxy_socket->setProperty("request_data", request_data);
        connect(proxy_socket, SIGNAL(connected()), this, SLOT(SendRequest()));
        connect(proxy_socket, SIGNAL(readyRead()), this, SLOT(TransferData()));
        connect(proxy_socket, SIGNAL(disconnected()), this, SLOT(CloseConnection()));
        connect(proxy_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(closeConnection()));
        proxy_socket->connectToHost(host, port);
    }
}

void PeerCacheDaemon::SendRequest() {
    QTcpSocket* proxy_socket = qobject_cast<QTcpSocket*>(sender());
    QByteArray request_data = proxy_socket->property("request_data").toByteArray();
    proxy_socket->write(request_data);
}

void PeerCacheDaemon::TransferData() {
    QTcpSocket* proxy_socket = qobject_cast<QTcpSocket*>(sender());
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(proxySocket->parent());
    socket->write(proxy_socket->readAll());
}

void PeerCacheDaemon::CloseConnection() {
    QTcpSocket* proxy_socket = qobject_cast<QTcpSocket*>(sender());
    if (proxySocket) {
        QTcpSocket *socket = qobject_cast<QTcpSocket*>(proxySocket->parent());
        if (socket)
            socket->disconnectFromHost();
        if (proxy_socket->error() != QTcpSocket::RemoteHostClosedError)
            qWarning() << "Error for:" << proxy_socket->property("url").toUrl()
                    << proxy_socket->errorString();
        proxy_socket->deleteLater();;
    }
}
