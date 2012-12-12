#include "kademlia_client_thread.hh"

KademliaClientThread::KademliaClientThread(QObject* parent) : QThread(parent)
{
    find_key_ = new QByteArray;
}

KademliaClientThread::~KademliaClientThread()
{
    delete find_key_;
    delete client_; // TODO: is that right??
}

void KademliaClientThread::Find(QKey key)
{
    mutex.lock();
    client_->Find(key);
    find_key_ = key; // FIXME
    mutex.unlock();
}

void KademliaClientThread::Store(QKey key, QIODevice* file)
{
    mutex.lock();
    client_->Store(key, file);
    mutex.unlock();
}

void KademliaClientThread::HandleLookupTermination(QKey key, QIODevice* device)
{
    mutex.lock();
    if (find_key_ == QString(key.constData())) {
        last_found_value_ = device;
    } else {
        last_found_value_ = NULL;
    }
    mutex.unlock();
}

void KademliaClientThread::run()
{
    client_ = new KademliaClient();
    connect(client_, ValueFound(QKey, QIODevice*), this,
        HandleLookupTermination(QKey, QIODevice*));
    connect(client_, ValueNotFound(QKey), this,
        HandleLookupTermination(QKey, QIODevice*));
    exec();
}
