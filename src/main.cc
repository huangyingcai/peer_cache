#include <QApplication>

#include "main.hh"
#include "kademlia_client.hh"

#include <QHostInfo>
#include <QFileDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>

KademliaClientDialog::KademliaClientDialog() : QDialog()
{
    setWindowTitle("Kademlia Client");

    client_ = new KademliaClient();

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

    KademliaClientDialog dialog;
    dialog.show();

    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}
