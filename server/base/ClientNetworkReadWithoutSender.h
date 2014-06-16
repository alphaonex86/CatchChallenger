#ifndef CATCHCHALLENGER_CLIENTNETWORKREADWITHOUTSENDER_H
#define CATCHCHALLENGER_CLIENTNETWORKREADWITHOUTSENDER_H

#include <QList>
#include <QObject>

namespace CatchChallenger {
class ClientNetworkRead;

class ClientNetworkReadWithoutSender
{
public:
    static ClientNetworkReadWithoutSender clientNetworkReadWithoutSender;
public:
    void doDDOSAction();
};
}

#endif // CLIENTNETWORKREAD_H
