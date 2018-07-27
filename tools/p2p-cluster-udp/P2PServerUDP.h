#ifndef EPOLLSERVERSTATS_H
#define EPOLLSERVERSTATS_H

#include "../../server/epoll/BaseClassSwitch.h"

#include <netinet/in.h>
#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <nettle/eddsa.h>
#include "HostConnected.h"

class P2PServerUDP : public BaseClassSwitch
{
public:
    P2PServerUDP();
    ~P2PServerUDP();
    bool tryListen(const uint16_t &port);
    void read();
    static std::string sockSerialised(const sockaddr_in &si_other);
    int write(const char * const data, const uint32_t dataSize, const sockaddr_in &si_other);
    EpollObjectType getType() const;
    const char *getPublicKey() const;
    const char * getCaSignature() const;

    std::unordered_map<std::string/*sockaddr_in serv_addr;*/,HostConnected *> hostConnectionEstablished;

    struct HostToSecondReply {
        uint8_t round;
        char reply[8+8+1+ED25519_SIGNATURE_SIZE];
        HostConnected *hostConnected;
    };
    std::unordered_map<std::string/*sockaddr_in serv_addr;*/,HostToSecondReply> hostToSecondReply;

    struct HostToFirstReply {
        uint8_t round;//if timeout, remove from connect
        char random[8];
        char reply[8+8+1+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE+ED25519_SIGNATURE_SIZE];
        HostConnected *hostConnected;
    };
    std::unordered_map<std::string/*sockaddr_in serv_addr;*/,HostToFirstReply> hostToFirstReply;

    struct HostToConnect {
        uint8_t round;
        sockaddr_in serv_addr;
        char random[8];
        std::string serialised_serv_addr;/*sockaddr_in serv_addr;*/
    };
    std::vector<HostToConnect> hostToConnect;
    size_t hostToConnectIndex;
    static P2PServerUDP *p2pserver;
    FILE *ptr_random;

    const uint8_t *get_privatekey() const;
    const uint8_t *get_ca_publickey() const;
protected:
    bool setKey(uint8_t *privatekey/*ED25519_KEY_SIZE*/, uint8_t *ca_publickey/*ED25519_KEY_SIZE*/, uint8_t *ca_signature/*ED25519_SIGNATURE_SIZE*/);
    virtual bool newPeer(const sockaddr_in &si_other) = 0;
private:
    int sfd;
    static char readBuffer[4096];

    uint8_t privatekey[ED25519_KEY_SIZE];
    uint8_t publickey[ED25519_KEY_SIZE];
    uint8_t ca_publickey[ED25519_KEY_SIZE];
    uint8_t ca_signature[ED25519_SIGNATURE_SIZE];

    //[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_KEY_SIZE(node)+ED25519_SIGNATURE_SIZE(ca)+ED25519_SIGNATURE_SIZE(node)]
    static char handShake2[8+8+1+ED25519_KEY_SIZE+ED25519_SIGNATURE_SIZE+ED25519_SIGNATURE_SIZE];
    //8+8 -> managed by class, [1(request type)+ED25519_SIGNATURE_SIZE]
    static char handShake3[1/*+ED25519_SIGNATURE_SIZE, passed to derived function*/];
};

#endif // EPOLLSERVERSTATS_H
