#include "includes.hh"
#include "utilities.hh"
#include "request.hh"
#include "request_manager.hh"

RequestManager::RequestManager(QNodeId id) : initialized_(false)
{
    node_id_ = new QNodeId(id);
    buckets_ = new QNodeList*[kKeyLength * 8];
    // Initialize buckets
    for (int i = 0; i < kKeyLength * 8; i++) {
        buckets_[i] = new QNodeList();
    }
    requests_ = new QHash<quint32, Request*>();
    request_number_to_id_map_ = new QHash<quint32, quint32>();

    qDebug() << "Request Manager created";
}

RequestManager::~RequestManager()
{
    delete node_id_;
    for (int i = 0; i < kKeyLength * 8; i++) {
        delete buckets_[i];
    }
    delete [] buckets_;
    delete requests_;
    delete request_number_to_id_map_;
}

// TODO: Make sure called before all else
void RequestManager::RequestManager::Init(QNodeAddress bootstrap_address)
{
    if (!initialized_) {
        qDebug() << "Initializing Request Manager";

        //QNode node = qMakePair(QByteArray(), bootstrap_address);
        QNode node = qMakePair(QByteArray(), qMakePair(QHostAddress(QHostAddress::Broadcast),
              (quint16)42600));
        QNodeList nodes;
        nodes << node;

        // TODO: fix this
        quint32 request_id = RandomId();
        quint32 request_number = RandomRequestNumber();

        FindNodeRequest* req = new FindNodeRequest(nodes, *node_id_);
        requests_->insert(request_id, req);
        request_number_to_id_map_->insert(request_number, request_id);
        emit HasRequest(FIND_NODE, request_number, node, *node_id_);

        initialized_ = true;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Kademlia State Management

void RequestManager::UpdateBuckets(QNode node)
{
    if (node.first == QString(node_id_->constData())) return;

    quint16 b = Bucket(node.first);

    int i = buckets_[b]->indexOf(node);
    if (i > 0) buckets_[b]->removeAt(i);

    // TODO: PING procedure
    buckets_[b]->append(node);
    qDebug() << "Updating bucket" << b << " with node " << node;
}

QNodeList RequestManager::ClosestNodes(QKey key, quint16 num)
{
    QList<QNode> nodes;

    for (qint16 b = Bucket(key); b >= 0 && nodes.size() < num; b--) {
        QList<QNode>::const_iterator i;
        for (i = buckets_[b]->constBegin();
                i != buckets_[b]->constEnd() && nodes.size() < num; i++) {
            nodes.append(*i);
        }
    }

    return nodes;
}

// FIXME: this is so bad
void RequestManager::RefreshBucket(quint16 bucket)
{
    ERROR("NOT IMPLEMENTED- REFRESH BUCKET");
    return;

//     QBitArray bits(kKeySize * 8, 0);
//     // FIXME: this is backwards?
//     for (int b = kKeySize - bucket; b <= kKeySize; b++) {
//         bits[b] = rand() % 2;
//     }
// 
//     QNodeId random_node_id;
//     for (int b = 0; b < bits.count(); b++) {
//           random_node_id[b/8] =
//               (random_node_id.at(b / 8) | ((bits[b] ? 1 : 0) << (b % 8)));
//     }
// 
//     // FIXME:
//     new PingRequest(random_node_id);
}

////////////////////////////////////////////////////////////////////////////////
// Private Helpers

quint32 RequestManager::RandomId()
{
    quint32 request_id = (quint32) qrand();
    while (request_id != 0 && requests_->value(request_id) != NULL) {
        request_id = (quint32) qrand();
    }
    return request_id;
}

quint32 RequestManager::RandomRequestNumber()
{
    quint32 request_number = (quint32) qrand();
    while (request_number_to_id_map_->value(request_number) != 0) {
        request_number = (quint32) qrand();
    }
    return request_number;
}

quint16 RequestManager::Bucket(QKey key)
{
    QBitArray dist = Distance(*node_id_, key);

    quint16 bucket;
    for (bucket = dist.size() - 1; !dist[bucket]; bucket--); // Find MSB

    return bucket;
}

////////////////////////////////////////////////////////////////////////////////
// Kademlia RPCs

void RequestManager::IssuePing(QNode node)
{
    qDebug() << "Issuing ping to: " << node;

    quint32 request_id = RandomId();
    quint32 request_number = RandomRequestNumber();

    PingRequest* ping = new PingRequest(node);
    requests_->insert(request_id, ping);
    request_number_to_id_map_->insert(request_number, request_id);

    emit HasRequest(ping->get_type(), request_number, ping->get_destination());
}

void RequestManager::IssueStore(QKey key)
{
    qDebug() << "Issuing store for: " << key;

    QNodeList closest = ClosestNodes(key, 1);
    if (closest.isEmpty()) return; // This node is supposed to store it
    QNode dest = closest.takeFirst();

    quint32 request_id = RandomId();
    quint32 request_number = RandomRequestNumber();

    StoreRequest* store = new StoreRequest(dest, key);
    requests_->insert(request_id, store);
    request_number_to_id_map_->insert(request_number, request_id);

    emit HasRequest(store->get_type(), request_number,
        store->get_destination(), store->get_resource_key());
}

// TODO: DRY
void RequestManager::IssueFindNode(QNodeId id)
{
    qDebug() << "Issuing find node: " << id;

    QNodeList closest = ClosestNodes(id);
    if (closest.isEmpty()) return; // This node is supposed to store it

    quint32 request_id = RandomId();

    // FIXME: messed up the reques_id thing
    FindNodeRequest* find_node = new FindNodeRequest(closest, id);
    requests_->insert(request_id, find_node);

    QNodeList::const_iterator d;
    QNodeList destinations = find_node->get_destinations();
    for (d = destinations.constBegin(); d != destinations.constEnd(); d++) {
        quint32 request_number = RandomRequestNumber();
        request_number_to_id_map_->insert(request_number, request_id);

        emit HasRequest(find_node->get_type(), request_number, *d,
            find_node->get_requested_node_id());
    }
}

void RequestManager::IssueFindValue(QKey key)
{
    qDebug() << "Issuing find value: " << key;

    QNodeList closest = ClosestNodes(key);
    if (closest.isEmpty()) {
        HandleMissingResource(key);
        return;
    }

    quint32 request_id = RandomId();

    FindValueRequest* find_value = new FindValueRequest(closest, key);
    requests_->insert(request_id, find_value);

    QNodeList::const_iterator d;
    QNodeList destinations = find_value->get_destinations();
    for (d = destinations.constBegin(); d != destinations.constEnd(); d++) {
        quint32 request_number = RandomRequestNumber();
        request_number_to_id_map_->insert(request_number, request_id);

        emit HasRequest(find_value->get_type(), request_number, *d,
            find_value->get_requested_key());
    }
}

////////////////////////////////////////////////////////////////////////////////
// Request Management

void RequestManager::UpdateRequest(quint32 request_number, QNode destination)
{

    quint32 request_id = request_number_to_id_map_->value(request_number);
    Request* request = requests_->value(request_id);
    if (!request) {
        qDebug() << "Could not find request for request number: " <<
            request_number;
        return;
    }

    if (!request->IsValidDestination(destination)) {
        qDebug() << "Incorrect node replied to request";
        return;
    }

    switch (request->get_type()) {
        case PING:
            // TODO: see if anything pending
        case STORE:
            requests_->remove(request_id);
            request_number_to_id_map_->remove(request_number);
            delete request;
            break;
        case FIND_VALUE: // TODO: this is hack-y for now
            ((FindValueRequest*)request)->set_found_value(true);
            // HandleFoundValue(destination,
            //    ((FindValueRequest*)request)->get_requested_key());
            break;
        case FIND_NODE: // No-op
            break;
        default:
            ERROR("Did not recognize request type");
            break;
    }
}

// TODO: destination -> from_node/responding_node
void RequestManager::UpdateRequest(quint32 request_number, QNode destination,
    QNodeList nodes)
{
    quint32 request_id = request_number_to_id_map_->value(request_number);
    Request* request = requests_->value(request_id);
    if (!request) {
        qDebug() << "Could not find request for request number: " <<
            request_number;
        return;
    }

    if (!request->IsValidDestination(destination)) {
        qDebug() << "Incorrect node replied to request";
        return;
    }

    // Update buckets. Delete the source node if it exists
    QNodeList::iterator i;
    for (i = nodes.begin(); i != nodes.end(); i++) {
        if (i->first == QString(node_id_->constData())) {
            i = nodes.erase(i) - 1;
        } else {
            UpdateBuckets(*i);
        }
    }

    FindRequest* find_request = dynamic_cast<FindRequest*>(request);
    if (!find_request) return;

    QNodeList new_destinations = find_request->Update(destination, nodes);

    // Issue requests for any new destinations
    QNodeList::const_iterator d;
    for (d = new_destinations.constBegin(); d != new_destinations.constEnd();
            d++) {
        quint32 request_number = RandomRequestNumber();
        request_number_to_id_map_->insert(request_number, request_id);

        if (find_request->get_type() == FIND_NODE) {
            emit HasRequest(find_request->get_type(), request_number, *d,
                ((FindNodeRequest*)find_request)->get_requested_node_id());
        } else {
            emit HasRequest(find_request->get_type(), request_number, *d,
                ((FindValueRequest*)find_request)->get_requested_key());
        }
    }

    // If there are no more requests outstanding, close the request
    if (find_request->get_destinations().isEmpty()) {
        requests_->remove(request_id);
        request_number_to_id_map_->remove(request_number);
        if (find_request->get_type() == FIND_VALUE &&
                ((FindValueRequest*)find_request)->get_found_value() == false) {
            HandleMissingResource(
                ((FindValueRequest*)request)->get_requested_key());
        }
        delete request;
    }
}

void RequestManager::HandleMissingResource(QKey key)
{
    qDebug() << "Method stub for RequestManager::HandleMissingResource";
    qDebug() << "Could not find resource for key: " << key;
}
