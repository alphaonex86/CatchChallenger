#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <vector>
#include <signal.h>

#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/Version.hpp"
#include "../cli/SocketUtil.hpp"
#include "../cli/EventLoop.hpp"
#include "EventLoopServerLoginSlave.hpp"
#include "EventLoopClientLoginSlave.hpp"
#include "LinkToMaster.hpp"
#include "LinkToGameServer.hpp"
#include "TimerDdos.hpp"
#include "TimerDetectTimeout.hpp"

#define MAXEVENTS 512
#define MAXCLIENTSINSUSPEND 16

using namespace CatchChallenger;

/* Catch Signal Handler functio */
void signal_callback_handler(int signum){

        printf("Caught signal SIGPIPE %d\n",signum);
}

int main(int argc, char *argv[])
{
    /* Catch Signal Handler SIGPIPE */
    if(signal(SIGPIPE, signal_callback_handler)==SIG_ERR)
    {
        std::cerr << "signal(SIGPIPE, signal_callback_handler)==SIG_ERR, errno: " << std::to_string(errno) << std::endl;
        abort();
    }

    std::cout << "CatchChallenger version: " << CatchChallenger::Version::str << std::endl;
    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];

    ProtocolParsing::initialiseTheVariable(ProtocolParsing::InitialiseTheVariableType::LoginServer);
    if(!EventLoop::loop.init())
        return EPOLLERR;

    //blocking due to db connexion
    EventLoopServerLoginSlave::unixServerLoginSlave=new EventLoopServerLoginSlave();
    #ifdef CATCHCHALLENGER_CACHE_HPS
    const bool save=argc==2 && strcmp(argv[1],"save")==0;
    if(save)
        EventLoopServerLoginSlave::unixServerLoginSlave->setSave(datapackCache);
    else
        EventLoopServerLoginSlave::unixServerLoginSlave->setLoad(datapackCache);
    #endif

    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    // SSL preamble byte removed; no per-connect sentinel byte is emitted.

    TimerDdos timerDdos;
    {
        if(!timerDdos.start(CATCHCHALLENGER_DDOS_COMPUTERINTERVAL*1000))
        {
            std::cerr << "timerDdos fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    TimerDetectTimeout timerDetectTimeout;
    {
        if(!timerDetectTimeout.start(60*1000))
        {
            std::cerr << "timerDetectTimeout fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }

    int numberOfConnectedClient=0;
    /* The event loop */

    int number_of_events, i;
    while(1)
    {
        number_of_events = EventLoop::loop.wait(events, MAXEVENTS);

        //delete part
        if(EventLoopClientLoginSlave::clientToDeleteSize>0 && number_of_events<MAXEVENTS)
        {
            if(EventLoopClientLoginSlave::clientToDeleteIndex>=15)
                EventLoopClientLoginSlave::clientToDeleteIndex=0;
            else
                ++EventLoopClientLoginSlave::clientToDeleteIndex;
            const std::vector<void *> &clientToDeleteSub=EventLoopClientLoginSlave::clientToDelete[EventLoopClientLoginSlave::clientToDeleteIndex];
            if(!clientToDeleteSub.empty())
            {
                unsigned int index=0;
                while(index<clientToDeleteSub.size())
                {
                    EventLoopClientLoginSlave *client=reinterpret_cast<EventLoopClientLoginSlave *>(clientToDeleteSub.at(index));
                    delete client;
                    EventLoopClientLoginSlave::detectDuplicateClientToDelete.erase(client);
                    index++;
                }
            }
            EventLoopClientLoginSlave::clientToDeleteSize-=clientToDeleteSub.size();
            EventLoopClientLoginSlave::clientToDelete[EventLoopClientLoginSlave::clientToDeleteIndex].clear();
        }
        if(LinkToGameServer::gameLinkToDeleteSize>0 && number_of_events<MAXEVENTS)
        {
            if(LinkToGameServer::gameLinkToDeleteIndex>=15)
                LinkToGameServer::gameLinkToDeleteIndex=0;
            else
                ++LinkToGameServer::gameLinkToDeleteIndex;
            const std::vector<void *> &gameLinkToDeleteSub=LinkToGameServer::gameLinkToDelete[LinkToGameServer::gameLinkToDeleteIndex];
            if(!gameLinkToDeleteSub.empty())
            {
                unsigned int index=0;
                while(index<gameLinkToDeleteSub.size())
                {
                    LinkToGameServer *client=reinterpret_cast<LinkToGameServer *>(gameLinkToDeleteSub.at(index));
                    delete client;
                    LinkToGameServer::detectDuplicateGameLinkToDelete.erase(client);
                    index++;
                }
            }
            LinkToGameServer::gameLinkToDeleteSize-=gameLinkToDeleteSub.size();
            LinkToGameServer::gameLinkToDelete[LinkToGameServer::gameLinkToDeleteIndex].clear();
        }

        for(i = 0; i < number_of_events; i++)
        {
            switch(static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType())
            {
                case BaseClassSwitch::EventLoopObjectType::Server:
                {
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occurred on this fd, or the socket is not
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
                        const int &infd = EventLoopServerLoginSlave::unixServerLoginSlave->accept(&in_addr, &in_len);
                        if(EventLoopClientLoginSlave::clientToDeleteSize>64 || BaseServerLogin::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION)
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

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        numberOfConnectedClient++;

                        int s = SocketUtil::make_non_blocking(infd);
                        if(s == -1)
                            std::cerr << "unable to make to socket non blocking" << std::endl;
                        else
                        {
                            if(EventLoopServerLoginSlave::unixServerLoginSlave->tcpCork)
                            {
                                //set cork for CatchChallener because don't have real time part
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp cork" << std::endl;
                            }
                            else if(EventLoopServerLoginSlave::unixServerLoginSlave->tcpNodelay)
                            {
                                //set no delay to don't try group the packet and improve the performance
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp no delay" << std::endl;
                            }

                            EventLoopClientLoginSlave *client=new EventLoopClientLoginSlave(infd
                                );
                            #ifdef CATCHCHALLENGER_HARDENED
                            if(static_cast<BaseClassSwitch *>(client)->getType()!=BaseClassSwitch::EventLoopObjectType::Client)
                            {
                                std::cerr << "Wrong post check type (abort)" << std::endl;
                                abort();
                            }
                            #endif
                            //just for informations
                            {
                                char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                                const int &s = getnameinfo(&in_addr, in_len,
                                hbuf, sizeof hbuf,
                                sbuf, sizeof sbuf,
                                NI_NUMERICHOST | NI_NUMERICSERV);
                                if(s == 0)
                                {
                                    //std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << "), client: " << client << std::endl;
                                    client->socketStringSize=strlen(hbuf)+strlen(sbuf)+1+1;
                                    client->socketString=new char[client->socketStringSize];
                                    strcpy(client->socketString,hbuf);
                                    strcat(client->socketString,":");
                                    strcat(client->socketString,sbuf);
                                    client->socketString[client->socketStringSize-1]='\0';
                                }
                            }
                            #ifdef CATCHCHALLENGER_IO_URING
                            if(EventLoop::loop.multishotEnabled())
                            {
                                if(!EventLoop::loop.armRecvMultishot(infd,
                                        static_cast<BaseClassSwitch *>(client)))
                                {
                                    std::cerr << "armRecvMultishot failed for fd " << infd << std::endl;
                                    delete client;
                                }
                            }
                            else
                            #endif
                            {
                                epoll_event event;
                                memset(&event,0,sizeof(event));
                                event.data.ptr = client;
                                event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLOUT;
                                s = EventLoop::loop.ctl(EPOLL_CTL_ADD, infd, &event);
                                if(s == -1)
                                {
                                    std::cerr << "epoll_ctl on socket error" << std::endl;
                                    delete client;
                                }
                            }
                            // SSL preamble byte removed; nothing to write here.
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::Client:
                {
                    EventLoopClientLoginSlave * const client=static_cast<EventLoopClientLoginSlave *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        std::cerr << "BaseClassSwitch::EventLoopObjectType::Client events[i].events : " << client << " (" << events[i].events  << ")" << std::endl;
                        /* An error has occurred on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "client epoll error: " << events[i].events << std::endl;
                        numberOfConnectedClient--;

                        client->disconnectClient();

                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                        client->parseIncommingData();
                    if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP || client->socketIsClosed())
                    {
                        numberOfConnectedClient--;
                        //disconnected, remove the object

                        client->disconnectClient();
                    }
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::Timer:
                {
                    static_cast<EventLoopTimer *>(events[i].data.ptr)->exec();
                    static_cast<EventLoopTimer *>(events[i].data.ptr)->validateTheTimer();
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::Database:
                {
                    EventLoopPostgresql * const db=static_cast<EventLoopPostgresql *>(events[i].data.ptr);
                    db->unixEvent(events[i].events);
                    if(!db->isConnected())
                    {
                        std::cerr << "database disconnect, quit now" << std::endl;
                        return EXIT_FAILURE;
                    }
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::MasterLink:
                {
                    LinkToMaster * const client=static_cast<LinkToMaster *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occurred on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "master epoll error: " << events[i].events << std::endl;
                        #ifdef CATCHCHALLENGER_HARDENED
                        std::cerr << "master link epoll bye: " << events[i].events << std::endl;
                        #endif
                        client->tryReconnect();
                        continue;
                    }
                    //ready to read
                    #ifdef PROTOCOLPARSINGINPUTOUTPUTDEBUG
                    std::cerr << __FILE__ << ":" << __LINE__ << " this bug: " << client << std::endl;
                    #endif
                    #ifdef CATCHCHALLENGER_HARDENED
                    std::cerr << "master parseIncommingData start: " << events[i].events << std::endl;
                    #endif
                    client->parseIncommingData();
                    #ifdef CATCHCHALLENGER_HARDENED
                    std::cerr << "master parseIncommingData stop: " << events[i].events << std::endl;
                    #endif
                    if(events[i].events & EPOLLRDHUP)
                    {
                        #ifdef CATCHCHALLENGER_HARDENED
                        std::cerr << "master link epoll EPOLLRDHUP: " << events[i].events << std::endl;
                        #endif
                        client->tryReconnect();
                    }
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::GameLink:
                {
                    LinkToGameServer * const client=static_cast<LinkToGameServer *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occurred on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "client epoll error: " << events[i].events << std::endl;
                        numberOfConnectedClient--;

                        client->disconnectClient();
                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                        client->parseIncommingData();
                    if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP || !client->isValid())
                    {
                        numberOfConnectedClient--;
                        //disconnected, remove the object

                        if(client!=nullptr)
                            client->disconnectClient();
                    }
                }
                break;
                default:
                    std::cerr << "unknown event: " << static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType() << std::endl;
                break;
            }
        }
    }
    EventLoopServerLoginSlave::unixServerLoginSlave->close();
    delete EventLoopServerLoginSlave::unixServerLoginSlave;
    EventLoopServerLoginSlave::unixServerLoginSlave=NULL;
    return EXIT_SUCCESS;
}
