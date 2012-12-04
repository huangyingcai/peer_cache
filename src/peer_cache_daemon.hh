#ifndef PEER_CACHE_DAEMON_HH
#define PEER_CACHE_DAEMON_HH

class PeerCacheDaemon : public QNetworkAccessManager, public QTcpServer
{
    // Listen on Port with TcpServer
    // Set cache of NAM to PeerCache
};

#endif // PEER_CACHE_DAEMON_HH
