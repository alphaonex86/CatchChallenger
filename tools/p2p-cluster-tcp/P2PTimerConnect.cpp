#include "P2PTimerConnect.h"
#include "P2PClient.h"
#include <iostream>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "../../server/epoll/Epoll.h"

using namespace CatchChallenger;

P2PTimerConnect::P2PTimerConnect()
{
    setInterval(1000*5);
}

void P2PTimerConnect::exec()
{
    if(P2PClient::hostToConnect.empty())
        return;
    const std::pair<std::string,uint16_t> &peerToConnect=P2PClient::hostToConnect.back();
    const std::string host=peerToConnect.first;
    const char * const hostC=host.c_str();
    const uint16_t portBigEndian=htons(peerToConnect.second);
    P2PClient::hostToConnect.pop_back();

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = portBigEndian;
    int infd=-1;
    const int convertResult=inet_pton(AF_INET6,hostC,&serv_addr.sin_addr);
    if(convertResult!=1)
    {
        const int convertResult=inet_pton(AF_INET,hostC,&serv_addr.sin_addr);
        if(convertResult!=1)
        {
            std::cerr << "not IPv4 and IPv6 address (abort), errno: " << std::to_string(errno) << std::endl;
            return;
        }
        else
        {
            infd=socket(AF_INET, SOCK_STREAM, 0);
            if(infd<0)
            {
                std::cerr << "ERROR opening socket to master server (abort)" << std::endl;
                return;
            }
        }
    }
    else
    {
        infd=socket(AF_INET6, SOCK_STREAM, 0);
        if(infd<0)
        {
            std::cerr << "ERROR opening socket to master server (abort)" << std::endl;
            return;
        }
    }

    int flags = fcntl(infd, F_GETFL, 0);
    if(flags == -1)
    {
        std::cerr << "fcntl get flags error" << std::endl;
        return;
    }
    flags |= O_NONBLOCK;
    {
        const int s = fcntl(infd, F_SETFL, flags);
        if(s == -1)
        {
            std::cerr << "fcntl set flags error" << std::endl;
            return;
        }
    }

    P2PClient *client=new P2PClient(infd);

    epoll_event event;
    event.data.ptr = client;
    event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLOUT | EPOLLHUP;
    {
        const int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
        if(s == -1)
        {
            ::close(infd);
            delete client;
            std::cerr << "epoll_ctl error" << std::endl;
            return;
        }
    }

    const int c=::connect(infd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(c == -1 && errno != EINPROGRESS)
    {
        std::cerr << "connect() failed, errno: " << std::to_string(errno) << std::endl;
        ::close(infd);
        delete client;
        return;
    }

    //std::cout << "P2PTimerConnect::exec()" << std::endl;
}
