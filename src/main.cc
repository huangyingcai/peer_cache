#include <QApplication>

#include "main.hh"
#include "kademlia_client.hh"

#include <QHostInfo>
#include <QFileDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>

KademliaClientDialog::KademliaClientDialog(QNodeAddress bootstrap_address) :
    QDialog()
{
    setWindowTitle("Kademlia Client");

    client_ = new KademliaClient(bootstrap_address);

    add_file_button_ = new QPushButton("Add File", this);
    add_file_button_->setAutoDefault(false);
    connect(add_file_button_, SIGNAL(clicked()), this,
        SLOT(DisplayFileDialog()));

    find_input_ = new QLineEdit(this);
    find_input_->setPlaceholderText("Enter a search query");
    connect(find_input_, SIGNAL(returnPressed()), this,
        SLOT(CaptureSearchRequestInput()));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(add_file_button_);
    layout->addWidget(find_input_);
    setLayout(layout);
}

KademliaClientDialog::~KademliaClientDialog()
{
    delete client_;
    // QObject/parent relationships handle the rest
}

void KademliaClientDialog::CaptureSearchRequestInput()
{
    QString find_string = find_input_->text();

    // Clear the messageInput to get ready for the next input message.
    find_input_->clear();

    client_->SearchForFile(find_string);
}

void KademliaClientDialog::DisplayFileDialog()
{
    QString filename = QFileDialog::getOpenFileName(this,
        "Select one or more files to share", "/home/accts/dkt2");

    if (!filename.isEmpty()) {
        client_->AddFile(filename);
    }
}

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

    if (argc != 3) {
        qDebug() << "usage: peer_cache node port";
        return 1;
    }

    QStringList args = app.arguments();

    info = QHostInfo::fromName(args.at(1));
    QHostAddress addr = info.addresses().first();
    qDebug() << "Bootstrap address: " << addr;
//    QHostAddress addr = QHostAddress(args.at(1));
    quint16 port = args.at(2).toUInt();

    KademliaClientDialog dialog(qMakePair(addr, port));
    dialog.show();

    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}
