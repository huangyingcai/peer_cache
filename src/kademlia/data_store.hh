#ifndef PEER_CACHE_DATA_MANAGER_HH
#define PEER_CACHE_DATA_MANAGER_HH


class DataServer : public QObject
{
    Q_OBJECT

    public:
        DataServer();

        bool has_key(QKey key);
        void store(QKey key, QByteArray data); // TODO: call download manager
            // and see if all parts, then save

    public slots:
        QList<QKey> closest_nodes(QKey key, quint16 num_nodes);
        QList<QByteArray> data(QKey key); // TODO: implement like file downlaods
        // download, etc

        // TODO: timers and such
    private:
        QMap<QKey, ifstream>* m_resources;
        QTcpServer* m_tcp_server;

        QMap<QTcpSocket*, QKey>* m_pending_downloads;
        QMap<QTcpSocket*, Download>* m_in_progress_downloads;

        // TODO: command line option to specify where to save data/location of
        // browser cache -- iterate over all files and add them to cache on
        // startup
};

#endif // PEER_CACHE_DATA_MANAGER_HH
