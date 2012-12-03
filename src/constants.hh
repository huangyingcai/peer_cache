#ifndef PEER_CACHE_CONSTANTS_HH
#define PEER_CACHE_CONSTANTS_HH

const quint16 kBucketSize = 4; // 'K' from Kademlia algorithm
const quint16 kReplicationFactor = 2; // 'Alpha' from Kademlia algorithm
const quint32 kExpiry = 1800; // How long to hold onto entries (in seconds)
const quint16 kKeySize = 20; // Length (in bytes) of SHA-1 Hash

#endif // PEER_CACHE_CONSTANTS_HH
