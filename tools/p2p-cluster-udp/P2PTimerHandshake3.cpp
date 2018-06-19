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
    if(P2PServerUDP::p2pserver->hostToFirstReply.empty())
        return;
    {
        const std::chrono::time_point<std::chrono::steady_clock> end=std::chrono::steady_clock::now();
        if(end<startTime)
            startTime=end;
    }
    unsigned int sendedClient=0;
    for( auto& n : P2PServerUDP::p2pserver->hostToFirstReply ) {
        if(clientSend.find(n.first)==clientSend.cend())
        {
            P2PServerUDP::HostToFirstReply peerToConnect=n.second;
            peerToConnect.round++;
            if(peerToConnect.round>=1 && peerToConnect.round<=5)
            {
                peerToConnect.hostConnected->sendData(reinterpret_cast<uint8_t *>(peerToConnect.reply),8+4+1+8+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE);

                std::cout << "P2PTimerHandshake3::exec() try co" << std::endl;
                return;
            }
            else if(peerToConnect.round > 5)//after try at 100ms and at 500ms, drop the reply
                P2PServerUDP::p2pserver->hostToFirstReply.erase(n.first);
            clientSend.insert(n.first);
            sendedClient++;
            break;
        }
    }

    //if no more data to send, reset and recall
    if(sendedClient==0)
    {
        clientSend.clear();
        startTime=std::chrono::steady_clock::now();
    }

    std::cout << "P2PTimerHandshake3::exec()" << std::endl;
}
