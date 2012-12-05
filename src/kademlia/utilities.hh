#ifndef KADEMLIA_UTILITIES_HH
#define KADEMLIA_UTILITIES_HH

#include "types.hh"

bool operator>(QBitArray& a1, QBitArray& a2);
QBitArray Distance(QKey& key_a, QKey& key_b);

#endif // KADEMLIA_UTILITIES_HH
