#ifndef CATCHCHALLENGER_TimeRangeEventScan_H
#define CATCHCHALLENGER_TimeRangeEventScan_H

#ifdef EPOLLCATCHCHALLENGERSERVER
#include "../epoll/EpollTimer.hpp"
#else
#include <QObject>
#include <QTimer>
#endif

#include <stdint.h>

namespace CatchChallenger {
class TimeRangeEventScan
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
    explicit TimeRangeEventScan();
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
