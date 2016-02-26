#ifndef TIMERPurgeTokenAuthList_H
#define TIMERPurgeTokenAuthList_H

#include "../EpollTimer.h"

class TimerPurgeTokenAuthList : public EpollTimer
{
public:
    TimerPurgeTokenAuthList();
    void exec();
};

#endif // TIMERPurgeTokenAuthList_H
