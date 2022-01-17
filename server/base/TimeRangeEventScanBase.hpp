#ifndef CATCHCHALLENGER_TimeRangeEventScanBase_H
#define CATCHCHALLENGER_TimeRangeEventScanBase_H

#include <stdint.h>

namespace CatchChallenger {
class TimeRangeEventScanBase
{
public:
    explicit TimeRangeEventScanBase();
private:
    virtual void initAll() = 0;
};
}

#endif // PLAYERUPDATER_H
