#ifndef CATCHCHALLENGER_QtTimeRangeEventScan_H
#define CATCHCHALLENGER_QtTimeRangeEventScan_H

#ifdef EPOLLCATCHCHALLENGERSERVER
#include "../epoll/EpollTimer.hpp"
#else
#include <QObject>
#include <QTimer>
#endif

#include <stdint.h>

namespace CatchChallenger {
class QtTimeRangeEventScan
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #else
        : public EpollTimer
        #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
public:
    explicit QtTimeRangeEventScan();
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
    void try_initAll() const;
    void timeRangeEventTrigger();
#endif
private:
    void exec();
    void initAll();
private:
    #ifndef EPOLLCATCHCHALLENGERSERVER
    QTimer *next_send_timer;
    #endif
};
}

#endif // PLAYERUPDATER_H
