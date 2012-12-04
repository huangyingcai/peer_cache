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

        // FIXME: STORE doesn't really need mgmt b/c no verification?
        // Verify that was downloaded?
        static Request* PingRequest(QNodeId dest, RequestManager* mgr,
            Request* parent = NULL);
        static Request* FindValueRequest(int type, QNodeId dest,
            QUrl url, Request* parent = NULL);
        static Request* FindNodeRequest(int type, QNodeId dest,
            QNodeId node, Request* parent = NULL);
        static void RemoveRequest(quint32 id);
        // PingRequest(dest, parent = 0) : Req (PING, dest, parent)
        // FindNodeReq(key, dest, par = 0) : FindReq (FIND_NODE, key, dest, par)
        // FindValueReq(url, dest, par = 0) : FindReq (FIND_VAL, ....)
        //             (key, dest, par = 0) : ...
        // StoreReq(key, dest);
        // Different Completion Procedures
        class Request : public QObject
        {
            Q_OBJECT

            // FIXME: dest should be QNode
            public:
                static quint32 RandomId();
                static QList<quint32, Request> get_requests();
                static Request* Get(quint32 id);

                Request(int type, QNodeId dest, quint32 parent);
                Request(int type, QNodeId dest, QUrl url, quint32 parent);
                Request(int type, QNodeId dest, QNodeId id, quint32 parent); // FIXME
                Request(const Request& other);
                quint32 get_id { return id_; };

                void UpdateResults(QList<QNodeId> results);
                quint32 MakeChildFind(QNodeId id);
                quint32 MakeChildPing(QNodeId id); // FIXME

            public slots:
                void ProcessChildCompletion(Request* child);

            signals:
            //FIXME
                void Ready(quint32); // FIXME: connect to has_whatever
                void Complete(quint32);
                void ChildComplete(quint32);
                void MissingResource(QUrl);

            private:
                static ConnectToParent(Request* child, Request* parent);
                static QHash<quint32, Request> requests_;

                quint32 id_;
                int type_;
                QNodeId destination_
                quint32 parent_;
                QList<quint32> children_;

                // Optional data parameters (for FIND_* requests)
                QUrl requested_url_;
                QKey requested_key_;
                QList<QNodeId> results_;
        };
};

#endif // PEER_CACHE_REQUEST_MANAGER_HH
