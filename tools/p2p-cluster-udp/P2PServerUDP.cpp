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

char P2PServerUDP::readBuffer[];
P2PServerUDP *P2PServerUDP::p2pserver=NULL;
char P2PServerUDP::handShake2[];
char P2PServerUDP::handShake3[];

P2PServerUDP::P2PServerUDP(uint8_t *privatekey/*ED25519_KEY_SIZE*/, uint8_t *ca_publickey/*ED25519_KEY_SIZE*/, uint8_t *ca_signature/*ED25519_SIGNATURE_SIZE*/) :
    hostToConnectIndex(0),
    sfd(-1)
{
    memcpy(this->privatekey,privatekey,ED25519_KEY_SIZE);
    memcpy(this->ca_publickey,ca_publickey,ED25519_KEY_SIZE);
    memcpy(this->ca_signature,ca_signature,ED25519_SIGNATURE_SIZE);
    ed25519_sha512_public_key(publickey,privatekey);

    /*const uint8_t *pub,
               size_t length, const uint8_t *msg,
               const uint8_t *signature*/
    const int rc = ed25519_sha512_verify(this->ca_publickey,//pub
        ED25519_KEY_SIZE,//length
        reinterpret_cast<const uint8_t *>(publickey),//msg
        reinterpret_cast<const uint8_t *>(this->ca_signature)//signature
    );
    if(rc != 1)
    {
        std::cerr << "ed25519_sha512_verify failed at " << __FILE__ << ":" << std::to_string(__LINE__)
                  << ", your public key seam not firmed by ca" << std::endl;
        abort();
    }

    std::cout << "public key: " << binarytoHexa(publickey,sizeof(publickey)) << std::endl;

    ptr_random = fopen("/dev/urandom","rb");  // r for read, b for binary
    if(ptr_random == NULL)
        abort();

    const uint8_t requestType3=3;
    const uint8_t requestType2=2;

    //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE+ED25519_SIGNATURE_SIZE]
    memset(handShake2,0,sizeof(handShake2));
    memcpy(handShake2+8+8,&requestType2,sizeof(requestType2));
    memcpy(handShake2+8+8+1,publickey,ED25519_KEY_SIZE);
    memcpy(handShake2+8+8+1+ED25519_KEY_SIZE,ca_signature,ED25519_SIGNATURE_SIZE);

    //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE(node)+ED25519_SIGNATURE_SIZE(ca)+ED25519_SIGNATURE_SIZE(node)]
    //check if the public key of node is signed by ca
    const int rc2 = ed25519_sha512_verify(ca_publickey,//pub
        ED25519_KEY_SIZE,//length
        reinterpret_cast<const uint8_t *>(handShake2+8+8+1),//msg
        reinterpret_cast<const uint8_t *>(handShake2+8+8+1+ED25519_KEY_SIZE)//signature
    );
    if(rc2 != 1)
    {
        std::cerr << "ed25519_sha512_verify failed at " << __FILE__ << ":" << std::to_string(__LINE__)
                  << ", handShake2 is corrupted" << std::endl;
        abort();
    }

    //8+8 -> managed by class, [1(request type)+ED25519_SIGNATURE_SIZE]
    memset(handShake3,0,sizeof(handShake3));
    memcpy(handShake3,&requestType3,sizeof(requestType3));
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
    const char * const readOnlyReadBuffer=readBuffer;
    const std::string remoteClient(sockSerialised(si_other));
    const std::string data(readOnlyReadBuffer,recv_len);

    //now reply the client with the same data
    if(recv_len>=(8+8+1+ED25519_SIGNATURE_SIZE))
    {
        uint8_t messageType=0;
        memcpy(&messageType,readOnlyReadBuffer+8+8,1);

        //print details of the client/peer and the data received
        std::string data1,data2,data3,data4;
        data1=data.substr(0,8+8);
        data2=data.substr(8+8,1);
        data3=data.substr(8+8+1,recv_len-8-8-1-ED25519_SIGNATURE_SIZE);
        data4=data.substr(recv_len-ED25519_SIGNATURE_SIZE,ED25519_SIGNATURE_SIZE);
        std::cout << "(" << std::to_string(messageType) << ") "
                  << inet_ntoa(si_other.sin_addr) << ":" << ntohs(si_other.sin_port) << ": "
                  << binarytoHexa(data1.data(),data1.size()) << " "
                  << binarytoHexa(data2.data(),data2.size()) << " "
                  << binarytoHexa(data3.data(),data3.size()) << " "
                  << binarytoHexa(data4.data(),data4.size()) << " "
                  << std::endl;

        switch(messageType)
        {
            case 0x01:
            {
                switch(recv_len)
                {
                    //in: handShake1, out: handShake2
                    case 8+8+1+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE+ED25519_SIGNATURE_SIZE:
                    {
                        if(P2PServerUDP::p2pserver->hostToFirstReply.size()>64)
                            return;
                        //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE(node)+ED25519_SIGNATURE_SIZE(ca)+ED25519_SIGNATURE_SIZE(node)]
                        //check if the public key of node is signed by ca
                        const int rc = ed25519_sha512_verify(ca_publickey,//pub
                            ED25519_KEY_SIZE,//length
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+8+8+1),//msg
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+8+8+1+ED25519_KEY_SIZE)//signature
                        );
                        if(rc != 1)
                            return;
                        //check the message content
                        const int rc2 = ed25519_sha512_verify(
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+8+8+1),//pub
                            recv_len-ED25519_SIGNATURE_SIZE,//length
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer),//msg
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+recv_len-ED25519_SIGNATURE_SIZE)//signature
                        );
                        if(rc2 != 1)
                        {
                            /*std::cerr << "ed25519_sha512_verify failed at " << __FILE__ << ":" << std::to_string(__LINE__)
                                      << ", data: " << binarytoHexa(readOnlyReadBuffer,recv_len) << std::endl;*/
                            return;
                        }

                        //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE(node)+ED25519_SIGNATURE_SIZE(ca)+ED25519_SIGNATURE_SIZE(node)]
                        memcpy(handShake2+8,readOnlyReadBuffer,8);//copy the ACK
                        const int readSize=fread(handShake2,1,8,P2PServerUDP::p2pserver->ptr_random);
                        if(readSize != 8)
                            abort();

                        P2PPeer::sign(reinterpret_cast<uint8_t *>(handShake2),sizeof(handShake2)-ED25519_SIGNATURE_SIZE);

                        HostToFirstReply hostToFirstReplyEntry;

                        uint8_t publickey[ED25519_KEY_SIZE];
                        memcpy(&publickey,readOnlyReadBuffer+8+8+1,ED25519_KEY_SIZE);
                        uint64_t local_sequence_number_validated=0;
                        memcpy(&local_sequence_number_validated,handShake2,8);
                        uint64_t remote_sequence_number=0;
                        memcpy(&remote_sequence_number,readOnlyReadBuffer/*==handShake2+8, see above*/,8);
                        hostToFirstReplyEntry.hostConnected=new P2PPeer(publickey,local_sequence_number_validated,
                            remote_sequence_number,si_other);
                        memcpy(hostToFirstReplyEntry.random,handShake2,8);
                        hostToFirstReplyEntry.round=0;

                        memcpy(hostToFirstReplyEntry.reply,handShake2,sizeof(handShake2));

                        #ifdef CATCHCHALLENGER_EXTRACHECK
                        {
                            const int returnFirm = ed25519_sha512_verify(
                                P2PServerUDP::p2pserver->publickey,//pub
                                sizeof(handShake2)-ED25519_SIGNATURE_SIZE,//length
                                reinterpret_cast<const uint8_t *>(handShake2),//msg
                                reinterpret_cast<const uint8_t *>(handShake2+sizeof(handShake2)-ED25519_SIGNATURE_SIZE)//signature
                            );
                            if(returnFirm != 1)
                            {
                                std::cerr << "out packet with wrong signature, blocked" << std::endl;
                                abort();
                                return;
                            }
                            if(sizeof(handShake2)<(8+8+1+ED25519_SIGNATURE_SIZE))
                            {
                                std::cerr << "too small data to be valid, blocked" << std::endl;
                                abort();
                                return;
                            }
                            uint8_t messageType=0;
                            memcpy(&messageType,handShake2+8+8,1);
                            if(messageType!=0xFF && (messageType<0x01 || messageType>0x04))
                            {
                                std::cerr << "messageType wrong (" << std::to_string(messageType) << "), blocked" << std::endl;
                                abort();
                                return;
                            }
                            char randomzero[sizeof(hostToFirstReplyEntry.random)];
                            memset(randomzero,0,sizeof(randomzero));
                            if(memcmp(randomzero,hostToFirstReplyEntry.random,sizeof(randomzero))==0)
                            {
                                std::cerr << "random empty (abort)" << std::endl;
                                abort();
                                return;
                            }
                        }
                        #endif

                        auto it=hostToFirstReply.find(remoteClient);
                        if(it!=hostToFirstReply.cend())
                            P2PServerUDP::p2pserver->write(hostToFirstReplyEntry.reply,sizeof(hostToFirstReplyEntry.reply),si_other);
                        hostToFirstReply[remoteClient]=hostToFirstReplyEntry;
                    }
                    break;
                    default:
                    {
                        std::cerr << "messageType, 0x01, some problem: " << std::to_string(recv_len) << std::endl;
                    }
                    return;
                }
            }
            break;
            case 0x02:
            {
                switch(recv_len)
                {
                    //in: handShake2, out: handShake3
                    case 8+8+1+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE+ED25519_SIGNATURE_SIZE:
                    {
                        //check if the public key of node is signed by ca
                        if(P2PServerUDP::p2pserver->hostToSecondReply.size()>64)
                            return;
                        //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE(node)+ED25519_SIGNATURE_SIZE(ca)+ED25519_SIGNATURE_SIZE(node)]
                        //check if the public key of node is signed by ca
                        const int rc = ed25519_sha512_verify(ca_publickey,//pub
                            ED25519_KEY_SIZE,//length
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+8+8+1),//msg
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+8+8+1+ED25519_KEY_SIZE)//signature
                        );
                        if(rc != 1)
                        {
                            std::cerr << "ed25519_sha512_verify failed at " << __FILE__ << ":" << std::to_string(__LINE__)
                                      << ", data: " << binarytoHexa(readOnlyReadBuffer,recv_len) << std::endl;
                            return;
                        }
                        //check the message content
                        const int rc2 = ed25519_sha512_verify(
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+8+8+1),//pub
                            recv_len-ED25519_SIGNATURE_SIZE,//length
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer),//msg
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+recv_len-ED25519_SIGNATURE_SIZE)//signature
                        );
                        if(rc2 != 1)
                        {
                            /*std::cerr << "ed25519_sha512_verify failed at " << __FILE__ << ":" << std::to_string(__LINE__)
                                      << ", data: " << binarytoHexa(readOnlyReadBuffer,recv_len) << std::endl;*/
                            return;
                        }

                        uint64_t local_sequence_number_validated=0;
                        uint64_t remote_sequence_number=0;
                        uint8_t publickey[ED25519_KEY_SIZE];
                        memcpy(&local_sequence_number_validated,readOnlyReadBuffer+8,8);
                        local_sequence_number_validated++;
                        memcpy(&remote_sequence_number,readOnlyReadBuffer,8);
                        memcpy(publickey,readOnlyReadBuffer+8+8+1,ED25519_KEY_SIZE);

                        //search into the connect and check the random
                        unsigned int indexSearch=0;
                        while(indexSearch<P2PServerUDP::p2pserver->hostToConnect.size())
                        {
                            HostToConnect &hostToConnect=P2PServerUDP::p2pserver->hostToConnect.at(indexSearch);
                            if(memcmp(&hostToConnect.serv_addr,&si_other,sizeof(sockaddr_in))==0 &&
                                    memcmp(hostToConnect.random,readOnlyReadBuffer+8,8)==0)
                            {
                                //new peer
                                std::cerr << "new peer at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                P2PPeer * const newHostConnected=new P2PPeer(publickey,local_sequence_number_validated,
                                                                     remote_sequence_number,si_other);
                                P2PServerUDP::p2pserver->hostToConnect.erase(P2PServerUDP::p2pserver->hostToConnect.cbegin()+indexSearch);
                                P2PServerUDP::hostConnectionEstablished[remoteClient]=newHostConnected;
                                break;
                            }
                            indexSearch++;
                        }
                        //reemit from handShake2 only if valided connected client
                        if(indexSearch>=P2PServerUDP::p2pserver->hostToConnect.size())
                        {
                            if(P2PServerUDP::hostConnectionEstablished.find(remoteClient)==P2PServerUDP::hostConnectionEstablished.cend())
                            {
                                std::cerr << "messageType, 0x02, some problem: " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                return;
                            }
                            else
                            {
                                P2PServerUDP::hostConnectionEstablished.erase(remoteClient);

                                //new peer
                                std::cerr << "new peer at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                P2PPeer * const newHostConnected=new P2PPeer(publickey,local_sequence_number_validated,
                                                                     remote_sequence_number,si_other);
                                P2PServerUDP::hostConnectionEstablished[remoteClient]=newHostConnected;
                            }
                        }

                        P2PPeer *currentHostConnected=P2PServerUDP::hostConnectionEstablished.at(remoteClient);
                        currentHostConnected->sendDataWithMessageType(reinterpret_cast<uint8_t *>(handShake3),sizeof(handShake3));//and emit
                    }
                    break;
                    default:
                    {
                        std::cerr << "messageType, 0x02, some problem: " << std::to_string(recv_len) << std::endl;
                    }
                    return;
                }
            }
            break;
            case 0x03:
            {
                switch(recv_len)
                {
                    //in: handShake3
                    case 8+8+1+ED25519_SIGNATURE_SIZE:
                    {
                        //get valid public key from in: handShake1, out: handShake2
                        if(P2PServerUDP::hostToFirstReply.find(remoteClient)==P2PServerUDP::hostToFirstReply.cend())
                        {
                            std::cerr << "client not found into P2PServerUDP::hostToFirstReply" << std::endl;
                            return;
                        }
                        HostToFirstReply &hostToFirstReply=P2PServerUDP::hostToFirstReply.at(remoteClient);
                        //check the message content
                        const int rc2 = ed25519_sha512_verify(
                            reinterpret_cast<const uint8_t *>(hostToFirstReply.hostConnected->getPublickey()),//pub
                            recv_len-ED25519_SIGNATURE_SIZE,//length
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer),//msg
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+recv_len-ED25519_SIGNATURE_SIZE)//signature
                                                             );
                        if(rc2 != 1)
                            return;

                        if(P2PServerUDP::hostToFirstReply.find(remoteClient)!=P2PServerUDP::hostToFirstReply.cend())
                        {
                            //hostToFirstReply -> P2PServerUDP::hostConnectionEstablished, first receive
                            HostToFirstReply &hostToFirstReply=P2PServerUDP::hostToFirstReply.at(remoteClient);

                            if(memcmp(hostToFirstReply.random,readOnlyReadBuffer+8,sizeof(hostToFirstReply.random))==0)
                            {
                                std::cerr << "new peer at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                //search into the connect and remove if address is same
                                unsigned int indexSearch=0;
                                while(indexSearch<P2PServerUDP::p2pserver->hostToConnect.size())
                                {
                                    HostToConnect &hostToConnect=P2PServerUDP::p2pserver->hostToConnect.at(indexSearch);
                                    if(memcmp(&hostToConnect.serv_addr,&si_other,sizeof(sockaddr_in))==0)
                                    {
                                        //new peer
                                        std::cerr << "remove peer at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                        P2PServerUDP::p2pserver->hostToConnect.erase(P2PServerUDP::p2pserver->hostToConnect.cbegin()+indexSearch);
                                        break;
                                    }
                                    indexSearch++;
                                }
                                P2PServerUDP::hostConnectionEstablished[remoteClient]=hostToFirstReply.hostConnected;

                                P2PServerUDP::hostToFirstReply.erase(remoteClient);
                            }
                            else
                                std::cerr << "wrong random key at handcheck3 at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                            return;
                        }

                        //hostToFirstReply -> P2PServerUDP::hostConnectionEstablished, check if not first receive
                        if(P2PServerUDP::hostConnectionEstablished.find(remoteClient)!=P2PServerUDP::hostConnectionEstablished.cend())
                        {
                            //then remit the first packet not ack
                            P2PPeer * peer=P2PServerUDP::hostConnectionEstablished.at(remoteClient);
                            peer->emitAck();
                        }
                        else
                        {
                            std::cerr << "not found from step 1" << std::endl;
                        }
                    }
                    break;
                    default:
                    {
                        std::cerr << "messageType, 0x03, some problem: " << std::to_string(recv_len) << std::endl;
                    }
                    return;
                }
            }
            break;
            case 0x04:
            case 0xFF:
            {
                switch(recv_len)
                {
                    //in: ACK or data
                    case 8+8+1+ED25519_SIGNATURE_SIZE:
                    {
                        //get valid public key from in: handShake1, out: handShake2
                        if(P2PServerUDP::hostConnectionEstablished.find(remoteClient)==P2PServerUDP::hostConnectionEstablished.cend())
                            return;
                        P2PPeer *hostConnected=P2PServerUDP::hostConnectionEstablished.at(remoteClient);
                        //check the message content
                        const int rc2 = ed25519_sha512_verify(
                            reinterpret_cast<const uint8_t *>(hostConnected->getPublickey()),//pub
                            recv_len-ED25519_SIGNATURE_SIZE,//length
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer),//msg
                            reinterpret_cast<const uint8_t *>(readOnlyReadBuffer+recv_len-ED25519_SIGNATURE_SIZE)//signature
                                                             );
                        if(rc2 != 1)
                            return;

                        //load the data size
                        const uint16_t &size=recv_len-8-8-1-ED25519_SIGNATURE_SIZE;
                        //the data
                        uint64_t sequenceNumber=0;
                        memcpy(&sequenceNumber,readOnlyReadBuffer,8);
                        if(memcmp(readOnlyReadBuffer+8,&hostConnected->get_remote_sequence_number(),8)==0)
                        {
                            //flush buffer, if have more buffer send else ACK
                            uint64_t ackNumber=0;
                            memcpy(&ackNumber,readOnlyReadBuffer+8,8);
                            if(!hostConnected->discardBuffer(ackNumber))
                                return;

                            hostConnected->incremente_remote_sequence_number();
                            switch(messageType)
                            {
                                case 0x04:
                                if(!hostConnected->parseData(reinterpret_cast<const uint8_t * const>(readOnlyReadBuffer+8+8+1),size))
                                {
                                    std::cerr << "P2P peer bug !hostConnected.parseData()" << std::endl;
                                    P2PServerUDP::hostConnectionEstablished.erase(remoteClient);
                                    return;
                                }
                                break;
                                case 0xFF:
                                    if(size!=0)
                                    {
                                        std::cerr << "P2P peer missing data (disconnect)" << std::endl;
                                        P2PServerUDP::hostConnectionEstablished.erase(remoteClient);
                                        return;
                                    }
                                break;
                                default:
                                    std::cerr << "P2P peer bug not known message" << std::endl;
                                break;
                            }
                        }
                        else
                            std::cerr << "P2P peer missing packet" << std::endl;
                    }
                    break;
                    default:
                    {
                        std::cerr << "messageType, 0x04 0xFF, some problem" << std::endl;
                    }
                    return;
                }
            }
            break;
            default:
            {
                std::cerr << "messageType, not known: " << messageType << std::endl;
            }
            return;
        }
    }
    else {
        std::cerr << "messageType, size to little" << std::endl;
    }
}

int P2PServerUDP::write(const char * const data,const uint32_t dataSize,const sockaddr_in &si_other)
{
    #ifdef CATCHCHALLENGER_EXTRACHECK
    {
        const int returnFirm = ed25519_sha512_verify(
            P2PServerUDP::p2pserver->publickey,//pub
            dataSize-ED25519_SIGNATURE_SIZE,//length
            reinterpret_cast<const uint8_t *>(data),//msg
            reinterpret_cast<const uint8_t *>(data+dataSize-ED25519_SIGNATURE_SIZE)//signature
        );
        if(returnFirm != 1)
        {
            std::cerr << "out packet with wrong signature, blocked" << std::endl;
            abort();
            return -1;
        }
        if(dataSize<(8+8+1+ED25519_SIGNATURE_SIZE))
        {
            std::cerr << "too small data to be valid, blocked" << std::endl;
            abort();
            return -1;
        }
        uint8_t messageType=0;
        memcpy(&messageType,data+8+8,1);
        if(messageType!=0xFF && (messageType<0x01 || messageType>0x04))
        {
            std::cerr << "messageType wrong (" << std::to_string(messageType) << "), blocked" << std::endl;
            abort();
            return -1;
        }
    }
    #endif
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

const char * P2PServerUDP::getPublicKey() const
{
    return reinterpret_cast<const char *>(publickey);
}

const char *P2PServerUDP::getCaSignature() const
{
    return reinterpret_cast<const char *>(ca_signature);
}

const uint8_t *P2PServerUDP::get_privatekey() const
{
    return privatekey;
}

const uint8_t *P2PServerUDP::get_ca_publickey() const
{
    return ca_publickey;
}
