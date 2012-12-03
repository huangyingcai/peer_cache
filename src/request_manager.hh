#ifndef PEER_CACHE_REQUEST_MANAGER_HH
#define PEER_CACHE_REQUEST_MANAGER_HH

#include "peer_cache_constants.hh"

class QBitArray;
class QList;
class QNode;

class RequestManager : public QObject
{
    public:
        RequestManager();

        static QBitArray distance(QKey a, QKey b);
        // http://qt-project.org/wiki/WorkingWithRawData
        static quint16 bucket(QKey key);

    private:
        QList<QNode>[160] m_k_buckets; // QNode == QKey node_id, Qhostaddr, quint16 port

        void refresh_bucket(quint16 bucket);
        void update_bucket(quint16 bucket, QNode new_node);
}

#endif // PEER_CACHE_REQUEST_MANAGER_HH
