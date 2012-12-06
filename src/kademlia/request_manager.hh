#ifndef KADEMLIA_REQUEST_MANAGER_HH
#define KADEMLIA_REQUEST_MANAGER_HH

#include "types.hh"
#include "constants.hh"

class Request;

class RequestManager : public QObject
{
    Q_OBJECT

    public:
        RequestManager(QNodeId id, QObject* parent = 0);
        ~RequestManager();
        // Must be called before Issue*() commands
        // Initialization that must occur after signals and slots are set up
        void Init(QNodeAddress bootstrap_address);

        // Bucket management
        quint16 Bucket(QKey key);
        void UpdateBuckets(QNode node);
        QList<QNode> ClosestNodes(QKey key, quint16 num = kBucketSize);

        // Request management
        void InitiateRequest(quint32 request_id, Request* request);
        void UpdateRequest(quint32 request_id, QNodeList nodes);
        void CloseRequest(quint32 request_id);

        void HandleMissingResource(QKey key); // TODO:

        // Kademlia RPCs
        void IssuePing(QNodeId id);
        void IssueStore(QKey key);
        void IssueFindNode(QNodeId id);
        void IssueFindValue(QKey key);

    public slots:
        void RefreshBucket(quint16 bucket);

    signals:
        void HasRequest(int, quint32, QNode, QKey);

    private:
        bool initialized_;
        QNodeId* node_id_;
        QVector<QNodeList*>* buckets_;
        QHash<quint32, Request*>* requests_;
};

#endif // KADEMLIA_REQUEST_MANAGER_HH
