#include "constants.hh"
#include "includes.hh"
#include "kademlia_client.hh"

QStringList KademliaClient::SerializeNodes(QList<QNode> nodes)
{
    QStringList node_strings;

    QList<QNode>::const_iterator i;
    for (i = nodes.constBegin(); i != nodes.constEnd(); i++) {
        QNodeId id = i->first;
        QHostAddress addr = i->second.first;
        quint16 port = i->second.second;
        node_strings << QString("%1%2:%3").arg(
            id.constData()).arg(addr.toString()).arg(port);
    }

    return node_strings;
}

QList<QNode> KademliaClient::DeserializeNodeStrings(QStringList node_strings)
{
    QList<QNode> nodes;

    QStringList::const_iterator i;
    for (i = node_strings.constBegin(); i != node_strings.constEnd();
            i++) {
        QNodeId id = QByteArray().append(i->mid(0, kKeyLength));
        QStringList remaining = i->mid(kKeyLength).split(":");
        QHostAddress addr = QHostAddress(remaining[0]);
        quint16 port = (quint16) remaining[1].toUInt();
        nodes << qMakePair(id, qMakePair(addr, port));
    }

    return nodes;
}

KademliaClient::KademliaClient(QNodeAddress bootstrap_addr)
{
    qsrand(time(NULL));
    QBitArray bits(kKeyLength * 8, 0);
    for (int b = 0; b <= kKeyLength * 8; b++) {
        bits[b] = qrand() % 2;
    }
    node_id_ = QByteArray(kKeyLength, 0);
    for (int b = 0; b < bits.count(); b++) {
        node_id_[b/8] = (node_id_.at(b / 8) | ((bits[b] ? 1 : 0) << (b % 8)));
    }
    qDebug() << "Node Id is: " << node_id_;

    udp_socket_ = new QUdpSocket(this);
    quint16 p = kDefaultPort;
    while (!udp_socket_->bind(p++));
    qDebug() << "Bound to port " << p - 1;
    // udp_socket_.bind(QHostAddress::LocalHost, kDefaultPort);
    connect(udp_socket_, SIGNAL(readyRead()), this,
        SLOT(ReadPendingDatagrams()));

    request_manager_ = new RequestManager(node_id_, bootstrap_addr, this);
    // Handle existing requests
    connect(this, SIGNAL(ResponseReceived(quint32, QNodeList)),
        request_manager_, SLOT(UpdateRequest(quint32, QNodeList)));
    // Issue new requests
    connect(request_manager_, SIGNAL(HasRequest(int, quint32, QNode, QKey)),
        SLOT(ProcessNewRequest(int, quint32, QNode, QKey)));

    // Connect remaining signals and slots to implement asynch server
    // TODO: Qt::Queued Connection
    connect(this, SIGNAL(DatagramReady(QNodeAddress, QVariantMap)),
        this, SLOT(ProcessDatagram(QNodeAddress, QVariantMap)));
    connect(this, SIGNAL(RequestReady(QNodeAddress, quint32, QVariantMap)),
        this, SLOT(SendRequest(QNodeAddress, quint32, QVariantMap)));
    connect(this, SIGNAL(ReplyReady(QNodeAddress, quint32, QVariantMap)), this,
        SLOT(SendReply(QNodeAddress, quint32, QVariantMap)));
}

void KademliaClient::ReadPendingDatagrams()
{
    while (udp_socket_->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress addr;
        quint16 port;
        datagram.resize(udp_socket_->pendingDatagramSize());
        udp_socket_->readDatagram(datagram.data(), datagram.size(), &addr,
                                  &port);

        QVariantMap message;
        QDataStream serializer(&datagram, QIODevice::ReadOnly);
        serializer >> message;
        if (serializer.status() != QDataStream::Ok) {
            ERROR("Failed to deserialize datagram into QVariantMap");
            return;
        }
        qDebug() << "Deserialized datagram to " << message;

        emit DatagramReady(qMakePair(addr, port), message);
    }
}

void KademliaClient::ProcessDatagram(QNodeAddress addr, QVariantMap message)
{
    // TODO: Was Node intended destination??

    QNodeId source_id = message.value("Source").toByteArray();
    if (source_id.isEmpty()) {
        ERROR("Invalid Source in request");
        return ;
    }
    QNode node(source_id, addr);

    // Handle request
    quint16 type = message.value("Type").toUInt();
    quint32 request_id = message.value("Request Id").toUInt();
    if (!type || !request_id) {
        ERROR("Invalid Type or Request Id"); // TODO: make 0 invalid request id
        return;
    }
    // Need declarations for all of these
    QKey key;
    QStringList node_list;
    QNodeList nodes;
    QNodeId id;
    switch (type) {
    // TODO: verify response came from right node
        case PING:
            ReplyPing(addr, request_id);
            break;
        case ACK:
            emit ResponseReceived(request_id);
            break;
        case STORE:
            key = message.value("Key").toByteArray();
            if (!key.isEmpty()) {
                InitiateDownload(addr, request_id, key);
            } else {
                ERROR("Improper STORE: no key");
            }
            break;
        case FIND_VALUE:
            key = message.value("Key").toByteArray();
            if (!key.isEmpty()) {
                ReplyFindValue(addr, request_id, key);
            } else {
                ERROR("Improper FIND_VALUE: no key");
            }
            break;
        case REPLY_VALUE:
            key = message.value("Key").toByteArray();
            node_list = message.value("Nodes").toStringList();
            if (!key.isEmpty()) {
                InitiateDownload(addr, request_id, key);
                request_manager_->CloseRequest(request_id); // FIXME:
            } else if (!nodes.isEmpty()) {
                nodes = KademliaClient::DeserializeNodeStrings(node_list);
                emit ResponseReceived(request_id, nodes);
            } else {
                ERROR("Improper REPLY_VALUE");
            }
            break;
        case FIND_NODE:
            id = message.value("Id").toByteArray();
            if (!id.isEmpty()) {
                ReplyFindNode(addr, request_id, id);
            } else {
                ERROR("Improper FIND_NODE: no id");
            }
            break;
        case REPLY_NODE:
            node_list = message.value("Nodes").toStringList();
            if (!nodes.isEmpty()) {
                nodes = KademliaClient::DeserializeNodeStrings(node_list);
                emit ResponseReceived(request_id, nodes);
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

    udp_socket_->writeDatagram(datagram, dest.first, dest.second);
}

// Sending outgoing request packets

void KademliaClient::ProcessNewRequest(int type, quint32 request_id, QNode dest,
    QKey key)
{
    QNodeAddress dest_addr = dest.second;
    switch (type) {
        case PING:
            SendPing(dest_addr, request_id);
            break;
        case STORE:
            SendStore(dest_addr, request_id, key);
            break;
        case FIND_NODE:
            SendFindNode(dest_addr, request_id, key);
            break;
        case FIND_VALUE:
            SendFindValue(dest_addr, request_id, key);
            break;
        default:
            break;
    }
}

void KademliaClient::SendRequest(QNodeAddress dest, quint32 request_id,
    QVariantMap message)
{
    // Insert standard message keys
    message.insert("Source", node_id_);
    message.insert("Request Id", request_id);

    SendDatagram(dest, message);
}

void KademliaClient::SendPing(QNodeAddress dest, quint32 request_id)
{
    QVariantMap message;
    message.insert("Type", PING);

    emit RequestReady(dest, request_id, message);
}

void KademliaClient::SendStore(QNodeAddress dest, quint32 request_id, QKey key)
{
    QVariantMap message;
    message.insert("Type", STORE);
    message.insert("Store", key);

    emit RequestReady(dest, request_id, message);
}

void KademliaClient::SendFindNode(QNodeAddress dest, quint32 request_id,
    QNodeId id)
{
    QVariantMap message;
    message.insert("Type", FIND_NODE);
    message.insert("Find Node", id);

    emit RequestReady(dest, request_id, message);
}

void KademliaClient::SendFindValue(QNodeAddress dest, quint32 request_id,
    QKey key)
{
    QVariantMap message;
    message.insert("Type", FIND_VALUE);
    message.insert("Key", key);

    emit RequestReady(dest, request_id, message);
}

// Sending outgoing reply packets

void KademliaClient::SendReply(QNodeAddress dest, quint32 request_id,
    QVariantMap message)
{
    // Insert standard message keys
    message.insert("Source", node_id_);
    message.insert("Request Id", request_id);

    SendDatagram(dest, message);
}

void KademliaClient::ReplyPing(QNodeAddress dest, quint32 request_id)
{
    QVariantMap message;
    message.insert("Type", ACK);

    emit ReplyReady(dest, request_id, message);
}

void KademliaClient::ReplyFindNode(QNodeAddress dest, quint32 request_id,
    QNodeId id)
{
    QVariantMap message;
    message.insert("Type", REPLY_NODE);

    QNodeList nodes = request_manager_->ClosestNodes(id);
    message.insert("Nodes", KademliaClient::SerializeNodes(nodes));

    emit ReplyReady(dest, request_id, message);
}

void KademliaClient::ReplyFindValue(QNodeAddress dest, quint32 request_id,
    QKey key)
{
    QVariantMap message;
    message.insert("Type", REPLY_VALUE);

    // If have cached resource, reply with port open for download, otherwise
    // reply with the closest k nodes to the requested key
    if (Value(key)) {
        message.insert("Key", key);
    } else {
        QNodeList nodes = request_manager_->ClosestNodes(key);
        message.insert("Nodes", KademliaClient::SerializeNodes(nodes));
    }

    emit ReplyReady(dest, request_id, message);
}

