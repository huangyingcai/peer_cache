#ifndef PEER_CACHE_TYPES_HH
#define PEER_CACHE_TYPES_HH

typedef enum { PING = 1, ACK,
               STORE, DOWNLOAD,
               FIND_NODE, REPLY_NODE,
               FIND_VALUE, REPLY_VALUE };

typedef QByteArray QKey;
typedef QByteArray QNodeId;
typedef QPair<QHostAddress, quint16> QNodeAddress;
typedef QPair<QNodeId, QNodeAddress> QNode;

#endif // PEER_CACHE_TYPES_HH
