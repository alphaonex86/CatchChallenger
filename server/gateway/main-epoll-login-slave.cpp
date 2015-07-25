#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <vector>
#include <QCoreApplication>

#include "../epoll/EpollSocket.h"
#include "../epoll/Epoll.h"
#include "EpollServerLoginSlave.h"
#include "EpollClientLoginSlave.h"
#include "TimerDdos.h"

#define MAXEVENTS 512
#define MAXCLIENTSINSUSPEND 16

using namespace CatchChallenger;

//list of char connected

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Q_UNUSED(a);

    (void)argc;
    (void)argv;

    ProtocolParsing::initialiseTheVariable(ProtocolParsing::InitialiseTheVariableType::LoginServer);
    if(!Epoll::epoll.init())
        return EPOLLERR;

    //blocking due to db connexion
    EpollServerLoginSlave::epollServerLoginSlave=new EpollServerLoginSlave();

    #ifndef SERVERNOBUFFER
    #ifdef SERVERSSL
    EpollSslClient::staticInit();
    #endif
    #endif
    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    char encodingBuff[1];
    #ifdef SERVERSSL
    encodingBuff[0]=0x01;
    #else
    encodingBuff[0]=0x00;
    #endif

    TimerDdos timerDdos;
    {
        if(!timerDdos.start(CATCHCHALLENGER_DDOS_COMPUTERINTERVAL*1000))
        {
            std::cerr << "timerDdos fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }

    int numberOfConnectedClient=0;
    /* The event loop */
    int number_of_events, i;
    while(1)
    {
        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
        for(i = 0; i < number_of_events; i++)
        {
            switch(static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType())
            {
                case BaseClassSwitch::EpollObjectType::Server:
                {
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
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
                        const int &infd = EpollServerLoginSlave::epollServerLoginSlave->accept(&in_addr, &in_len);
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

                        int s = EpollSocket::make_non_blocking(infd);
                        if(s == -1)
                            std::cerr << "unable to make to socket non blocking" << std::endl;
                        else
                        {
                            if(EpollServerLoginSlave::epollServerLoginSlave->tcpCork)
                            {
                                //set cork for CatchChallener because don't have real time part
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp cork" << std::endl;
                            }
                            else if(EpollServerLoginSlave::epollServerLoginSlave->tcpNodelay)
                            {
                                //set no delay to don't try group the packet and improve the performance
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp no delay" << std::endl;
                            }

                            EpollClientLoginSlave *client=new EpollClientLoginSlave(infd
                                               #ifdef SERVERSSL
                                               ,EpollServerLoginSlave::epollServerLoginSlave->getCtx()
                                               #endif
                                );
                            //just for informations
                            {
                                char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                                const int &s = getnameinfo(&in_addr, in_len,
                                hbuf, sizeof hbuf,
                                sbuf, sizeof sbuf,
                                NI_NUMERICHOST | NI_NUMERICSERV);
                                if(s == 0)
                                {
                                    //std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << ")" << std::endl;
                                    client->socketStringSize=strlen(hbuf)+strlen(sbuf)+1+1;
                                    client->socketString=new char[client->socketStringSize];
                                    strcpy(client->socketString,hbuf);
                                    strcat(client->socketString,":");
                                    strcat(client->socketString,sbuf);
                                    client->socketString[client->socketStringSize-1]='\0';
                                }
                            }
                            epoll_event event;
                            event.data.ptr = client;
                            event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
                            s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                            if(s == -1)
                            {
                                std::cerr << "epoll_ctl on socket error" << std::endl;
                                delete client;
                            }
                            else
                                ::write(infd,encodingBuff,sizeof(encodingBuff));
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::EpollObjectType::Client:
                {
                    EpollClientLoginSlave * const client=static_cast<EpollClientLoginSlave *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "client epoll error: " << events[i].events << std::endl;
                        numberOfConnectedClient--;
                        delete client;
                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                        client->parseIncommingData();
                    if(events[i].events & EPOLLHUP || events[i].events & EPOLLRDHUP)
                    {
                        numberOfConnectedClient--;
                        delete client;//disconnected, remove the object
                    }
                }
                break;
                case BaseClassSwitch::EpollObjectType::Timer:
                {
                    static_cast<EpollTimer *>(events[i].data.ptr)->exec();
                    static_cast<EpollTimer *>(events[i].data.ptr)->validateTheTimer();
                }
                break;
                default:
                    std::cerr << "unknown event: " << static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType() << std::endl;
                break;
            }
        }
    }
    EpollServerLoginSlave::epollServerLoginSlave->close();
    delete EpollServerLoginSlave::epollServerLoginSlave;
    EpollServerLoginSlave::epollServerLoginSlave=NULL;
    return EXIT_SUCCESS;
}
