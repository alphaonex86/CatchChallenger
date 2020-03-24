#ifndef CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H
#define CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QObject>
#endif
#include <vector>
#include "../VariableServer.hpp"

namespace CatchChallenger {
class LocalClientHandlerWithoutSender
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
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
