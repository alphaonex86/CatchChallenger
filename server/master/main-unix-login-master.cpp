#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <vector>
#include <signal.h>

#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/Version.hpp"
#include "../cli/EventLoop.hpp"
#include "EventLoopServerLoginMaster.hpp"
#include "EventLoopClientLoginMaster.hpp"
#include "PlayerUpdaterToLogin.hpp"

#define MAXEVENTS 512
#define MAXCLIENTSINSUSPEND 16

#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
extern "C" {
const char* __asan_default_options() { return "alloc_dealloc_mismatch=0:detect_container_overflow=0"; }
}  // extern "C"
#  endif
#endif

using namespace CatchChallenger;

/* Catch Signal Handler functio */
void signal_callback_handler(int signum){

        printf("Caught signal SIGPIPE %d\n",signum);
}

int main(int argc, const char *argv[])
{
    /* Catch Signal Handler SIGPIPE */
    if(signal(SIGPIPE, signal_callback_handler)==SIG_ERR)
    {
        std::cerr << "signal(SIGPIPE, signal_callback_handler)==SIG_ERR, errno: " << std::to_string(errno) << std::endl;
        abort();
    }

    std::cout << "CatchChallenger version: " << CatchChallenger::Version::str << std::endl;

    {
        DIR* dir = opendir("datapack/");
        if (dir)
        {
            /* Directory exists. */
            closedir(dir);
        }
        else if (ENOENT == errno)
        {
            /* Directory does not exist. */
            std::cerr << "Directory does not exist (abort)" << std::endl;
            abort();
        }
        else
        {
            /* opendir() failed for some other reason. */
            std::cerr << "opendir(\"datapack/\") failed for some other reason. (abort)" << std::endl;
            abort();
        }
    }

    std::cout << "CatchChallenger version: " << CatchChallenger::Version::str << std::endl;
    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];

    ProtocolParsing::initialiseTheVariable(ProtocolParsing::InitialiseTheVariableType::MasterServer);
    if(!EventLoop::loop.init())
        return EPOLLERR;

    EventLoopServerLoginMaster::unixServerLoginMaster=new EventLoopServerLoginMaster();
    #ifdef CATCHCHALLENGER_CACHE_HPS
    const bool save=argc==2 && strcmp(argv[1],"save")==0;
    if(save)
        EventLoopServerLoginMaster::unixServerLoginMaster->setSave(datapackCache);
    else
        EventLoopServerLoginMaster::unixServerLoginMaster->setLoad(datapackCache);
    #endif

    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    bool tcpCork=false,tcpNodelay=false;

    // SSL preamble byte removed; no per-connect sentinel byte is emitted.

    PlayerUpdaterToLogin playerUpdaterToLogin;
    if(!playerUpdaterToLogin.start())
    {
        std::cerr << "timerPositionSync fail to set" << std::endl;
        return EXIT_FAILURE;
    }

    int numberOfConnectedClient=0;
    /* The event loop */
    std::vector<void *> elementsToDelete[16];
    size_t elementsToDeleteSize=0;
    uint8_t elementsToDeleteIndex=0;

    int number_of_events, i;
    while(1)
    {
        number_of_events = EventLoop::loop.wait(events, MAXEVENTS);
        if(elementsToDeleteSize>0 && number_of_events<MAXEVENTS)
        {
            if(elementsToDeleteIndex>=15)
                elementsToDeleteIndex=0;
            else
                ++elementsToDeleteIndex;
            const std::vector<void *> &elementsToDeleteSub=elementsToDelete[elementsToDeleteIndex];
            if(!elementsToDeleteSub.empty())
            {
                unsigned int index=0;
                while(index<elementsToDeleteSub.size())
                {
                    delete static_cast<EventLoopClientLoginMaster *>(elementsToDeleteSub.at(index));
                    index++;
                }
            }
            elementsToDeleteSize-=elementsToDeleteSub.size();
            elementsToDelete[elementsToDeleteIndex].clear();
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
                        const int &infd = EventLoopServerLoginMaster::unixServerLoginMaster->accept(&in_addr, &in_len);
                        if(elementsToDeleteSize>64)
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

                        /*int s = SocketUtil::make_non_blocking(infd);
                        if(s == -1)
                            std::cerr << "unable to make to socket non blocking" << std::endl;
                        else*/
                        {
                            if(tcpCork)
                            {
                                //set cork for CatchChallener because don't have real time part
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp cork" << std::endl;
                            }
                            else if(tcpNodelay)
                            {
                                //set no delay to don't try group the packet and improve the performance
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp no delay" << std::endl;
                            }

                            EventLoopClientLoginMaster *client=new EventLoopClientLoginMaster(infd
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
                                    //std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << ")" << std::endl;
                                    client->socketString=hbuf;
                                    client->socketString+=":";
                                    client->socketString+=sbuf;
                                    //always concat to ": "
                                    client->socketString+=": ";
                                    client->ip=hbuf;
                                }
                            }
                            epoll_event event;
                            event.data.ptr = client;
                            event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLOUT /*EPOLLOUT: CLOSE_WAIT but put the cpu at 100%, loop between user and kernel space as EventLoopTimer::validateTheTimer() missing */;
                            const int s = EventLoop::loop.ctl(EPOLL_CTL_ADD, infd, &event);
                            if(s == -1)
                            {
                                std::cerr << "epoll_ctl on socket error" << std::endl;
                                delete client;
                            }
                            // SSL preamble byte removed; nothing to write here.
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::Client:
                {
                    EventLoopClientLoginMaster * const client=static_cast<EventLoopClientLoginMaster *>(events[i].data.ptr);
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
                        elementsToDelete[elementsToDeleteIndex].push_back(events[i].data.ptr);
                        elementsToDeleteSize++;

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
                        elementsToDelete[elementsToDeleteIndex].push_back(events[i].data.ptr);
                        elementsToDeleteSize++;
                    }
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::Timer:
                {
                    EventLoopTimer * const timer=static_cast<EventLoopTimer *>(events[i].data.ptr);
                    timer->exec();
                    timer->validateTheTimer();
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::Database:
                {
                    EventLoopPostgresql * const db=static_cast<EventLoopPostgresql *>(events[i].data.ptr);
                    db->unixEvent(events[i].events);

                    //disconnected after finish of use
                    if(!db->isConnected())
                    {
                        /*std::cerr << "database disconnect, quit now" << std::endl;
                        return EXIT_FAILURE;*/
                    }
                    if(CharactersGroup::serverWaitedToBeReady==0)
                    {
                        if(!EventLoopServerLoginMaster::unixServerLoginMaster->tryListen())
                            abort();
                        CharactersGroup::serverWaitedToBeReady--;
                    }
                }
                break;
                default:
                    std::cerr << "unknown event" << std::endl;
                break;
            }
        }
    }
    EventLoopServerLoginMaster::unixServerLoginMaster->close();
    delete EventLoopServerLoginMaster::unixServerLoginMaster;
    EventLoopServerLoginMaster::unixServerLoginMaster=NULL;
    return EXIT_SUCCESS;
}
