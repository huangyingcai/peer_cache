#include "includes.hh"

#include "kademlia/kademlia_client.hh"
#include "peer_cache.hh"

///////////////////////////////////////////////////////////////////////////////
// QNetworkDiskCache overrides

virtual QIODevice* data(const QUrl& url)
{
    QIODevice* data = QNetworkDiskCache::data(url);
    if (!data) {
        // queue lookup request
        QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
        request_manager_->IssueFindValueBlocking(key);
        // if found, data = resulst;
    }
    return data;
}

virtual void insert(QIODevice* device)
{
    QNetworkDiskCache::insert(device);

    QUrl url = prepared_devices_to_url_map_->value(device);
    QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
    Store(key, device);

    prepared_devices_to_url_map_->removeAll(device);
}

// virtual QNetworkCacheMetaData metaData(const QUrl& url)
// {
//     QNetworkDiskCache::metaData(url);
//     // FindValue( in DHT
//     // time out the call
//     // then call file metaData; see QNetworkDiskCach
// }

virtual QIODevice* prepare(const QNetworkCacheMetaData& metaData)
{
    QIODevice* prepared_device = QNetworkDiskCache::prepare(metaData);

    QUrl url = metaData.url();
    prepared_devices_to_url_map_->insert(url, prepared_device);

    return prepared_device;
}

virtual bool remove(const QUrl& url)
{
    QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
    Remove(key); // FIXME
    QNetworkCache::remove(url);
}

virtual void updateMetaData(const QNetworkCacheMetaData& metaData)
{
  // will only be called if i already have it??
  // do it; if no op, find_value (which stores), timer; call super; then
  // store again
    // super;
    // store
}
