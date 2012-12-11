#ifndef PEER_CACHE_DAEMON_HH
#define PEER_CACHE_DAEMON_HH

class PeerCacheDaemon
{
    // Listen on Port with TcpServer
    // Set cache of NAM to PeerCache
    private:
        const static quint16 kDaemonPort = 8080;

        QNetworkAccessManager* network_access_manager_;
        QTcpServer* proxy_server_;
};

#endif // PEER_CACHE_DAEMON_HH
