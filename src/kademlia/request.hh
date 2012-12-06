#ifndef KADEMLIA_REQUEST_HH
#define KADEMLIA_REQUEST_HH

#include "types.hh"
#include "constants.hh"

#include <QHash>
#include <QTimer>

class Request
{
    public:
        static quint32 RandomId();

        // Request lifecycle
        Request(int type, QNode dest, QObject* manager,
            FindRequest* parent = NULL); // FIXME: that's ugly with FindRequest*
        ~Request();
        virtual void Update(QNodeList nodes = QNodeList());
        void Terminate();

        quint32 get_id() { return id_; };
        int get_type() { return type_; };
        QNode get_destination() { return *destination_; };
        FindRequest* get_parent() { return parent_; };
        QKey get_resource_key() { // FIXME??
            return (resource_key_ ? *resource_key_ : QKey());
        }

    protected:
        // Basic attributes
        quint32 id_;
        int type_;
        QNode* destination_;
        FindRequest* parent_; // Observer
        QObject* manager_; // Observer

        // Optional Data Field for requested/specified resource
        QKey* resource_key_;
};

class PingRequest : public Request
{
    public:
        PingRequest(QNode dest, QObject* manager, FindRequest* parent = NULL);
};

class StoreRequest : public Request
{
    public:
        StoreRequest(QNode dest, QKey key, QObject* manager,
            FindRequest* parent = NULL);
        ~StoreRequest();
};

class FindRequest : public Request
{
    public:
        FindRequest(int type, QNode dest, QKey key, QObject* manager,
            FindRequest* parent = NULL);
        ~FindRequest();

        Request* get_child(child_id) { return children_->value(child_id); };
        QNodeList get_results() { return *results_; };

        virtual void Update(QNodeList nodes);

        // Child management
        void MakeChild(QNode dest);
        void ProcessChildCompletion(quint32 child_id);
        void RemoveChild(quint32 child_id);

    protected:
        QHash<quint32, Request*>* children_;
        QNodeList* results_;
};

class FindNodeRequest : public FindRequest
{
    public:
        FindNodeRequest(QNode dest, QKey key, QObject* manager,
            FindRequest* parent = NULL);

        virtual void ProcessChildCompletion(quint32 child_id)
};

class FindValueRequest : public FindRequest
{
    public:
        FindValueRequest(QNode dest, QKey key, QObject* manager,
            FindRequest* parent = NULL);

        virtual void ProcessChildCompletion(quint32 child_id);
};

#endif // KADEMLIA_REQUEST_HH
