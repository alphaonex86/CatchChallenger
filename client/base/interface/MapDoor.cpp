#include "MapDoor.h"

#include <QApplication>

MapDoor::MapDoor(Tiled::MapObject* object, const quint8 &framesCount, const quint16 &ms) :
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

void MapDoor::startOpen()
{
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

void MapDoor::timerFinish()
{
    QTimer *timer         = qobject_cast<QTimer *>(sender());
    quint8 highterFrame=0;
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
                const quint8 &reverseFrameCount=framesCount*2-eventCall.frame-1;
                if(highterFrame<reverseFrameCount)
                    highterFrame=reverseFrameCount;
            }
        }
        index++;
    }
    cell.tile=baseTile->tileset()->tileAt(baseTile->id()+highterFrame);
    object->setCell(cell);
}
