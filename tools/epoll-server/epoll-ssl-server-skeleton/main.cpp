#include <sys/epoll.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <unistd.h>

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
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
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
    TimerDisplayEventBySeconds timerDisplayEventBySeconds;
    if(!timerDisplayEventBySeconds.init())
        abort();

    #ifndef SERVERNOBUFFER
    #ifndef SERVERNOSSL
    EpollSslClient::staticInit();
    #endif
    #endif
    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    bool closed;
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
                        fprintf(stderr, "server epoll error\n");
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
                                perror("accept");
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
                                printf("Accepted connection on descriptor %d (host=%s, port=%s)\n", infd, hbuf, sbuf);
                        }

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        #ifndef SERVERNOSSL
                        EpollSslClient *client=new EpollSslClient(infd,server.getCtx());
                        #else
                        EpollClient *client=new EpollClient(infd);
                        #endif
                        numberOfConnectedClient++;
                        int s = EpollSocket::make_non_blocking(infd);
                        if(s == -1)
                            abort();
                        epoll_event event;
                        event.data.ptr = client;
                        #ifndef SERVERNOBUFFER
                        event.events = EPOLLIN | EPOLLET | EPOLLOUT;
                        #else
                        event.events = EPOLLIN | EPOLLET;
                        #endif
                        s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                        if(s == -1)
                        {
                            perror("epoll_ctl");
                            abort();
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::Type::Client:
                {
                    closed=true;
                    timerDisplayEventBySeconds.addCount();
                    #ifndef SERVERNOSSL
                    EpollSslClient *client=static_cast<EpollSslClient *>(events[i].data.ptr);
                    #else
                    EpollClient *client=static_cast<EpollClient *>(events[i].data.ptr);
                    #endif
                    if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        fprintf(stderr, "client epoll error\n");
                        delete client;
                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                    {
                        /* We have data on the fd waiting to be read. Read and
                        display it. We must read whatever data is available
                        completely, as we are running in edge-triggered mode
                        and won't get a notification again for the same
                        data. */
                        const ssize_t &count = client->read(buf,sizeof(buf));
                        //bug or close, or buffer full
                        if(count<0)
                        {
                            delete client;
                            numberOfConnectedClient--;
                        }
                        else
                        {
                            if(client->write(buf,count)!=count)
                            {
                                //buffer full, we disconnect this client
                                delete client;
                                numberOfConnectedClient--;
                            }
                            else
                                closed=false;
                        }
                    }
                    #ifndef SERVERNOBUFFER
                    //ready to write
                    if(events[i].events & EPOLLOUT)
                        if(!closed)
                            client->flush();
                    #else
                    #ifndef SERVERNOSSL
                    if(!closed)
                    {
                        if(client->doRealWrite())
                        {
                            //buffer full, we disconnect this client
                            delete client;
                            numberOfConnectedClient--;
                            break;
                        }
                    }
                    #endif
                    #endif
                }
                break;
                case BaseClassSwitch::Type::Timer:
                    static_cast<EpollTimer *>(events[i].data.ptr)->exec();
                break;
                default:
                    fprintf(stderr, "unknown event\n");
                break;
            }
        }
    }
    server.close();
    return EXIT_SUCCESS;
}
