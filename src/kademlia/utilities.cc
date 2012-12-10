#include "constants.hh"
#include "utilities.hh"

#include <QBitArray>

bool operator>(QBitArray& a1, QBitArray& a2)
{
    for (int i = 0; i < kKeyLength * 8; i++) {
        if (a1[i] > a2[i]) return true;
        if (a1[i] < a2[i]) return false;
    }
    return false;
}

QBitArray Distance(QKey& key_a, QKey& key_b)
{
     // Create bit arrays of the appropriate size
     QBitArray a_bits(kKeyLength * 8);
     QBitArray b_bits(kKeyLength * 8);

     // Convert from QByteArray to QBitArray
     for (int i = 0; i < kKeyLength; i++) {
         for (int b = 0; b < 8; ++b) {
                 a_bits.setBit(i * 8 + b, key_a.at(i) & (1 << b));
                 b_bits.setBit(i * 8 + b, key_b.at(i) & (1 << b));
         }
     }

     return a_bits ^ b_bits;
}
