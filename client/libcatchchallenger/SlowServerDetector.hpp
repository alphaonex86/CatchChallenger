#ifndef CATCHCHALLENGER_SLOWSERVERDETECTOR_H
#define CATCHCHALLENGER_SLOWSERVERDETECTOR_H

#include <cstdint>

#include "../../general/base/lib.h"

namespace CatchChallenger {
/// \brief when to show the "slow server" sign: a slow reply stays shown TIMETOSHOWTHETURTLE ms,
/// else a query still waiting longer than TIMEINMSTOBESLOW. Times in ms of Api_protocol::queryClockMs().
class DLL_PUBLIC SlowServerDetector
{
public:
    enum State : uint8_t
    {
        State_Fast,
        State_LastReplySlow,//slowReplyTime() gives the time
        State_QueryWaiting//the worst running query time given to state()
    };
    SlowServerDetector();
    void reset();
    void replyReceived(const uint32_t &replyTimeMs,const uint64_t &nowMs);
    State state(const uint32_t &worstRunningQueryTimeMs,const uint64_t &nowMs) const;
    uint32_t slowReplyTime() const;
private:
    bool slowReplyStillShown(const uint64_t &nowMs) const;

    uint32_t slow_reply_time_;
    uint64_t slow_reply_at_;
};
}

#endif // CATCHCHALLENGER_SLOWSERVERDETECTOR_H
