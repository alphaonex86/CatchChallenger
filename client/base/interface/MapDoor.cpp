#include "MapDoor.h"

#include <QApplication>

MapDoor::MapDoor(Tiled::MapObject* object, const uint8_t &framesCount, const uint16_t &ms) :
    object(object),
    cell(object->cell()),
    baseTile(object->cell().tile),
    framesCount(framesCount),
    ms(ms)
{
    moveToThread(QApplication::instance()->thread());
}

MapDoor::~MapDoor()
{
    {
        int index=0;
        while(index<events.size())
        {
            const EventCall &eventCall=events.at(index);
            eventCall.timer->stop();
            delete eventCall.timer;
        }
    }
}

void MapDoor::startOpen(const uint16_t &timeRemainOpen)
{
    Q_UNUSED(timeRemainOpen);
    EventCall eventCall;
    eventCall.timer=new QTimer(this);
    eventCall.timer->start(ms);
    eventCall.frame=0;
    connect(eventCall.timer,&QTimer::timeout,this,&MapDoor::timerFinish,Qt::QueuedConnection);
    events << eventCall;
}

void MapDoor::startClose()
{
    EventCall eventCall;
    eventCall.timer=new QTimer(this);
    eventCall.timer->start(ms);
    eventCall.frame=framesCount;
    connect(eventCall.timer,&QTimer::timeout,this,&MapDoor::timerFinish,Qt::QueuedConnection);
    events << eventCall;
}

uint16_t MapDoor::timeToOpen()
{
    return ms*framesCount;
}

void MapDoor::timerFinish()
{
    QTimer *timer         = qobject_cast<QTimer *>(sender());
    uint8_t highterFrame=0;
    int index=0;
    while(index<events.size())
    {
        const EventCall &eventCall=events.at(index);
        if(eventCall.timer==timer)
            events[index].frame++;
        if(eventCall.frame>=(framesCount*2))
        {
            eventCall.timer->stop();
            delete eventCall.timer;
            events.removeAt(index);
            index--;
        }
        else
        {
            if(eventCall.frame<framesCount)
            {
                if(highterFrame<eventCall.frame)
                    highterFrame=eventCall.frame;
            }
            else if(eventCall.frame>=(framesCount*2))
            {
            }
            else
            {
                const uint8_t &reverseFrameCount=framesCount*2-eventCall.frame-1;
                if(highterFrame<reverseFrameCount)
                    highterFrame=reverseFrameCount;
            }
        }
        index++;
    }
    cell.tile=baseTile->tileset()->tileAt(baseTile->id()+highterFrame);
    object->setCell(cell);
}
