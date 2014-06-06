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
#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/ConnectedSocket.h"
#include "../../general/base/Api_protocol.h"

namespace CatchChallenger {
class Api_client_virtual : public Api_protocol
{
public:
    explicit Api_client_virtual(ConnectedSocket *socket,const QString &forcedDatapack);
    ~Api_client_virtual();
    void sendDatapackContent();
    void tryDisconnect();
    virtual QString datapackPath() const;
protected:
    //general data
    void defineMaxPlayers(const quint16 &);
private:
    QString forcedDatapack;
};
}

#endif // Protocol_and_virtual_data_H
