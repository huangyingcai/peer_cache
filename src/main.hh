#include "types.hh"
#include "includes.hh"

#include <QDialog>

class QPushButton;
class QLineEdit;
class KademliaClient;

class KademliaClientDialog : public QDialog
{
    Q_OBJECT

    public:
        KademliaClientDialog(QNodeAddress bootstrap_address);
        ~KademliaClientDialog();

    public slots:
        void CaptureSearchRequestInput();
        void DisplayFileDialog();

    private:
        KademliaClient* client_;

        QPushButton* add_file_button_;
        QLineEdit* find_input_;
};
