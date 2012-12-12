#include <QApplication>

#include "main.hh"
#include "kademlia_client.hh"

#include <QHostInfo>
#include <QFileDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>

NetworkAccessDialog::NetworkAccessDialog() : QDialog()
{
    network_manager_ = new NetworkAccessManager(this);
    connect(network_manager_, SIGNAL(finished(QNetworkReply*)), this,
        SLOT(GetRequestFinished(QNetworkReply*)));

    PeerCache* peer_cache = new PeerCache(this);
    network_manager_->setCache(peer_cache);

    get_input_ = new QLineEdit(this);
    get_input_->setPlaceholderText("Enter a url");
    connect(get_input_, SIGNAL(returnPressed()), this,
        SLOT(CaptureGetRequestInput()));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(get_input_);
    setLayout(layout);
}

NetworkAccessDialog::~NetworkAccessDialog()
{
    // QObject/parent relationships handle the rest
}

void KademliaClientDialog::CaptureGetRequestInput()
{
    QString url_string = get_input_->text();

    // Clear the messageInput to get ready for the next input message.
    get_input_->clear();

    QNetworkRequest request(QUrl(url_string));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    network_manager_->get(request);
}

void NetworkAccessDialog::GetRequestFinished(QNetworkReply* reply)
{
    QVariant from_cache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute);
    qDebug() << "page from cache?" << from_cache.toBool();

    reply->deleteLater();
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

    NetworkAccessDialog dialog;
    dialog.show();

    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}
