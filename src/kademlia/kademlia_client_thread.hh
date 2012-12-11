#include <QThread>

class KademliaClientThread : public QThread
{
    Q_OBJECT

    public:
        void run();
};
