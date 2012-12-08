#ifndef PEER_CACHE_HH
#define PEER_CACHE_HH

class PeerCache : public QNetworkDiskCache, public KademliaClient
{
  // Probably need to keep track of outstanding find requests, etc with timers
  // on them
};

#endif // PEER_CACHE_HH
