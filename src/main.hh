#include "types.hh"
#include "includes.hh"

#include <QDialog>

class QNetworkAccessManager;
class QNetworkReply;
class QLineEdit;

class NetworkAccessDialog : public QDialog
{
    Q_OBJECT

    public:
        NetworkAccessDialog();
        ~NetworkAccessDialog();

    public slots:
        void CaptureGetRequestInput();
        void GetRequestFinished(QNetworkReply* reply);

    private:
        QNetworkAccessManager* network_manager_;

        QLineEdit* get_input_;
};
