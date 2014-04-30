#ifndef CATCHCHALLENGER_CLIENTNETWORKREADWITHOUTSENDER_H
#define CATCHCHALLENGER_CLIENTNETWORKREADWITHOUTSENDER_H

#include <QList>
#include <QObject>

namespace CatchChallenger {
class ClientNetworkRead;

class ClientNetworkReadWithoutSender : public QObject
{
    Q_OBJECT
public:
    static ClientNetworkReadWithoutSender clientNetworkReadWithoutSender;
    QList<ClientNetworkRead *> clientList;
public slots:
    void doDDOSAction();
};
}

#endif // CLIENTNETWORKREAD_H
