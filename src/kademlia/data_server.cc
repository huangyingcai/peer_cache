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

QFile* Value(QKey key)
{
    return files_.value(key, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// Data Transmission (TCP)

void AcceptIncomingConnection()
{
    QTcpSocket* connection = nextPendingConnection();
    connect(connection, SIGNAL(disconnected()), connection,
        SLOT(deleteLater()));
    connect(onnection, SIGNAL(readyRead()), this, ProcessDownloadRequest());
}

// Sending a file

void ProcessDownloadRequest()
{
    QTcpSocket* connection = (QTcpSocket*) QObject::sender();

    if (connection->bytesAvailable() < (int) kKeySize) return;

    QDataStream in(connection);
    QKey key;
    in >> key;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    QFile* upload = Value(key);
    out << upload->size();

    // TODO: Async write
    char buffer[kBufferSize];
    quint64 bytes_read = 0;
    while ((bytes_read = upload->read(buffer, kBufferSize)) > 0) {
        out.writeBytes(buffer, bytes_read);
    }
    if (bytes_read < 0) ERROR("Error reading file");
    upload->seek(0); // Reset to head of file

    m_connection->close();
}

// Receiving a file

void InitiateDownload(QHostAddress addr, quint32 request_id, QKey key)
{
    // TODO: Eventually check request_id

    QTcpSocket* connection = new QTcpSocket();
    connect(connection, SIGNAL(connected()), this, SLOT(requestDownload()));
    connection->connectToHost(addr.first, addr.second);
    // TODO: connect SIGNAL(error(QAbstractError::SocketError));
    pending_downloads_->insert(connection, key);
}

void RequestDownload()
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

void ProcessDownload()
{
    QTcpSocket* connection = (QTcpSocket*) QObject::sender();
    Download download = in_progress_downloads_->value(connection);

    QDataStream in(m_connection);
    if (download.get_size() == 0) {
        if (connection->bytesAvailable() < (int) sizeof(quint32)) return;
        quint64 s;
        in >> s;
        download.set_size(s);
    }

    char buffer[kBufferSize];
    while (connection->bytesAvailable() && !download.complete()) {
        quint64 bytes_read = connection->readData(buffer, kBufferSize);
        download.Write(buffer, bytes_read);
    }

    if (download.complete()) {
        if (download.get_bytes_read() == download.get_size()) {
            Store(download.key, download.file);
        }
        in_progress_downloads_->remove(connection);
    } else {
        in_progress_downloads_->insert(connection, download);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Download

Download::Download(QKey key)
{
    key_ = key;
    file_ = new File(QString(key.constData())); // TODO: error handling
    file_.open(QIODevice::ReadWrite)
    size_ = 0;
    bytes_read_ = 0;
}

Download::Download(const Download& other)
{
    key_ = other.key_;
    file_ = other.file_;
    size_ = other.size_;
    bytes_read = other.bytes_read_;
}

void DataServer::Download::Write(char* buff, quint64 num_bytes)
{
    file_->writeData(buff, num_bytes);
}


// QNetworkAccessManager
