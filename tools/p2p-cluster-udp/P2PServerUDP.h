#ifndef EPOLLSERVERSTATS_H
#define EPOLLSERVERSTATS_H

#include "../../server/epoll/BaseClassSwitch.h"

#include <netinet/in.h>
#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <nettle/eddsa.h>
#include "P2PPeer.h"

namespace CatchChallenger {
class P2PServerUDP : public BaseClassSwitch
{
public:
    P2PServerUDP(uint8_t *privatekey/*ED25519_KEY_SIZE*/, uint8_t *ca_publickey/*ED25519_KEY_SIZE*/, uint8_t *ca_signature/*ED25519_SIGNATURE_SIZE*/);
    ~P2PServerUDP();
    bool tryListen(const uint16_t &port);
    void read();
    static std::string sockSerialised(const sockaddr_in &si_other);
    int write(const char * const data, const uint32_t dataSize, const sockaddr_in &si_other);
    EpollObjectType getType() const;
    char * getPublicKey();
    char * getCaSignature();

    static char readBuffer[4096];

    std::unordered_map<std::string/*sockaddr_in serv_addr;*/,P2PPeer *> hostConnectionEstablished;

    struct HostToSecondReply {
        uint8_t round;
        char reply[8+2+1+8+ED25519_SIGNATURE_SIZE];
        P2PPeer *hostConnected;
    };
    std::unordered_map<std::string/*sockaddr_in serv_addr;*/,HostToSecondReply> hostToSecondReply;
    size_t hostToSecondReplyIndex;

    struct HostToFirstReply {
        uint8_t round;//if timeout, remove from connect
        char random[8];
        char reply[8+2+1+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE];
        P2PPeer *hostConnected;
    };
    std::unordered_map<std::string/*sockaddr_in serv_addr;*/,HostToFirstReply> hostToFirstReply;
    size_t hostToFirstReplyIndex;

    struct HostToConnect {
        uint8_t round;
        sockaddr_in serv_addr;
        char random[8];
    };
    std::vector<HostToConnect> hostToConnect;
    size_t hostToConnectIndex;
    static P2PServerUDP *p2pserver;
    FILE *ptr_random;
private:
    int sfd;

    uint8_t privatekey[ED25519_KEY_SIZE];
    uint8_t publickey[ED25519_KEY_SIZE];
    uint8_t ca_publickey[ED25519_KEY_SIZE];
    uint8_t ca_signature[ED25519_SIGNATURE_SIZE];

    //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE(node)+ED25519_SIGNATURE_SIZE(ca)+ED25519_SIGNATURE_SIZE(node)]
    static char handShake2[8+8+1+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE];
    //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_SIGNATURE_SIZE(node)]
    static char handShake3[8+8+1/*+ED25519_SIGNATURE_SIZE, passed to derived function*/];
};
}

#endif // EPOLLSERVERSTATS_H
