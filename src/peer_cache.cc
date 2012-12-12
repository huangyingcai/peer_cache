#include <QEventLoop>
#include <QTemporaryFile>

#include "includes.hh"
#include "peer_cache.hh"
#include "kademlia/kademlia_client_thread.hh"

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
    client_thread_->Find(key);
    // Default Timeout; if the resource is so long
    // that the download takes a while, that's okay;
    // it will be available the next time around
    QTimer::singleShot(10000, &lookup_loop, SLOT(quit()));
    lookup_loop.exec();

    qDebug() << "Lookup result: " << client_thread_->get_last_found_value();

    return client_thread_->get_last_found_value();
}

///////////////////////////////////////////////////////////////////////////////
// QAbstractNetworkCache overrides

QIODevice* PeerCache::data(const QUrl& url)
{
    qDebug() << "PeerCache::data for " << url;

    if (!url.isValid()) return NULL;

    QKey key = PeerCache::ToKey(url);

    QIODevice* device = BlockingLookup(key);
    if (device) {
        QTemporaryFile* temp_file = new QTemporaryFile();
        temp_file->open();

        device->seek(0);
        QNetworkCacheMetaData meta_data;
        QDataStream in(device);
        in >> meta_data;

        char data[1024]; // FIXME: constant
        while (!device->atEnd()) {
            qint64 bytes_read = device->read(data, 1024);
            temp_file->write(data, bytes_read);
        }

        temp_file->seek(0);
        return temp_file;
    } else {
        return device;
    }
}

void PeerCache::insert(QIODevice* device)
{
    if (prepared_devices_to_url_map_->value(device).isEmpty()) {
        qDebug() << "Could not find device in prepared devices";
        return;
    }

    QUrl url = prepared_devices_to_url_map_->value(device);
    QKey key = PeerCache::ToKey(url);
    qDebug() << "PeerCache::insert " << key << " , " << device;
    device->seek(0);
    client_thread_->Store(key, device); // Save into DHT

    prepared_devices_to_url_map_->remove(device);
}

QNetworkCacheMetaData PeerCache::metaData(const QUrl& url)
{
    qDebug() << "PeerCache::metaData for " << url;

    QKey key = PeerCache::ToKey(url);
    QIODevice* device = BlockingLookup(key);

    QNetworkCacheMetaData meta_data;
    if (device) {
        device->seek(0);
        QDataStream in(device);
        in >> meta_data;

        qDebug() << "Got meta data " << meta_data.attributes();
        qDebug() << "with url " << meta_data.url() << meta_data.expirationDate();
    }
    return meta_data;
}

QIODevice* PeerCache::prepare(const QNetworkCacheMetaData& metaData)
{
    qDebug() << "PeerCache::prepare for " << metaData.url();

    if (!metaData.isValid() || !metaData.url().isValid()) {
            //!metaData.saveToDisk()) { Just cache everything for now
        qDebug() << "Invalid metadata";
        return NULL;
    }
    qDebug() << "Preparing meta data " << metaData.attributes();
    qDebug() << "with url " << metaData.url() << metaData.expirationDate();

    QUrl url = metaData.url();
    QKey key = PeerCache::ToKey(url);
    QFile* prepared_device = new QFile(QString("tmp/%1").arg(key.constData())); // FIXME: hard-coded directory
    prepared_device->open(QIODevice::ReadWrite | QIODevice::Truncate);

    prepared_devices_to_url_map_->insert(prepared_device, url);

    QDataStream out(prepared_device);
    out << metaData;

    return prepared_device;
}

bool PeerCache::remove(const QUrl& url)
{
    qDebug() << "PeerCache::remove for " << url;

    QKey key = PeerCache::ToKey(url);
    client_thread_->Remove(key);
    return true; // FIXME:
}

void PeerCache::updateMetaData(const QNetworkCacheMetaData& metaData)
{
    qDebug() << "PeerCache::updateMetaData()" << metaData.url();

    if (!metaData.isValid() || !metaData.url().isValid()) {
        //    !metaData.saveToDisk()) { (see above)
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

// FIXME: just need to be implemented; not necessary for functioning
qint64 PeerCache::cacheSize() const
{
    return (qint64) 1024 * 1024 * 5;
}

void PeerCache::clear()
{
}
