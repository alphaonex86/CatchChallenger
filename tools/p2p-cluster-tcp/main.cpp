#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/cpp11addition.h"
#include "../../server/base/TinyXMLSettings.h"
#include "../../server/epoll/Epoll.h"
#include "../../server/epoll/EpollSocket.h"
#include "../../server/VariableServer.h"
#include "P2PServer.h"
#include "P2PClient.h"
#include "P2PTimerConnect.h"

#define MAXEVENTS 512

using namespace CatchChallenger;

/* Catch Signal Handler functio */
void signal_callback_handler(int signum){
    printf("Caught signal SIGPIPE %d\n",signum);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    /* Catch Signal Handler SIGPIPE */
    if(signal(SIGPIPE, signal_callback_handler)==SIG_ERR)
    {
        std::cerr << "signal(SIGPIPE, signal_callback_handler)==SIG_ERR, errno: " << std::to_string(errno) << std::endl;
        abort();
    }

    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    char replyNotConnected[]="{\"error\":\"Not connected to login server (X)\"}";
    unsigned int replyNotConnectedPos=0;
    {
        const unsigned int size=sizeof(replyNotConnected);
        while(replyNotConnectedPos<size)
        {
            if(replyNotConnected[replyNotConnectedPos]=='X')
                break;
            replyNotConnectedPos++;
        }
        if(replyNotConnectedPos>=sizeof(replyNotConnected))
        {
            std::cerr << "X not found into replyNotConnected" << std::endl;
            abort();
        }
        replyNotConnected[replyNotConnectedPos]=48+(uint8_t)1;
        if(strcmp(replyNotConnected,"{\"error\":\"Not connected to login server (1)\"}")!=0)
        {
            std::cerr << "replyNotConnected have the wrong content" << std::endl;
            abort();
        }
    }

    std::string host;
    uint16_t port;
    {
        TinyXMLSettings settings("p2p.xml");

        if(!settings.contains("known_node"))
            settings.setValue("known_node","");

        if(!settings.contains("host"))
            settings.setValue("host","localhost");
        host=settings.value("host");
        if(!settings.contains("port"))
            settings.setValue("port",rand()%40000+10000);
        port=stringtouint16(settings.value("port"));

        const std::vector<std::string> &known_nodes=stringsplit(settings.value("known_node"),',');
        if((known_nodes.size()%2)!=0)
        {
            std::cerr << "(known_nodes.size()%2)!=0" << std::endl;
            abort();
        }
        size_t index=0;
        while(index<known_nodes.size())
        {
            bool ok=false;
            const std::string &host=known_nodes.at(index);
            const std::string &portstring=known_nodes.at(index+1);
            const uint16_t &port=stringtouint16(portstring,&ok);

            P2PClient::hostToConnect.push_back(std::pair<std::string,uint16_t>(host,port));
            index+=2;
        }

        settings.sync();

        if(!Epoll::epoll.init())
            return EPOLLERR;
    }

    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];
    //P2PClient p2pClient(P2PClient::create_socket(host.c_str(),port));
    P2PServer p2pserver;
    if(!p2pserver.tryListen(host.c_str(),port))
    {
        std::cerr << "can't listen, abort" << std::endl;
        abort();
    }
    P2PTimerConnect p2pTimerConnect;
    p2pTimerConnect.start();

    /* The event loop */
    std::vector<std::pair<void *,BaseClassSwitch::EpollObjectType> > elementsToDelete;
    int number_of_events, i;
    while(1)
    {
        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
        if(!elementsToDelete.empty())
        {
            unsigned int index=0;
            while(index<elementsToDelete.size())
            {
                switch(elementsToDelete.at(index).second)
                {
                    case BaseClassSwitch::EpollObjectType::ClientP2P:
                        //delete static_cast<LinkToLogin *>(elementsToDelete.at(index).first);
                    break;
                    default:
                    break;
                }
                index++;
            }
            elementsToDelete.clear();
        }
        for(i = 0; i < number_of_events; i++)
        {
            const epoll_event &event=events[i];
            const BaseClassSwitch::EpollObjectType &epollObjectType=static_cast<BaseClassSwitch *>(event.data.ptr)->getType();
            switch(epollObjectType)
            {
                case BaseClassSwitch::EpollObjectType::ServerP2P:
                {
                    if((event.events & EPOLLERR) ||
                    (event.events & EPOLLHUP) ||
                    (!(event.events & EPOLLIN) && !(event.events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "server epoll error" << std::endl;
                        continue;
                    }
                    /* We have a notification on the listening socket, which
                    means one or more incoming connections. */
                    while(1)
                    {
                        sockaddr in_addr;
                        socklen_t in_len = sizeof(in_addr);
                        const int &infd = p2pserver.accept(&in_addr, &in_len);
                        std::cout << "p2pserver.accept" << std::endl;
                        if(elementsToDelete.size()>64)
                        {
                            /// \todo dont clean error on client into this case
                            std::cerr << "server overload" << std::endl;
                            ::close(infd);
                            break;
                        }
                        if(infd == -1)
                        {
                            if((errno == EAGAIN) ||
                            (errno == EWOULDBLOCK))
                            {
                                /* We have processed all incoming
                                connections. */
                                break;
                            }
                            else
                            {
                                std::cout << "connexion accepted" << std::endl;
                                break;
                            }
                        }
                        /* do at the protocol negociation to send the reason
                        if(numberOfConnectedClient>=GlobalServerData::serverSettings.max_players)
                        {
                            ::close(infd);
                            break;
                        }*/

                        //non blocking
                        int flags = fcntl(infd, F_GETFL, 0);
                        if(flags == -1)
                        {
                            std::cerr << "fcntl get flags error" << std::endl;
                            ::close(infd);
                        }
                        else
                        {
                            flags |= O_NONBLOCK;
                            int s = fcntl(infd, F_SETFL, flags);
                            if(s == -1)
                            {
                                std::cerr << "fcntl set flags error" << std::endl;
                                ::close(infd);
                            }
                            else
                            {
                                P2PClient *client=new P2PClient(infd);

                                epoll_event event;
                                event.data.ptr = client;
                                event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLOUT | EPOLLHUP;
                                const int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                                if(s == -1)
                                {
                                    std::cerr << "epoll_ctl on socket error" << std::endl;
                                    delete client;
                                    ::close(infd);
                                }
                                else
                                {
                                    char toSend[]="0000Hello!";
                                    const uint32_t dataSize=sizeof(toSend)-sizeof(uint32_t);
                                    memcpy(toSend,&dataSize,sizeof(dataSize));
                                    client->write(toSend,sizeof(toSend));
                                }
                            }
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::EpollObjectType::ClientP2P:
                {
                    P2PClient * const client=static_cast<P2PClient *>(event.data.ptr);

                    /*if(event.events & EPOLLIN)
                        std::cout << "EPOLLIN" << std::endl;
                    if(event.events & EPOLLPRI)
                        std::cout << "EPOLLPRI" << std::endl;
                    if(event.events & EPOLLOUT)
                        std::cout << "EPOLLOUT" << std::endl;
                    if(event.events & EPOLLRDNORM)
                        std::cout << "EPOLLRDNORM" << std::endl;
                    if(event.events & EPOLLRDBAND)
                        std::cout << "EPOLLRDBAND" << std::endl;
                    if(event.events & EPOLLWRNORM)
                        std::cout << "EPOLLWRNORM" << std::endl;
                    if(event.events & EPOLLWRBAND)
                        std::cout << "EPOLLWRBAND" << std::endl;
                    if(event.events & EPOLLMSG)
                        std::cout << "EPOLLMSG" << std::endl;
                    if(event.events & EPOLLERR)
                        std::cout << "EPOLLERR" << std::endl;
                    if(event.events & EPOLLHUP)
                        std::cout << "EPOLLHUP" << std::endl;
                    if(event.events & EPOLLRDHUP)
                        std::cout << "EPOLLRDHUP" << std::endl;
                    if(event.events & EPOLLEXCLUSIVE)
                        std::cout << "EPOLLEXCLUSIVE" << std::endl;
                    if(event.events & EPOLLWAKEUP)
                        std::cout << "EPOLLWAKEUP" << std::endl;
                    if(event.events & EPOLLONESHOT)
                        std::cout << "EPOLLONESHOT" << std::endl;
                    if(event.events & EPOLLET)
                        std::cout << "EPOLLET" << std::endl;*/

                    if((event.events & EPOLLERR) ||
                    (event.events & EPOLLHUP) ||
                    (!(event.events & EPOLLIN) && !(event.events & EPOLLOUT)))
                    {
                        if(!(event.events & EPOLLHUP))
                            std::cerr << "client epoll error: " << event.events << std::endl;

                        int       error = 0;
                        socklen_t errlen = sizeof(error);
                        if (getsockopt(client->getinfd(), SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
                            std::cerr << "error = " << strerror(error) << std::endl;

                        elementsToDelete.push_back(
                            std::pair<void *,BaseClassSwitch::EpollObjectType>(
                                event.data.ptr,
                                static_cast<BaseClassSwitch *>(event.data.ptr)->getType()
                                )
                            );
                        continue;
                    }
                    //ready to read
                    if(event.events & EPOLLIN || event.events & EPOLLOUT)
                        client->parseIncommingData();
                    if(event.events & EPOLLHUP || event.events & EPOLLRDHUP)
                        elementsToDelete.push_back(
                                    std::pair<void *,BaseClassSwitch::EpollObjectType>(
                                        event.data.ptr,
                                        static_cast<BaseClassSwitch *>(event.data.ptr)->getType()
                                        )
                                    );
                }
                break;
                case BaseClassSwitch::EpollObjectType::Timer:
                {
                    P2PTimerConnect * const timer=static_cast<P2PTimerConnect *>(event.data.ptr);
                    timer->exec();
                    timer->validateTheTimer();
                }
                break;
                default:
                    std::cout << "case BaseClassSwitch::EpollObjectType::?" << std::endl;
                    abort();
                break;
            }
        }
    }
    return EXIT_SUCCESS;
}
