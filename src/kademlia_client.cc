#include "includes.hh"
#include "kademlia_client.hh"
#include "data_manager.hh"
#include "request_manager.hh"

KademliaClient::KademliaClient()
{
    udp_socket_ = new QUdpSocket(this); // TODO: parents and memory?
    udp_socket_.bind(QHostAddress::LocalHost, kDefaultPort);
    connect(udp_socket_, SIGNAL(readyRead()), this,
        SLOT(ReadPendingDatagrams()));

    data_manager_ = new DataManager(this);
    connect(this, SIGNAL(StoreReceived(QNodeAddress, quint32, QKey)),
        data_manager_, SLOT(InitiateDownload(QNodeAddress, quint32, QKey)),
        Qt::QueuedConnection);
    connect(this, SIGNAL(ValueFound(QNodeAddress, quint32, QKey)),
        data_manager_, SLOT(InitiateDownload(QNodeAddress, quint32, QKey)),
        Qt::QueuedConnection);
    // TODO: connect the signal that download is completed connect some signal to slot to send

    request_manager_ = new RequestManager(this);
    // Handle existing requests
    connect(this, SIGNAL(AckReceived(QNodeAddress, quint32)), request_manager_,
        SLOT(ClosePing(QNodeAddress, quint32)), Qt::QueuedConnection);
    connect(this, SIGNAL(NodesFound(QNodeAddress, quint32, QKey)),
        request_manager_, SLOT(UpdateFindRequest(QNodeAddress, quint32, QKey)),
        Qt::QueuedConnection);
    connect(this, SIGNAL(ValueFound(QNodeAddress, quint32, QKey)),
        request_manager_, SLOT(CloseFindRequest(QNodeAddress, quint32, QKey)),
        Qt::QueuedConnection);
    // Issue new requests
    connect(request_manager_, SIGNAL(HasPingRequest(QNodeAddress, quint32)),
        this, SLOT(SendPing(QNodeAddress, quint32)), Qt::QueuedConnection);
    connect(request_manager_,
        SIGNAL(HasFindNodeRequest(QNodeAddress, quint32, QNodeId)), this,
        SLOT(SendFindNode(QNodeAddress, quint32, QNodeId)), Qt::QueuedConnection);
    connect(request_manager_,
        SIGNAL(HasFindValueRequest(QNodeAddress, quint32, QKey)), this,
        SLOT(SendFindValue(QNodeAddress, quint32, QKey)), Qt::QueuedConnection);

    // Connect remaining signals and slots to implement asynch server
    connect(this, SIGNAL(DatagramReady(QNodeAddress, QVariantMap)),
        this, SLOT(ProcessDatagram(QNodeAddress, QVariantMap)),
        Qt::QueuedConnection);
    connect(this, SIGNAL(RequestReady(QNodeAddress, QVariantMap)), this,
        SLOT(SendRequest(QNodeAddress, QVariantMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(ReplyReady(QNodeAddress, quint32, QVariantMap)), this,
        SLOT(SendReply(QNodeAddress, quint32, QVariantMap)),
        Qt::QueuedConnection);
}

void KademliaClient::ReadPendingDatagrams()
{
    while (udp_socket_->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress addr;
        quint16 port;
        datagram.resize(udp_socket_->pendingDatagramSize());
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

        emit DatagramReady(qMakepair(addr, port), message);
    }
}

void KademliaClient::ProcessDatagram(QNodeAddress addr, QVariantMap message)
{
    // TODO: Was Node intended destination??

    // Update K-Buckets
    QNodeId source_id = message.value("Source").toString();
    if (!source_id) {
        ERROR("Invalid Source in request");
        return ;
    }
    QNode node = qMakePair(source_id, addr);
    // UpdateKBuckets(source_id, addr); // FIXME-- put in request_manager
    // Make a node??? and pass instead of addr

    // Handle request
    quint16 type = message.value("Type").toUInt();
    quint32 request_id = message.value("Request Id").toUInt();
    if (!type || !request_id) {
        ERROR("Invalid Type or Request Id"); // TODO: make 0 invalid request id
        return;
    }
    switch (type) {
        case PING:
            ReplyPing(addr, request_id);
            break;
        case ACK:
            emit AckReceived(node, request_id);
            break;
        case STORE:
            QKey key = message.value("Key").toByteArray();
            if (!key.isEmpty()) {
                emit StoreReceived(node, request_id, key);
            } else {
                ERROR("Improper STORE: no key");
            }
            break;
        case FIND_VALUE:
            QKey key = messageMap.value("Key").toByteArray();
            if (!key.isEmpty()) {
                ReplyFindValue(addr, key);
            } else {
                ERROR("Improper FIND_VALUE: no key");
            }
            break;
        case REPLY_VALUE:
            QKey key = message.value("Key").toByteArray();
            QList<QVariant> nodes = message.value("Nodes").toList();
            if (!key.isEmpty()) {
                emit ValueFound(node, request_id, key);
            } else if (!nodes.isEmpty()) {
                emit NodesFound(node, request_id, key);
            } else {
                ERROR("Improper REPLY_VALUE");
            }
            break;
        case FIND_NODE:
            QNodeId id = messageMap.value("Id").toByteArray();
            if (!id.isEmpty()) {
                ReplyFindNode(addr, id);
            } else {
                ERROR("Improper FIND_NODE: no id");
            }
            break;
        case REPLY_NODE:
            QList<QVariant> nodes = message.value("Nodes").toList();
            if (!nodes.isEmpty()) {
                emit NodesFound(node, request_id, key);
            } else {
                ERROR("Improper REPLY_NODE");
            }
            break;
        default:
            qDebug() << "Dropping malformed packet";
     }
}

void KademliaClient::SendDatagram(QNodeAddress dest, QVariantMap& message)
{
    // Serialize into a datagram
    QByteArray datagram;
    QDataStream serializer(&datagram, QIODevice::ReadWrite);
    serializer << message;

    udp_socket_->writeDatagram(datagram, dest.addr(), dest.port());
}

// Sending outgoing request packets

void KademliaClient::SendRequest(QNodeAddress dest, QVariantMap message)
{
    // Insert standard message keys
    message.insert("Source", kNodeId);
    quint32 request_id = KademliaClient.RandomId(this);
    message.insert("Request Id", request_id);

    SendDatagram(dest, message);
}

void KademliaClient::SendPing(QNodeAddress dest)
{
    QVariantMap message;
    message.insert("Type", PING);

    emit RequestReady(dest, message);
}

void KademliaClient::SendStore(QNodeAddress dest, QKey key, QByteArray sha)
{
    QVariantMap message;
    message.insert("Type", STORE);
    message.insert("Store", key);
    message.insert("Hash", sha);

    emit RequestReady(dest, message);
}

void KademliaClient::SendFindNode(QNodeAddress dest, QNodeId id)
{
    QVariantMap message;
    message.insert("Type", FIND_NODE);
    message.insert("Find Node", id);

    emit RequestReady(dest, message);
}

void KademliaClient::SendFindValue(QNodeAddress dest, QKey key)
{
    QVariantMap message;
    message.insert("Type", FIND_VALUE);
    message.insert("Key", key);

    emit RequestReady(dest, message);
}

// Sending outgoing reply packets

void KademliaClient::SendReply(QNodeAddress dest, quint32 request_id,
    QVariantMap message)
{
    // Insert standard message keys
    message.insert("Source", kNodeId);
    message.insert("Request Id", request_id);

    SendDatagram(dest, message);
}

void KademliaClient::ReplyPing(QNodeAddress dest, quint32 request_id)
{
    QVariantMap message;
    message.insert("Type", ACK);

    emit ReplyReady(dest, message);
}

void KademliaClient::ReplyFindNode(QNodeAddress dest, quint32 request_id,
    QNodeId id)
{
    QVariantMap message;
    message.insert("Type", REPLY_NODE);

    QList<QNodeAddress> nodes = Find(key);
    message.insert("Nodes", nodes);

    emit ReplyReady(dest, request_id, message);
}

void KademliaClient::ReplyFindValue(QNodeAddress dest, quint32 request_id,
    QKey key)
{
    QVariantMap message;
    message.insert("Type", REPLY_VALUE);

    // If have cached resource, reply with port open for download, otherwise
    // reply with the closest k nodes to the requested key
    if (data_manager_->HasValue(key)) {
        message.insert("Key", key);
    } else {
        QList<QNodeAddress> nodes = m_request_manager->NodesFor(key);
        message.insert("Nodes", nodes);
    }

    emit ReplyReady(dest, request_id, message);
}
