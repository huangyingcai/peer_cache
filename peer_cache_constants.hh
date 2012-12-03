#ifndef PEER_CACHE_CONSTANTS_HH
#define PEER_CACHE_CONSTANTS_HH

const quint16 kBucketSize = 4; // 'K' from Kademlia algorithm
const quint16 kReplicationFactor = 2; // 'Alpha' from Kademlia algorithm
const quint32 kExpiry = 1800; // How long to hold onto entries (in seconds)

typedef enum { PING = 1, ACK,
               STORE, DOWNLOAD,
               FIND_NODE, REPLY_NODE,
               FIND_VALUE, REPLY_VALUE };

typedef QByteArray QKey;
typedef QByteArray QNodeId;
typedef QPair<QHostAddress, quint16> QNodeAddress;

#endif // PEER_CACHE_CONSTANTS_HH
