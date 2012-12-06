#ifndef KADEMLIA_REQUEST_HH
#define KADEMLIA_REQUEST_HH

#include "types.hh"
#include "constants.hh"

#include <QHash>
#include <QTimer>

class Request : public QObject
{
    Q_OBJECT

    public:
        static quint32 RandomId();
        static Request* Get(quint32 id);
        static void RegisterRequest(quint32 id, Request* req);
        static void RemoveRequest(quint32 id);

        Request(int type, QNode dest, QObject* observer,
            Request* parent = NULL);
        void Init();

        quint32 get_id() { return id_; };
        int get_type() { return type_; };
        QNode get_destination() { return destination_; };
        QList<quint32> get_children() { return children_; };
        Request* get_parent() { return parent_; };
        virtual QKey get_requested_key() { return QByteArray(); };

    public slots:
        void AddChild(quint32 id);
        virtual void UpdateResults(QNodeList nodes = QNodeList());
        void Timeout();

    signals:
        void Ready(Request*);
        void Complete(quint32);

    protected:
        const static quint32 kDefaultTimeout = 10000;
        static QHash<quint32, Request*> requests_;

        quint32 id_;
        int type_;
        QNode destination_;
        Request* parent_;
        QObject* observer_;
        QList<quint32> children_;

        QTimer timer_;
};

class PingRequest : public Request
{
    Q_OBJECT

    public:
        PingRequest(QNode dest, QObject* observer, Request* parent = NULL);
};

class StoreRequest : public Request
{
    Q_OBJECT

    public:
        StoreRequest(QNode dest, QKey key, QObject* observer,
            Request* parent = NULL);

    protected:
        QKey resource_key_;
};

class FindRequest : public Request
{
    Q_OBJECT

    public:
        FindRequest(int type, QNode dest, QKey key, QObject* observer,
            Request* parent = NULL);

        virtual QKey get_requested_key() { return requested_key_; };
        virtual void UpdateResults(QList<QNode> results);

    protected slots:
        void ProcessChildCompletion(quint32 child_id);

    protected:
        void MakeChild(QNode dest);

        QKey requested_key_;
        QNodeList results_;

};

class FindNodeRequest : public FindRequest
{
    Q_OBJECT

    public:
        FindNodeRequest(QNode dest, QKey key, QObject* observer,
            Request* parent = NULL);
};

class FindValueRequest : public FindRequest
{
    Q_OBJECT

    public:
        FindValueRequest(QNode dest, QKey key, QObject* observer,
            Request* parent = NULL);

        virtual void UpdateResults(QList<QNode> results);

    signals:
        void ResourceNotFound(QKey);
};

#endif // KADEMLIA_REQUEST_HH
