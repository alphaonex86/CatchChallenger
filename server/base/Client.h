#ifndef CATCHCHALLENGER_CLIENT_H
#define CATCHCHALLENGER_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QTimer>
#include <QHash>

#include "../../general/base/DebugClass.h"
#include "ServerStructures.h"
#include "ClientBroadCast.h"
#include "ClientLocalBroadcast.h"
#include "ClientHeavyLoad.h"
#include "ClientMapManagement/ClientMapManagement.h"
#include "ClientNetworkRead.h"
#include "ClientNetworkWrite.h"
#include "LocalClientHandler.h"
#include "EventThreader.h"
#include "BroadCastWithoutSender.h"
#include "../../general/base/GeneralStructures.h"
#include "../VariableServer.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnReceiver.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnReceiver.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_None.h"

namespace CatchChallenger {
class Client
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
public:
    explicit Client(ConnectedSocket *socket, ClientMapManagement *clientMapManagement);
    ~Client();
    //to get some info
    QString getPseudo();
private:
    bool ask_is_ready_to_stop;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    quint8 stopped_object;
    #endif
    ConnectedSocket *socket;
    //-------------------
#ifndef EPOLLCATCHCHALLENGERSERVER
private:
#else
public:
#endif
    Player_internal_informations player_informations;
    ClientBroadCast clientBroadCast;
    ClientHeavyLoad clientHeavyLoad;
    ClientNetworkRead clientNetworkRead;
    ClientNetworkWrite clientNetworkWrite;
    LocalClientHandler localClientHandler;
    ClientLocalBroadcast clientLocalBroadcast;
    ClientMapManagement *clientMapManagement;
#ifndef EPOLLCATCHCHALLENGERSERVER
private slots:
#else
public:
#endif
    //socket related
    void connectionError(QAbstractSocket::SocketError);
    //normal management related
    void errorOutput(const QString &errorString);
    void kick();
    void normalOutput(const QString &message);
    //internal management related
    void send_player_informations();
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //remove and stop related
    void disconnectNextStep();
    #endif
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
    //remove and stop related
    void askIfIsReadyToStop() const;
    void isReadyToDelete() const;

    //to async the message
    void send_fakeLogin(quint32 last_fake_player_id,quint16 x,quint16 y,Map_server_MapVisibility_Simple_StoreOnReceiver *map,Orientation orientation,QString skin) const;
    void fake_send_data(const QByteArray &data) const;
    void fake_send_received_data(const QByteArray &data) const;
    void try_ask_stop() const;
    void kicked(const QHostAddress &host);
#endif
public:
    void disconnectClient();
    #ifndef EPOLLCATCHCHALLENGERSERVER
    /// \warning it need be complete protocol trame
    void fake_receive_data(QByteArray data);
    #endif
};
}

#endif // CLIENT_H
