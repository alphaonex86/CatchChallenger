#include <QObject>
#include <QSettings>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <QList>
#include <QByteArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDir>
#include <QSemaphore>
#include <QString>

#include "../../../general/base/DebugClass.h"
#include "../../../server/base/ServerStructures.h"
#include "../../../server/base/Client.h"
#include "../../../general/base/Map_loader.h"
#include "../../../general/base/ProtocolParsing.h"
#include "../../../general/base/QFakeServer.h"
#include "../../../general/base/QFakeSocket.h"
#include "../../../server/base/MapServer.h"
#include "../../../server/base/QtServer.h"

#ifndef CATCHCHALLENGER_INTERNALSERVER_H
#define CATCHCHALLENGER_INTERNALSERVER_H

namespace CatchChallenger {
class InternalServer : public QtServer
{
    Q_OBJECT
public:
    explicit InternalServer();
    virtual ~InternalServer();
private slots:
    //starting function
    void start_internal_server();
protected:
    QString sqlitePath();
protected slots:
    //remove all finished client
    virtual void removeOneClient();
};
}

#endif
