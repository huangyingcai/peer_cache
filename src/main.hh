#include "types.hh"
#include "includes.hh"

#include <QDialog>

class QPushButton;
class QLineEdit;

class NetworkAccessDialog : public QDialog
{
    Q_OBJECT

    public:
        NetworkAccessDialog();
        ~NetworkAccessDialog();

    public slots:
        void CaptureGetRequestInput();

    private:
        NetworkAccessManager* network_manager_;

        QLineEdit* get_input_;
};
