#ifndef CATCHCHALLENGER_SERVER_STRUCTURESEpoll_H
#define CATCHCHALLENGER_SERVER_STRUCTURESEpoll_H

#include "../epoll/timer/TimerCityCapture.hpp"
#include "../epoll/timer/TimerDdos.hpp"
#include "../epoll/timer/TimerPositionSync.hpp"
#include "../epoll/timer/TimerSendInsertMoveRemove.hpp"
#include "../epoll/timer/TimerEvents.hpp"
#include "../epoll/timer/TimeRangeEventScan.hpp"
#include "../base/DatabaseBase.hpp"

class ServerPrivateVariablesEpoll
{
public:
    ServerPrivateVariablesEpoll();
    static ServerPrivateVariablesEpoll serverPrivateVariablesEpoll;

    std::vector<TimerEvents *> timerEvents;

    TimerDdos ddosTimer;
    TimerPositionSync positionSync;
    TimerSendInsertMoveRemove *timer_to_send_insert_move_remove;

    TimeRangeEventScan timeRangeEventScan;
    uint64_t nextTimeStampsToCaptureCity;
};

#endif // STRUCTURES_SERVER_H
