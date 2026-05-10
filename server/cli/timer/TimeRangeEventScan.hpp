#ifndef CATCHCHALLENGER_TimeRangeEventScanEPOLL_H
#define CATCHCHALLENGER_TimeRangeEventScanEPOLL_H

#include "../EventLoopTimer.hpp"
#include "../../base/TimeRangeEventScanBase.hpp"

class TimeRangeEventScan : public EventLoopTimer, public CatchChallenger::TimeRangeEventScanBase
{
public:
    explicit TimeRangeEventScan();
private:
    void exec();
    void initAll();
};

#endif // PLAYERUPDATER_H
