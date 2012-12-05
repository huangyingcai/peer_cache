#include <QApplication>

#include "types.hh"
#include "includes.hh"
#include "kademlia_client.hh"

int main(int argc, char** argv)
{
    // Initialize QCA
    QCA::Initializer qcainit; // (QCA::MemoryMode::Practical, blockLength);


   KademliaClient* client = new KademliaClient(qMakePair(QHostAddress(), (quint16)0));

    return 0;
}
