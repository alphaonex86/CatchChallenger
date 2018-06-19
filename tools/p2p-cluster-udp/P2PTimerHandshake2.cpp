#include "P2PTimerHandshake2.h"
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

P2PTimerHandshake2::P2PTimerHandshake2()
{
    setInterval(107);
    startTime=std::chrono::steady_clock::now();
}

void P2PTimerHandshake2::exec()
{
    if(P2PServerUDP::p2pserver->hostToFirstReply.empty())
        return;
    {
        const std::chrono::time_point<std::chrono::steady_clock> end=std::chrono::steady_clock::now();
        if(end<startTime)
            startTime=end;
        else if(std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime).count()<5000 &&
                P2PServerUDP::p2pserver->hostToFirstReplyIndex>=P2PServerUDP::p2pserver->hostToFirstReply.size())
            return;
    }
    if(P2PServerUDP::p2pserver->hostToFirstReplyIndex>=P2PServerUDP::p2pserver->hostToFirstReply.size())
    {
        P2PServerUDP::p2pserver->hostToFirstReplyIndex=0;
        startTime=std::chrono::steady_clock::now();
    }
    size_t lastScannedIndex=P2PServerUDP::p2pserver->hostToFirstReplyIndex;
    do
    {
        P2PServerUDP::HostToFirstReply *peerToConnect=P2PServerUDP::p2pserver->hostToFirstReply.at(lastScannedIndex);
        lastScannedIndex++;
        if(lastScannedIndex>=P2PServerUDP::p2pserver->hostToFirstReply.size())
            lastScannedIndex=0;

        peerToConnect.round++;
        if(peerToConnect.round==1 || peerToConnect.round==5)
        {
            P2PServerUDP::p2pserver->write(peerToConnect.reply,8+4+1+8+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE,peerToConnect.hostConnected.serv_addr);

            P2PServerUDP::p2pserver->hostToFirstReplyIndex=lastScannedIndex;
            std::cout << "P2PTimerHandshake2::exec() try co" << std::endl;
            return;
        }
        else if(peerToConnect.round > 5)//after try at 100ms and at 500ms, drop the reply
        {
            lastScannedIndex--;
            P2PServerUDP::p2pserver->hostToFirstReply.erase(P2PServerUDP::p2pserver->hostToFirstReply.cbegin()+lastScannedIndex);
        }
    } while(lastScannedIndex!=P2PServerUDP::p2pserver->hostToFirstReplyIndex);
    P2PServerUDP::p2pserver->hostToFirstReplyIndex=lastScannedIndex;

    std::cout << "P2PTimerHandshake2::exec()" << std::endl;
}
