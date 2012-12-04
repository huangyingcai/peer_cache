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

RequestManager::RequestManager(QNodeId id, QObject* parent = 0) : QObject(parent)
{
    node_id_ = id;
}


quint16 RequestManager::Bucket(QKey key)
{
    QBitArray dist = RequestManager.distance(node_id_, key);

    quint bucket = dist.size() - 1;
    for (; !dist[bucket]; bucket--); // Find MSB

    return bucket;
}

void RequestManager::ClosePing(QNodeAddress addr, quint32 request_id);
void RequestManager::UpdateFindRequest(QNodeAddress addr, quint32 request_id,
    QKey key);
void RequestManager::CloseFindRequest(QNodeAddress addr, quint32 request_id,
    QKey key);
// void CloseRequest

void RequestManager::ClosestNodes(QKey key);
void RequestManager::RefreshBucket(quint16 bucket);
void RequestManager::UpdateBucket(quint16 bucket, QNode node);
