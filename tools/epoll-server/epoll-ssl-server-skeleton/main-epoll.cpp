#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include "EpollSocket.h"
#include "EpollSslClient.h"
#include "EpollSslServer.h"
#include "EpollClient.h"
#include "EpollServer.h"
#include "Epoll.h"
#include "EpollTimer.h"
#include "TimerDisplayEventBySeconds.h"

#define MAXEVENTS 512
#define MAXCLIENT 6000

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        std::cerr << "Usage:" << argv[0] << "[port]" << std::endl;
        exit(EXIT_FAILURE);
    }
    if(!Epoll::epoll.init())
        abort();

    #ifndef SERVERNOSSL
    EpollSslServer server;
    #else
    EpollServer server;
    #endif
    if(!server.tryListen(argv[1]))
        abort();
    /*TimerDisplayEventBySeconds timerDisplayEventBySeconds;
    if(!timerDisplayEventBySeconds.init())
        abort();*/

    #ifndef SERVERNOBUFFER
    #ifndef SERVERNOSSL
    EpollSslClient::staticInit();
    #endif
    #endif
    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

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
                case BaseClassSwitch::Type::Server:
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
                        const int &infd = server.accept(&in_addr, &in_len);

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
                        if(numberOfConnectedClient>=MAXCLIENT)
                        {
                            ::close(infd);
                            break;
                        }

                        //just for informations
                        {
                            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                            const int &s = getnameinfo(&in_addr, in_len,
                            hbuf, sizeof hbuf,
                            sbuf, sizeof sbuf,
                            NI_NUMERICHOST | NI_NUMERICSERV);
                            if(s == 0)
                                std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << ")" << std::endl;
                        }

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        #ifndef SERVERNOSSL
                        EpollSslClient *client=new EpollSslClient(infd,server.getCtx());
                        #else
                        EpollClient *client=new EpollClient(infd);
                        #endif
                        std::cout << "new pointer " << client << std::endl;
                        numberOfConnectedClient++;
                        int s = EpollSocket::make_non_blocking(infd);
                        if(s == -1)
                            abort();
                        epoll_event event;
                        event.data.ptr = client;
                        #ifndef SERVERNOBUFFER
                        event.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLHUP | EPOLLET | EPOLLRDHUP;
                        #else
                        event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLET | EPOLLRDHUP;
                        #endif
                        s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                        if(s == -1)
                        {
                            std::cerr << "epoll_ctl on socket error" << std::endl;
                            abort();
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::Type::Client:
                {
                    //timerDisplayEventBySeconds.addCount();
                    #ifndef SERVERNOSSL
                    EpollSslClient *client=static_cast<EpollSslClient *>(events[i].data.ptr);
                    #else
                    EpollClient *client=static_cast<EpollClient *>(events[i].data.ptr);
                    #endif
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "client epoll error: " << events[i].events << std::endl;
                        std::cout << "delete pointer " << client << std::endl;
                        delete client;
                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                    {
                        std::cout << "use pointer " << client << std::endl;
                        /* We have data on the fd waiting to be read. Read and
                        display it. We must read whatever data is available
                        completely, as we are running in edge-triggered mode
                        and won't get a notification again for the same
                        data. */
                        const ssize_t &count = client->read(buf,sizeof(buf));
                        //bug or close, or buffer full
                        if(count<0)
                        {
                            std::cout << "delete pointer " << client << std::endl;
                            delete client;
                            numberOfConnectedClient--;
                        }
                        else
                        {
                            if(client->write(buf,count)!=count)
                            {
                                //buffer full, we disconnect this client
                                std::cout << "delete pointer " << client << std::endl;
                                delete client;
                                numberOfConnectedClient--;
                            }
                        }
                    }
                    #ifndef SERVERNOBUFFER
                    //ready to write
                    if(events[i].events & EPOLLOUT)
                        if(!closed)
                            client->flush();
                    #endif
                    if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP)
                    {
                        // Crash at 51th: /usr/bin/php -f loginserver-json-generator.php 127.0.0.1 39034
                        numberOfConnectedClient--;
                        std::cout << "delete pointer " << client << std::endl;
                        delete client;
                        //disconnected, remove the object
/*
                        std::pair<void *,BaseClassSwitch::EpollObjectType> tempElementsToDelete;
                        tempElementsToDelete.first=events[i].data.ptr;
                        tempElementsToDelete.second=static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType();
                        elementsToDelete.back().push_back(tempElementsToDelete);
                        std::cerr << "elementsToDelete.back().push_back(: " << tempElementsToDelete.first << ") pointer: " << events[i].data.ptr << ", file: " << __FILE__ << ":" << __LINE__ << std::endl;*/
                    }
                }
                break;
                case BaseClassSwitch::Type::Timer:
                    static_cast<EpollTimer *>(events[i].data.ptr)->exec();
                break;
                default:
                    std::cerr << "unknown event" << std::endl;
                break;
            }
        }
    }
    server.close();
    return EXIT_SUCCESS;
}
