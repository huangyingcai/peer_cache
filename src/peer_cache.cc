#include "includes.hh"

#include "kademlia/kademlia_client.hh"
#include "peer_cache.hh"

///////////////////////////////////////////////////////////////////////////////
// QAbstractNetworkCache overrides

virtual qint64 cacheSize() const
{
}

virtual QIODevice* data(const QUrl& url)
{
  // Lookup locally, then DHT
}

virtual void insert(QIODevice* device)
{
}

virtual QNetworkCacheMetaData metaData(const QUrl& url)
{
}

virtual QIODevice* prepare (const QNetworkCacheMetaData& metaData)
{
}

virtual bool remove(const QUrl& url)
{
}

virtual void updateMetaData(const QNetworkCacheMetaData & metaData)
{
}
