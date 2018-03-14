#ifndef BASECLASSSWITCH_H
#define BASECLASSSWITCH_H

#include <stdint.h>

class BaseClassSwitch
{
public:
    virtual ~BaseClassSwitch() {}
    enum EpollObjectType : uint8_t
    {
        Server,
        UnixServer,
        Client,
        UnixClient,
        Timer,
        Database,
        MasterLink,
        #ifdef CATCHCHALLENGER_CLASS_GATEWAY
        ClientServer,
        #endif
    };
    virtual EpollObjectType getType() const = 0;
};

#endif // BASECLASSSWITCH_H
