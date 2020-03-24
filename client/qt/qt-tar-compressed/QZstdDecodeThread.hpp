#ifndef QZstdDecodeThread_h
#define QZstdDecodeThread_h

#ifndef NOTHREADS
#include <QThread>
#else
#include <QObject>
#endif

/// \brief to decode the xz via a thread
class QZstdDecodeThread
        #ifndef NOTHREADS
        : public QThread
        #else
        : public QObject
        #endif
{
        Q_OBJECT
        public:
                QZstdDecodeThread();
                ~QZstdDecodeThread();
                /// \brief to return if the error have been found
                bool errorFound();
                /// \brief to return the error string
                QString errorString();
                /// \brief to get the decoded data
                std::vector<char> decodedData();
                /// \brief to send the data
                void setData(std::vector<char> data);
        #ifndef NOTHREADS
        protected:
        #else
        public:
        #endif
                void run();
        private:
                std::vector<char> mDataToDecode;
                QString mErrorString;
        signals:
                void decodedIsFinish();
};

#endif // QZstdDecodeThread_h
