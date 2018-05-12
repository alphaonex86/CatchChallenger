#ifndef HostConnected_H
#define HostConnected_H

#include <stdint.h>

class HostConnected
{
public:
    HostConnected();
    bool parseData(const uint8_t * const data, const uint16_t &size);
    virtual bool sendData(const uint8_t * const data, const uint16_t &size) = 0;
};

#endif // HostConnected_H
