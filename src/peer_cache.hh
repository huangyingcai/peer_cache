#ifndef PEER_CACHE_HH
#define PEER_CACHE_HH

#include <QAbstractNetworkCache>
#include <QHash>

#include "types.hh" // FIXME: this include structure is hideous

class KademliaClientThread;

class PeerCache : public QAbstractNetworkCache
{
    Q_OBJECT

    public:
        static QKey ToKey(const QUrl& url);

        PeerCache();
        ~PeerCache();

        QIODevice* BlockingLookup(const QKey& key);

        virtual QIODevice* data(const QUrl& url);
        virtual void insert(QIODevice* device);
        virtual QNetworkCacheMetaData metaData(const QUrl& url);
        virtual QIODevice* prepare(const QNetworkCacheMetaData& metaData);
        virtual bool remove(const QUrl& url);
        virtual void updateMetaData(const QNetworkCacheMetaData& metaData);
        virtual qint64 cacheSize() const;

    public slots:
        virtual void clear();

        //TODO: set cache location

    private:
        KademliaClientThread* client_thread_;
        QHash<QIODevice*, QUrl>* prepared_devices_to_url_map_;
};

#endif // PEER_CACHE_HH
