#include "includes.hh"

#include "kademlia/kademlia_client.hh"
#include "peer_cache.hh"

PeerCache::PeerCache()
{
    client_ = new KademliaClient();
}

QIODevice* PeerCache::BlockingLookup(const QUrl& url)
{
    QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();

    QEventLoop lookup_loop;
    lookup_loop.connect(client_, SIGNAL(ResourceFound()), SLOT(quit()));
    lookup_loop.connect(client_, SIGNAL(ResourceNotFound()), SLOT(quit()));
    client_->SearchForFile(key);
    lookup_loop.exec();

    return client_->Get(key);
}

///////////////////////////////////////////////////////////////////////////////
// QNetworkDiskCache overrides

virtual QIODevice* PeerCache::data(const QUrl& url)
{
    QIODevice* data = QNetworkDiskCache::data(url);
    if (!data) {
        QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
        data = BlockingLookup(url);
    }
    return data;
}

virtual void PeerCache::insert(QIODevice* device)
{
    QNetworkDiskCache::insert(device);

    QUrl url = prepared_devices_to_url_map_->value(device);
    QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
    client_->Store(key, device);

    prepared_devices_to_url_map_->removeAll(device);
}

// virtual QNetworkCacheMetaData metaData(const QUrl& url)
// {
//     QNetworkDiskCache::metaData(url);
//     // FindValue( in DHT
//     // time out the call
//     // then call file metaData; see QNetworkDiskCach
// }

virtual QIODevice* PeerCache::prepare(const QNetworkCacheMetaData& metaData)
{
    QIODevice* prepared_device = QNetworkDiskCache::prepare(metaData);

    QUrl url = metaData.url();
    prepared_devices_to_url_map_->insert(url, prepared_device);

    return prepared_device;
}

virtual bool PeerCache::remove(const QUrl& url)
{
    QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
    client_->Remove(key);
    QNetworkCache::remove(url);
}

virtual void PeerCache::updateMetaData(const QNetworkCacheMetaData& metaData)
{
  // get the url from it
  // will only be called if i already have it??
  // do it; if no op, find_value (which stores), timer; call super; then
  // store again
    // super;
    // store
}
