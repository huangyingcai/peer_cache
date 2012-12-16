#include "constants.hh"

using namespace kademlia;

enum RpcType { JOIN = 1, JOIN_REPLY,
               PING, ACK,
               STORE, READY_DOWNLOAD,
               FIND_NODE, FIND_NODE_REPLY,
               FIND_VALUE, FIND_VALUE_REPLY };

const int kBucketSize = 2; // 'K' from Kademlia algorithm
const int kReplicationFactor = 2; // 'Alpha' from Kademlia algorithm
const int kExpiry = 1800; // How long to hold onto entries (in seconds)
const int kKeyLength = 20; // Length (in bytes) of SHA-1 Hash
