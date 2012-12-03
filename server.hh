#ifndef PEER_CACHE_SERVER_HH
#define PEER_CACHE_SERVER_HH

#include "peer_cache_includes.hh"


class Server : public QObject
{
    Q_OBJECT

    public:
        Server();
        ~Server();

    public slots:
        QList<QKey> find_node(QNodeId id);
        QVariant find_value(QKey key);
        void ping(QNodeId id);
        void store(QKey key, QByteArray data);

        void get(QUrl url);

    private slots:
        void send_ack(const QNode& node);

    signals:
        // void request_ready();
        // void reply_ready();
        // void ping_received(const QNode&);

    private:

        ResourceManager* m_resource_manager;

        // RequestManager* m_request_manager;
        QList<QVariantMap>* m_request_queue; // Requests ready to be processed
        QMap<QByteArray, QNodeId>* m_sent_requests; // Requests server has sent
                                                    // for which it is waiting
                                                    // on a response

        QVariantMap parse(QByteArray packet); // add to request queue


};
#endif // PEERSTER_SERVER_HH
