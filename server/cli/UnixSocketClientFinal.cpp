#ifndef _WIN32
#include "UnixSocketClientFinal.h"

#include <iostream>

using namespace CatchChallenger;


UnixSocketClientFinal::UnixSocketClientFinal(const int &infd) :
    UnixSocketClient(infd)
{
}

UnixSocketClientFinal::~UnixSocketClientFinal()
{
}

void UnixSocketClientFinal::parseIncommingData()
{
    char buffer[4096];
    int size=UnixSocketClient::read(buffer,sizeof(buffer));
    if(size<=0)
        return;
    int cursor=0;

    while(cursor<size)
    {
        if(buffer[cursor]==0x01)
        {
                static const unsigned long long timeUsed=0;
                UnixSocketClient::write(reinterpret_cast<const char *>(&timeUsed),sizeof(timeUsed));
            cursor+=1;
        }
        else
        {
            std::cerr << "EventLoop socket: unknown main code" << std::endl;
            return;
        }
    }
}
#endif // !_WIN32
