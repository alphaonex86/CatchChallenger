#ifdef SERVERBENCHMARKFULL

#include "TimerDisplayEventBySeconds.h"
#include "../Epoll.h"

#include <iostream>
#include <string.h>
#include <std::string>

TimerDisplayEventBySeconds::TimerDisplayEventBySeconds() :
    serverCount(0),
    clientCount(0),
    dbCount(0),
    timerCount(0),
    otherCount(0)
{
}

void TimerDisplayEventBySeconds::exec()
{
    if(timerCount<=1)
        timerCount=0;
    if(serverCount==0 && clientCount==0 && dbCount==0 && /*timerCount==0 &&*/ otherCount==0)
    {
        timerCount=0;
        return;
    }
    char tempString[4096];
    strcpy(tempString,"event by minutes:");
    if(serverCount>0)
    {
        strcat(tempString," server:");
        strcat(tempString,std::string::number(serverCount).toLatin1().constData());
    }
    if(clientCount>0)
    {
        strcat(tempString," client:");
        strcat(tempString,std::string::number(clientCount).toLatin1().constData());
    }
    if(dbCount>0)
    {
        strcat(tempString," db:");
        strcat(tempString,std::string::number(dbCount).toLatin1().constData());
    }
    if(timerCount>0)
    {
        strcat(tempString," timer:");
        strcat(tempString,std::string::number(timerCount).toLatin1().constData());
    }
    if(otherCount>0)
    {
        strcat(tempString," server:");
        strcat(tempString,std::string::number(serverCount).toLatin1().constData());
    }
    std::cout << tempString << std::endl;
    serverCount=0;
    clientCount=0;
    dbCount=0;
    timerCount=0;
    otherCount=0;
}

void TimerDisplayEventBySeconds::addServerCount()
{
    if(serverCount<=2*1000*1000*1000)
        serverCount++;
}

void TimerDisplayEventBySeconds::addClientCount()
{
    if(clientCount<=2*1000*1000*1000)
        clientCount++;
}

void TimerDisplayEventBySeconds::addDbCount()
{
    if(dbCount<=2*1000*1000*1000)
        dbCount++;
}

void TimerDisplayEventBySeconds::addTimerCount()
{
    if(timerCount<=2*1000*1000*1000)
        timerCount++;
}

void TimerDisplayEventBySeconds::addOtherCount()
{
    if(otherCount<=2*1000*1000*1000)
        otherCount++;
}

#endif
