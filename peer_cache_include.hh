#ifndef PEER_CACHE_CONSTANTS_HH
#define PEER_CACHE_CONSTANTS_HH

final quint16 kBucketSize = 4; // 'K' from Kademlia algorithm
final quint16 kReplicationFactor = 2; // 'Alpha' from Kademlia algorithm
final quint32 kExpiry = 1800; // How long to hold onto entries (in seconds)

typedef QByteArray QKey;
typedef QByteArray QNodeId;

#endif // PEER_CACHE_CONSTANTS_HH
