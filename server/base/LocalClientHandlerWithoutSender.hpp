#ifndef CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H
#define CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H

#include <vector>
#include "VariableServer.hpp"

namespace CatchChallenger {
class LocalClientHandlerWithoutSender
{
public:
    explicit LocalClientHandlerWithoutSender();
    virtual ~LocalClientHandlerWithoutSender();
    static LocalClientHandlerWithoutSender localClientHandlerWithoutSender;
    std::vector<void*> allClient;
public:
    void doAllAction();
    void doDDOSChat();
};
}

#endif
