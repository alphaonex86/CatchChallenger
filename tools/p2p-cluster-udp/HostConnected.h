#ifndef HostConnected_H
#define HostConnected_H

#include "P2PPeer.h"

class HostConnected : public P2PPeer
{
public:
    HostConnected(const uint8_t * const publickey, const uint64_t &local_sequence_number_validated,
                  const uint64_t &remote_sequence_number,const sockaddr_in6 &si_other);
    ~HostConnected();
    bool parseData(const uint8_t * const data, const uint16_t &size);
};

#endif // HostConnected_H
