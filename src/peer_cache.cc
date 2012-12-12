#include "includes.hh"

#include "kademlia/kademlia_client.hh"
#include "peer_cache.hh"

PeerCache::PeerCache()
{
    client_thread_ = new KademliaClientThread();
}

QIODevice* PeerCache::BlockingLookup(const QKey& key)
{
    QEventLoop lookup_loop;
    lookup_loop.connect(client_thread_, SIGNAL(FindRequestComplete()),
        SLOT(quit()));
    QTimer timeout_timer;
    lookup_loop.connect(&timeout_timer, SIGNAL(complete()), SLOT(quit()));
    client_thread_->Find(key);
    timeout_timer.start(10); // Default Timeout; if the resource is so long
                             // that the download takes a while, that's okay;
                             // it will be available the next time around
    lookup_loop.exec();

    return client_thread_->get_last_found_value();
}

///////////////////////////////////////////////////////////////////////////////
// QNetworkDiskCache overrides

virtual QIODevice* PeerCache::data(const QUrl& url)
{
    QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
    return BlockingLookup(key);
}

virtual void PeerCache::insert(QIODevice* device)
{
    QUrl url = prepared_devices_to_url_map_->value(device);
    QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
    client_thread_->Store(key, device);

    prepared_devices_to_url_map_->removeAll(device);
}

virtual QNetworkCacheMetaData metaData(const QUrl& url)
{
    QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
    QIODevice* data = BlockingLookup(url);
    if (data) {
        // FIXME: read the data from it
    } else {
        // FIXME: return invalid metadata
    }
    return meta_data;
}

virtual QIODevice* PeerCache::prepare(const QNetworkCacheMetaData& metaData)
{
    QIODevice* prepared_device = QNetworkDiskCache::prepare(metaData);

    QUrl url = metaData.url();
    prepared_devices_to_url_map_->insert(url, prepared_device);

    return prepared_device;
}

// Current action is to only delete locally
virtual bool PeerCache::remove(const QUrl& url)
{
    QNetworkDiskCache::remove(url);
}

virtual void PeerCache::updateMetaData(const QNetworkCacheMetaData& metaData)
{
    if (!QNetworkDiskCache::data(metaData.url())) { // don't have it locally
        // Try to look it up
        QUrl url = metaData.url();
        QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
        QIODevice* data = BlockingLookup(url);
        // FIXME: figure out store, etc; -- Blocking lookup stores in local dht
    }

    QNetworkDiskCache::updateMetaData(metaData);
}
