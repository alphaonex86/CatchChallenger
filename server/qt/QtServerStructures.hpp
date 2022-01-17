#ifndef QTCATCHCHALLENGER_SERVER_STRUCTURES_H
#define QTCATCHCHALLENGER_SERVER_STRUCTURES_H

#include <QTimer>
#include "QtTimerEvents.hpp"
#include "QtDatabase.hpp"
#include "QtPlayerUpdater.hpp"
#include "QtTimeRangeEventScanBase.hpp"

struct QtServerPrivateVariables
{
    //bd
    std::vector<QtTimerEvents *> timerEvents;

    QTimer *timer_city_capture;//moved to epoll loop
    QTimer *timer_to_send_insert_move_remove;
    QTimer positionSync;
    QTimer ddosTimer;

    CatchChallenger::QtPlayerUpdater player_updater;
    CatchChallenger::QtTimeRangeEventScan timeRangeEventScan;
};

#endif // STRUCTURES_SERVER_H
