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


    protected slots:
        void ReadPendingDatagrams();
        void ProcessDatagram(QNodeAddress addr, QVariantMap message);
        void SendDatagram(QNodeAddress dest, QVariantMap& message);;

        void ProcessNewRequest(int type, quint32 request_id, QNode dest, QKey key);
        void SendRequest(QNodeAddress dest, quint32 request_id, QVariantMap message);
        void SendPing(QNodeAddress dest, quint32 request_id);
        void SendStore(QNodeAddress dest, quint32 request_id, QKey key);
        void SendFindNode(QNodeAddress dest, quint32 request_id, QNodeId id);
        void SendFindValue(QNodeAddress dest, quint32 request_id, QKey key);

        void SendReply(QNodeAddress dest, quint32 request_id, QVariantMap message)
        void ReplyPing(QNodeAddress dest, quint32 request_id)
        // NOTE: ReplyDownload does not exist. Downloads (i.e. replies to store) are
        // handled by the data manager.
        void ReplyFindNode(QNodeAddress dest, quint32 request_id, QNodeId id)
        void ReplyFindValue(QNodeAddress dest, quint32 request_id, QKey key)

    signals:
        void DatagramReady(QNodeAddress, QVariantMap);
        void RequestReady(QNodeAddress, QVariantMap);
        void ReplyReady(QNodeAddress, quint32, QKey);

        void ResponseReceived(quint32 request_id,
            QList<QNode> nodes = QList<QNode>());
        void ValueFound(QNodeAddress, quint32, QKey);

    protected:
        const static quint16 kDefaultPort = 42600;

        QUdpSocket* udp_socket_;
        DataStore* data_store_;
        RequestManager* request_manager_;
};

#endif // PEER_CACHE_KADEMLIA_CLIENT_HH
