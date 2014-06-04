#ifndef CATCHCHALLENGER_CLIENTNETWORKREADWITHOUTSENDER_H
#define CATCHCHALLENGER_CLIENTNETWORKREADWITHOUTSENDER_H

#include <QList>
#include <QObject>

namespace CatchChallenger {
class ClientNetworkRead;

class ClientNetworkReadWithoutSender
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
public:
    static ClientNetworkReadWithoutSender clientNetworkReadWithoutSender;
    QList<ClientNetworkRead *> clientList;
public:
    void doDDOSAction();
};
}

#endif // CLIENTNETWORKREAD_H
