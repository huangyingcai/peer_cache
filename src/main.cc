#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHostInfo>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "peer_cache.hh"
#include "main.hh"

NetworkAccessDialog::NetworkAccessDialog() : QDialog(),
    cache_load_control_(QNetworkRequest::PreferCache)
{
    network_manager_ = new QNetworkAccessManager(this);
    connect(network_manager_, SIGNAL(finished(QNetworkReply*)),
        SLOT(GetRequestFinished(QNetworkReply*)));
    PeerCache* peer_cache = new PeerCache();
    network_manager_->setCache(peer_cache);

    switch_to_offline_mode_button_ = new QPushButton(this);
    switch_to_offline_mode_button_->setAutoDefault(false);
    switch_to_offline_mode_button_->setText("Switch to Offline (Cache-only)");
    connect(switch_to_offline_mode_button_, SIGNAL(clicked()),
        SLOT(SwitchConnectionMode()));

    get_input_ = new QLineEdit(this);
    get_input_->setPlaceholderText("Enter a url");
    connect(get_input_, SIGNAL(returnPressed()),
        SLOT(CaptureGetRequestInput()));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(get_input_);
    layout->addWidget(switch_to_offline_mode_button_);
    setLayout(layout);
}

NetworkAccessDialog::~NetworkAccessDialog()
{
    // QObject/parent relationships handle the rest
}

void NetworkAccessDialog::SwitchConnectionMode()
{
    if (cache_load_control_ == QNetworkRequest::PreferCache) {
        cache_load_control_ = QNetworkRequest::AlwaysCache;
        switch_to_offline_mode_button_->setText("Switch to Online");
    } else {
        cache_load_control_ = QNetworkRequest::PreferCache;
        switch_to_offline_mode_button_->setText(
            "Switch to Offline (Cache-only)");
    }
}

void NetworkAccessDialog::CaptureGetRequestInput()
{
    QString url_string = get_input_->text();

    // Clear the messageInput to get ready for the next input message.
    get_input_->clear();

    QNetworkRequest request;
    request.setUrl(QUrl(url_string));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
        cache_load_control_);
    network_manager_->get(request);
}

void NetworkAccessDialog::GetRequestFinished(QNetworkReply* reply)
{
    QVariant from_cache =
        reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute);
    qDebug() << "From cache?: " << from_cache.toBool();

    // Output the data so I know the request is working properly
    QFile last_request(QString("tmp/last_request.html"));
    last_request.open(QIODevice::ReadWrite | QIODevice::Truncate);
    char data[1024]; // FIXME: constant
    while (!reply->atEnd()) {
        qint64 bytes_read = reply->read(data, 1024);
        last_request.write(data, bytes_read);
    }
    last_request.close();

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

    qRegisterMetaType<QKey>("QKey");
    qRegisterMetaType<QNode>("QNode");

    NetworkAccessDialog dialog;
    dialog.show();

    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}
