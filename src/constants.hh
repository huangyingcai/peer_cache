#ifndef KADEMLIA_CONSTANTS_HH
#define KADEMLIA_CONSTANTS_HH

enum RpcType { PING = 1, ACK,
               STORE, READY_DOWNLOAD,
               FIND_NODE, REPLY_NODE,
               FIND_VALUE, REPLY_VALUE };

const int kBucketSize = 4; // 'K' from Kademlia algorithm
const int kReplicationFactor = 2; // 'Alpha' from Kademlia algorithm
const int kExpiry = 1800; // How long to hold onto entries (in seconds)
const int kKeyLength = 20; // Length (in bytes) of SHA-1 Hash

#endif // KADEMLIA_CONSTANTS_HH
