#include "includes.hh"
#include "constants.hh"

#include "peer_cache_daemon.hh"

PeerCacheDaemon::PeerCacheDaemon() : QNetworkAccessManager(), QTcpServer()
{
    setCache();
}
