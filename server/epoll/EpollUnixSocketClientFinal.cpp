#include "EpollUnixSocketClientFinal.h"

#include <iostream>

using namespace CatchChallenger;

#ifdef SERVERBENCHMARK
std::chrono::time_point<std::chrono::high_resolution_clock> EpollUnixSocketClientFinal::start;
unsigned long long EpollUnixSocketClientFinal::timeUsed=0;
#ifdef SERVERBENCHMARKFULL
unsigned long long EpollUnixSocketClientFinal::timeUsedForTimer=0;
unsigned long long EpollUnixSocketClientFinal::timeUsedForUser=0;
unsigned long long EpollUnixSocketClientFinal::timeUsedForDatabase=0;
#endif
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
            #ifdef SERVERBENCHMARKFULL
                {
                    std::chrono::duration<unsigned long long int,std::nano> elapsed_seconds = std::chrono::high_resolution_clock::now()-start;
                    EpollUnixSocketClientFinal::timeUsed+=elapsed_seconds.count();
                    EpollUnixSocketClientFinal::start=std::chrono::high_resolution_clock::now();
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
                #ifdef SERVERBENCHMARK
                {
                    std::chrono::duration<unsigned long long int,std::nano> elapsed_seconds = std::chrono::high_resolution_clock::now()-start;
                    EpollUnixSocketClientFinal::timeUsed+=elapsed_seconds.count();
                    EpollUnixSocketClientFinal::start=std::chrono::high_resolution_clock::now();
                }
                unsigned long long tempSend[4];
                tempSend[0]=EpollUnixSocketClientFinal::timeUsed;
                tempSend[1]=0;
                tempSend[2]=0;
                tempSend[3]=0;
                EpollUnixSocketClient::write(reinterpret_cast<char *>(&tempSend),sizeof(tempSend));
                EpollUnixSocketClientFinal::timeUsed=0;
                #else
                static const unsigned long long timeUsed=0;
                EpollUnixSocketClient::write(reinterpret_cast<const char *>(&timeUsed),sizeof(timeUsed)*4);
                #endif
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
