#include "P2PTimerHandshake3.h"
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

P2PTimerHandshake3::P2PTimerHandshake3()
{
    setInterval(250);
    startTime=std::chrono::steady_clock::now();
}

void P2PTimerHandshake3::exec()
{
    if(P2PServerUDP::p2pserver->hostToSecondReply.empty())
        return;
    {
        const std::chrono::time_point<std::chrono::steady_clock> end=std::chrono::steady_clock::now();
        if(end<startTime)
            startTime=end;
        else if(std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime).count()<5000 &&
                P2PServerUDP::p2pserver->hostToSecondReplyIndex>=P2PServerUDP::p2pserver->hostToSecondReply.size())
            return;
    }
    if(P2PServerUDP::p2pserver->hostToSecondReplyIndex>=P2PServerUDP::p2pserver->hostToSecondReply.size())
    {
        P2PServerUDP::p2pserver->hostToSecondReplyIndex=0;
        startTime=std::chrono::steady_clock::now();
    }
    size_t lastScannedIndex=P2PServerUDP::p2pserver->hostToSecondReplyIndex;
    do
    {
        P2PServerUDP::hostToSecondReply &peerToConnect=P2PServerUDP::p2pserver->hostToSecondReply.at(lastScannedIndex);
        lastScannedIndex++;
        if(lastScannedIndex>=P2PServerUDP::p2pserver->hostToSecondReply.size())
            lastScannedIndex=0;

        peerToConnect.round++;
        if(peerToConnect.round<4*5)
        {
            P2PServerUDP::p2pserver->write(peerToConnect.reply,8+4+1+8+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE,peerToConnect.hostConnected.serv_addr);

            P2PServerUDP::p2pserver->hostToSecondReplyIndex=lastScannedIndex;
            std::cout << "P2PTimerHandshake3::exec() try co" << std::endl;
            return;
        }
        else
        {
            lastScannedIndex--;
            P2PServerUDP::p2pserver->hostToSecondReply.erase(peerToConnect.socketdest);
            P2PServerUDP::p2pserver->hostConnectionEstablished.erase(peerToConnect.socketdest);
        }
    } while(lastScannedIndex!=P2PServerUDP::p2pserver->hostToSecondReplyIndex);
    P2PServerUDP::p2pserver->hostToSecondReplyIndex=lastScannedIndex;

    std::cout << "P2PTimerHandshake3::exec()" << std::endl;
}
