#ifndef CATCHCHALLENGER_CLIENTNETWORKWRITE_H
#define CATCHCHALLENGER_CLIENTNETWORKWRITE_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

#include "ServerStructures.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

namespace CatchChallenger {
#ifdef EPOLLCATCHCHALLENGERSERVER
class Client;
#endif
class ClientNetworkWrite : public ProtocolParsingOutput
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
public:
    explicit ClientNetworkWrite(Player_internal_informations *player_informations,ConnectedSocket * socket);
    ~ClientNetworkWrite();
#ifdef EPOLLCATCHCHALLENGERSERVER
    Client *client;
#endif
public:
    void sendFullPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data);
    void sendPacket(const quint8 &mainIdent,const QByteArray &data);
    void sendRawSmallPacket(const QByteArray &data);

    void sendQuery(const quint8 &mainIdent,const quint16 &subIdent,const quint8 &queryNumber,const QByteArray &data);
    //send reply
    void postReply(const quint8 &queryNumber,const QByteArray &data);
    //normal slots
    void askIfIsReadyToStop();
private:
    ConnectedSocket * socket;
    Player_internal_informations *player_informations;
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
    void isReadyToStop() const;
#endif
};
}

#endif // CLIENTNETWORKWRITE_H
