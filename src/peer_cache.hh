#ifndef PEER_CACHE_HH
#define PEER_CACHE_HH

class PeerCache : public QAbstractNetworkCache
{
    public:
        static ToKey(const QUrl& url);

        PeerCache();
        ~PeerCache();

        QIODevice* BlockingLookup(const QUrl& url);

        //TODO: set cache location

    private:
        KademliaClientThread* client_thread_;
        QHash<QIODevice*, QUrl>* prepared_devices_to_url_map_;
};

#endif // PEER_CACHE_HH
