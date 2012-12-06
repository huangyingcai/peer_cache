#ifndef KADEMLIA_REQUEST_MANAGER_HH
#define KADEMLIA_REQUEST_MANAGER_HH

#include "types.hh"
#include "constants.hh"

class Request;

class RequestManager : public QObject
{
    Q_OBJECT

    public:
        RequestManager(QNodeId id, QNodeAddress bootstrap_addr,
            QObject* parent = 0);
        void Init(); // Must be called before Issue*() commands

        quint16 Bucket(QKey key);
        void UpdateBuckets(QNode node);
        QList<QNode> ClosestNodes(QKey key, quint16 num = kBucketSize);

        // Kademlia RPCs
        void IssueStore(QKey key);
        void IssueFindNode(QNodeId id);
        void IssueFindValue(QKey key);

    public slots:
        void InitiateRequest(Request* req);
        void UpdateRequest(quint32 request_id, QNodeList nodes);
        void CloseRequest(quint32 request_id);

        void RefreshBucket(quint16 bucket);

    signals:
        void HasRequest(int, quint32, QNode, QKey);

    private:
        QNodeId node_id_;
        bool initialized_;
        QNodeAddress bootstrap_addr_;
        QVector<QNodeList*> buckets_;

};

#endif // KADEMLIA_REQUEST_MANAGER_HH
