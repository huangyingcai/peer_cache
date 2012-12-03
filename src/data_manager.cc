#include "data_manager.hh"
#include "includes.hh"

DataServer::DataServer()
{
    m_tcp_server = new QTcpServer(this);
    m_tcp_server->listen(QHostAddress::LocalHost, kDefaultPort);
    connect(m_tcp_server, SIGNAL(newConnection()), this,
        SLOT(AcceptIncomingConnection()));

    m_file_manager = new FileManager(this);
}

////////////////////////////////////////////////////////////////////////////////
// Data Transmission (TCP)

void AcceptIncomingConnection()
{
    m_connection = m_tcp_server->nextPendingConnection();
    connect(m_connection, SIGNAL(disconnected()), connection,
        SLOT(deleteLater()));
    connect(m_connection, SIGNAL(readyRead()), this, ProcessDownloadRequest());
}

void ProcessDownloadRequest()
{
    if (m_connection->bytesAvailable() < (int) kKeySize) return;

    QDataStream in(m_connection);
    QKey key;
    in >> key;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    // TODO: buffered write; asynch

    m_connection->close();
}

void InitiateDownload(QHostAddress addr, QKey key)
{
// TODO:    m_client_socket;
// m_client_socket->connectToHost(addr.first, addr.second);
// connect(m_client_socket, SIGNAL(connected()),
//    this, requestDownload());
// add connection to table; conn->key
// then requestDownload() writes to it; then connects SIGNAL(readyRead()) with
// ProcessDownload() - size, data- can read partial, just keep appending to file
// keep Download{key, ofstream*, bytesRead, size}
// when it's done, emit signal with Download* => save it as a file
}
