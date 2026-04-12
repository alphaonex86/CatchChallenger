#ifndef CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H
#define CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H

#include <vector>
#include <cstdint>

namespace CatchChallenger {
class LocalClientHandlerWithoutSender
{
public:
    explicit LocalClientHandlerWithoutSender();
    virtual ~LocalClientHandlerWithoutSender();
    static LocalClientHandlerWithoutSender localClientHandlerWithoutSender;
public:
    void doAllAction();
    void doDDOSChat();
};
}

#endif
