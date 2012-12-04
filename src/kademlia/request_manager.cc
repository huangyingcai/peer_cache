#include "constants.hh"
#include "includes.hh"

#include "request_manager.hh"

static QBitArray RequestManager::Distance(QKey a, QKey b)
{
     // Create bit arrays of the appropriate size
     QBitArray a_bits(kKeyLength * 8);
     QBitArray b_bits(kKeyLength * 8);

     // Convert from QByteArray to QBitArray
     for (int i = 0; i < kKeyLength; i++) {
         for (int b = 0; b < 8; ++b) {
                 a_bits.setBit(i * 8 + b, a_bytes.at(i) & (1 << b));
                 b_bits.setBit(i * 8 + b, b_bytes.at(i) & (1 << b));
         }
     }

     return a_bits & b_bits;
}

RequestManager::RequestManager(QNodeId id, QNodeId bootstrap,
    QObject* parent = 0) : QObject(parent)
{
    node_id_ = id;

    // TODO:Broadcast a join -- just get 1
    UpdateBucket(Bucket(bootstrap)); // Insert into bucket
    Request* req = RequestManager::FindNode(id, bootstrap);
    InitiateRequest(req->get_id());
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

QList<QNode> RequestManager::ClosestNodes(QKey key, quint16 num)
{
    QList<QNode> nodes;

    quint16 b = Bucket(key);
    for (quint b = Bucket(key); b >= 0 && nodes.size() < num; b--) {
        QList<QNode>::const_iterator i;
        for (i = buckets_[b].constBegin();
                i != buckets_[b].constEnd() && nodes.size() < num; i++) {
            nodes.append(*i);
        }
    }

    return nodes;
}

void RequestManager::RefreshBucket(quint16 bucket)
{
    QBitArray bits(kKeySize * 8, 0);
    for (int b = bucket; b >= 0; b--) {
        bits[b] = rand() % 2;
    }

    QNodeId random_node_id;
    for (int b = 0; b < bits.count(); b++) {
          random_node_id[b/8] =
              (random_node_id.at(b / 8) | ((bits[b] ? 1 : 0) << (b % 8)));
    }

    Request::PingRequest(random_node_id);
}

void RequestManager::UpdateBucket(quint16 bucket, QNode node);
{
    // TODO: PING procedure
    buckets_[bucket].append(node);
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

static void RegisterRequest(quint32 id, const Request* req)
{
    requests_.insert(id, *req); // Add to list of all requests
    if (req->parent != 0) { // Connect to parent if exists
        Request* par = Get(req->parent);
        connect(child, SIGNAL(ChildComplete(quint32)), par,
            SLOT(ProcessChildCompletion(quint32))); // TODO: Qt::QueuedConnection?
    }
}

static void RequestManager::Request::RemoveRequest(quint32 id)
{
    requests_.remove(id);
}

// Factory methods
static quint32 RequestManager::PingRequest(QNodeId dest, QObject* observer,
    Request* parent = NULL)
{
    PingRequest new_request(PING, dest, (parent ? parent->id_ : 0));
    connect(&new_request, SIGNAL(Ready(quint32)), observer,
        SLOT(InitiateRequest(quint32)));
    connect(&new_request, SIGNAL(Complete(quint32)), observer,
        SLOT(CloseRequest(quint32)));
    return new_request.get_id();
}

static quint32 RequestManager::FindNodeRequest(QNodeId dest, QNodeId id,
    QObject* observer, Request* parent = NULL)
{
    FindNodeRequest new_request(PING, dest, key, (parent ? parent->id_ : 0));
    connect(&new_request, SIGNAL(Ready(quint32)), observer,
        SLOT(InitiateRequest(quint32)));
    connect(&new_request, SIGNAL(Complete(quint32)), observer,
        SLOT(CloseRequest(quint32)));
    return new_request.get_id();
}

static quint32 RequestManager::FindValueRequest(QNodeId dest, QUrl url,
    QObject* observer, Request* parent = NULL)
{
    FindValueRequest new_request(PING, dest, url, (parent ? parent->id_ : 0));
    connect(&new_request, SIGNAL(Ready(quint32)), observer,
        SLOT(InitiateRequest(quint32)));
    connect(&new_request, SIGNAL(Complete(quint32)), observer,
        SLOT(CloseRequest(quint32)));
    connect(&new_request, SIGNAL(ResourceNotFound(QUrl)), observer,
        SLOT(GetResource(QUrl)), Qt::QueuedConnection); // TODO: name of slot
    return new_request.get_id();
}

// Base class Implementation

RequestManager::Request(int type, QNodeId dest, quint32 parent)
{
    id_ = Request.RandomId();
    type_ = type;
    parent_ = parent;
    children_ = QList<quint32>();
    Request.RegisterRequest(id_, this);
}

RequestManager::Request::Request(const Request& other)
{
    id_ = other.id_;
    type_ = other.type_;
    destination_ = other.destination_;
    parent_ = other.parent_;
    children_ = other.children_;
}

virtual void UpdateResults(QList<QNodeId> results = QList())
{
    (parent) ? emit ChildComplete(id_) : emit Complete(id_)
}

// Ping Request

RequestManager::PingRequest::PingRequest(QNode dest, quint32 parent = 0) :
    Request(PING, dest, parent) {}

RequestManager::PingRequest::PingRequest(const PingRequest& other) :
    Request(other) {}

// Find Request

RequestManager::FindRequest::FindRequest(int type, QNode dest, QKey key,
    quint32 parent = 0) : Request(type, dest, parent)
{
    requested_key_ = key;
    results_ = QList<QNodeId>();
}

RequestManager::FindRequest::FindRequest(const FindRequest& other) :
    Request(other)
{
    requested_key_ = other.requested_key_;
    results_ = other.results_;
}

void RequestManager::FindRequest::UpdateResults(QList<QNodeId> results)
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
            emit Complete(id_);
        } else {
            emit Ready(id_); // FIXME: figure out signals; also, when ping is done, need to re-request
        }
    }
}
virtual void RequestManager::FindRequest::ProcessChildCompletion(Request* child)
{
    if (child.type_ == FIND_NODE || child.type_ == FIND_VALUE) {
        UpdateResults(child->results_);
    }
    children_.remove(child->id_);
    emit Ready(id_);
}

// Find Node Request

RequestManager::FindNodeRequest::FindNodeRequest(QNode dest, QKey key,
    quint32 parent = 0) : FindRequest(FIND_NODE, dest, key, parent) {}

quint32 RequestManager::FindNodeRequest::MakeChild(int type, QNode dest)
{
    quint32 child_id;
    if (type == PING) {
        child_id = Request.PingRequest(dest, id_);
    }
    parent.children_.insert(child_id);
    emit Ready(child_id);
    return child_id;
}

// Find Value Request

RequestManager::FindValueRequest::FindValueRequest(QNode dest, QKey key,
    quint32 parent = 0) : FindRequest(FIND_VALUE, dest, key, parent) {}

RequestManager::FindValueRequest::FindValueRequest(QNode dest, QUrl url,
    quint32 parent = 0) : FindRequest(FIND_VALUE, dest, QByteArray(), parent) {}
    requested_url_ = url;
    requested_key_ =
        QCA::Hash("sha1").hash(QByteArray(url.toEncoded())).toByteArray();
}

RequestManager::FindValueRequest::FindValueRequest(
    const FindValueRequest& other) : FindRequest(other)
{
    requested_url_ = url;
}

virtual void RequestManager::FindNode::UpdateResults(QList<QNodeId> results)
{
    super(results);
    if (children_.isEmpty()) {
        emit ResourceNotFound(requested_url_);
    }
}

// FIXME:
// FIXME: SIGNALS AND SLOTS
// FIXME: Factory Methods

quint32 RequestManager::Request::MakeChildFind(QNodeId dest)
{
    quint32 child_id = 0;
    if (type_ == FIND_VALUE) {
      // TODO: get rid of value/node distinction
        child_id =
            Request.FindValueRequest(type_, requested_url_, dest, id_);
    } else { // FIND_NODE
        child_id =
            Request.FindNodeRequest(type_, requested_key_, dest, id_);
    }
    parent.children_.insert(child_id);
    emit Ready(child_id);
    return child_id;
}
