#include "types.hh"
#include "includes.hh"

#include <QDialog>

#include <QNetworkRequest>

class QNetworkAccessManager;
class QNetworkReply;
class QLineEdit;
class QPushButton;

class NetworkAccessDialog : public QDialog
{
    Q_OBJECT

    public:
        NetworkAccessDialog();
        ~NetworkAccessDialog();

    public slots:
        void SwitchConnectionMode();
        void CaptureGetRequestInput();
        void GetRequestFinished(QNetworkReply* reply);

    private:
        QNetworkAccessManager* network_manager_;
        QNetworkRequest::CacheLoadControl cache_load_control_;
        QPushButton* switch_to_offline_mode_button_;
        QLineEdit* get_input_;
};
