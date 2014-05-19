#include <sys/epoll.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <sys/socket.h>
#include <netdb.h>

#include "Client.h"
#include "Socket.h"
#include "Server.h"
#include "Epoll.h"
#include "Timer.h"
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

    Server server;
    if(!server.tryListen(argv[1]))
        abort();
    TimerDisplayEventBySeconds timerDisplayEventBySeconds;
    if(!timerDisplayEventBySeconds.init())
        abort();

    char buf[512];
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    Client* clients[MAXCLIENT];
    int clientListSize=0;
    /* The event loop */
    int number_of_events, i;
    while(1)
    {
        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
        for(i = 0; i < number_of_events; i++)
        {
            timerDisplayEventBySeconds.addCount();
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
                        if(clientListSize>=MAXCLIENT)
                            break;
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
                        Client *client=new Client();
                        client->infd=infd;
                        clients[clientListSize]=client;
                        clientListSize++;
                        int s = Socket::make_non_blocking(infd);
                        if(s == -1)
                            abort();
                        epoll_event event;
                        event.data.ptr = client;
                        event.events = EPOLLIN | EPOLLET | EPOLLOUT;
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
                    Client *client=static_cast<Client *>(events[i].data.ptr);
                    if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        fprintf(stderr, "client epoll error\n");
                        client->close();
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
                        while (1)
                        {
                            const ssize_t &count = client->read(buf, sizeof buf);
                            if(count>0)
                            {
                                //broat cast to all the client
                                int index=0;
                                while(index<clientListSize)
                                {
                                    clients[index]->write(buf,count);
                                    index++;
                                }
                            }
                            else
                                break;
                        }
                    }
                    //ready to write
                    if(events[i].events & EPOLLOUT)
                    {
                        client->flush();
                    }
                }
                break;
                case BaseClassSwitch::Type::Timer:
                    static_cast<Timer *>(events[i].data.ptr)->exec();
                break;
                default:
                    fprintf(stderr, "unknow event\n");
                break;
            }
        }
    }
    server.close();
    return EXIT_SUCCESS;
}
