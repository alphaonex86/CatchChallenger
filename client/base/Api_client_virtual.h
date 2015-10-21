#ifndef CATCHCHALLENGER_API_CLIENT_VIRTUAL_H
#define CATCHCHALLENGER_API_CLIENT_VIRTUAL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QTcpSocket>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>

#include "../../general/base/QFakeSocket.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/ConnectedSocket.h"
#include "Api_protocol.h"

namespace CatchChallenger {
class Api_client_virtual : public Api_protocol
{
public:
    explicit Api_client_virtual(ConnectedSocket *socket);
    ~Api_client_virtual();
    void sendDatapackContentBase();
    void sendDatapackContentMainSub();
    void tryDisconnect();
protected:
    //general data
    void defineMaxPlayers(const uint16_t &);
};
}

#endif // Protocol_and_virtual_data_H
