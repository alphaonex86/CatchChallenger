#include "P2PPeer.h"

P2PPeer::P2PPeer(const uint8_t * const publickey, const uint64_t &local_sequence_number,const uint64_t &remote_sequence_number)
{
    memcpy(this->publickey,publickey,sizeof(publickey));
    this->local_sequence_number=local_sequence_number;
    this->remote_sequence_number=remote_sequence_number;
}

void P2PPeer::emitAck()
{
    to write
}

bool P2PPeer::sendData(const uint8_t * const data, const uint16_t &size)
{
    to write
}
