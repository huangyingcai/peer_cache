#ifndef PEER_CACHE_RESOURCE_MANAGER_HH
#define PEER_CACHE_RESOURCE_MANAGER_HH

#include "peer_cache_constants.hh"

class ResourceManager : public QObject
{
    Q_OBJECT

    public:
        ResourceManager();
        ~ResourceManager();

        static QBitArray distance(QKey a, QKey b);
        // http://qt-project.org/wiki/WorkingWithRawData
        static quint16 bucket(QKey key);

        bool has_key(QKey key);
        void store(QKey key, QByteArray data); // TODO: call download manager
            // and see if all parts, then save
        const QNode& lookup_node(QNodeId id) const;

    public slots:
        QList<QKey> closest_nodes(QKey key, quint16 num_nodes);
        QList<QByteArray> data(QKey key); // TODO: implement like file downlaods
        // download, etc

        // TODO: timers and such
    private:
        QList<QNode>[160] m_k_buckets; // QNode == QKey node_id, Qhostaddr, quint16 port
        QMap<QKey, ifstream>* m_resources;

        // TODO: command line option to specify where to save data/location of
        // browser cache -- iterate over all files and add them to cache on
        // startup

        void refresh_bucket(quint16 bucket);
        void update_bucket(quint16 bucket, QNode new_node);
};

#endif // PEER_CACHE_RESOURCE_MANAGER_HH
