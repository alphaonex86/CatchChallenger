#include "Status.h"
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

Status Status::status;

Status::Status()
{
    setInterval(250);
}
void Status::exec()
{
    if(P2PServerUDP::p2pserver==NULL)
        return;
    std::string newStatus;

    //status code
    for( const auto& n : P2PServerUDP::p2pserver->hostToFirstReply )
        newStatus+="First Reply: "+n.second.hostConnected->toString()+"\n";
    for( const auto& n : P2PServerUDP::p2pserver->hostToSecondReply )
        newStatus+="Second Reply: "+n.second.hostConnected->toString()+"\n";
    for( const auto& n : P2PServerUDP::p2pserver->hostConnectionEstablished )
        newStatus+="Connected: "+n.second->toString()+"\n";

    if(oldStatus!=newStatus)
    {
        oldStatus=newStatus;
        std::cout << "-----------------\n"
                  << newStatus
                   << "-----------------"
                   << std::endl;
    }
}

void Status::clear()
{
    oldStatus.clear();
}
