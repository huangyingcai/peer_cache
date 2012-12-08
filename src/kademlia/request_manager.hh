#ifndef KADEMLIA_REQUEST_MANAGER_HH
#define KADEMLIA_REQUEST_MANAGER_HH

#include "types.hh"
#include "constants.hh"

#include <QHash>
#include <QTimer>

class Request;

class RequestManager : public QObject
{
    Q_OBJECT

    public:
        RequestManager(QNodeId id);
        ~RequestManager();
        // Must be called before Issue*() commands
        // Initialization that must occur after signals and slots are set up
        void Init(QNodeAddress bootstrap_address);

        // Kademlia State Management
        quint16 Bucket(QKey key);
        void UpdateBuckets(QNode node);
        QList<QNode> ClosestNodes(QKey key, quint16 num = kBucketSize);

        // Kademlia RPCs
        void IssuePing(QNode node);
        void IssueStore(QKey key);
        void IssueFindNode(QNodeId id);
        void IssueFindValue(QKey key);

        // Request management
        void UpdateRequest(quint32 request_number, QNode destination); // == Close
        void UpdateRequest(quint32 request_number, QNode destination,
            QNodeList nodes);

        // TODO:
        void HandleMissingResource(QKey key); 

    public slots:
        void RefreshBucket(quint16 bucket);

    signals:
        void HasRequest(int, quint32, QNode, QKey key = QKey());

    private:
        bool initialized_;
        QNodeId* node_id_;
        QNodeList** buckets_;
        QHash<quint32, Request*>* requests_; // Map of internal request id to
                                             // request
        QHash<quint32, quint32>* request_number_to_id_map_; // Map of external request
                                                  // numbers to internal ids

        // Helpers
        quint32 RandomId();
        quint32 RandomRequestNumber();
};

#endif // KADEMLIA_REQUEST_MANAGER_HH
