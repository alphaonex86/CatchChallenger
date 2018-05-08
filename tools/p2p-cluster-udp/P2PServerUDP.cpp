#include "P2PServerUDP.h"
#include <string>
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "../../server/epoll/Epoll.h"
#include "../../general/base/cpp11addition.h"
#include <cstring>
#include <arpa/inet.h> // for inet_ntoa, drop after debug

using namespace CatchChallenger;

char P2PServerUDP::readBuffer[];
P2PServerUDP *P2PServerUDP::p2pserver=NULL;
char P2PServerUDP::handShake2[];
char P2PServerUDP::handShake3[];
char P2PServerUDP::handShake4[];

P2PServerUDP::P2PServerUDP(uint8_t *privatekey/*ED25519_KEY_SIZE*/, uint8_t *ca_publickey/*ED25519_KEY_SIZE*/, uint8_t *ca_signature/*ED25519_SIGNATURE_SIZE*/) :
    hostToConnectIndex(0),
    sfd(-1)
{
    memcpy(this->privatekey,privatekey,ED25519_KEY_SIZE);
    memcpy(this->ca_publickey,ca_publickey,ED25519_KEY_SIZE);
    memcpy(this->ca_signature,ca_signature,ED25519_SIGNATURE_SIZE);
    ed25519_sha512_public_key(publickey,privatekey);

    ptr_random = fopen("/dev/urandom","rb");  // r for read, b for binary
    if(ptr_random == NULL)
        abort();

    const uint8_t requestType1=1;

    //[8(current sequence number)+8(acknowledgement number)+2(size)+1(request type)+8(sequence number from 1)+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE]
    memset(handShake2,0,sizeof(handShake2));
    const uint16_t size=1+8;
    memcpy(handShake2+8+8,&size,sizeof(size));
    memcpy(handShake2+8+8+sizeof(size),&requestType1,sizeof(requestType1));
    memcpy(handShake2+8+8+sizeof(size)+1+8+ED25519_SIGNATURE_SIZE,publickey,ED25519_KEY_SIZE);
    memcpy(handShake2+8+8+sizeof(size)+1+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE,ca_signature,ED25519_SIGNATURE_SIZE);

    //[8(current sequence number)+8(acknowledgement number)+2(size)+1(request type)+8(sequence number from 2)+ED25519_SIGNATURE_SIZE
    memset(handShake3,0,sizeof(handShake3));
    const uint16_t size2=1+8;
    memcpy(handShake3+8+8,&size2,sizeof(size2));
    memcpy(handShake3+8+8+sizeof(size),&requestType1,sizeof(requestType1));

    //[8(current sequence number)+8(acknowledgement number)+2(size)+1(request type)+ED25519_SIGNATURE_SIZE]
    memset(handShake4,0,sizeof(handShake4));
    const uint16_t size3=1;
    memcpy(handShake4+8+8,&size3,sizeof(size3));
    memcpy(handShake4+8+8+sizeof(size),&requestType1,sizeof(requestType1));
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

std::string P2PServerUDP::sockSerialised(const sockaddr_in &si_other)
{
    if (si_other.sin_family == AF_INET) {
        return std::string(reinterpret_cast<const char *>(&si_other.sin_addr.s_addr),sizeof(si_other.sin_addr.s_addr))+
                std::string(reinterpret_cast<const char *>(&si_other.sin_port),sizeof(si_other.sin_port));
    } else if (si_other.sin_family == AF_INET6) {
        const sockaddr_in6 *x6 = reinterpret_cast<const sockaddr_in6 *>(&si_other);
        return std::string(reinterpret_cast<const char *>(&x6->sin6_addr.s6_addr),sizeof(x6->sin6_addr.s6_addr))+
                std::string(reinterpret_cast<const char *>(&x6->sin6_port),sizeof(x6->sin6_port));
    } else {
        std::cerr << "unknown sa_family" << std::endl;
        abort();
    }
}

void P2PServerUDP::read()
{
    sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);
    memset(&si_other,0,sizeof(si_other));

    //try to receive some data, this is a blocking call
    const int recv_len = recvfrom(sfd, P2PServerUDP::readBuffer, sizeof(P2PServerUDP::readBuffer), 0, (struct sockaddr *) &si_other, &slen);
    if (recv_len == -1)
    {
        std::cerr << "P2PServerUDP::parseIncommingData(): recvfrom() problem" << std::endl;
        abort();
    }
    const std::string removeClient(sockSerialised(si_other));
    const std::string data(P2PServerUDP::readBuffer,recv_len);

    //print details of the client/peer and the data received
    std::cout << "Received packet from " << inet_ntoa(si_other.sin_addr) << ":" << ntohs(si_other.sin_port) << std::endl;
    std::cout << "Data: " << data << std::endl;

    //now reply the client with the same data
    //[8(sequence number)+2(size)+1(request type)+8(random)+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE]
    if(recv_len>=(8+2+1+ED25519_SIGNATURE_SIZE))
    {
        switch(P2PServerUDP::readBuffer[8+2])
        {
            case 0x01:
            {
                //in: handShake1, out: handShake2
                if(recv_len==(8+2+1+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE))
                {
                    if(P2PServerUDP::p2pserver->hostToFirstReply.size()>64)
                        return;
                    //check if the public key of node is signed by ca
                    const int rc = ed25519_sha512_verify(ca_publickey,//pub
                        ED25519_KEY_SIZE,//length
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8+ED25519_SIGNATURE_SIZE),//msg
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE)//signature
                                                         );
                    if(rc != 1)
                        return;
                    //check the message content
                    const int rc2 = ed25519_sha512_verify(
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8+ED25519_SIGNATURE_SIZE),//pub
                        8+2+1+8,//length
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer),//msg
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8)//signature
                                                         );
                    if(rc2 != 1)
                        return;

                    //[8(sequence number)+2(size)+1(request type)+8(random from 1)+8(random for 2)+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE]
                    memcpy(handShake2+8+2+1,P2PServerUDP::readBuffer+8+2+1,8);
                    const int readSize=fread(handShake2+8+2+1+8,1,8,P2PServerUDP::p2pserver->ptr_random);
                    if(readSize != 8)
                        abort();
                    P2PServerUDP::p2pserver->sign(8+2+1+8+8,reinterpret_cast<uint8_t *>(handShake2));

                    HostToFirstReply hostToFirstReply;
                    memcpy(hostToFirstReply.random,P2PServerUDP::readBuffer+8+2+1+8,8);
                    hostToFirstReply.round=0;
                    hostToFirstReply.hostConnected.local_sequence_number=0;
                    hostToFirstReply.hostConnected.remote_sequence_number=0;
                    hostToFirstReply.hostConnected..serv_addr=si_other;
                    memcpy(hostToFirstReply.hostConnected.publickey,P2PServerUDP::readBuffer+8+2+1+8+ED25519_SIGNATURE_SIZE,ED25519_KEY_SIZE);
                    memcpy(hostToFirstReply.reply,handShake2,sizeof(handShake2));
                    uint32_t indexSearch=0;
                    while(indexSearch<P2PServerUDP::hostToFirstReply.size())
                    {
                        HostToFirstReply &hostToFirstReplyNew=P2PServerUDP::hostToFirstReply.at(indexSearch);
                        if(hostToFirstReplyNew.hostConnected.publickey==hostToFirstReply.hostConnected.publickey ||
                                memcmp(&hostToFirstReplyNew.hostConnected.serv_addr,&hostToFirstReply.hostConnected.serv_addr,sizeof(sockaddr_in))==0)
                        {
                            hostToFirstReplyNew=hostToFirstReply;
                            break;
                        }
                        indexSearch++;
                    }
                    if(indexSearch>=P2PServerUDP::hostToFirstReply.size())
                    {
                        P2PServerUDP::hostToFirstReply[removeClient]=hostToFirstReply;
                        P2PServerUDP::p2pserver->write(handShake2,sizeof(handShake2),hostToFirstReply.hostConnected.serv_addr);
                    }
                }
                //in: handShake2, out: handShake3
                else if(recv_len==(8+2+1+8+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE))
                {
                    //check if the public key of node is signed by ca
                    const int rc = ed25519_sha512_verify(ca_publickey,//pub
                        ED25519_KEY_SIZE,//length
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8+8+ED25519_SIGNATURE_SIZE),//msg
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE)//signature
                                                         );
                    if(rc != 1)
                        return;
                    //check the message content
                    const int rc2 = ed25519_sha512_verify(
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8+8+ED25519_SIGNATURE_SIZE),//pub
                        8+2+1+8+8,//length
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer),//msg
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8+8)//signature
                                                         );
                    if(rc2 != 1)
                        return;

                    std::string randomFrom1(P2PServerUDP::readBuffer+8+2+1,8);
                    std::string randomToReplyFrom2(P2PServerUDP::readBuffer+8+2+1+8,8);
                    std::string nodePublicKey(P2PServerUDP::readBuffer+8+2+1+8+8,ED25519_KEY_SIZE);
                    //search into the connect and check the random
                    unsigned int indexSearch=0;
                    while(indexSearch<P2PServerUDP::p2pserver->hostToConnect.size())
                    {
                        HostToConnect &hostToConnect=P2PServerUDP::p2pserver->hostToConnect.at(indexSearch);
                        if(memcmp(&hostToConnect.serv_addr,&si_other,sizeof(sockaddr_in))==0 &&
                                memcmp(hostToConnect.random,randomFrom1.data(),randomFrom1.size())==0)
                            break;
                        indexSearch++;
                    }
                    //reemit from handShake2 only if valided connected client
                    if(indexSearch>=P2PServerUDP::p2pserver->hostToConnect.size())
                        if(P2PServerUDP::hostConnectionEstablished.find(removeClient)==P2PServerUDP::hostConnectionEstablished.cend())
                            return;

                    //[8(sequence number)+2(size)+1(request type)+8(random from 2)+ED25519_SIGNATURE_SIZE
                    memcpy(handShake3+8+2+1,randomToReplyFrom2.data(),8);
                    sign(8+2+1+8,handShake3);

                    HostConnected newHostConnected;
                    newHostConnected.serv_addr=si_other;
                    newHostConnected.local_sequence_number=0;
                    newHostConnected.remote_sequence_number=0;
                    memcpy(newHostConnected.publickey,nodePublicKey.data(),nodePublicKey.size());
                    P2PServerUDP::hostConnectionValidated[removeClient]=newHostConnected;
                    P2PServerUDP::p2pserver->hostToConnect.erase(hostToConnect);

                    P2PServerUDP::hostToSecondReply[removeClient]=std::string(handShake3,sizeof(handShake3));
                    P2PServerUDP::p2pserver->write(handShake3,sizeof(handShake3),si_other);

                    if(currentIndex<P2PServerUDP::p2pserver->hostToSecondReplyIndex)
                        lastScannedIndex--;
                    hostToConnectIndex.erase();
                }
                //in: handShake3, out: handShake4
                else if(recv_len==(8+2+1+8+ED25519_SIGNATURE_SIZE))
                {
                    //get valid public key from in: handShake1, out: handShake2
                    char *publickey=;
                    //check the message content
                    const int rc2 = ed25519_sha512_verify(
                        reinterpret_cast<const uint8_t *>(hostConnectionEstablished.publickey),//pub
                        8+2+1+8,//length
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer),//msg
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8)//signature
                                                         );
                    if(rc2 != 1)
                        return;

                    //remove the first step
                    P2PServerUDP::hostToFirstReply.erase(removeClient);

                    //search into the connect and check the random
                    unsigned int indexSearch=0;
                    while(indexSearch<P2PServerUDP::p2pserver->hostToConnect.size())
                    {
                        HostToConnect &hostToConnect=P2PServerUDP::p2pserver->hostToConnect.at(indexSearch);
                        if(memcmp(&hostToConnect.serv_addr,&si_other,sizeof(sockaddr_in))==0 &&
                                memcmp(hostToConnect.random,randomFrom1.data(),randomFrom1.size())==0)
                        {
                            HostConnected newHostConnected;
                            newHostConnected.serv_addr=si_other;
                            newHostConnected.local_sequence_number=0;
                            newHostConnected.remote_sequence_number=0;
                            memcpy(newHostConnected.publickey,nodePublicKey.data(),nodePublicKey.size());
                            P2PServerUDP::hostConnectionEstablished[removeClient]=newHostConnected;
                            P2PServerUDP::p2pserver->hostToConnect.erase(hostToConnect);
                            break;
                        }
                        indexSearch++;
                    }
                    //reemit from handShake2 only if valided connected client
                    if(indexSearch>=P2PServerUDP::p2pserver->hostToConnect.size())
                        if(P2PServerUDP::hostConnectionEstablished.find(nodePublicKey)==P2PServerUDP::hostConnectionEstablished.cend())
                            return;

                    //[8(sequence number)+2(size)+1(request type)+8(random from 2)+ED25519_SIGNATURE_SIZE
                    memcpy(handShake3+8+2+1,randomToReplyFrom2.data(),8);
                    sign(8+2+1+8,handShake3);

                    P2PServerUDP::p2pserver->write(handShake3,sizeof(handShake3),si_other);

                    if(currentIndex<P2PServerUDP::p2pserver->hostToSecondReplyIndex)
                        lastScannedIndex--;
                    hostToConnectIndex.erase();
                }
                //in: handShake4
                else if(recv_len==(8+2+1+ED25519_SIGNATURE_SIZE))
                {
                    //get valid public key from in: handShake1, out: handShake2
                    char *publickey=;
                    //check the message content
                    const int rc2 = ed25519_sha512_verify(
                        reinterpret_cast<const uint8_t *>(hostConnectionEstablished.publickey),//pub
                        8+2+1+8,//length
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer),//msg
                        reinterpret_cast<const uint8_t *>(P2PServerUDP::readBuffer+8+2+1+8)//signature
                                                         );
                    if(rc2 != 1)
                        return;

                    P2PServerUDP::hostConnectionEstablished[]=P2PServerUDP::hostToSecondReply.at(XXX);
                    P2PServerUDP::hostToSecondReply.erase(XXX);
                }
            }
            break;
            default:
            return;
        }
    }
}

int P2PServerUDP::write(const char * const data,const uint32_t dataSize,const sockaddr_in &si_other)
{
    const int returnVal=sendto(sfd, data, dataSize, 0, (struct sockaddr*) &si_other, sizeof(si_other));
    if (returnVal < 0)
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem" << std::endl;
    else if ((uint32_t)returnVal != dataSize)
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem dataSize" << std::endl;
    if (dataSize > 1200)
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem dataSize (2)" << std::endl;
    return returnVal;
}

P2PServerUDP::EpollObjectType P2PServerUDP::getType() const
{
    return P2PServerUDP::EpollObjectType::ServerP2P;
}

void P2PServerUDP::sign(size_t length, uint8_t *msg)
{
    ::ed25519_sha512_sign(ca_publickey,privatekey,length,msg,msg+length);
}

char * P2PServerUDP::getPublicKey()
{
    return reinterpret_cast<char *>(publickey);
}

char * P2PServerUDP::getCaSignature()
{
    return reinterpret_cast<char *>(ca_signature);
}
