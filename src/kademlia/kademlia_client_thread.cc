#include "includes.hh"
#include "kademlia_client.hh"

#include "kademlia_client_thread.hh"

#include <QMutex> // TODO: why do I actually need this??

KademliaClientThread::KademliaClientThread(QObject* parent) : QThread(parent)
{
    find_key_ = new QByteArray;
}

KademliaClientThread::~KademliaClientThread()
{
//    delete find_key_;
//    delete client_; // TODO: is that right??
}

void KademliaClientThread::Find(QKey key)
{
    mutex_.lock();
    client_->Find(key);
    delete find_key_;
    find_key_ = new QKey(key); // FIXME
    mutex_.unlock();
}

void KademliaClientThread::Store(QKey key, QIODevice* file)
{
    mutex_.lock();
    client_->Store(key, file);
    mutex_.unlock();
}

void KademliaClientThread::Remove(QKey key)
{
    mutex_.lock();
    client_->Remove(key);
    mutex_.unlock();
}

void KademliaClientThread::HandleLookupTermination(QKey key, QIODevice* device)
{
    mutex_.lock();
    if (*find_key_ == QString(key.constData())) {
        last_found_value_ = device;
    } else {
        last_found_value_ = NULL;
    }
    mutex_.unlock();
}

void KademliaClientThread::run()
{
    mutex_.lock();
    client_ = new KademliaClient();
    connect(client_, SIGNAL(ValueFound(QKey, QIODevice*)), this,
        SLOT(HandleLookupTermination(QKey, QIODevice*)));
    connect(client_, SIGNAL(ValueNotFound(QKey)), this,
        SLOT(HandleLookupTermination(QKey, QIODevice*)));
    mutex_.unlock();
    exec();
}
