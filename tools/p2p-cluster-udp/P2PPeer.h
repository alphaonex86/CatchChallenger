#ifndef P2PPEER_H
#define P2PPEER_H

#include <nettle/eddsa.h>
#include <stdint.h>

#include "HostConnected.h"

class P2PPeer : public HostConnected
{
public:
    P2PPeer(const uint8_t * const publickey, const uint64_t &local_sequence_number,const uint64_t &remote_sequence_number);
public:
    HostConnected();
    void emitAck();
    bool sendData(const uint8_t * const data, const uint16_t &size);
private:
    uint8_t publickey[ED25519_KEY_SIZE];
    uint64_t local_sequence_number;
    uint64_t remote_sequence_number;
};

#endif // P2PPEER_H
