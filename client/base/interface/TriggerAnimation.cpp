#include "TriggerAnimation.h"

#include <QApplication>

TriggerAnimation::TriggerAnimation(Tiled::MapObject* object,
                          const quint8 &framesCountEnter, const quint16 &msEnter,
                          const quint8 &framesCountLeave, const quint16 &msLeave,
                          const quint8 &framesCountAgain, const quint16 &msAgain,
                          bool over
                          ) :
    object(object),
    objectOver(NULL),
    cell(object->cell()),
    baseTile(object->cell().tile),
    baseTileOver(NULL),
    framesCountEnter(framesCountEnter),
    msEnter(msEnter),
    framesCountLeave(framesCountLeave),
    msLeave(msLeave),
    framesCountAgain(framesCountAgain),
    msAgain(msAgain),
    over(over),
    firstTime(true)
{
    moveToThread(QApplication::instance()->thread());
}

TriggerAnimation::TriggerAnimation(Tiled::MapObject* object,
                          const quint8 &framesCountEnter, const quint16 &msEnter,
                          const quint8 &framesCountLeave, const quint16 &msLeave,
                          bool over
                          ) :
    object(object),
    objectOver(NULL),
    cell(object->cell()),
    baseTile(object->cell().tile),
    baseTileOver(NULL),
    framesCountEnter(framesCountEnter),
    msEnter(msEnter),
    framesCountLeave(framesCountLeave),
    msLeave(msLeave),
    framesCountAgain(0),
    msAgain(0),
    over(over),
    firstTime(true)
{
    moveToThread(QApplication::instance()->thread());
}

TriggerAnimation::TriggerAnimation(Tiled::MapObject* object,
                                   Tiled::MapObject* objectOver,
                          const quint8 &framesCountEnter, const quint16 &msEnter,
                          const quint8 &framesCountLeave, const quint16 &msLeave,
                          const quint8 &framesCountAgain, const quint16 &msAgain,
                          bool over
                          ) :
    object(object),
    objectOver(objectOver),
    cell(object->cell()),
    baseTile(object->cell().tile),
    framesCountEnter(framesCountEnter),
    msEnter(msEnter),
    framesCountLeave(framesCountLeave),
    msLeave(msLeave),
    framesCountAgain(framesCountAgain),
    msAgain(msAgain),
    over(over),
    firstTime(true)
{
    moveToThread(QApplication::instance()->thread());
    if(objectOver!=NULL)
    {
        baseTileOver=objectOver->cell().tile;
        cellOver=objectOver->cell();
    }
}

TriggerAnimation::TriggerAnimation(Tiled::MapObject* object,
                                   Tiled::MapObject* objectOver,
                          const quint8 &framesCountEnter, const quint16 &msEnter,
                          const quint8 &framesCountLeave, const quint16 &msLeave,
                          bool over
                          ) :
    object(object),
    objectOver(objectOver),
    cell(object->cell()),
    baseTile(object->cell().tile),
    framesCountEnter(framesCountEnter),
    msEnter(msEnter),
    framesCountLeave(framesCountLeave),
    msLeave(msLeave),
    framesCountAgain(0),
    msAgain(0),
    over(over),
    firstTime(true)
{
    moveToThread(QApplication::instance()->thread());
    if(objectOver!=NULL)
    {
        baseTileOver=objectOver->cell().tile;
        cellOver=objectOver->cell();
    }
}

TriggerAnimation::~TriggerAnimation()
{
}

void TriggerAnimation::startEnter()
{
    if(framesCountAgain==0 || firstTime)
    {
        firstTime=false;
        EnterEventCall eventCall;
        eventCall.timer=new QTimer(this);
        eventCall.timer->start(msEnter);
        eventCall.frame=0;
        connect(eventCall.timer,&QTimer::timeout,this,&TriggerAnimation::timerFinishEnter,Qt::QueuedConnection);
        enterEvents << eventCall;
    }
    else
    {
        firstTime=false;
        EnterEventCall eventCall;
        eventCall.timer=new QTimer(this);
        eventCall.timer->start(msAgain);
        eventCall.frame=framesCountEnter+framesCountLeave+0;
        connect(eventCall.timer,&QTimer::timeout,this,&TriggerAnimation::timerFinishEnter,Qt::QueuedConnection);
        enterEvents << eventCall;
    }
}

void TriggerAnimation::startLeave()
{
    playerOnThisTile--;
    LeaveEventCall eventCall;
    eventCall.timer=new QTimer(this);
    eventCall.timer->start(msLeave);
    eventCall.frame=framesCountEnter+0;
    connect(eventCall.timer,&QTimer::timeout,this,&TriggerAnimation::timerFinishLeave,Qt::QueuedConnection);
    leaveEvents << eventCall;
}

void TriggerAnimation::setTileOffset(const quint8 &offset)
{
    cell.tile=baseTile->tileset()->tileAt(baseTile->id()+offset);
    object->setCell(cell);
    if(objectOver!=NULL)
    {
        cellOver.tile=baseTileOver->tileset()->tileAt(baseTileOver->id()+offset);
        objectOver->setCell(cellOver);
    }
}

void TriggerAnimation::timerFinishEnter()
{
    QTimer *timer         = qobject_cast<QTimer *>(sender());
    quint8 lowerFrame=framesCountEnter+framesCountLeave+framesCountAgain;
    if(playerOnThisTile>0)
        lowerFrame=framesCountEnter;
    else if(enterEvents.isEmpty() && leaveEvents.isEmpty())
    {
        if(framesCountAgain==0)
            setTileOffset(0);
        else
            setTileOffset(framesCountEnter+0);
    }
    else
    {
        int index=0;
        while(index<enterEvents.size())
        {
            const EnterEventCall &eventCall=enterEvents.at(index);
            if(eventCall.timer==timer)
                enterEvents[index].frame++;
            if(eventCall.frame==framesCountEnter || eventCall.frame>=framesCountAgain)
            {
                eventCall.timer->stop();
                delete eventCall.timer;
                enterEvents.removeAt(index);
                index--;
                playerOnThisTile++;
            }
            else
            {
                if(eventCall.frame<lowerFrame)
                    lowerFrame=eventCall.frame;
            }
            index++;
        }
        setTileOffset(lowerFrame);
    }
}

void TriggerAnimation::timerFinishLeave()
{
    QTimer *timer         = qobject_cast<QTimer *>(sender());
    quint8 lowerFrame=framesCountEnter+framesCountLeave+framesCountAgain;
    if(playerOnThisTile>0)
        lowerFrame=framesCountEnter;
    else if(enterEvents.isEmpty() && leaveEvents.isEmpty())
    {
        if(framesCountAgain==0)
            setTileOffset(0);
        else
            setTileOffset(framesCountEnter+0);
    }
    else
    {
        int index=0;
        while(index<leaveEvents.size())
        {
            const LeaveEventCall &eventCall=leaveEvents.at(index);
            if(eventCall.timer==timer)
                leaveEvents[index].frame++;
            if(eventCall.frame>=(framesCountEnter+framesCountLeave))
            {
                eventCall.timer->stop();
                delete eventCall.timer;
                leaveEvents.removeAt(index);
                index--;
            }
            else
            {
                if(eventCall.frame<lowerFrame)
                    lowerFrame=eventCall.frame;
            }
            index++;
        }
        index=0;
        while(index<enterEvents.size())
        {
            const EnterEventCall &eventCall=enterEvents.at(index);
            if(eventCall.frame<lowerFrame)
                lowerFrame=eventCall.frame;
            index++;
        }
        setTileOffset(lowerFrame);
    }
}
