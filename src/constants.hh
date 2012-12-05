#ifndef PEER_CACHE_CONSTANTS_HH
#define PEER_CACHE_CONSTANTS_HH

enum { JOIN = 1, BOOTSTRAP_OK,
       PING, ACK,
       STORE, DOWNLOAD,
       FIND_NODE, REPLY_NODE,
       FIND_VALUE, REPLY_VALUE };

const int kBucketSize = 4; // 'K' from Kademlia algorithm
const int kReplicationFactor = 2; // 'Alpha' from Kademlia algorithm
const int kExpiry = 1800; // How long to hold onto entries (in seconds)
const int kKeySize = 20; // Length (in bytes) of SHA-1 Hash

#endif // PEER_CACHE_CONSTANTS_HH
