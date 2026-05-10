#include "EpollUnixSocketClientFinal.h"

#include <iostream>

using namespace CatchChallenger;


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
                static const unsigned long long timeUsed=0;
                EpollUnixSocketClient::write(reinterpret_cast<const char *>(&timeUsed),sizeof(timeUsed)*4);
            cursor+=1;
        }
        else
        {
            std::cerr << "Unix socket: unknown main code" << std::endl;
            return;
        }
    }
}
