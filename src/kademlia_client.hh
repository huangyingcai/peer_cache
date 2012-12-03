#ifndef PEER_CACHE_KADEMLIA_CLIENT_HH
#define PEER_CACHE_KADEMLIA_CLIENT_HH

class QVariantMap;
class QNodeAddress;
class QByteArray;
class QKey;
class QNodeId;
class DataManager;

public KademliaClient : public QObject
{
    Q_OBJECT

    public:
        KademliaClient();

    public slots:


    private slots:
        ReadPendingDatagrams();
        ProcessDatagram(QNodeAddress addr, QVariantMap message);
        SendDatagram(QNodeAddress dest, QVariantMap& message);;

        SendRequest(QNodeAddress dest, quint32 request_id, QVariantMap message);
        SendPing(QNodeAddress dest, quint32 request_id);
        SendStore(QNodeAddress dest, quint32 request_id, QKey key);
        SendFindNode(QNodeAddress dest, quint32 request_id, QNodeId id);
        SendFindValue(QNodeAddress dest, quint32 request_id, QKey key);

        SendReply(QNodeAddress dest, quint32 request_id, QVariantMap message)
        ReplyPing(QNodeAddress dest, quint32 request_id)
        // NOTE: ReplyDownload does not exist. Downloads (i.e. replies to store) are
        // handled by the data manager.
        ReplyFindNode(QNodeAddress dest, quint32 request_id, QNodeId id)
        ReplyFindValue(QNodeAddress dest, quint32 request_id, QKey key)

    signals:
        void DatagramReady(QNodeAddress, QVariantMap);
        void RequestReady(QNodeAddress, QVariantMap);
        void ReplyReady(QNodeAddress, quint32, QKey);

        void AckReceived(QNodeAddress, quint32);
        void StoreReceived(QNodeAddress, quint32, QKey);
        void ValueFound(QNodeAddress, quint32, QKey);
        void NodesFound(QNodeAddress, quint32, QKey); // update request

    protected:
        const static quint16 kDefaultPort = 42600; // TODO: right place?
        // const static quint16 kDefaultBufferSize = 8192;

        QUdpSocket* udp_socket_;
        DataManager* data_manager_;
        RequestManager* request_manager_;
};

#endif // PEER_CACHE_KADEMLIA_CLIENT_HH
