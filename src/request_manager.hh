#ifndef PEER_CACHE_REQUEST_MANAGER_HH
#define PEER_CACHE_REQUEST_MANAGER_HH

class QBitArray;
class QList;
class QNode;

class RequestManager : public QObject
{
    public:
        RequestManager();

        static QBitArray distance(QKey a, QKey b);
        // http://qt-project.org/wiki/WorkingWithRawData
        static quint16 bucket(QKey key);

    private:
        QList<QNode>[160] m_k_buckets; // QNode == QKey node_id, Qhostaddr, quint16 port

        void refresh_bucket(quint16 bucket);
        void update_bucket(quint16 bucket, QNode new_node);

        class Request
        {
            // TYPE, Dependencies, Data
        };
        // QList<QVariantMap>* request_queue_; // Requests ready to be processed
        // QMap<QByteArray, QNodeId>* sent_requests_; // Requests server has sent
        //                                            // for which it is waiting
        //                                            // on a response
    // QList<NodeAddress>::const_iterator node_id;
    // QList<QNodeAddress> nodes = Find(id);
    // for (node = node_ids.constBegin(); node != node_ids.constEnd(); node++) {
    //     emit RequestReady(*node, message);
    // }
    // connect(this, SIGNAL(AckReceived(QNodeAddress, quint32)), request_manager_,
    //     SLOT(ClosePing(QNodeAddress, quint32)), Qt::QueuedConnection);
    // connect(this, SIGNAL(NodesFound(QNodeAddress, quint32, QKey)),
    //     request_manager_, SLOT(UpdateFindRequest(QNodeAddress, quint32, QKey)),
    //     Qt::QueuedConnection);
    // connect(this, SIGNAL(ValueFound(QNodeAddress, quint32, QKey)),
    //     request_manager_, SLOT(CloseFindRequest(QNodeAddress, quint32, QKey)),
    //     Qt::QueuedConnection);
    // // Issue new requests
    // connect(request_manager_, SIGNAL(HasPingRequest(QNodeAddress, quint32)),
    //     this, SLOT(SendPing(QNodeAddress, quint32)), Qt::QueuedConnection);
    // connect(request_manager_,
    //     SIGNAL(HasFindNodeRequest(QNodeAddress, quint32, QNodeId)), this,
    //     SLOT(SendFindNode(QNodeAddress, quint32, QNodeId)), Qt::QueuedConnection);
    // connect(request_manager_,
    //     SIGNAL(HasFindValueRequest(QNodeAddress, quint32, QKey)), this,
    //     SLOT(SendFindValue(QNodeAddress, quint32, QKey)), Qt::QueuedConnection);
}

#endif // PEER_CACHE_REQUEST_MANAGER_HH
