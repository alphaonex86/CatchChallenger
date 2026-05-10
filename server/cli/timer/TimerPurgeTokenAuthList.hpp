#ifndef TIMERPurgeTokenAuthList_H
#define TIMERPurgeTokenAuthList_H

#include "../EventLoopTimer.hpp"

class TimerPurgeTokenAuthList : public EventLoopTimer
{
public:
    TimerPurgeTokenAuthList();
    void exec();
};

#endif // TIMERPurgeTokenAuthList_H
