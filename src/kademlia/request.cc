#include "utilities.hh"
#include "includes.hh"
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

// void Request::RegisterRequest(quint32 id, Request* req)
// {
//     requests_->insert(id, req); // Add to list of all requests
//     qDebug() << requests_->keys().size() << " requests remaining";
// }
// 
// void Request::RemoveRequest(quint32 id, Request* req)
// {
//     qDebug() << "Request: " << id << " closed";
//     requests_->remove(id);
//     qDebug() << requests_->keys().size() << " requests remaining";
// }


// Base class Implementation

Request::Request(int type, QNode dest, QObject* manager,
    FindRequest* parent) : type_(type), parent_(parent), manager_(manager),
                       resource_key_(NULL)
{
    qDebug() << "Initializing a new request" << id_;

    id_ = Request::RandomId();
    destination_ = new QPair<QNodeId, QNodeAddress>(dest.first, dest.second);

    manager_->InitiateRequest(id_, this);
}

Request::~Request()
{
    delete destination_;
}


void Request::Update(QNodeList nodes)
{
    qDebug() << "Updating request: " << id_;
    Terminate();
}

void Request::Terminate()
{
    qDebug() << "Terminating request" << id_;

    if (parent_) {
        parent->ProcessChildCompletion(id_);
    }

    manager_->CloseRequest(id_); // TODO: might be obviated
    // TODO: Use for when FindValue request terminates with success, etc
}

// PingRequest

// FIXME: interface for RequestManager?  or Request manager??? -- signals, etc
PingRequest::PingRequest(QNode dest, QObject* manager,
    Request* parent) : Request(PING, dest, manager, parent) {}

// StoreRequest

StoreRequest::StoreRequest(QNode dest, QKey key,
    QObject* manager, Request* parent) :
    Request(STORE, dest, manager, parent)
{
    resource_key_ = new QKey(key);
}

StoreRequest::~StoreRequest()
{
    delete resource_key_;
}

// FindRequest

FindRequest::FindRequest(int type, QNode dest, QKey key,
    QObject* manager, Request* parent) :
    Request(type, dest, manager, parent)
{
    resource_key_ = new QKey(key);
    results_ = new QNodeList();
}

FindRequest::~FindRequest()
{
    delete resource_key_;
    delete results_;
}

void FindRequest::Update(QNodeList nodes)
{
    if (parent_) { // it's a child
        results_ = QNodeList(nodes);
        Terminate();
    } else { // it's a parent
        QNodeList sorted;
        // TODO: DRY; qsort?
        if (!nodes.isEmpty()) {
            // Basic insertion sort (okay because n is small)
            sorted << nodes.takeFirst();
            while (!nodes.isEmpty()) {
                QNode cur_node = nodes.takeFirst();
                QBitArray cur_dist = Distance(cur_node.first,
                    *resource_key_);

                QNodeList::iterator i;
                for (i = sorted.begin(); i != sorted.end(); i++) {
                    QBitArray dist =
                        Distance(i->first, *resource_key_);
                    // TODO: fix spacing
                    if (cur_dist > dist) break;
                }
                sorted.insert(i, cur_node);
            }
        }

        // TODO: qBinaryFind?
        QNodeList::iterator i, j;
        for (i = sorted.begin(); i != sorted.end(); i++) {
            if (results_->indexOf(*i) > 0) continue;
            if (results_->size() < kBucketSize) {
                results_->append(*i);
                MakeChild(*i);
            } else {
                for (j = results_->begin(); j != results_->end(); j++) {
                    QBitArray new_dist =
                        Distance(i->first, *resource_key_);
                    QBitArray dist =
                        Distance(j->first, *resource_key_);
                    if (new_dist > dist) {
                        results_->insert(j, *i);
                        results_->removeLast();
                        MakeChild(*i);
                        break;
                    }
                }
            }
        }
        if (children_->isEmpty()) emit Complete(id_);
    }
}


void FindRequest::MakeChild(QNode dest)
{
    Request* child;
    if (type_ == FIND_NODE) {
        child = new FindNodeRequest(dest, *resource_key_, manager_, this);
    } else { // FIND_VALUE
        child = new FindValueRequest(dest, *resource_key_, manager_, this);
    }
    children_->insert(child->get_id(), child);
}

void FindRequest::ProcessChildCompletion(quint32 child_id)
{
    Request* child = get_child(child_id);
    if (child) {
        if (child->get_type() == FIND_NODE || child->get_type() == FIND_VALUE) {
            QNodeList child_results = ((FindRequest*)child)->get_results();
            Update(child_results);
            children_->removeAll(child_id);
        } else if (child->get_type() == PING) {
            manager->InitiateRequest(this);
        }
        RemoveChild(child_id);
    }
}

void FindRequest::RemoveChild(quint32 child_id)
{
    children->removeAll(id);
}

// Find Node Request

FindNodeRequest::FindNodeRequest(QNode dest, QKey key,
    QObject* manager, Request* parent) :
    FindRequest(FIND_NODE, dest, key, manager, parent) {}

void FindNodeRequest::ProcessChildCompletion(quint32 child_id)
{
    FindRequest::ProcessChildCompletion(child_id);
    if (children_->isEmpty()) Terminate();
}

// Find Value Request

FindValueRequest::FindValueRequest(QNode dest, QKey key,
    QObject* manager, Request* parent) :
    FindRequest(FIND_VALUE, dest, key, manager, parent) {}

void FindValueRequest::ProcessChildCompletion(quint32 child_id)
{
    FindRequest::ProcessChildCompletion(child_id);
    if (children_->isEmpty()) {
        manager->HandleMissingResource(*resource_key_);
        Terminate();
    }
}
