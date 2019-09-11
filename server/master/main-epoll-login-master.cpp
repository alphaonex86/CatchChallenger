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

#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/Version.h"
#include "../epoll/EpollSocket.h"
#include "../epoll/Epoll.h"
#include "EpollServerLoginMaster.h"
#include "EpollClientLoginMaster.h"
#include "PlayerUpdaterToLogin.h"

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

int main(int argc, char *argv[])
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
    if(!Epoll::epoll.init())
        return EPOLLERR;

    EpollServerLoginMaster::epollServerLoginMaster=new EpollServerLoginMaster();

    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    bool tcpCork=false,tcpNodelay=false;

    char encodingBuff[1];
    #ifdef SERVERSSL
    encodingBuff[0]=0x01;
    #else
    encodingBuff[0]=0x00;
    #endif

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
        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
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
                    delete static_cast<EpollClientLoginMaster *>(elementsToDeleteSub.at(index));
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
                        const int &infd = EpollServerLoginMaster::epollServerLoginMaster->accept(&in_addr, &in_len);
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

                        /*int s = EpollSocket::make_non_blocking(infd);
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

                            EpollClientLoginMaster *client=new EpollClientLoginMaster(infd
                                               #ifdef SERVERSSL
                                               ,server->getCtx()
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
                                    client->socketString=hbuf;
                                    client->socketString+=":";
                                    client->socketString+=sbuf;
                                    //always concat to ": "
                                    client->socketString+=": ";
                                }
                            }
                            epoll_event event;
                            event.data.ptr = client;
                            event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLOUT /*EPOLLOUT: CLOSE_WAIT but put the cpu at 100%, loop between user and kernel space as EpollTimer::validateTheTimer() missing */;
                            const int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                            if(s == -1)
                            {
                                std::cerr << "epoll_ctl on socket error" << std::endl;
                                delete client;
                            }
                            else
                            {
                                if(::write(infd,encodingBuff,sizeof(encodingBuff))!=sizeof(encodingBuff))
                                {
                                    std::cerr << "epoll_ctl on socket write error" << std::endl;
                                    delete client;
                                }
                            }
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::EpollObjectType::Client:
                {
                    EpollClientLoginMaster * const client=static_cast<EpollClientLoginMaster *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
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
                case BaseClassSwitch::EpollObjectType::Timer:
                {
                    EpollTimer * const timer=static_cast<EpollTimer *>(events[i].data.ptr);
                    timer->exec();
                    timer->validateTheTimer();
                }
                break;
                case BaseClassSwitch::EpollObjectType::Database:
                {
                    EpollPostgresql * const db=static_cast<EpollPostgresql *>(events[i].data.ptr);
                    db->epollEvent(events[i].events);

                    //disconnected after finish of use
                    if(!db->isConnected())
                    {
                        /*std::cerr << "database disconnect, quit now" << std::endl;
                        return EXIT_FAILURE;*/
                    }
                    if(CharactersGroup::serverWaitedToBeReady==0)
                    {
                        if(!EpollServerLoginMaster::epollServerLoginMaster->tryListen())
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
    EpollServerLoginMaster::epollServerLoginMaster->close();
    delete EpollServerLoginMaster::epollServerLoginMaster;
    EpollServerLoginMaster::epollServerLoginMaster=NULL;
    return EXIT_SUCCESS;
}
