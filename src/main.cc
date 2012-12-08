#include <QApplication>

#include "types.hh"
#include "includes.hh"
#include "kademlia_client.hh"

#include <QHostInfo>

int main(int argc, char** argv)
{
    // Initialize Qt toolkit
    QApplication app(argc,argv);

    // Initialize QCA
    QCA::Initializer qcainit; // (QCA::MemoryMode::Practical, blockLength);

    QString nodeInfoString = QString("%1.%2").arg(
        QHostInfo::localHostName()).arg(QHostInfo::localDomainName());
    qDebug() << nodeInfoString;
    QHostInfo info = QHostInfo::fromName(nodeInfoString);
    qDebug() << "Local host address: " << info.addresses().first();

//    info = QHostInfo::fromName(QString(argv[1]));
//    QHostAddress addr = info.addresses().first();
    QHostAddress addr = QHostAddress(QString(argv[1]));
    quint16 port = QString(argv[2]).toUInt();
    KademliaClient* client = new KademliaClient(qMakePair(addr, port));

    // while (getline()) {
    //    QStringList commands = line.split(" ");
    //    if (commands[0] == QString("Get")) {
      //      Issue Find Request (QHash(resource))
    //    } else if (commands[1] == QString("Store") {
    //        new QFile; dataserver.store
    //    }
    // }
    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}
