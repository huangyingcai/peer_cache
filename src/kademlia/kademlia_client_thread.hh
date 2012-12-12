#include <QThread>

class KademliaClientThread : public QThread
{
    Q_OBJECT

    public:
        KademliaClientThread(QObject* parent = 0); // FIXME: parent
        ~KademliaClientThread();

        void run();

    public slots:
        void Find(QKey key);
        void Store(QKey key, QIODevice* file);

        void HandleLookupTermination(QKey key, QIODevice* device = NULL);
        QIODevice* get_last_found_value() { return last_found_value_; };

    signals:
        void FindRequestComplete();

    private:
        QMutex mutex_;
        KademliaClient* client_;
        QKey* find_key_;
        QIODevice* last_found_value_;
};
