#ifndef EPOLLSERVERSTATS_H
#define EPOLLSERVERSTATS_H

#include "../../server/epoll/BaseClassSwitch.h"

#include <netinet/in.h>
#include <vector>
#include <string>
#include <utility>
#include <nettle/eddsa.h>

namespace CatchChallenger {
class P2PServerUDP : public BaseClassSwitch
{
public:
    P2PServerUDP(uint8_t *privatekey/*ED25519_KEY_SIZE*/, uint8_t *ca_publickey/*ED25519_KEY_SIZE*/, uint8_t *ca_signature/*ED25519_SIGNATURE_SIZE*/);
    ~P2PServerUDP();
    bool tryListen(const uint16_t &port);
    void read();
    int write(const std::string &data,const sockaddr_in &si_other);
    EpollObjectType getType() const;
    void ed25519_sha512_sign(size_t length, const uint8_t *msg,uint8_t *signature);
    char * getPublicKey();
    char * getCaSignature();

    static char readBuffer[4096];
    enum HostStatus {
        NoHandShakeValidated,
        HandShake1Validated,
        HandShake2Validated,
        HandShake3Validated,
        HandShake4Validated,
    };
    struct HostToConnect {
        std::string host;
        uint16_t port;
        uint8_t round;
        sockaddr_in serv_addr;
        HostStatus hostStatus;
        uint8_t publickey[ED25519_KEY_SIZE];
        uint64_t local_sequence_number;
        uint64_t remote_sequence_number;
    };
    static std::vector<HostToConnect> hostToConnect;
    static size_t hostToConnectIndex;
    static P2PServerUDP *p2pserver;
    FILE *ptr_random;
private:
    int sfd;

    uint8_t privatekey[ED25519_KEY_SIZE];
    uint8_t publickey[ED25519_KEY_SIZE];
    uint8_t ca_publickey[ED25519_KEY_SIZE];
    uint8_t ca_signature[ED25519_SIGNATURE_SIZE];

    //[8(sequence number)+4(size)+1(request type)+8(random from 1)+8(random for 2)+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE]
    static char handShake2[8+4+1+8+8+ED25519_SIGNATURE_SIZE+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE];

    //[8(sequence number)+4(size)+1(request type)+ED25519_SIGNATURE_SIZE]
    static char handShake4[8+4+1+8+8+ED25519_SIGNATURE_SIZE];
};
}

#endif // EPOLLSERVERSTATS_H
