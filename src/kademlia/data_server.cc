#include "constants.hh"
#include "includes.hh"
#include "data_server.hh"

DataServer::DataServer()
{
    listen(QHostAddress::LocalHost, kDefaultPort);
    connect(this, SIGNAL(newConnection()), this,
        SLOT(AcceptIncomingConnection()));
}

// TODO: DataServer::~DataServer()
// {
//     // Close and free each thing in files_.values
// }

void DataServer::Store(QKey key, QFile* file)
{
    files_.insert(key, file);
}

QFile* DataServer::Value(QKey key)
{
    return files_.value(key, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// Data Transmission (TCP)

void DataServer::AcceptIncomingConnection()
{
    QTcpSocket* connection = nextPendingConnection();
    connect(connection, SIGNAL(disconnected()), connection,
        SLOT(deleteLater()));
    connect(connection, SIGNAL(readyRead()), this,
        SLOT(ProcessDownloadRequest()));
}

// Sending a file

void DataServer::ProcessDownloadRequest()
{
    QTcpSocket* connection = (QTcpSocket*) QObject::sender();

    if (connection->bytesAvailable() < (int) kKeyLength) return;

    QDataStream in(connection);
    QKey key;
    in >> key;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    QFile* upload = Value(key);
    out << upload->size();

    // TODO: Async write
    char buffer[kBufferSize];
    qint64 bytes_read = 0;
    while ((bytes_read = upload->read(buffer, kBufferSize)) > 0) {
        out.writeBytes(buffer, bytes_read);
    }
    if (bytes_read < 0) ERROR("Error reading file");
    upload->seek(0); // Reset to head of file

    connection->close();
}

// Receiving a file

void DataServer::InitiateDownload(QNodeAddress addr, quint32 request_id, QKey key)
{
    // TODO: Eventually check request_id

    QTcpSocket* connection = new QTcpSocket();
    connect(connection, SIGNAL(connected()), this, SLOT(requestDownload()));
    connection->connectToHost(addr.first, addr.second);
    // TODO: connect SIGNAL(error(QAbstractError::SocketError));
    pending_downloads_->insert(connection, key);
}

void DataServer::RequestDownload()
{
    QTcpSocket* connection = (QTcpSocket*) QObject::sender();
    connect(connection, SIGNAL(disconnected()), connection, SLOT(deleteLater()));

    QKey key = pending_downloads_->value(connection);
    pending_downloads_->remove(connection);

    Download dl(key);
    in_progress_downloads_->insert(connection, dl);
    connect(connection, SIGNAL(readyRead()), this, SLOT(ProcessDownload()));

    QByteArray block;
    QDataStream out(&block, QIODevice::ReadWrite);
    out << key;
    connection->write(block);
}

void DataServer::ProcessDownload()
{
    QTcpSocket* connection = (QTcpSocket*) QObject::sender();
    Download download = in_progress_downloads_->value(connection);

    QDataStream in(connection);
    if (download.get_size() == 0) {
        if (connection->bytesAvailable() < (int) sizeof(quint32)) return;
        quint64 s;
        in >> s;
        download.set_size(s);
    }

    char buffer[kBufferSize];
    while (connection->bytesAvailable() && !download.Complete()) {
        quint64 bytes_read = connection->read(buffer, kBufferSize);
        download.Write(buffer, bytes_read);
    }

    if (download.Complete()) {
        if (download.get_bytes_read() == download.get_size()) {
            Store(download.get_key(), download.get_file());
        }
        in_progress_downloads_->remove(connection);
        connection->close();
    } else {
        in_progress_downloads_->insert(connection, download);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Download

DataServer::Download::Download() {}

DataServer::Download::Download(QKey key) : size_(0), bytes_read_(0)
{
    key_ = key;
    file_ = new QFile(QString(key.constData())); // TODO: error handling
    file_->open(QIODevice::ReadWrite);
}

DataServer::Download::Download(const Download& other)
{
    key_ = other.key_;
    file_ = other.file_;
    size_ = other.size_;
    bytes_read_ = other.bytes_read_;
}

void DataServer::Download::Write(char* buff, quint64 num_bytes)
{
    file_->write(buff, num_bytes);
}
