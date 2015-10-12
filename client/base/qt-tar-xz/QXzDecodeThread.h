/** \file QXzDecodeThread.h
\brief To have thread to decode the data
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef QXZDECODETHREAD_H
#define QXZDECODETHREAD_H

#include <QThread>
#include "QXzDecode.h"

/// \brief to decode the xz via a thread
class QXzDecodeThread
        #ifndef QT_NO_EMIT
        : public QThread
        #endif
{
    #ifndef QT_NO_EMIT
    Q_OBJECT
    #endif
    public:
        QXzDecodeThread();
        ~QXzDecodeThread();
        /// \brief to return if the error have been found
        bool errorFound();
        /// \brief to return the error string
        std::string errorString();
        /// \brief to get the decoded data
        std::vector<char> decodedData();
        /// \brief to send the data
        void setData(std::vector<char> data, uint64_t maxSize=0);
    #ifndef QT_NO_EMIT
    protected:
        void run();
    #else
    public:
        void run();
    #endif
    private:
        /// \brief to have temporary storage
        QXzDecode *DataToDecode;
        bool error;
    #ifndef QT_NO_EMIT
    signals:
        void decodedIsFinish();
    #endif
};

#endif // QXZDECODETHREAD_H
