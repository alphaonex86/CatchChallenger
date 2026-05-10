#ifndef CATCHCHALLENGER_SERVER_STRUCTURES_EVENT_LOOP_H
#define CATCHCHALLENGER_SERVER_STRUCTURES_EVENT_LOOP_H

#include "../cli/timer/TimerDdos.hpp"
#include "../cli/timer/TimerPositionSync.hpp"
#include "../cli/timer/TimerSendInsertMoveRemove.hpp"
#include "../cli/timer/TimeRangeEventScan.hpp"
#include "timer/TimerEvents.hpp"
#include <vector>

class ServerPrivateVariablesEventLoop
{
public:
    ServerPrivateVariablesEventLoop();
    static ServerPrivateVariablesEventLoop serverPrivateVariablesEventLoop;

    std::vector<TimerEvents *> timerEvents;

    TimerDdos ddosTimer;
    TimerPositionSync positionSync;
    TimerSendInsertMoveRemove *timer_to_send_insert_move_remove;

    TimeRangeEventScan timeRangeEventScan;
    uint64_t nextTimeStampsToCaptureCity;
};

#endif // STRUCTURES_SERVER_H
