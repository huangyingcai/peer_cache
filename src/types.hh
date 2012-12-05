#ifndef PEER_CACHE_TYPES_HH
#define PEER_CACHE_TYPES_HH

class QByteArray;
class QFile;
class QBitArray;

typedef QByteArray QKey;
typedef QByteArray QNodeId;
typedef QPair<QHostAddress, quint16> QNodeAddress;
typedef QPair<QNodeId, QNodeAddress> QNode;
typedef QList<QNode> QNodeList;

#endif // PEER_CACHE_TYPES_HH
