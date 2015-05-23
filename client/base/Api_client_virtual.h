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
#include "Api_protocol.h"

namespace CatchChallenger {
class Api_client_virtual : public Api_protocol
{
public:
    explicit Api_client_virtual(ConnectedSocket *socket,const QString &forcedDatapack,const QString &mainDatapackCode,const QString &subDatapackCode);
    ~Api_client_virtual();
    void sendDatapackContentBase();
    void tryDisconnect();
    virtual QString datapackPathBase() const;
    virtual QString datapackPathMain() const;
    virtual QString datapackPathSub() const;
    QString mainDatapackCode() const;
    QString subDatapackCode() const;
protected:
    //general data
    void defineMaxPlayers(const quint16 &);
private:
    QString forcedDatapackBase;
    QString forcedDatapackMain;
    QString forcedDatapackSub;
    QString mMainDatapackCode;
    QString mSubDatapackCode;
};
}

#endif // Protocol_and_virtual_data_H
