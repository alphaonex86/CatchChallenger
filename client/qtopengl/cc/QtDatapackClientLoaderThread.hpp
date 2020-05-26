#ifndef QTDATAPACKCLIENTLOADERTHREAD_H
#define QTDATAPACKCLIENTLOADERTHREAD_H

#include <QThread>

class QtDatapackClientLoaderThread : public QThread
{
    Q_OBJECT
public:
    QtDatapackClientLoaderThread();
#ifndef NOTHREADS
    void run() override;
#else
    void run();
#endif
};

#endif // QTDATAPACKCLIENTLOADERTHREAD_H
