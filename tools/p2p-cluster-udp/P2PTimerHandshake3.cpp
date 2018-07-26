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
    if(P2PServerUDP::p2pserver->hostConnectionEstablished.empty())
        return;
    {
        const std::chrono::time_point<std::chrono::steady_clock> end=std::chrono::steady_clock::now();
        if(end<startTime)
            startTime=end;
    }
    unsigned int sendedClient=0;
    for( auto& n : P2PServerUDP::p2pserver->hostConnectionEstablished ) {
        if(clientSend.find(n.first)==clientSend.cend())
        {
            //have data to send..., or just 3 ACK
            /*P2PPeer *peerToConnect=n.second;
            peerToConnect->sendRawDataWithoutPutInQueue(reinterpret_cast<uint8_t *>(peerToConnect->reply),
                                           sizeof(peerToConnect->reply)
                                           );
                return;
                //P2PServerUDP::p2pserver->hostToFirstReply.erase(n.first);
            clientSend.insert(n.first);
            sendedClient++;*/
            return;
            break;
        }
    }

    //if no more data to send, reset and recall
    if(sendedClient==0)
    {
        clientSend.clear();
        startTime=std::chrono::steady_clock::now();
    }

    //std::cout << "P2PTimerHandshake3::exec()" << std::endl;
}
