#include "includes.hh"
#include "utilities.hh"
#include "request.hh"
#include "request_manager.hh"

QHash<quint32, Request*> Request::requests_;

RequestManager::RequestManager(QNodeId id, QNodeAddress bootstrap_addr,
    QObject* parent) : QObject(parent)
{
    node_id_ = id;
    initialized_ = false;
    bootstrap_addr_ = qMakePair(bootstrap_addr.first, bootstrap_addr.second);
    // Initialize buckets
    for (int i = 0; i < kKeyLength * 8; i++) {
        buckets_[i] = new QNodeList;    // TODO: memory
    }
    qDebug() << "Request Manager created";
}

// TODO: Make sure called before all else
void RequestManager::RequestManager::Init()
{
    if (!initialized_) {
        QNode node(QByteArray(), bootstrap_addr_);
        FindNodeRequest* req =
            new FindNodeRequest(node, node_id_, this);
        req->Init();
        initialized_ = true;
    }
}

quint16 RequestManager::Bucket(QKey key)
{
    QBitArray dist = Distance(node_id_, key);

    quint16 bucket;
    for (bucket = dist.size() - 1; !dist[bucket]; bucket--); // Find MSB

    return bucket;
}

// TODO: verify that responded from right node
void RequestManager::UpdateRequest(quint32 request_id, QNodeList nodes)
{
    qDebug() << "Updating request: " << request_id;
    QNodeList::iterator i;
    for (i = nodes.begin(); i != nodes.end(); i++) {
        UpdateBuckets(*i);
    }

    // Delete the source node if it exists
    for (i = nodes.begin(); i != nodes.end(); i++) {
        if (i->first == QString(node_id_.constData())) {
            nodes.erase(i);
            break;
        }
    }

    Request* req;
    if ((req = Request::Get(request_id))) {
        req->UpdateResults(nodes); // Works for PING
    }
    qDebug() << "Request Updated";
}

void RequestManager::CloseRequest(quint32 request_id)
{
    Request* req;
    if ((req = Request::Get(request_id))) {
        // Close Children
        QList<quint32>::const_iterator i;
        for (i = req->get_children().constBegin();
                i != req->get_children().constEnd(); i++) {
            CloseRequest(*i);
        }

        // De-register request
        Request::RemoveRequest(request_id);
        // Free memory allocated by Issue*() methods
        delete req;
    }
    // FIXME: need to deregister with parent and also, need to kill parents for
    // FIND_VALUE

    //     // Close Parent
    //     if (req->get_parent()) {
    //         CloseRequest(req->get_parent()->get_id());
    //     }

}

void RequestManager::InitiateRequest(Request* req)
{
    emit HasRequest(req->get_type(), req->get_id(), req->get_destination(),
        req->get_requested_key());
}

void RequestManager::IssueStore(QKey key)
{
    QNodeList closest = ClosestNodes(key, 1);

    if (closest.isEmpty()) return; // This node is supposed to store it

    QNode node = closest.takeFirst();
    StoreRequest* req = new StoreRequest(node, key, this);
    req->Init();
}

// TODO: DRY
void RequestManager::IssueFindNode(QNodeId id)
{
    QNodeList closest = ClosestNodes(id);

    if (closest.isEmpty()) return; // This node is supposed to store it

    QNodeList::const_iterator i = closest.constBegin();
    // Choose one arbitrarily to be the parent
    FindNodeRequest* parent = new FindNodeRequest(*i, id, this);
    parent->Init();
    for (i += 1; i != closest.constEnd(); i++) {
        FindNodeRequest* req = new FindNodeRequest(*i, id, parent);
        req->Init();
    }
}

void RequestManager::IssueFindValue(QNodeId id)
{
    QNodeList closest = ClosestNodes(id);

    if (closest.isEmpty()) return; // This node is supposed to store it
    // FIXME: Emit node found

    QNodeList::const_iterator i = closest.constBegin();
    // Choose one arbitrarily to be the parent
    FindValueRequest* parent = new FindValueRequest(*i, id, this);
    parent->Init();
    for (i += 1; i != closest.constEnd(); i++) {
        FindValueRequest* req = new FindValueRequest(*i, id, parent);
        req->Init();
    }
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

void RequestManager::UpdateBuckets(QNode node)
{
    // TODO: PING procedure
    if (node.first == QString(node_id_.constData())) return;

    quint16 b = Bucket(node.first);

    int i = buckets_[b]->indexOf(node);
    if (i > 0) buckets_[b]->removeAt(i);

    buckets_[b]->append(node);
    qDebug() << "Updating bucket" << b << " with node " << node;
}
