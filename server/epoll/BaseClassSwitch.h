#ifndef BASECLASSSWITCH_H
#define BASECLASSSWITCH_H

#include <stdint.h>

class BaseClassSwitch
{
public:
    virtual ~BaseClassSwitch() {}
    enum EpollObjectType : uint8_t
    {
        #ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
            Server,
            Client,
            Timer,
            Database,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            Server,
            Client,
            Timer,
            Database,
            MasterLink,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_LOGIN
            Server,
            Client,
            Timer,
            Database,
            MasterLink,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_MASTER
            Server,
            Client,
            Timer,
            Database,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_GATEWAY
            Server,
            Client,
            Timer,
            ClientServer,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_STATS
            Client,
            UnixServer,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_QT
            Database,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_P2PCLUSTER
            ServerP2P,
            Timer,
            Stdin
        #endif
    };
    virtual EpollObjectType getType() const = 0;
};

#endif // BASECLASSSWITCH_H
