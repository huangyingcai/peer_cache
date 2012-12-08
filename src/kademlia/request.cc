#include "utilities.hh"
#include "includes.hh"
#include "request.hh"

#include <QtAlgorithms>

// Simple Request

SimpleRequest::~SimpleRequest()
{
    delete destination_;
}

bool SimpleRequest::IsValidDestination(QNode node)
{
    return destination_->second == node.second;
}

// PingRequest

PingRequest::PingRequest(QNode dest) : type_(PING)
{
    destination_ = new QNode(dest.first, dest.second);
}

// StoreRequest

StoreRequest::StoreRequest(QNode dest, QKey key) : type_(STORE)
{
    destination_ = new QNode(dest.first, dest.second);
    resource_key_ = new QKey(key);
}

StoreRequest::~StoreRequest()
{
    delete resource_key_;
}

// FindRequest

FindRequest::~FindRequest()
{
    delete destinations_;
    delete results_;
}

bool FindRequest::IsValidDestination(QNode node)
{
    QNodeList::const_iterator dest;
    for (dest = destinations_.constBegin(); dest != destinations_.constEnd();
            dest++) {
        if (dest->second == node.second) return true;
    }
    return false;
}

bool FindRequest::ResultsSortOrder(const QNode& n1, const QNode& n2)
{
    QBitArray a1 = Distance(n1.first, *resource_key_);
    QBitArray a2 = Distance(n2.first, *resource_key_);
    for (int i = 0; i < kKeyLength * 8; i++) {
        if (a1[i] > a2[i]) return true;
        if (a1[i] < a2[i]) return false;
    }
    return false;
}

QNodeList FindRequest::Update(QNode destination, QNodeList nodes)
{
    destinations_->remove(destination);
    qsort(nodes, ResultsSortOrder);

    QNodeList new_destinations = QNodeList();
    QNodeList::iterator i, j;
    for (i = nodes.begin(); i != nodes.end(); i++) {
        if (results_->indexOf(*i) > 0) continue;
        if (results_->size() < kBucketSize) {
            results_->append(*i);
            new_destinations.append(*i);
            destinations_->append(*i);
        } else {
            for (j = results_->begin(); j != results_->end(); j++) {
                QBitArray new_dist =
                    Distance(i->first, *resource_key_);
                QBitArray dist =
                    Distance(j->first, *resource_key_);
                if (new_dist > dist) {
                    results_->insert(j, *i);
                    results_->removeLast();
                    new_destinations.append(*i);
                    destinations_->append(*i);
                    break;
                }
            }
        }
    }
    return new_destinations;
}

// Find Node Request

FindNodeRequest::FindNodeRequest(QNodeList destinations, QNodeId id) :
    type_(FIND_NODE)
{
    destinations_ = new QNodeList(destinations);
    results_ = new QNodeList();
    requested_node_id_ = new QNodeId(id);
}

// Find Value Request

FindValueRequest::FindValueRequest(QNodeList destinations, QKey key) :
    type_(FIND_VALUE), found_value_(false)
{
    destinations_ = new QNodeList(destinations);
    results_ = new QNodeList();
    requested_key_ = new QKey(key);
}
