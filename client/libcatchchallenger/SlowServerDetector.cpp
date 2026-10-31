#include "SlowServerDetector.hpp"
#include "ClientVariable.hpp"

using namespace CatchChallenger;

SlowServerDetector::SlowServerDetector() :
    slow_reply_time_(0),
    slow_reply_at_(0)
{
}

void SlowServerDetector::reset()
{
    slow_reply_time_=0;
    slow_reply_at_=0;
}

bool SlowServerDetector::slowReplyStillShown(const uint64_t &nowMs) const
{
    return slow_reply_time_>TIMEINMSTOBESLOW && nowMs>=slow_reply_at_ && (nowMs-slow_reply_at_)<TIMETOSHOWTHETURTLE;
}

void SlowServerDetector::replyReceived(const uint32_t &replyTimeMs,const uint64_t &nowMs)
{
    //a fast reply does not hide a slow one still shown
    if(replyTimeMs>TIMEINMSTOBESLOW || !slowReplyStillShown(nowMs))
    {
        slow_reply_time_=replyTimeMs;
        slow_reply_at_=nowMs;
    }
}

SlowServerDetector::State SlowServerDetector::state(const uint32_t &worstRunningQueryTimeMs,const uint64_t &nowMs) const
{
    if(slowReplyStillShown(nowMs))
        return State_LastReplySlow;
    if(worstRunningQueryTimeMs>TIMEINMSTOBESLOW)
        return State_QueryWaiting;
    return State_Fast;
}

uint32_t SlowServerDetector::slowReplyTime() const
{
    return slow_reply_time_;
}
