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

RequestManager::RequestManager(QNodeId id, QNode bootstrap_node,
    QObject* parent = 0) : QObject(parent)
{
    node_id_ = id;

    UpdateBucket(bootstrap_node);
    Request* req = RequestManager::FindNode(id, bootstrap);
}

quint16 RequestManager::Bucket(QKey key)
{
    QBitArray dist = RequestManager.distance(requested_key_, key);

    quint bucket = dist.size() - 1;
    for (; !dist[bucket]; bucket--); // Find MSB

    return bucket;
}

void RequestManager::UpdateRequest(quint32 request_id, QNode node,
    QList<QNode> results);
{
    QList<QNode>::const_iterator i;
    for (i = results.constBegin(); i != results.constEnd(); i++) {
        UpdateBuckets(*i);
    }

    Request* req;
    if (req = Request.Get(request_id)) {
        req->UpdateResults(results); // Works for PING
    }
}

void RequestManager::CloseRequest(quint32 request_id)
{
    Request* req;
    if (req = Request.Get(request_id)) {
        RemoveRequest(request_id);

        // Close Children
        QList<quint32>::const_iterator i;
        for (i = req->children_.constBegin(); i != req->children_.constEnd();
                i++) {
            CloseRequest(i->get_id());
        }

        // Close Parent
        if (req->parent) {
            CloseRequest(req->parent->get_id());
        }
    }
}

void RequestManager::InitiateRequest(quint32 request_id)
{
    Request req = Request.Get(request_id);
    if (req.get_type() == FIND_VALUE || req.get_type() == FIND_NODE) {
        emit HasRequest(req.get_type(), req.get_id(), req.get_destination(),
            req.get_requested_key());
    } else {
        emit HasRequest(req.get_type(), req.get_id(), req.get_destination(),
            QByteArray());
    }
}

void IssueStore(QKey key)
{
    QList<QNode> closest = ClosestNodes(key, 1);

    if (closest.isEmpty()) return; // This node is supposed to store it

    QNode = closest.takeFirst();
    Request.StoreRequest(QNode, QKey);
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
//     Request::PingRequest(random_node_id);
}

void RequestManager::UpdateBuckets(QNode node);
{
    // TODO: PING procedure
    quint16 b = Bucket(node.first);

    int i = buckets_[b].indexOf(node);
    if (i > 0) buckets_[b].removeAt(i);

    buckets_[b].append(node);
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
    if (req->get_type() != STORE) {
        requests_.insert(id, *req); // Add to list of all requests
    }
}

static void RequestManager::Request::RemoveRequest(quint32 id)
{
    requests_.remove(id);
}

// Factory methods
static quint32 RequestManager::PingRequest(QNode dest, QObject* observer,
    Request* parent = NULL)
{
    PingRequest new_request(dest, observer, (parent ? parent->id_ : 0));
    return new_request.get_id();
}

static quint32 RequestManager::FindNodeRequest(QNode dest, QNodeId id,
    QObject* observer, Request* parent = NULL)
{
    FindNodeRequest new_request(dest, key, observer,
        (parent ? parent->get_id() : 0));
    return new_request.get_id();
}

static quint32 RequestManager::FindValueRequest(QNode dest, QUrl url,
    QObject* observer, Request* parent = NULL)
{
    FindValueRequest new_request(dest, url, observer,
        (parent ? parent->id_ : 0));
    return new_request.get_id();
}

// Base class Implementation

RequestManager::Request(int type, QNode dest, QObject* observer,
    Request* parent)
{
    id_ = Request.RandomId();
    type_ = type;
    parent_ = parent;
    children_ = QList<quint32>();
    Init();
    emit Ready(id_);
}

RequestManager::Request::Request(const Request& other)
{
    id_ = other.id_;
    type_ = other.type_;
    destination_ = other.destination_;
    parent_ = other.parent_;
    observer_ = other.observer_;
    children_ = other.children_;
    Init();
}

void Init()
{
    connect(this, SIGNAL(Ready(quint32)), observer_,
        SLOT(InitiateRequest(quint32)), Qt::QueuedConnection);
    connect(this, SIGNAL(Complete(quint32)), observer_,
        SLOT(CloseRequest(quint32)), Qt::QueuedConnection);

    if (parent_) {
        connect(this, SIGNAL(Complete(quint32)), parent_,
            SLOT(ProcessChildCompletion(quint32)), Qt::QueuedConnection);
        parent_->AddChild(id_);
    }

    Request.RegisterRequest(id_, this);
}

void AddChild(quint32 id)
{
    children_.append(id);
}

virtual void UpdateResults(QList<QNode> results = QList())
{
    emit Complete(id_);
}

// PingRequest

// FIXME: interface for RequestManager?  or Request Observer??? -- signals, etc
RequestManager::PingRequest::PingRequest(QNode dest, QObject* observer,
    Request* parent = NULL) : Request(PING, dest, observer, parent) {}

RequestManager::PingRequest::PingRequest(const PingRequest& other) :
    Request(other) {}

// StoreRequest

RequestManager::StoreRequest::StoreRequest(QNode dest, QKey key,
    QObject* observer, Request* parent = NULL) :
    Request(STORE, dest, observer, parent)
{
    resource_key_ = key;
}

RequestManager::StoreRequest::StoreRequest(const PingRequest& other) :
    Request(other)
{
    resource_key_ = other.resource_key_;
}

// FindRequest

RequestManager::FindRequest::FindRequest(int type, QNode dest, QKey key,
    QObject* observer, Request* parent = NULL) :
    Request(type, dest, observer, parent)
{
    requested_key_ = key;
    results_ = QList<QNode>();
}

RequestManager::FindRequest::FindRequest(const FindRequest& other) :
    Request(other)
{
    requested_key_ = other.requested_key_;
    results_ = other.results_;
}

virtual void RequestManager::FindRequest::UpdateResults(QList<QNode> results)
{
    if (parent) { // i.e. it's a child
        results_ = results;
    } else {
        QList<QNode> sorted;
        // TODO: DRY
        // Basic insertion sort (okay because n is small)
        sorted.insert(results.takeFirst());
        while (!results.isEmpty()) {
            QNode cur_node = results.takeFirst();
            QList<QNode>::const_iterator i;
            for (i = sorted.constBegin(); i != sorted.constEnd(); i++) {
                quint16 cur_dist = RequestManager.Distance(cur_node.first, requested_key_);
                quint16 dist = RequestManager.Distance(*i, requested_key_);
                if (cur_dist > dist) break;
            }
            sorted.insert(i, cur_node);
        }

        QList<QNode>::const_iterator i, j;
        for (i = sorted.constBegin(); i != sorted.constEnd(); i++) {
            if (results_.indexOf(*i) > 0) continue;
            if (results_.size() < kBucketSize) {
                results_.append(*i);
                MakeChild(*i);
            } else {
                for (j = results_.constBegin(); j != results_.constEnd();
                        j++) {
                    quint16 new_dist =
                        RequestManager.Distance(i->first, requested_key_);
                    quint16 dist = RequestManager.Distance(*j, requested_key_);
                    if (new_dist > dist) {
                        results_.insert(j, *i);
                        results_.removeLast();
                        MakeChild(*i);
                    }
                }
            }
        }
    }
}
virtual void RequestManager::FindRequest::ProcessChildCompletion(
    quint32 child_id)
{
    if (child.type_ == FIND_NODE || child.type_ == FIND_VALUE) {
        Request* child = Requests.Get(child_id);
        UpdateResults(child.results_);
        children_.remove(child_id);
        if (children_.isEmpty()) emit Complete(id_);
    } else if (child.type_ == PING) {
        children_.remove(child_id_);
        emit Ready(id_);
    }
}

void MakeChild(QNode dest)
{
      if (type_ == FIND_NODE) {
          Request.FindNodeRequest(dest, requested_key_, observer_, this);
      } else { // FIND_VALUE
          Request.FindValueRequest(dest, requested_url_, observer_, this);
      }
}

// Find Node Request

RequestManager::FindNodeRequest::FindNodeRequest(QNode dest, QKey key,
    QObject* observer, Request* parent = 0) :
    FindRequest(FIND_NODE, dest, key, observer, parent) {}

// Find Value Request

RequestManager::FindValueRequest::FindValueRequest(QNode dest, QUrl url,
    QObject* observer, Request* parent = 0) :
    FindRequest(FIND_VALUE, dest, QByteArray(), observer, parent)
{
    requested_url_ = url;
    requested_key_ =
        QCA::Hash("sha1").hash(QByteArray(url.toEncoded())).toByteArray();
    connect(&new_request, SIGNAL(ResourceNotFound(QUrl)), observer,
        SLOT(GetResource(QUrl)), Qt::QueuedConnection);
}

RequestManager::FindValueRequest::FindValueRequest(QNode dest, QUrl url,
    QKey key, QObject* observer, Request* parent = 0) :
    FindRequest(FIND_VALUE, dest, key, observer, parent)
{
    requested_url_ = url;
    requested_key_ = key;
    connect(&new_request, SIGNAL(ResourceNotFound(QUrl)), observer,
        SLOT(GetResource(QUrl)), Qt::QueuedConnection);
}

RequestManager::FindValueRequest::FindValueRequest(
    const FindValueRequest& other) : FindRequest(other)
{
    requested_url_ = url;
    connect(&new_request, SIGNAL(ResourceNotFound(QUrl)), observer,
        SLOT(GetResource(QUrl)), Qt::QueuedConnection);
}

virtual void RequestManager::FindNode::UpdateResults(QList<QNode> results)
{
    super(results);
    if (children_.isEmpty()) {
        emit ResourceNotFound(requested_url_);
    }
}
