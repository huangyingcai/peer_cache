#include "server.hh"
#include "peer_cache_constants.hh"

QList<QKey> find_node(QKey node_id)
{
    return list;
}

QVariant find_value(QKey key)
{
  // if (lookup(sha)) return value
  // else, return find_node
}

QVariantMap parse(QByteArray packet)
{
  // see if this is new request or response to one of mine
  // randomRPC
  // NodeId -> node ->update
  // Kind -> ping, store, find_node, find_value
  // if ping -> verify nodeid == stored value of address
}
