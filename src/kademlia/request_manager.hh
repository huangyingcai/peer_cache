#ifndef PEER_CACHE_REQUEST_MANAGER_HH
#define PEER_CACHE_REQUEST_MANAGER_HH

#include "types.hh"
#include "constants.hh"

class RequestManager : public QObject
{
    Q_OBJECT

    public:
        RequestManager(QNodeId id, QObject* parent = 0);

        static QBitArray Distance(QKey a, QKey b);
        quint16 Bucket(QKey key);

    public slots:
        void InitiateRequest(quint32 request_id);
        void UpdateRequest(quint32 request_id, QNode node,
            QList<QNode> results);
        void CloseRequest(quint32 request_id);

        void IssueStore(QKey key); // TODO: get rid of this

    signals:
        void HasRequest(int, quint32, QNode, QKey);

    private:
        QNodeId node_id_;
        QVector<QNodeList*> buckets_;

        QList<QNode> ClosestNodes(QKey key, quint16 num = kBucketSize);
        void RefreshBucket(quint16 bucket);
        void UpdateBuckets(QNode node);

        class Request : public QObject
        {
            Q_OBJECT

            public:
                static void RegisterRequest(quint32 id, const Request* req);
                static void RemoveRequest(quint32 id);

                // Factory Methods -- TODO - this is sloppy
                static quint32 PingRequest(QNode dest, QObject* observer,
                    Request* parent = NULL);
                static quint32 StoreRequest();
                static quint32 FindNodeRequest(QNode dest, QNodeId id,
                    QObject* observer, Request* parent = NULL);
                static quint32 FindValueRequest(QNode dest, QKey key,
                    QObject* observer, Request* parent = NULL);

                static quint32 RandomId();
                static Request* Get(quint32 id);

                Request(int type, QNode dest, QObject* observer,
                    Request* parent = NULL);
                Request(const Request& other);
                void Init();

                quint32 get_id() { return id_; };

                void AddChild(quint32 id);
                virtual void UpdateResults(QNodeList results = QNodeList()); 

            signals:
                void Ready(Request*);
                void Complete(quint32);

            private:
                static QHash<quint32, Request> requests_;

                quint32 id_;
                int type_;
                QNode destination_;
                Request* parent_;
                QObject* observer_;
                QList<quint32> children_;
        };

        class PingRequest : public Request
        {
            public:
                PingRequest(QNode dest, QObject* observer, quint32 parent = 0);
                PingRequest(const PingRequest& other);
        };

        class StoreRequest : public Request
        {
            public:
                StoreRequest(QNode dest, QKey key, QObject* observer,
                    quint32 parent = 0);
                StoreRequest(const StoreRequest& other);

            private:
                QKey resource_key_;
        };

        class FindRequest : public Request
        {
            public:
                FindRequest(int type, QNode dest, QKey key, QObject* observer,
                    quint32 parent = 0);
                FindRequest(const FindRequest& other);

                void ProcessChildCompletion(Request* child);
                virtual void UpdateResults(QList<QNode> results);
                void MakeChild(QNode dest);

            private:
                QKey requested_key_;
                QNodeList results_;
        };

        class FindNodeRequest : public FindRequest
        {
            public:
                FindNodeRequest(QNode dest, QKey key, QObject* observer,
                    quint32 parent = 0);
        };

        class FindValueRequest : public FindRequest
        {
            public:
                FindValueRequest(QNode dest, QKey key, QObject* observer,
                    quint32 parent = 0);
                FindValueRequest(const FindValueRequest& other);

                virtual void UpdateResults(QList<QNode> results);

            signals:
                void ResourceNotFound(QKey);
        };
};

#endif // PEER_CACHE_REQUEST_MANAGER_HH
