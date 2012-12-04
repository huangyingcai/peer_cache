#ifndef PEER_CACHE_REQUEST_MANAGER_HH
#define PEER_CACHE_REQUEST_MANAGER_HH

class QBitArray;
class QList;
class QNode;

class RequestManager : public QObject
{
    public:
        RequestManager(QNodeId id, QObject* parent = 0);

        // http://qt-project.org/wiki/WorkingWithRawData
        static QBitArray Distance(QKey a, QKey b);
        quint16 Bucket(QKey key);

    public slots:
        void ClosePing(QNodeAddress addr, quint32 request_id);
        void UpdateFindRequest(QNodeAddress addr, quint32 request_id, QKey key);
        void CloseFindRequest(QNodeAddress addr, quint32 request_id, QKey key);
        // void CloseRequest

    signals:
        void HasPingRequest(QNodeAddress, quint32);
        void HasFindNodeRequest(QNodeAddress, quint32, QNodeId);
        void HasFindValueRequest(QNodeAddress, quint32, QKey);

    private:
        QList<QNode>[160] buckets_; // QNode == QKey node_id, Qhostaddr, quint16 port
        // TODO: heap memory

        void ClosestNodes(QKey key);
        void RefreshBucket(quint16 bucket);
        void UpdateBucket(quint16 bucket, QNode node);

        class Request
        {
            public:
                quint32 id_;
                int type_;
                QList<quint32> dependents; // request_ids of dependents
                QList<quint32> dependencies; // request_ids of dependencies
                QVariant data;
                // TODO: when a request is closed, notify all dependent parents that
                // okay

                void NotifyDependents()
                {
                    QList<quint32>::const_iterator request;
                    for (request = dependents.constBegin();
                            request != dependents.constEnd(); request++) {
                        emit DependencyCompleted(*request, id_);
                    }
                }
        };

        QMap<quint32, Request> requests; // Map of request_id to request
        QList<Request*> request_queue; // Requests ready for processing

    // QList<NodeAddress>::const_iterator node_id;
    // QList<QNodeAddress> nodes = Find(id);
    // for (node = node_ids.constBegin(); node != node_ids.constEnd(); node++) {
    //     emit RequestReady(*node, message);
    // }
}

#endif // PEER_CACHE_REQUEST_MANAGER_HH
