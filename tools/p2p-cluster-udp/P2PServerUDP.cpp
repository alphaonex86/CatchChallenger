#include "P2PServerUDP.h"
#include <string>
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "../../server/epoll/Epoll.h"
#include <cstring>
#include <arpa/inet.h> // for inet_ntoa, drop after debug

using namespace CatchChallenger;

char P2PServerUDP::readBuffer[];
std::vector<P2PServerUDP::HostToConnect> P2PServerUDP::hostToConnect;
size_t P2PServerUDP::hostToConnectIndex=0;
P2PServerUDP *P2PServerUDP::p2pserver=NULL;

P2PServerUDP::P2PServerUDP(uint8_t *privatekey/*ED25519_KEY_SIZE*/, uint8_t *ca_publickey/*ED25519_KEY_SIZE*/, uint8_t *ca_signature/*ED25519_SIGNATURE_SIZE*/) :
    sfd(-1)
{
    memcpy(this->privatekey,privatekey,ED25519_KEY_SIZE);
    memcpy(this->ca_publickey,ca_publickey,ED25519_KEY_SIZE);
    memcpy(this->ca_signature,ca_signature,ED25519_SIGNATURE_SIZE);
    ed25519_sha512_public_key(publickey,privatekey);

    *ptr_random = fopen("/dev/urandom","rb");  // r for read, b for binary
    if(ptr_random == NULL)
        abort();
}

P2PServerUDP::~P2PServerUDP()
{
    fclose(ptr_random);
}

bool P2PServerUDP::tryListen(const uint16_t &port)
{
    int sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //create a UDP socket
    if (sfd == -1)
    {
        std::cerr << "P2PServerUDP::tryListen(): unable to create UDP socket" << std::endl;
        return false;
    }

    // zero out the structure
    sockaddr_in si_me;
    memset((char *) &si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if( bind(sfd , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        ::close(sfd);
        std::cerr << "P2PServerUDP::tryListen(): unable to bind UDP socket" << std::endl;
        return false;
    }

    int flags = fcntl(sfd, F_GETFL, 0);
    if(flags == -1)
    {
        ::close(sfd);
        std::cerr << "fcntl get flags error" << std::endl;
        return false;
    }
    flags |= O_NONBLOCK;
    int s = fcntl(sfd, F_SETFL, flags);
    if(s == -1)
    {
        ::close(sfd);
        std::cerr << "fcntl set flags error" << std::endl;
        return false;
    }

    epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR;
    if(Epoll::epoll.ctl(EPOLL_CTL_ADD, sfd, &event) == -1)
    {
        ::close(sfd);
        std::cerr << "P2PServerUDP::tryListen(): unable to EPOLL_CTL_ADD UDP socket" << std::endl;
        return false;
    }

    this->sfd=sfd;
    return true;
}

void P2PServerUDP::read()
{
    sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);

    //try to receive some data, this is a blocking call
    const int recv_len = recvfrom(sfd, P2PServerUDP::readBuffer, sizeof(P2PServerUDP::readBuffer), 0, (struct sockaddr *) &si_other, &slen);
    if (recv_len == -1)
    {
        std::cerr << "P2PServerUDP::parseIncommingData(): recvfrom() problem" << std::endl;
        abort();
    }
    const std::string data(P2PServerUDP::readBuffer,recv_len);

    //print details of the client/peer and the data received
    std::cout << "Received packet from " << inet_ntoa(si_other.sin_addr) << ":" << ntohs(si_other.sin_port) << std::endl;
    std::cout << "Data: " << data << std::endl;

    //now reply the client with the same data
    if(data=="request")
    {
        std::string reply("reply");
        if(write(reply,si_other) == -1)
        {
            std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem" << std::endl;
            abort();
        }
    }
}

int P2PServerUDP::write(const std::string &data,const sockaddr_in &si_other)
{
    /*//first reply
    P2PServerUDP::p2pserver.write("publique key, firmed by master",serv_addr);
    P2PServerUDP::p2pserver.write("reply, firmed by local",serv_addr);

    //send neighbour reply: only the missing host
    firm local(sequence number (32Bits) + data size + data)*/

    const int returnVal=sendto(sfd, data.data(), data.size(), 0, (struct sockaddr*) &si_other, sizeof(si_other));
    if (returnVal == -1)
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem" << std::endl;
    return returnVal;
}

P2PServerUDP::EpollObjectType P2PServerUDP::getType() const
{
    return P2PServerUDP::EpollObjectType::ServerP2P;
}

void P2PServerUDP::ed25519_sha512_sign(size_t length, const uint8_t *msg,uint8_t *signature)
{
    ::ed25519_sha512_sign(ca_publickey,privatekey,length,msg,signature);
}

char * P2PServerUDP::getPublicKey()
{
    return publickey;
}

char * P2PServerUDP::getCaSignature()
{
    return ca_signature;
}
