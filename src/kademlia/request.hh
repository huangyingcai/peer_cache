#ifndef KADEMLIA_REQUEST_HH
#define KADEMLIA_REQUEST_HH

#include "types.hh"
#include "constants.hh"

class Request
{
    public:
        virtual bool IsValidDestination(QNode node) = 0;
        virtual void Update() = 0;

        quint32 get_request_number() { return request_number_; };
        int get_type() { return type_; };

    protected:
        quint32 request_number_;
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

    private:
        QNode* destination_;
};

class PingRequest : public SimpleRequest
{
    public:
        PingRequest(quint32 request_number, QNode dest);
};

class StoreRequest : public SimpleRequest
{
    public:
        StoreRequest(quint32 request_number, QNode dest, QKey key);
        ~StoreRequest();

        QKey get_resource_key() { return *resource_key_; };

    private:
        QKey* resource_key_;
};

class FindRequest : public Request
{
    public:
        ~FindRequest();

        virtual bool IsValidDestination(QNode node);
        bool ResultsSortOrder(const QBitArray& a1, const QBitArray& a2);
        virtual void Update() = 0;
        virtual QNodeList Update(QNodeList nodes);

        QNodeList get_destinations() { return *destinations_; };
        QNodeList get_results() { return *results_; };

    protected:
        QNodeList* destinations_;
        QNodeList* results_;
};

class FindNodeRequest : public FindRequest
{
    public:
        FindNodeRequest(quint32 request_number, QNodeList destinations,
            QNodeId id);
        ~FindNodeRequest();

        QNodeId get_requested_node_id() { return *requested_node_id_; };

    private:
        QNodeId* requested_node_id_;
};

class FindValueRequest : public FindRequest
{
    public:
        FindValueRequest(quint32 request_number, QNodeList destinations
            QKey key);
        ~FindValueRequest();

        QNodeId get_requested_key() { return *requested_key_; };
        bool set_found_value(bool found) { found_value_ = found; };
        bool get_found_value() { return found_value_; };

    private:
        QKey* requested_key_;
        bool found_value_;
};

#endif // KADEMLIA_REQUEST_HH
