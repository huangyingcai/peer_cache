#ifndef PEER_CACHE_HH
#define PEER_CACHE_HH

class PeerCache : public QNetworkDiskCache
{
    public:
        QIODevice* BlockingLookup(const QUrl& url);

    private:
        KademliaClientThread* client_thread_;
        QHash<QIODevice*, QUrl>* prepared_devices_to_url_map_;
};

#endif // PEER_CACHE_HH
