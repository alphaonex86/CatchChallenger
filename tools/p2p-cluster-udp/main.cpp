#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <nettle/eddsa.h>

#include "../../general/base/FacilityLibGeneral.h"
#include "../../server/epoll/Epoll.h"
#include "../../server/epoll/EpollSocket.h"
#include "../../server/VariableServer.h"
#include "P2PServerUDP.h"
#include "P2PServerUDPSettings.h"
#include "P2PTimerConnect.h"
#include "P2PTimerHandshake2.h"
#include "P2PTimerHandshake3.h"
#include "Status.h"
#include "Stdin.h"

#define MAXEVENTS 512

using namespace CatchChallenger;

/* Catch Signal Handler function */
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

    uint8_t privatekey[ED25519_KEY_SIZE];
    uint8_t ca_publickey[ED25519_KEY_SIZE];
    uint8_t ca_signature[ED25519_SIGNATURE_SIZE];
    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];
    memset(events,0,sizeof(events));
    memset(privatekey,0,ED25519_KEY_SIZE);
    memset(ca_publickey,0,ED25519_KEY_SIZE);
    memset(ca_signature,0,ED25519_SIGNATURE_SIZE);
    if(!Epoll::epoll.init())
        return EPOLLERR;

    P2PServerUDP::p2pserver=new P2PServerUDPSettings(privatekey,ca_publickey,ca_signature);
    P2PServerUDP::p2pserver->hostToConnect=static_cast<P2PServerUDPSettings *>(P2PServerUDP::p2pserver)->hostToConnectTemp;
    static_cast<P2PServerUDPSettings *>(P2PServerUDP::p2pserver)->hostToConnectTemp.clear();
    if(!P2PServerUDP::p2pserver->tryListen(static_cast<P2PServerUDPSettings *>(P2PServerUDP::p2pserver)->port))
    {
        std::cerr << "can't listen, abort" << std::endl;
        abort();
    }
    P2PTimerConnect p2pTimerConnect;
    p2pTimerConnect.start();
    P2PTimerHandshake2 p2pTimerHandshake2;
    p2pTimerHandshake2.start();
    P2PTimerHandshake3 p2pTimerHandshake3;
    p2pTimerHandshake3.start();
    Status::status.start();
    Stdin s;

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
                /*switch(elementsToDelete.at(index).second)
                {
                    case BaseClassSwitch::EpollObjectType::ClientP2P:
                        //delete static_cast<LinkToLogin *>(elementsToDelete.at(index).first);
                    break;
                    default:
                    break;
                }*/
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
                    if(event.events & EPOLLERR)
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "server epoll error" << std::endl;
                        continue;
                    }
                    /* We have a notification on the listening socket, which
                    means one or more incoming connections. */
                    if(event.events & EPOLLIN)
                        P2PServerUDP::p2pserver->read();
                    continue;
                }
                break;
                case BaseClassSwitch::EpollObjectType::Timer:
                {
                    P2PTimerConnect * const timer=static_cast<P2PTimerConnect *>(event.data.ptr);
                    timer->exec();
                    timer->validateTheTimer();
                }
                break;
                case BaseClassSwitch::EpollObjectType::Stdin:
                {
                    Stdin * const s=static_cast<Stdin *>(event.data.ptr);
                    s->read();
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
