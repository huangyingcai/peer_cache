#include "includes.hh"

#include "kademlia/kademlia_client.hh"
#include "peer_cache.hh"

// NOTE: Implementation loosely derived from QNetworkDiskCache (an actual
// license to come)

// TODO: get rid of this
QByteArray PeerCache::ToKey(const QUrl& url)
{
    if (!url.isValid()) return QByteArray();

    QByteArray key = QCA::Hash("sha1").hash(url.toEncoded()).toByteArray();
    return key;
}

PeerCache::PeerCache()
{
    client_thread_ = new KademliaClientThread();
    client_thread_->start();

    prepared_devices_to_url_map_ = new QHash<QIODevice*, QUrl>();
}

PeerCache::~PeerCache()
{
    client_thread_->quit();
    delete client_thread_;
    delete prepared_devices_to_url_map_;
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
// QAbstractNetworkCache overrides

virtual QIODevice* PeerCache::data(const QUrl& url)
{
    qDebug() << "PeerCache::data for " << url;

    if (!url.isValid()) return NULL;

    return BlockingLookup(key);
}

virtual void PeerCache::insert(QIODevice* device)
{
    qDebug() << "PeerCache::insert";

    if (!prepared_devices_to_url_map_->value(device)) {
        qDebug() << "Could not find device in prepared devices";
        return;
    }

    QUrl url = prepared_devices_to_url_map_->value(device);
    client_thread_->Store(PeerCache::ToKey(url), device); // Save into DHT

    prepared_devices_to_url_map_->removeAll(device);
}

virtual QNetworkCacheMetaData metaData(const QUrl& url)
{
    qDebug() << "PeerCache::metaData for " << url;

    QIODevice* data = BlockingLookup(PeerCache::ToKey(url));

    QNetworkCacheMetaData meta_data;
    if (data) {
        QDataStream in(data);
        in >> meta_data;
    }
    return meta_data;
}

virtual QIODevice* PeerCache::prepare(const QNetworkCacheMetaData& metaData)
{
    qDebug() << "PeerCache::prepare for " << metaData.url();

    if (!metaData.isValid() || !metaData.url().isValid() ||
            !metaData.saveToDisk()) {
        qDebug() << "Invalid metadata";
        return NULL;
    }

    QUrl url = metaData.url();
    QKey key = PeerCache::ToKey(url);
    QFile* prepared_device = new QFile(QString("tmp/%1").arg(key.constData())); // FIXME: hard-coded directory
    new_file->open(QIODevice::ReadWrite);

    prepared_devices_to_url_map_->insert(prepared_devices_to_url_map_, url);

    QDataStream out(prepared_device);
    out << metaData;

    return prepared_device;
}

virtual bool PeerCache::remove(const QUrl& url)
{
    qDebug() << "PeerCache::remove for " << url;

    client_thread_->Remove(PeerCache::ToKey(key));
}

virtual void PeerCache::updateMetaData(const QNetworkCacheMetaData& metaData)
{
    qDebug() << "PeerCache::updateMetaData()" << metaData.url();

    if (!metaData.isValid() || !metaData.url().isValid() ||
            !metaData.saveToDisk()) {
        qDebug() << "Invalid metadata";
        return;
    }

    QUrl url = metaData.url();
    QIODevice* old_device = data(url);
    if (!old_device) {
        qDebug() << "No file cached";
        return;
    }

    QIODevice* new_device = prepare(metaData);
    if (!new_device) {
        qDebug() << "No new device!" << url;
        return;
    }

    char data[1024]; // FIXME: constant
    while (!old_device->atEnd()) {
        qint64 bytes_read = old_device->read(data, 1024);
        new_device->write(data, bytes_read);
    }
    delete old_device;
    insert(new_device);
}
