#ifndef KADEMLIA_DATA_SERVER_HH
#define KADEMLIA_DATA_SERVER_HH

#include "types.hh"
#include <QTcpServer>

class DataServer : public QTcpServer
{
    Q_OBJECT

    public:
        DataServer();

        void Store(QKey key, QFile* file);
        QFile* Value(QKey key);

    public slots:
        // Data transmission
        void AcceptIncomingConnection();
        void ProcessDownloadRequest();
        void InitiateDownload(QNodeAddress addr, quint32 request_id, QKey key);
        void RequestDownload();
        void ProcessDownload();

    private:
        const static quint16 kDefaultPort = 42600;
        const static quint64 kBufferSize = 1024;

        QHash<QKey, QFile*> files_;

        class Download
        {
            public:
                Download();
                Download(QKey key);
                Download(const Download& other);

                quint64 get_size() { return size_; };
                void set_size(quint32 s) { size_ = s; };
                quint64 get_bytes_read() { return bytes_read_; };
                QKey get_key() { return key_; };
                QFile* get_file() { return file_; };

                bool Complete() { return bytes_read_ >= size_; };
                void Write(char* buff, quint64 num_bytes);

            private:
                QKey key_;
                QFile* file_; // Never freed
                quint64 size_;
                quint64 bytes_read_;

        };

        QHash<QTcpSocket*, QKey>* pending_downloads_;
        QHash<QTcpSocket*, Download>* in_progress_downloads_;

        // TODO: command line option to specify where to save data/location of
        // browser cache -- iterate over all files and add them to cache on
        // startup
};

#endif // KADEMLIA_DATA_SERVER_HH
