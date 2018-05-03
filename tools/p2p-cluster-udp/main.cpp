#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <nettle/eddsa.h>
#include <arpa/inet.h>

#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/cpp11addition.h"
#include "../../server/base/TinyXMLSettings.h"
#include "../../server/epoll/Epoll.h"
#include "../../server/epoll/EpollSocket.h"
#include "../../server/VariableServer.h"
#include "P2PServerUDP.h"
#include "P2PTimerConnect.h"

#define MAXEVENTS 512

using namespace CatchChallenger;

/* Catch Signal Handler functio */
void signal_callback_handler(int signum){
    printf("Caught signal SIGPIPE %d\n",signum);
}

void genPrivateKey(uint8_t *privatekey)
{
    FILE *ptr = fopen("/dev/random","rb");  // r for read, b for binary
    if(ptr == NULL)
        abort();
    const int readSize=fread(privatekey,1,sizeof(privatekey),ptr);
    if(readSize != sizeof(privatekey))
        abort();
    fclose(ptr);
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
    uint16_t port;
    {
        TinyXMLSettings settings("p2p.xml");

        if(!settings.contains("privatekey"))
        {
            genPrivateKey(privatekey);
            settings.setValue("privatekey",binarytoHexa(privatekey,sizeof(privatekey)));
        }
        else
        {
            const std::vector<char> tempData=hexatoBinary(settings.value("privatekey"));
            if(tempData.size()==ED25519_KEY_SIZE)
                memcpy(privatekey,tempData.data(),ED25519_KEY_SIZE);
            else
            {
                genPrivateKey(privatekey);
                settings.setValue("privatekey",binarytoHexa(privatekey,sizeof(privatekey)));
                settings.sync();
            }
        }
        if(!settings.contains("ca_signature"))
        {
            std::cerr << "You need define CA signature of your public key" << std::endl;
            settings.setValue("ca_signature","");
            settings.sync();
            abort();
        }
        else
        {
            const std::vector<char> tempData=hexatoBinary(settings.value("ca_signature"));
            if(tempData.size()==ED25519_SIGNATURE_SIZE)
                memcpy(ca_signature,tempData.data(),ED25519_SIGNATURE_SIZE);
            else
            {
                std::cerr << "You need define CA signature of your public key" << std::endl;
                abort();
            }
        }
        if(!settings.contains("ca_publickey"))
        {
            std::cerr << "You need define CA public key" << std::endl;
            settings.setValue("ca_publickey","");
            settings.sync();
            abort();
        }
        else
        {
            const std::vector<char> tempData=hexatoBinary(settings.value("ca_publickey"));
            if(tempData.size()==ED25519_KEY_SIZE)
                memcpy(ca_publickey,tempData.data(),ED25519_KEY_SIZE);
            else
            {
                std::cerr << "You need define CA public key" << std::endl;
                abort();
            }
        }

        if(!settings.contains("known_node"))
            settings.setValue("known_node","");

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

            P2PServerUDP::HostToConnect hostToConnect;
            const std::string host=known_nodes.at(index);
            const std::string &portstring=known_nodes.at(index+1);
            const uint16_t port=stringtouint16(portstring,&ok);
            if(ok)
            {
                sockaddr_in serv_addr;
                memset(&serv_addr, 0, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htobe16(port);
                const char * const hostC=host.c_str();
                const int convertResult=inet_pton(AF_INET6,hostC,&serv_addr.sin_addr);
                if(convertResult!=1)
                {
                    const int convertResult=inet_pton(AF_INET,hostC,&serv_addr.sin_addr);
                    if(convertResult!=1)
                        std::cerr << "not IPv4 and IPv6 address, errno: " << std::to_string(errno) << std::endl;
                }
                if(convertResult==1)
                {
                    hostToConnect.round=0;
                    hostToConnect.serv_addr=serv_addr;
                    P2PServerUDP::p2pserver->hostToConnect.push_back(hostToConnect);
                }
            }

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
    P2PServerUDP::p2pserver=new P2PServerUDP(privatekey,ca_publickey,ca_signature);
    memset(privatekey,0,ED25519_KEY_SIZE);
    memset(ca_publickey,0,ED25519_KEY_SIZE);
    memset(ca_signature,0,ED25519_SIGNATURE_SIZE);
    if(!P2PServerUDP::p2pserver->tryListen(port))
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
                default:
                    std::cout << "case BaseClassSwitch::EpollObjectType::?" << std::endl;
                    abort();
                break;
            }
        }
    }
    return EXIT_SUCCESS;
}
