#ifndef PEER_CACHE_REQUEST_MANAGER_HH
#define PEER_CACHE_REQUEST_MANAGER_HH

class QBitArray;
class QList;
class QNode;

class RequestManager : public QObject
{
    Q_OBJECT

    public:
        RequestManager(QNodeId id, QObject* parent = 0);

        // http://qt-project.org/wiki/WorkingWithRawData
        static QBitArray Distance(QKey a, QKey b);
        // static quint16 toInt(QBitArray arr);
        quint16 Bucket(QKey key);

    public slots:
        void UpdateRequest(quint32 request_id, QNodeAddress addr,
            QList<QNodeId> results = QList<QNodeId>());
        void CloseRequest(quint32 request_id);
        void InitiateRequest(quint32 request_id);

    signals:
        void HasRequest(Request*);

    private:
        QNodeId node_id_;
        QList<QNode>[160] buckets_;

        QList<QNode> ClosestNodes(QKey key, quint16 num = kBucketSize);
        void RefreshBucket(quint16 bucket);
        void UpdateBucket(quint16 bucket, QNode node);

        class Request : public QObject
        {
            Q_OBJECT

            public:
                // Factory Methods -- TODO - this is sloppy
                static void RegisterRequest(quint32 id, const Request* req);
                static void RemoveRequest(quint32 id);
                static quint32 PingRequest(QNodeId dest, QObject* observer,
                    Request* parent = NULL);
                // static quint32 StoreRequest() TODO
                static quint32 FindNodeRequest(QNode dest, QNodeId id,
                    QObject* observer, Request* parent = NULL);
                static quint32 FindNodeRequest(QNode dest, QUrl url,
                    QObject* observer, Request* parent = NULL);

                static quint32 RandomId();
                static QList<quint32, Request> get_requests();
                static Request* Get(quint32 id);

                Request(int type, QNode dest, quint32 parent);
                Request(const Request& other);
                quint32 get_id { return id_; };

                virtual void UpdateResults(QList<QNodeId> results);

            public slots:

            signals:
                //FIXME
                void Ready(quint32); // FIXME: connect to has_whatever
                void Complete(quint32);
                void ChildComplete(quint32);

            private:
                static QHash<quint32, Request> requests_;

                quint32 id_;
                int type_;
                QNode destination_
                quint32 parent_;
                QList<quint32> children_;
        };

        class PingRequest : public Request
        {
            public:
                PingRequest(QNode dest, quint32 parent = 0);
                PingRequest(const PingRequest& other);
        };

        class FindRequest : public Request
        {
            public:
                FindRequest(int type, QNode dest, QKey key, quint32 parent = 0);
                FindRequest(const FindRequest& other);

                virtual void UpdateResults(QList<QNodeId> results);
                virtual void MakeChild(QNode dest) = 0;
                void ProcessChildCompletion(Request* child);

            private:
                QKey requested_key_;
                QList<QNodeId> results_;
        };

        class FindNodeRequest : public FindRequest
        {
            public:
                FindNodeRequest(QNode dest, QKey key, quint32 parent = 0);
        };

        class FindValueRequest : public FindRequest
        {
            public:
                FindValueRequest(QNode dest, QKey key, quint32 parent = 0);
                FindValueRequest(QNode dest, QUrl url, quint32 parent = 0);
                FindValueRequest(const FindValueRequest& other);

                virtual void UpdateResults(QList<QNodeId> results);

            signals:
                void ResourceNotFound(QUrl);

            private:
                QUrl requested_url_;
        };
};

#endif // PEER_CACHE_REQUEST_MANAGER_HH
