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

        quint16 Bucket(QKey key);
        QList<QNode> ClosestNodes(QKey key, quint16 num = kBucketSize);

    public slots:
        void InitiateRequest(Request* req);
        void UpdateRequest(quint32 request_id, QNodeList nodes);
        void CloseRequest(quint32 request_id);

        void IssueStore(QKey key); // TODO: get rid of this
        // void IssueStore(QKey key); // TODO: get rid of this
        // void IssueFindValue, FindNode

        void RefreshBucket(quint16 bucket);

    signals:
        void HasRequest(int, quint32, QNode, QKey);

    private:
        QNodeId node_id_;
        QVector<QNodeList*> buckets_;

        void UpdateBuckets(QNode node);

};

#endif // KADEMLIA_REQUEST_MANAGER_HH
