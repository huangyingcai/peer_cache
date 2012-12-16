#include "utilities.hh"
#include "includes.hh"
#include "request.hh"

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

PingRequest::PingRequest(QNode dest)
{
    type_ = PING;
    destination_ = new QNode(dest.first, dest.second);
}

// StoreRequest

StoreRequest::StoreRequest(QNode dest, QKey key)
{
    type_ = STORE;
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
    delete requested_key_;
    delete destinations_;
    delete results_;
}

bool FindRequest::IsValidDestination(QNode node)
{
    QNodeList::const_iterator dest;
    for (dest = destinations_->constBegin(); dest != destinations_->constEnd();
            dest++) {
        if (dest->second == node.second) return true;
    }
    return false;
}

QNodeList FindRequest::Update(QNode destination, QNodeList nodes)
{
    destinations_->removeAll(destination);
    QNodeList sorted;
    while (!nodes.isEmpty()) {
        QNode node = nodes.takeFirst();
        QBitArray distance = Distance(node.first, *requested_key_);

        // Figure out where to insert the node
        QNodeList::iterator i;
        for (i = sorted.begin(); i != sorted.end(); i++) {
            QBitArray current_distance = Distance(i->first, *requested_key_);
            if (distance > current_distance) break;
        }
        sorted.insert(i, node);
    }

    QNodeList new_destinations = QNodeList();
    QNodeList::iterator i, j;
    for (i = sorted.begin(); i != sorted.end(); i++) {
        if (results_->indexOf(*i) > 0) continue;
        if (results_->size() < kBucketSize) {
            results_->append(*i);
            new_destinations.append(*i);
            destinations_->append(*i);
        } else {
            QBitArray distance = Distance(i->first, *requested_key_);
            for (j = results_->begin(); j != results_->end(); j++) {
                QBitArray current_distance =
                    Distance(j->first, *requested_key_);
                if (distance > current_distance) {
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

FindNodeRequest::FindNodeRequest(QNodeList destinations, QNodeId id)
{
    type_ = FIND_NODE;
    destinations_ = new QNodeList(destinations);
    results_ = new QNodeList();
    requested_key_ = new QNodeId(id);
}

// Find Value Request

FindValueRequest::FindValueRequest(QNodeList destinations, QKey key) :
    found_value_(false)
{
    type_ = FIND_VALUE;
    destinations_ = new QNodeList(destinations);
    results_ = new QNodeList();
    requested_key_ = new QKey(key);
}
