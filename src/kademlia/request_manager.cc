#include "constants.hh"
#include "includes.hh"

#include "request_manager.hh"

static QBitArray RequestManager::Distance(QKey a, QKey b)
{
     // Create bit arrays of the appropriate size
     QBitArray a_bits(kKeyLength * 8);
     QBitArray b_bits(kKeyLength * 8);

     // Convert from QByteArray to QBitArray
     for (int i = 0; i < kKeyLength; ++i) {
         for (int b = 0; b < 8; ++b) {
                 a_bits.setBit(i*8 + b, a_bytes.at(i) & (1 << b));
                 b_bits.setBit(i*8 + b, b_bytes.at(i) & (1 << b));
         }
     }

     return a_bits & b_bits;
}

RequestManager::RequestManager(QObject* parent = 0) : QObject(parent)
{
    // TODO: join algorithm
}

quint16 RequestManager::Bucket(QKey key)
{
    QBitArray dist = RequestManager.distance(requested_key_, key);

    quint bucket = dist.size() - 1;
    for (; !dist[bucket]; bucket--); // Find MSB

    return bucket;
}

void RequestManager::UpdateRequest(quint32 request_id, QNodeAddress addr,
    QList<QNodeId> results);
{
    Request* req;
    if (req = Request.Get(request_id)) {
        req->UpdateResults(results); // Works for PING
    }
}

void RequestManager::CloseRequest(quint32 request_id)
{
    Request* req;
    if (req = Request.Get(request_id)) {
        QList<quint32>::const_iterator i;
        for (i = req->children.constBegin(); i != req->children.constEnd();
                i++) {
            CloseRequest(i->get_id());
        }
        RemoveRequest(request_id);
    }
}

void RequestManager::InitiateRequest(quint32 request_id)
{
    emit HasRequest(Request.Get(request_id));
}

void RequestManager::ClosestNodes(QKey key, quint16 num);
void RequestManager::RefreshBucket(quint16 bucket);
void RequestManager::UpdateBucket(quint16 bucket, QNode node);
{
  // Start just by adding
}

////////////////////////////////////////////////////////////////////////////////
// RequestManager::Request Implementation

static quint32 RequestManager::Request::RandomId()
{
    static bool initialized = false;
    if (!initialized) {
        qsrand(QTime::currentTime().msec());
        initialized = true;
    }
    return qrand();
}

static QList<quint32, Request> RequestManager::Request::get_requests()
{
    return requests_;
}

static ConnectToParent(Request* child, Request* parent)
{
    connect(child, SIGNAL(ChildComplete(quint32)), parent,
        SLOT(ProcessChildCompletion(quint32))); // TODO: Qt::QueuedConnection?
}

// TODO: DRY!!!
static Request* RequestManager::Request::PingRequest(int type, QNodeId dest,
    Request* parent = NULL)
{
    Request new_request(PING, dest, (parent ? parent->id_ : 0));

    requests_.insert(new_request->get_id(), new_request);
    if (parent) ConnectToParent(new_request, parent);

    return new_request->get_id();
}

static quint32 RequestManager::Request::FindValueRequest(QNodeId dest,
    QUrl url, Request* parent = NULL)
{
    Request new_request(FIND_VALUE, dest, url, (parent ? parent->id_ : 0));
    requests_.insert(new_request->get_id(), new_request);
    if (parent) ConnectToParent(new_request, parent);

    return new_request->get_id();
}

static quint32 RequestManager::Request::FindNodeRequest(QNodeId dest,
    QNodeId id, Request* parent = NULL)
{
    Request new_request(FIND_NODE, dest, id, (parent ? parent->id_ : 0));
    requests_.insert(new_request->get_id(), new_request);
    if (parent) ConnectToParent(new_request, parent);

    return new_request->get_id();
}

static void RequestManager::Request::RemoveRequest(quint32 id)
{
    requests_.remove(id);
}

RequestManager::Request(int type, QNodeId dest, quint32 parent)
{
    id_ = Request.RandomId();
    type_ = type;
    parent_ = parent;
    children_ = QList<quint32>();
}

RequestManager::Request::Request(int type, QNodeId id , quint32 parent)
{
    id_ = Request.RandomId();
    type_ = type;
    parent_ = parent;
    children_ = QList<quint32>();
    requested_key_ = id;
    results_ = QList<QNodeId>();
}
RequestManager::Request::Request(int type, QUrl url, quint32 parent)
{
    id_ = Request.RandomId();
    type_ = type;
    parent_ = parent;
    children_ = QList<quint32>();
    requested_url_ = url;
    requested_key_ =
        QCA::Hash("sha1").hash(QByteArray(url.toEncoded())).toByteArray();
    results_ = QList<QNodeId>();
}

RequestManager::Request::Request(const Request& other)
{
    id_ = other.id_;
    type_ = other.type_;
    destination_ = other.destination_;
    parent_ = other.parent_;
    children_ = other.children_;
    requested_url_ = other.requested_url_;
    requested_key_ = other.requested_key_;
    results_ = other.results_;
}

void RequestManager::Request::UpdateResults(QList<QNodeId> results)
{
    if (parent) { // i.e. it's a child
        results_ = results;
        emit ChildComplete(this);
    } else {
        QList<QNodeId> sorted;
        // TODO: DRY
        // Basic insertion sort (okay because n is small)
        sorted.insert(results.takeFirst());
        while (!results.isEmpty()) {
            QNodeId cur_id = results.takeFirst();
            QList<QNodeId>::const_iterator i;
            for (i = sorted.constBegin(); i != sorted.constEnd(); i++) {
                quint16 cur_dist = RequestManager.Distance(cur_id, requested_key_);
                quint16 dist = RequestManager.Distance(*i, requested_key_);
                if (cur_dist > dist) break;
            }
            sorted.insert(i, cur_id);
        }

        QList<QNodeId>::const_iterator i, j;
        for (i = sorted.constBegin(); i != sorted.constEnd(); i++) {
            if (results_.indexOf(*i) > 0) continue;
            if (results_.size() < kBucketSize) {
                results_.append(*i);
                MakeChildFind(*i);
            } else {
                for (j = results_.constBegin(); j != results_.constEnd();
                        j++) {
                    quint16 new_dist = RequestManager.Distance(*id, requested_key_);
                    quint16 dist = RequestManager.Distance(*j, requested_key_);
                    if (new_dist > dist) {
                        results_.insert(j, *i);
                        results_.removeLast();
                        MakeChildFind(*i);
                    }
                }
            }
        }

        if (children_.isEmpty()) {
            if (type_ == FIND_VALUE) {
                emit MissingResource(requested_url_);
            }
            emit Complete(id_);
        }
    }
}

quint32 RequestManager::Request::MakeChildFind(QNodeId dest)
{
    quint32 child_id = 0;
    if (type_ == FIND_VALUE) {
      // TODO: get rid of value/node distinction
        child_id =
            Request::FindValueRequest(type_, requested_url_, dest, id_);
    } else { // FIND_NODE
        child_id =
            Request::FindNodeRequest(type_, requested_key_, dest, id_);
    }
    parent.children_.insert(child_id);
    emit Ready(child_id);
    return child_id;
}


void RequestManager::Request::ProcessChildCompletion(Request* child)
{
    switch (child->type_) {
        case FIND_NODE:
            UpdateResults(child->results_);
            break;
        case FIND_VALUE:
            UpdateResults(child->results_);
            break;
        case PING:
        default:
            break;
    }
    children_.remove(child->id_);
    emit Ready(id_);
}

