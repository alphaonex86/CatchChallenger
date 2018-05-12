#include "P2PTimerConnect.h"
#include "P2PServerUDP.h"
#include <iostream>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "../../server/epoll/Epoll.h"
#include "../../general/base/PortableEndian.h"

using namespace CatchChallenger;

//[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE(node)+ED25519_SIGNATURE_SIZE(ca)+ED25519_SIGNATURE_SIZE(node)]
char P2PTimerConnect::handShake1[];

P2PTimerConnect::P2PTimerConnect()
{
    setInterval(100);
    startTime=std::chrono::steady_clock::now();

    //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE(node)+ED25519_SIGNATURE_SIZE(ca)+ED25519_SIGNATURE_SIZE(node)]
    memset(handShake1,0,sizeof(handShake1));
    const uint8_t requestType=1;
    memcpy(handShake1+8+8,&requestType,sizeof(requestType));
    memcpy(handShake1+8+8+1,P2PServerUDP::p2pserver->getPublicKey(),ED25519_KEY_SIZE);
    memcpy(handShake1+8+8+1+ED25519_KEY_SIZE,P2PServerUDP::p2pserver->getCaSignature(),ED25519_SIGNATURE_SIZE);
}

void P2PTimerConnect::exec()
{
    if(P2PServerUDP::p2pserver->hostToConnect.empty())
        return;
    {
        const std::chrono::time_point<std::chrono::steady_clock> end=std::chrono::steady_clock::now();
        if(end<startTime)
            startTime=end;
        else if(std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime).count()<5000 &&
                P2PServerUDP::p2pserver->hostToConnectIndex>=P2PServerUDP::p2pserver->hostToConnect.size())
            return;
    }
    if(P2PServerUDP::p2pserver->hostToConnectIndex>=P2PServerUDP::p2pserver->hostToConnect.size())
    {
        P2PServerUDP::p2pserver->hostToConnectIndex=0;
        startTime=std::chrono::steady_clock::now();
    }
    size_t lastScannedIndex=P2PServerUDP::p2pserver->hostToConnectIndex;
    do
    {
        P2PServerUDP::HostToConnect &peerToConnect=P2PServerUDP::p2pserver->hostToConnect.at(lastScannedIndex);
        lastScannedIndex++;
        if(lastScannedIndex>=P2PServerUDP::p2pserver->hostToConnect.size())
            lastScannedIndex=0;

        peerToConnect.round++;
        if(peerToConnect.round<3 || peerToConnect.round==13)
        {
            if(peerToConnect.round==13)
                peerToConnect.round=3;

            //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE(node)+ED25519_SIGNATURE_SIZE(ca)+ED25519_SIGNATURE_SIZE(node)]
            do
            {
                const int readSize=fread(handShake1,1,8,P2PServerUDP::p2pserver->ptr_random);
                if(readSize != 8)
                    abort();
            } while(*static_cast<uint64_t *>(handShake1)==0);
            P2PServerUDP::p2pserver->sign(8+8+1+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE+,reinterpret_cast<uint8_t *>(handShake1));
            P2PServerUDP::p2pserver->write(handShake1,sizeof(handShake1),peerToConnect.serv_addr);

            P2PServerUDP::p2pserver->hostToConnectIndex=lastScannedIndex;
            std::cout << "P2PTimerConnect::exec() try co" << std::endl;
            return;
        }
    } while(lastScannedIndex!=P2PServerUDP::p2pserver->hostToConnectIndex);
    P2PServerUDP::p2pserver->hostToConnectIndex=lastScannedIndex;

    std::cout << "P2PTimerConnect::exec()" << std::endl;
}
