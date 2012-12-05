#include "includes.hh"
#include "utilities.hh"
#include "request.hh"

quint32 Request::RandomId()
{
    static bool initialized = false;
    if (!initialized) {
        qsrand(QTime::currentTime().msec());
        initialized = true;
    }
    return qrand();
}

Request* Request::Get(quint32 id)
{
    return requests_.value(id);
}

void Request::RegisterRequest(quint32 id, Request* req)
{
    if (req->get_type() != STORE) {
        requests_.insert(id, req); // Add to list of all requests
    }
}

void Request::RemoveRequest(quint32 id)
{
    delete requests_.value(id);
    requests_.remove(id);
}


// Base class Implementation

Request::Request(int type, QNode dest, QObject* observer,
    Request* parent)
{
    id_ = Request::RandomId();
    type_ = type;
    destination_ = dest;
    parent_ = parent;
    observer_ = observer;
    children_ = QList<quint32>();
    Init();
    emit Ready(this);
}

Request::Request(const Request& other) : QObject()
{
    id_ = other.id_;
    type_ = other.type_;
    destination_ = other.destination_;
    parent_ = other.parent_;
    observer_ = other.observer_;
    children_ = other.children_;
    Init();
}

void Request::Init()
{
    connect(this, SIGNAL(Ready(Request*)), observer_,
        SLOT(InitiateRequest(Request*)), Qt::QueuedConnection);
    connect(this, SIGNAL(Complete(quint32)), observer_,
        SLOT(CloseRequest(quint32)), Qt::QueuedConnection);

    if (parent_) {
        connect(this, SIGNAL(Complete(quint32)), parent_,
            SLOT(ProcessChildCompletion(quint32)), Qt::QueuedConnection);
        parent_->AddChild(id_);
    }

    Request::RegisterRequest(id_, this);
}

void Request::AddChild(quint32 id)
{
    children_.append(id);
}

void Request::UpdateResults(QNodeList nodes)
{
    emit Complete(id_);
}

// PingRequest

// FIXME: interface for RequestManager?  or Request Observer??? -- signals, etc
PingRequest::PingRequest(QNode dest, QObject* observer,
    Request* parent) : Request(PING, dest, observer, parent) {}

PingRequest::PingRequest(const PingRequest& other) :
    Request(other) {}

// StoreRequest

StoreRequest::StoreRequest(QNode dest, QKey key,
    QObject* observer, Request* parent) :
    Request(STORE, dest, observer, parent)
{
    resource_key_ = key;
}

StoreRequest::StoreRequest(const StoreRequest& other) :
    Request(other)
{
    resource_key_ = other.resource_key_;
}

// FindRequest

FindRequest::FindRequest(int type, QNode dest, QKey key,
    QObject* observer, Request* parent) :
    Request(type, dest, observer, parent)
{
    requested_key_ = key;
    results_ = QList<QNode>();
}

FindRequest::FindRequest(const FindRequest& other) :
    Request(other)
{
    requested_key_ = other.requested_key_;
    results_ = other.results_;
}

void FindRequest::UpdateResults(QNodeList nodes)
{
    if (parent_) { // i.e. it's a child
        results_ = nodes;
    } else {
        QNodeList sorted;
        // TODO: DRY
        // Basic insertion sort (okay because n is small)
        sorted << nodes.takeFirst();
        while (!nodes.isEmpty()) {
            QNode cur_node = nodes.takeFirst();
            QBitArray cur_dist = Distance(cur_node.first,
                requested_key_);

            QList<QNode>::iterator i;
            for (i = sorted.begin(); i != sorted.end(); i++) {
                QBitArray dist =
                    Distance(i->first, requested_key_);
                // TODO: fix spacing
                if (cur_dist > dist) break;
            }
            sorted.insert(i, cur_node);
        }

        QList<QNode>::iterator i, j;
        for (i = sorted.begin(); i != sorted.end(); i++) {
            if (results_.indexOf(*i) > 0) continue;
            if (results_.size() < kBucketSize) {
                results_.append(*i);
                MakeChild(*i);
            } else {
                for (j = results_.begin(); j != results_.end(); j++) {
                    QBitArray new_dist =
                        Distance(i->first, requested_key_);
                    QBitArray dist =
                        Distance(j->first, requested_key_);
                    if (new_dist > dist) {
                        results_.insert(j, *i);
                        results_.removeLast();
                        MakeChild(*i);
                        break;
                    }
                }
            }
        }
    }
}
void FindRequest::ProcessChildCompletion(quint32 child_id)
{
    Request* child = Request::Get(child_id);
    if (!child) return;
    if (child->get_type() == FIND_NODE || child->get_type() == FIND_VALUE) {
        UpdateResults(((FindRequest*)child)->results_);
        children_.removeAll(child_id);
        if (children_.isEmpty()) emit Complete(id_);
    } else if (child->get_type() == PING) {
        children_.removeAll(child_id);
        emit Ready(this);
    }
}

void FindRequest::MakeChild(QNode dest)
{
      if (type_ == FIND_NODE) {
          new FindNodeRequest(dest, requested_key_, observer_, this);
      } else { // FIND_VALUE
          new FindValueRequest(dest, requested_key_, observer_, this);
      }
}

// Find Node Request

FindNodeRequest::FindNodeRequest(QNode dest, QKey key,
    QObject* observer, Request* parent) :
    FindRequest(FIND_NODE, dest, key, observer, parent) {}

// Find Value Request

FindValueRequest::FindValueRequest(QNode dest, QKey key,
    QObject* observer, Request* parent) :
    FindRequest(FIND_VALUE, dest, key, observer, parent)
{
    // TODO: connect(&new_request, SIGNAL(ResourceNotFound(QKey)), observer,
      //  SLOT(GetResource(QKey)), Qt::QueuedConnection);
}

FindValueRequest::FindValueRequest(
    const FindValueRequest& other) : FindRequest(other)
{
    // TODO: connect(&new_request, SIGNAL(ResourceNotFound(QKey)), observer,
        // SLOT(GetResource(QKey)), Qt::QueuedConnection);
}

void FindValueRequest::UpdateResults(QList<QNode> results)
{
    FindRequest::UpdateResults(results);
    if (children_.isEmpty()) {
        emit ResourceNotFound(requested_key_);
    }
}
