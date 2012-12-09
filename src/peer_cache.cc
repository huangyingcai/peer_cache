#include "includes.hh"

#include "kademlia/kademlia_client.hh"
#include "peer_cache.hh"

///////////////////////////////////////////////////////////////////////////////
// QNetworkDiskCache overrides

virtual QIODevice* data(const QUrl& url)
{
  // Lookup locally, then DHT
}

virtual void insert(QIODevice* device)
{
    // super
    // STORE (QHash(url), whatever)
}

virtual QNetworkCacheMetaData metaData(const QUrl& url)
{
    // super
    // FindValue( in DHT
    // time out the call
    // then call file metaData; see QNetworkDiskCach
}

virtual QIODevice* prepare (const QNetworkCacheMetaData& metaData)
{
  // probably need to add map of key->url here
}

virtual bool remove(const QUrl& url)
{
  // remove key->url
  // remove locally; don't worry about DHT
  // super
}

virtual void updateMetaData(const QNetworkCacheMetaData & metaData)
{
  // will only be called if i already have it??
  // do it; if no op, find_value (which stores), timer; call super; then
  // store again
    // super;
    // store
}
