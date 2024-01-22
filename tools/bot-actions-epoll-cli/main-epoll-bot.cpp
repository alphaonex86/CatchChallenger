#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <vector>
#include <arpa/inet.h>
#include <signal.h>

#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../../server/epoll/Epoll.hpp"
#include "TimerMove.hpp"
#include "Bot.hpp"

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

    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];

    ProtocolParsing::initialiseTheVariable(ProtocolParsing::InitialiseTheVariableType::LoginServer);
    if(!Epoll::epoll.init())
        return EPOLLERR;


    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    TimerMove timerMove;
    {
        if(!timerMove.start(1000))
        {
            std::cerr << "timerMove fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }

    int numberOfConnectedClient=0;
    std::vector<void *> elementsToDelete[16];
    size_t elementsToDeleteSize=0;
    uint8_t elementsToDeleteIndex=0;

    int status, client_fd;
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    if(argc<5)
    {
        std::cerr << "wrong arg count, need be: " << argv[0] << " [Server IP] [Server Port] [Login] [Pass]" << std::endl;
        return EXIT_FAILURE;
    }
    if(!Bot::parseServerHostAndPort(argv[1],argv[2]))
        abort();
    Bot::setLoginPass(argv[3],argv[4]);
    DatapackDownloaderBase::mDatapackBase=CatchChallenger::FacilityLibGeneral::applicationDirPath+"/datapack/"+std::string(argv[1])+":"+std::to_string(argv[2])+"/";

    Bot *first_bot=new Bot(client_fd);
    struct sockaddr_in6 serv_addr=Bot::get_serv_addr();
    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr,sizeof(serv_addr))) < 0)
    {
        printf("\nConnection Failed, errno: %d\n",errno);
        return -1;
    }

    epoll_event event;
    memset(&event,0,sizeof(event));
    event.data.ptr = first_bot;
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
    int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, client_fd, &event);
    if(s == -1)
    {
        std::cerr << "epoll_ctl on socket (master link) error" << std::endl;
        abort();
    }

    {
        const std::vector<std::string> &extensionAllowedTemp=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
        unsigned int index=0;
        while(index<extensionAllowedTemp.size())
        {
            DatapackDownloaderMainSub::extensionAllowed.insert(extensionAllowedTemp.at(index));
            index++;
        }
    }
    {
        const std::vector<std::string> &extensionAllowedTemp=stringsplit(CATCHCHALLENGER_EXTENSION_COMPRESSED,';');
        unsigned int index=0;
        while(index<extensionAllowedTemp.size())
        {
            EpollClientLoginSlave::compressedExtension.insert(extensionAllowedTemp.at(index));
            index++;
        }
    }
    DatapackDownloaderBase::extensionAllowed=DatapackDownloaderMainSub::extensionAllowed;

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
                    delete static_cast<Bot *>(elementsToDeleteSub.at(index));
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
                case BaseClassSwitch::EpollObjectType::Client:
                {
                    Bot * const client=static_cast<Bot *>(events[i].data.ptr);
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

                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                        client->parseIncommingData();
                    if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP || client->socketIsClosed())
                    {
                        numberOfConnectedClient--;

                        if(client->disconnectClient())
                            elementsToDelete[elementsToDeleteIndex].push_back(events[i].data.ptr);
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
                    std::cerr << "unknown event: " << (int)static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType() << std::endl;
                    abort();
                break;
            }
        }
    }
    return EXIT_SUCCESS;
}
