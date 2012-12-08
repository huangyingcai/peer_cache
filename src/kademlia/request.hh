#ifndef KADEMLIA_REQUEST_HH
#define KADEMLIA_REQUEST_HH

#include "types.hh"
#include "constants.hh"

class Request
{
    public:
        virtual bool IsValidDestination(QNode node) = 0;
        virtual void Update() = 0;

        int get_type() { return type_; };

    protected:
        int type_;
};

class SimpleRequest : public Request
{
    public:
      // TODO: change type enum type
        ~SimpleRequest();

        virtual bool IsValidDestination(QNode node);
        virtual void Update() = 0;

        QNode get_destination() { return *destination_; };

    protected:
        QNode* destination_;
};

class PingRequest : public SimpleRequest
{
    public:
        PingRequest(QNode dest);

        virtual void Update() {};
};

class StoreRequest : public SimpleRequest
{
    public:
        StoreRequest(QNode dest, QKey key);
        ~StoreRequest();

        virtual void Update() {};

        QKey get_resource_key() { return *resource_key_; };

    private:
        QKey* resource_key_;
};

class FindRequest : public Request
{
    public:
        ~FindRequest();

        virtual bool IsValidDestination(QNode node);
        bool ResultsSortOrder(const QNode& n1, const QNode& n2);
        virtual void Update() = 0;
        virtual QNodeList Update(QNode destination, QNodeList nodes);

        QNodeList get_destinations() { return *destinations_; };
        QNodeList get_results() { return *results_; };

    protected:
        QNodeList* destinations_;
        QNodeList* results_;
};

class FindNodeRequest : public FindRequest
{
    public:
        FindNodeRequest(QNodeList destinations, QNodeId id);
        ~FindNodeRequest();

        virtual void Update() {};

        QNodeId get_requested_node_id() { return *requested_node_id_; };

    private:
        QNodeId* requested_node_id_;
};

class FindValueRequest : public FindRequest
{
    public:
        FindValueRequest(QNodeList destinations, QKey key);
        ~FindValueRequest();

        virtual void Update() {};

        QNodeId get_requested_key() { return *requested_key_; };
        void set_found_value(bool found) { found_value_ = found; };
        bool get_found_value() { return found_value_; };

    private:
        QKey* requested_key_;
        bool found_value_;
};

#endif // KADEMLIA_REQUEST_HH
