#include "server.hh"
#include "peer_cache_constants.hh"

const quint16 kDefaultPort = 42600; // TODO: const static?
const quint16 kDefaultBufferSize = 8192; // TODO: const static?

Server::Server()
{
    m_udp_socket = new QUdpSocket(this); // TODO: parents and memory?
    m_udp_socket.bind(QHostAddress::LocalHost, kDefaultPort);
    connect(m_udp_socket, SIGNAL(readyRead()), this,
        SLOT(ReadPendingDatagrams()));

   // TODO: resource manager; has download manager
    // connect downloadReceived to process download (see peerster)

    // Connect remaining signals and slots to implement asynch server
    connect(this, SIGNAL(DatagramReady(QHostAddress, quint16, QVariantMap)),
        this, SLOT(ProcessDatagram(QHostAddress, quint16, QVariantMap)),
        Qt::QueuedConnection);

    connect(this, SIGNAL(RequestReady(QNodeAddress, QVariantMap)), this,
        SLOT(SendRequest(QNodeAddress, QVariantMap)), Qt::QueuedConnection);

    connect(this, SIGNAL(ReplyReady(QNodeAddress, quint32, QVariantMap)), this,
        SLOT(SendReply(QNodeAddress, quint32, QVariantMap)),
        Qt::QueuedConnection);
}

////////////////////////////////////////////////////////////////////////////////
// Kademlia Protocol Message Handling (UDP)

void Server::ReadPendingDatagrams()
{
    while (m_udp_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress addr;
        quint16 port;
        datagram.resize(m_udp_socket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &addr,
                                  &port);

        QVariantMap message;
        QDataStream serializer(&datagram, QIODevice::ReadOnly);
        serializer >> message;
        if (serializer.status() != QDataStream::Ok) {
            ERROR("Failed to deserialize datagram into QVariantMap");
            return;
        }
        qDebug() << "Deserialized datagram to " << message;

        emit ReadyProcess(qMakepair(addr, port), message);
    }
}

void Server::ProcessDatagram(QNodeAddress addr, QVariantMap message)
{
    // TODO: Was Node intended destination??

    // Update K-Buckets
    QNodeId source_id = message.value("Source").toString();
    if (!source_id) {
        ERROR("Invalid Source in request");
        return ;
    }
    UpdateKBuckets(source_id, addr);

    // Handle request
    quint16 type = message.value("Type").toUInt();
    quint32 request_id = message.value("Request Id").toUInt();
    if (!type || !request_id) {
        ERROR("Invalid Type or Request Id"); // TODO: make 0 invalid request id
        return;
    }
    switch (type) {
        case PING:
            emit PingReceived(addr, request_id);
            break;
        case ACK:
            // TODO: Check that I made the request
            // FIXME:
            emit AckReceived(source_id, addr); // TODO: outstanding acks
            break;
        case STORE:
            QKey key = message.value("Key").toByteArray();
            if (key.isEmpty()) {
                ERROR("Improper STORE: no key");
            } else {
                m_data_manager->InitiateDownload(addr, request_id, key); // TODO: Use a TCP socket to download
            }
            break;
        case FIND_VALUE:
            QKey key = messageMap.value("Key").toByteArray();
            if (key.isEmpty()) {
                ERROR("Improper FIND_VALUE: no key");
            } else {
                ReplyFindValue(addr, key);
            }
            break;
        case REPLY_VALUE:
            QKey key = message.value("Key").toByteArray();
            QList<QVariant> nodes = message.value("Nodes").toList();
            if (key) { 
                // TODO: check that I made request
                m_data_manager->InitiateDownload(addr, request_id, key);
            } else if (!nodes.isEmpty()) {
                  // TODO: check that I made request
                  // re-forward request -- pass type FIND_VALUE
            } else {
                ERROR("Improper REPLY_VALUE");
            }
            break;
        case FIND_NODE:
            QNodeId id = messageMap.value("Id").toByteArray();
            if (id.isEmpty()) {
                ERROR("Improper FIND_NODE: no id");
            } else {
                ReplyFindNode(addr, id);
            }
            break;
        case REPLY_NODE:
            QList<QVariant> nodes = message.value("Nodes").toList();
            if (!nodes.isEmpty()) {
                // TODO: check I made request
                // update tables and reforward if necessary -- pass type
                // FIND_NODE
            }
            break;
        default:
            qDebug() << "Dropping malformed packet";
     }
  // if ping -> verify nodeid == stored value of address
}

void Server::SendDatagram(const QNodeAddress dest, QVariantMap& message)
{
    // Serialize into a datagram
    QByteArray datagram;
    QDataStream serializer(&datagram, QIODevice::ReadWrite);
    serializer << message;

    m_udp_socket->writeDatagram(datagram, dest.addr(), dest.port());
}

// Sending outgoing request packets

void Server::SendRequest(const QNodeAddress dest, QVariantMap message)
{
    // Insert standard message keys
    message.insert("Source", kNodeId);
    quint32 request_id = Server.RandomId(this);
    message.insert("Request Id", request_id);

    SendDatagram(dest, message);
    // TODO: make sure remove as appropriate
    AddSentRequest(request_id, message.value("Type")); // Keep track of
                                                       // outstanding requests
}

void Server::SendPing(const QNodeAddress dest)
{
    QVariantMap message;
    message.insert("Type", PING);

    emit RequestReady(dest, message);
}

void Server::SendStore(const QNodeAddress dest, const QKey key,
    const QByteArray sha)
{
    QVariantMap message;
    message.insert("Type", STORE);
    message.insert("Store", key);
    message.insert("Hash", sha);

    emit RequestReady(dest, message);
}

void Server::SendFindNode(const QNodeId id)
{
    QVariantMap message;
    message.insert("Type", FIND_NODE);
    message.insert("Find Node", id);

    QList<QNodeAddress> nodes = Find(id);
    QList<NodeAddress>::const_iterator node_id;
    for (node = node_ids.constBegin(); node != node_ids.constEnd(); node++) {
        emit RequestReady(*node, message);
    }
}

void Server::SendFindValue(const QKey key)
{
    QVariantMap message;
    message.insert("Type", FIND_VALUE);
    message.insert("Key", key);

    QList<QNodeAddress> nodes = Find(id);
    QList<NodeAddress>::const_iterator node_id;
    for (node = node_ids.constBegin(); node != node_ids.constEnd(); node++) {
        emit RequestReady(*node, message);
    }
}

// Sending outgoing reply packets

void Server::SendReply(const QNodeAddress dest, quint32 request_id,
    QVariantMap message)
{
    // Insert standard message keys
    message.insert("Source", kNodeId);
    message.insert("Request Id", request_id);

    SendDatagram(dest, message);
}

void Server::ReplyPing(const QNodeAddress dest, quint32 request_id)
{
    QVariantMap message;
    message.insert("Type", ACK);

    emit ReplyReady(dest, message);
}

// NOTE: ReplyDownload does not exist. Downloads (i.e. replies to store) are
// handled by the data manager.

void Server::ReplyFindNode(const QNodeAddress dest, quint32 request_id,
    QNodeId id)
{
    QVariantMap message;
    message.insert("Type", REPLY_NODE);

    QList<QNodeAddress> nodes = Find(key);
    message.insert("Nodes", nodes);

    emit ReplyReady(dest, request_id, message);
}

void Server::ReplyFindValue(const QNodeAddress dest, quint32 request_id,
    QKey key)
{
    QVariantMap message;
    message.insert("Type", REPLY_VALUE);

    // If have cached resource, reply with port open for download, otherwise
    // reply with the closest k nodes to the requested key
    if (m_data_manager->HasValue(key)) {
        message.insert("Key", key);
    } else {
        QList<QNodeAddress> nodes = m_request_manager->NodesFor(key);
        message.insert("Nodes", nodes);
    }

    emit ReplyReady(dest, request_id, message);
}
