#ifndef QZstdDecodeThread_h
#define QZstdDecodeThread_h

#include <QThread>

/// \brief to decode the xz via a thread
class QZstdDecodeThread : public QThread
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
        protected:
                void run();
        private:
                std::vector<char> mDataToDecode;
                QString mErrorString;
        signals:
                void decodedIsFinish();
};

#endif // QZstdDecodeThread_h
