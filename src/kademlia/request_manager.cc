#include "includes.hh"
#include "utilities.hh"
#include "request.hh"
#include "request_manager.hh"

QHash<quint32, Request*> Request::requests_;

RequestManager::RequestManager(QNodeId id, QNodeAddress bootstrap_node,
    QObject* parent) : QObject(parent)
{
    node_id_ = id;
    for (int i = 0; i < kKeyLength * 8; i++) {
        buckets_[i] = new QNodeList;
    }

    QNode node = qMakePair(QByteArray(), bootstrap_node); // FIXME
    new FindNodeRequest(node, id, this);
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
    QList<QNode>::const_iterator i;
    for (i = nodes.constBegin(); i != nodes.constEnd(); i++) {
        UpdateBuckets(*i);
    }

    Request* req;
    if ((req = Request::Get(request_id))) {
        req->UpdateResults(nodes); // Works for PING
    }
}

void RequestManager::CloseRequest(quint32 request_id)
{
    Request* req;
    if ((req = Request::Get(request_id))) {
        Request::RemoveRequest(request_id);

        // Close Children
        QList<quint32>::const_iterator i;
        for (i = req->get_children().constBegin(); i != req->get_children().constEnd();
                i++) {
            CloseRequest(*i);
        }

        // Close Parent
        if (req->get_parent()) {
            CloseRequest(req->get_parent()->get_id());
        }
    }
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
    new StoreRequest(node, key, this);
}

QList<QNode> RequestManager::ClosestNodes(QKey key, quint16 num)
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
    quint16 b = Bucket(node.first);

    int i = buckets_[b]->indexOf(node);
    if (i > 0) buckets_[b]->removeAt(i);

    buckets_[b]->append(node);
}
