#include "constants.hh"
#include "includes.hh"
#include "data_server.hh"

DataServer::DataServer()
{
    quint16 p = kDefaultPort;
    listen(QHostAddress::Any, kDefaultPort);
//    while (!listen(QHostAddress::Any, p++));
 //   qDebug() << "Data Server connection on " << p;
    connect(this, SIGNAL(newConnection()), this,
        SLOT(AcceptIncomingConnection()));

    files_ = new QHash<QKey, QIODevice*>();
    pending_downloads_ = new QHash<QTcpSocket*, QKey>();
    in_progress_downloads_ = new QHash<QTcpSocket*, Download*>();
}

DataServer::~DataServer()
{
    qDebug() << "~DataServer called";
    QList<QIODevice*>::iterator file_pointer;
    for (file_pointer = files_->values().begin();
            file_pointer != files_->values().end(); file_pointer++) {
        delete *file_pointer;
    }
    delete files_;
    delete pending_downloads_;
    QList<Download*>::iterator download_pointer;
    for (download_pointer = in_progress_downloads_->values().begin();
            download_pointer != in_progress_downloads_->values().end();
            download_pointer++) {
        delete *download_pointer;
    }
    delete in_progress_downloads_;
    // Close and free each thing in files_.values
}

void DataServer::Store(QKey key, QIODevice* file) // TODO: QIODEvice??:vs
{
    qDebug() << "DataServer::Store " << key << " , " << file;

    files_->insert(key, file);
}

void DataServer::Remove(QKey key)
{
    QIODevice* file = files_->take(key);
    // FIXME: close file; delete from fs
    delete file;
}

// FIXME: when connecting to cache, just need to keep map of
// QKey->QUrl, and call super.data(url), etc; let it deal with insertion and
// such

QIODevice* DataServer::Get(QKey key)
{
    qDebug() << "DataServer::Get";
    // qDebug() << "Getting key " << key << " with value " << files_->value(key);
    return files_->value(key);
}

////////////////////////////////////////////////////////////////////////////////
// Data Transmission (TCP)

void DataServer::AcceptIncomingConnection()
{
    QTcpSocket* connection = nextPendingConnection();
    qDebug() << "Incoming connection from " << connection;
    connect(connection, SIGNAL(disconnected()), connection,
        SLOT(deleteLater()));
    connect(connection, SIGNAL(readyRead()), this,
        SLOT(ProcessDownloadRequest()));
}

// Sending a file
void DataServer::ProcessDownloadRequest()
{
    qDebug() << "Processing Download Request";

    QTcpSocket* connection = (QTcpSocket*) QObject::sender();

    if (connection->bytesAvailable() < (int) kKeyLength) return;

    QDataStream in(connection);
    QKey key;
    in >> key;

    qDebug() << "Got request for key " << key;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    QIODevice* upload = Get(key);
    out << upload->size();
    qDebug() << "Upload has file size " << upload->size();
    connection->write(block); // TODO: write in pieces
    block.clear();

    // TODO: Async write
    char buffer[kBufferSize];
    qint64 bytes_read = 0;
    while ((bytes_read = upload->read(buffer, kBufferSize)) > 0) {
        block = QByteArray(buffer, bytes_read);
        connection->write(block);
    }
    if (bytes_read < 0) ERROR("Error reading file");
    upload->seek(0); // Reset to head of file

    qDebug() << "Finished writing bytes";
}

// Receiving a file

void DataServer::InitiateDownload(QNodeAddress addr, quint32 request_id,
    QKey key)
{
    qDebug() << "Initiating download for key " << key;
    // TODO: Eventually check request_id
    QTcpSocket* connection = new QTcpSocket();
    qDebug() << "Connection " << connection;
    connect(connection, SIGNAL(connected()), this, SLOT(RequestDownload()));
    connect(connection, SIGNAL(readyRead()), this, SLOT(ProcessDownload()));
    connect(connection, SIGNAL(disconnected()), connection,
        SLOT(deleteLater()));
    connection->connectToHost(addr.first, addr.second);
    // TODO: connect SIGNAL(error(QAbstractError::SocketError));
    pending_downloads_->insert(connection, key);
}

void DataServer::RequestDownload()
{
    QTcpSocket* connection = (QTcpSocket*) QObject::sender();
    qDebug() << "Connection " << connection;

    QKey key = pending_downloads_->value(connection);
    qDebug() << "Requesting download for key " << key;
    pending_downloads_->remove(connection);

    Download* new_download = new Download(key);
    in_progress_downloads_->insert(connection, new_download);

    QByteArray block;
    QDataStream out(&block, QIODevice::ReadWrite);
    out << key;
    connection->write(block);
}

void DataServer::ProcessDownload()
{
    qDebug() << "Process Download called";

    QTcpSocket* connection = (QTcpSocket*) QObject::sender();
    Download* download = in_progress_downloads_->value(connection);

    qDebug() << "Have bytes to read";
    QDataStream in(connection);
    if (download->get_size() == 0) {
        if (connection->bytesAvailable() < (int) sizeof(quint32)) return;
        quint64 s;
        in >> s;
        download->set_size(s);
    }
    qDebug() << "Read number of bytes " << download->get_size();

    char buffer[kBufferSize];
    while (connection->bytesAvailable() && !download->Complete()) {
        quint64 bytes_read = connection->read(buffer, kBufferSize);
        download->Write(buffer, bytes_read);
    }

    if (download->Complete()) {
        qDebug() << "Download complete";
        if (download->get_bytes_read() == download->get_size()) {
            Store(download->get_key(), download->get_file());
        }
        delete download;
        in_progress_downloads_->remove(connection);

        qDebug() << "Closed connection";
        connection->disconnectFromHost();
        delete connection;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Download

DataServer::Download::Download(QKey key) : size_(0), bytes_read_(0)
{
    key_ = new QKey(key);
    file_ = new QFile(QString("tmp/%1").arg(key.constData())); // TODO: error handling, FIXME: directory
    file_->open(QIODevice::ReadWrite);
}

DataServer::Download::~Download()
{
    delete key_;
    delete file_;
}

void DataServer::Download::Write(char* buff, quint64 num_bytes)
{
    file_->write(buff, num_bytes);
    bytes_read_ += num_bytes;
}
