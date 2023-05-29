#ifndef QTDATAPACKCLIENTLOADERTHREAD_H
#define QTDATAPACKCLIENTLOADERTHREAD_H

#include <QThread>
#include <unordered_map>
#include "../../general/base/lib.h"

class DLL_PUBLIC QtDatapackClientLoaderThread : public QThread
{
    Q_OBJECT
public:
    QtDatapackClientLoaderThread();
    ~QtDatapackClientLoaderThread();
    void stop();
#ifndef NOTHREADS
    void run() override;
#else
    void run();
#endif
signals:
    void loadItemImage(uint16_t id,void *ImageitemsExtra);
    void loadMonsterImage(uint16_t id,void *ImagemonsterExtra);
private:
    bool stopIt;
};

#endif // QTDATAPACKCLIENTLOADERTHREAD_H
