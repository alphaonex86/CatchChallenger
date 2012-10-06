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

#include "../../general/base/DebugClass.h"
#include "../../server/base/ServerStructures.h"
#include "../../server/base/Client.h"
#include "../../general/base/Map_loader.h"
#include "../../server/base/Bot/FakeBot.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/QFakeServer.h"
#include "../../general/base/QFakeSocket.h"
#include "../../server/base/Map_server.h"
#include "../../server/base/BaseServer.h"

#ifndef POKECRAFT_INTERNALSERVER_H
#define POKECRAFT_INTERNALSERVER_H

namespace Pokecraft {
class InternalServer : public BaseServer
{
    Q_OBJECT
public:
    explicit InternalServer(const QString &dbPath);
    virtual ~InternalServer();
private:
    EventThreader thread;
    QString dbPath;
private slots:
    //starting function
    void start_internal_server();
protected:
    QString sqlitePath();
};
}

#endif
