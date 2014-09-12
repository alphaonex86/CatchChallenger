#include "EpollUnixSocketClientFinal.h"

#include <iostream>

using namespace CatchChallenger;

#ifdef SERVERBENCHMARK
timespec EpollUnixSocketClientFinal::ts;
unsigned long long EpollUnixSocketClientFinal::timeUsed=0;
unsigned long long EpollUnixSocketClientFinal::timeUsedForTimer=0;
unsigned long long EpollUnixSocketClientFinal::timeUsedForUser=0;
unsigned long long EpollUnixSocketClientFinal::timeUsedForDatabase=0;
#endif

EpollUnixSocketClientFinal::EpollUnixSocketClientFinal(const int &infd) :
    EpollUnixSocketClient(infd)
{
}

EpollUnixSocketClientFinal::~EpollUnixSocketClientFinal()
{
}

void EpollUnixSocketClientFinal::parseIncommingData()
{
    char buffer[4096];
    int size=EpollUnixSocketClient::read(buffer,sizeof(buffer));
    if(size<=0)
        return;
    int cursor=0;

    while(cursor<size)
    {
        if(buffer[0x0000]==0x01)
        {
            #ifdef SERVERBENCHMARK
            {
                timespec ts;
                ::clock_gettime(CLOCK_REALTIME, &ts);
                EpollUnixSocketClientFinal::timeUsed+=ts.tv_nsec-EpollUnixSocketClientFinal::ts.tv_nsec;
                EpollUnixSocketClientFinal::timeUsed+=(unsigned long long)ts.tv_sec*1000000-EpollUnixSocketClientFinal::ts.tv_sec*1000000;
                EpollUnixSocketClientFinal::ts=ts;
            }
            unsigned long long tempSend[4];
            tempSend[0]=EpollUnixSocketClientFinal::timeUsed;
            tempSend[1]=EpollUnixSocketClientFinal::timeUsedForUser;
            tempSend[2]=EpollUnixSocketClientFinal::timeUsedForTimer;
            tempSend[3]=EpollUnixSocketClientFinal::timeUsedForDatabase;
            EpollUnixSocketClient::write(reinterpret_cast<char *>(&tempSend),sizeof(tempSend));
            EpollUnixSocketClientFinal::timeUsed=0;
            EpollUnixSocketClientFinal::timeUsedForUser=0;
            EpollUnixSocketClientFinal::timeUsedForTimer=0;
            EpollUnixSocketClientFinal::timeUsedForDatabase=0;
            #else
            static const unsigned long long timeUsed=0;
            EpollUnixSocketClient::write(reinterpret_cast<const char *>(&timeUsed),sizeof(timeUsed)*4);
            #endif
            cursor+=1;
        }
        else
        {
            std::cerr << "Unix socket: unknown main code" << std::endl;
            return;
        }
    }
}
