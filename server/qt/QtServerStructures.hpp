#ifndef QTCATCHCHALLENGER_SERVER_STRUCTURES_H
#define QTCATCHCHALLENGER_SERVER_STRUCTURES_H

#include <QTimer>
#include "timer/QtTimerEvents.hpp"
#include "db/QtDatabase.hpp"
#include "timer/QtPlayerUpdater.hpp"
#include "timer/QtTimeRangeEventScanBase.hpp"

struct QtServerPrivateVariables
{
    //bd
    std::vector<QtTimerEvents *> timerEvents;

    QTimer *timer_city_capture;//moved to epoll loop
    QTimer *timer_to_send_insert_move_remove;
    QTimer positionSync;
    QTimer ddosTimer;

    QtPlayerUpdater player_updater;
    QtTimeRangeEventScan timeRangeEventScan;
};

#endif // STRUCTURES_SERVER_H
