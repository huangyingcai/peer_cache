#include "data_manager.hh"
#include "includes.hh"

DataServer::DataServer()
{
    m_tcp_server = new QTcpServer(this);
    m_tcp_server->listen(QHostAddress::LocalHost, kDefaultPort);
    connect(m_tcp_server, SIGNAL(newConnection()), this,
        SLOT(AcceptIncomingConnection()));

    m_file_manager = new FileManager(this);

    connect(this, SIGNAL(DownloadComplete(QKey, fstream*)), m_file_manager,
        SLOT(Store(QKey, fstream*))); // TODO: should error check on key
}

////////////////////////////////////////////////////////////////////////////////
// Data Transmission (TCP)

void AcceptIncomingConnection()
{
    QTcpSocket* connection = m_tcp_server->nextPendingConnection();
    connect(connection, SIGNAL(disconnected()), connection,
        SLOT(deleteLater()));
    connect(onnection, SIGNAL(readyRead()), this, ProcessDownloadRequest());
}

void ProcessDownloadRequest()
{
    connection = (QTcpSocket*) QObject::sender();

    if (m_connection->bytesAvailable() < (int) kKeySize) return;

    QDataStream in(m_connection);
    QKey key;
    in >> key;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    // out << size

    // TODO: buffered write; asynch

    m_connection->close();
}

void InitiateDownload(QHostAddress addr, quint32 request_id, QKey key)
{
    // TODO: Eventually check request_id
    QTcpSocket* connection = new QTcpSocket();
    connect(connection, SIGNAL(connected()), this, SLOT(requestDownload()));
    connection->connectToHost(addr.first, addr.second);
    // connect SIGNAL(error(QAbstractError::SocketError));
    m_pending_downloads->insert(connection, key);
}

void RequestDownload()
{
    QTcpSocket* connection = (QTcpSocket*) QObject::sender();
    connect(connection, SIGNAL(disconnected()), connection, SLOT(deleteLater()));
    QKey key = m_pending_downloads->value(connection);
    m_in_progress_downloads->insert(connection, key);
    m_pending_downloads->remove(connection);
    connect(connection, SIGNAL(readyRead()), this, SLOT(ProcessDownload()));

    QByteArray block;
    QDataStream out(&block, QIODevice::ReadWrite);
    out << m_pending_downloads->value(connection);
    connection->write(block);
}

void ProcessDownload()
{
    QTcpSocket* connection = (QTcpSocket*) QObject::sender();
    Download download = m_in_progress_downloads->value(connection);

    QDataStream in(m_connection);
    if (download.size == 0) {
        if (connection->bytesAvailable() < (int) sizeof(quint32)) return;
        in >> download.size;
    }

    while (connection->bytesAvailable() > BUFFER_SIZE && !download.complete()) {
        // TODO: READ in what I can
    }

    if (download.complete()) {
        emit DownloadComplete(download.key, download.file);
        m_in_progress_downloads->remove(connection);
    } else {
        m_in_progress_downloads->insert(connection, download);
    }
}

///////////////////////////////////////////////////////////////////////////////
// HTTP Methods


// QNetworkAccessManager
