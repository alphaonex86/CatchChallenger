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

P2PTimerConnect::P2PTimerConnect()
{
    setInterval(100);

    //[8(sequence number)+4(size)+1(request type)+8(random)+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE]
    uint64_t sequencenumber=0;
    memcpy(handShake1,&sequencenumber,sizeof(sequencenumber));
    const uint32_t size=htole16(1+8);
    memcpy(handShake1+8,&size,sizeof(size));
    const uint8_t requestType=1;
    memcpy(handShake1+8+4,&requestType,sizeof(requestType));
    memset(handShake1+8+4+1,0,8+ED25519_SIGNATURE_SIZE);
    memcpy(handShake1+8+4+1+8+ED25519_SIGNATURE_SIZE,P2PServerUDP::p2pserver->getPublicKey(),ED25519_KEY_SIZE);
    memcpy(handShake1+8+4+1+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE,P2PServerUDP::p2pserver->getCaSignature(),ED25519_SIGNATURE_SIZE);
}

void P2PTimerConnect::exec()
{
    if(P2PServerUDP::hostToConnect.empty())
        return;
    {
        const std::chrono::time_point<std::chrono::steady_clock> end=std::chrono::steady_clock::now();
        if(end<start)
            start=end;
        else if(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()<5000)
            return;
    }
    if(P2PServerUDP::hostToConnectIndex>=P2PServerUDP::hostToConnect.size())
        P2PServerUDP::hostToConnectIndex=0;
    if(P2PServerUDP::hostToConnectIndex==0)
        start=std::chrono::steady_clock::now();
    size_t lastScannedIndex=P2PServerUDP::hostToConnectIndex;
    do
    {
        const P2PServerUDP::HostToConnect &peerToConnect=P2PServerUDP::hostToConnect.at(lastScannedIndex);
        lastScannedIndex++;
        if(lastScannedIndex>=P2PServerUDP::hostToConnect.size())
            lastScannedIndex=0;
        if(peerToConnect.hostStatus==P2PServerUDP::HostStatus::NoHandShakeValidated)
        {
            const std::string host=peerToConnect.first;
            const char * const hostC=host.c_str();
            const uint16_t portBigEndian=htons(peerToConnect.second);

            sockaddr_in serv_addr;
            memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = portBigEndian;
            const int convertResult=inet_pton(AF_INET6,hostC,&serv_addr.sin_addr);
            if(convertResult!=1)
            {
                const int convertResult=inet_pton(AF_INET,hostC,&serv_addr.sin_addr);
                if(convertResult!=1)
                {
                    std::cerr << "not IPv4 and IPv6 address (abort), errno: " << std::to_string(errno) << std::endl;
                    return;
                }
            }

            //[8(sequence number)+4(size)+1(request type)+8(random)+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE]
            const int readSize=fread(handShake1+8+4+1,1,8,P2PServerUDP::p2pserver->ptr_random);
            if(readSize != 8)
                abort();
            P2PServerUDP::p2pserver->ed25519_sha512_sign(8+4+1+8,reinterpret_cast<const uint8_t *>(handShake1),handShake1+8+4+1+8);
            P2PServerUDP::p2pserver->write(handShake1,serv_addr);

            P2PServerUDP::hostToConnectIndex=lastScannedIndex;
            return;
        }
    } while(lastScannedIndex!=P2PServerUDP::hostToConnectIndex);

    /*//first time
    P2PServerUDP::p2pserver.write("publique key, firmed by master",serv_addr);
    P2PServerUDP::p2pserver.write("request, firmed by local",serv_addr);

    //send neighbour, second time
    firm local(sequence number (32Bits) + data size + data)*/

    std::cout << "P2PTimerConnect::exec()" << std::endl;
}
