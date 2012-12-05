#ifndef PEER_CACHE_DATA_STORE_HH
#define PEER_CACHE_DATA_STORE_HH

class DataServer : public QTcpServer
{
    Q_OBJECT

    public:
        DataServer();

        void Store(QKey key, QFile* file);
        QFile* Value(QKey key);

    private:
        const static kBufferSize = 1024;

        QHash<QKey, QFile*> files_;

        QHash<QTcpSocket*, QKey>* pending_downloads_;
        QHash<QTcpSocket*, Download>* in_progress_downloads_;

        class Download
        {
            public:
                Download(QKey key);
                Download(const Download& other);

                quint64 get_size() { return size_; };
                void set_size(quint32 s) { size_ = s; };
                quint64 get_bytes_read() { return bytes_read_; };
                bool Complete() { return bytes_read_ >= size_; };
                void Write(char* buff, quint64 num_bytes);

            private:
                QKey key_;
                QFile* file_; // Never freed
                quint64 size_;
                quint64 bytes_read_;

        };
        typedef Download Upload;

        // TODO: command line option to specify where to save data/location of
        // browser cache -- iterate over all files and add them to cache on
        // startup
};

#endif // PEER_CACHE_DATA_STORE_HH
