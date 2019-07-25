#include <QString>
#include <QSettings>
#include <QTimer>

#include "../../../server/base/ServerStructures.h"
#include "../../../server/base/Client.h"
#include "../../../general/base/Map_loader.h"
#include "../../../general/base/ProtocolParsing.h"
#include "../QFakeServer.h"
#include "../QFakeSocket.h"
#include "../../../server/base/MapServer.h"
#include "../../../server/base/QtServer.h"

#ifndef CATCHCHALLENGER_INTERNALSERVER_H
#define CATCHCHALLENGER_INTERNALSERVER_H

namespace CatchChallenger {
class InternalServer : public QtServer
{
    Q_OBJECT
public:
    /// \param settings ref is destroyed after this call
    explicit InternalServer(QSettings &settings);
    virtual ~InternalServer();
private slots:
    //starting function
    void start_internal_server();
    void timerGiftSlot();
protected:
    QString sqlitePath();
protected slots:
    //remove all finished client
    virtual void removeOneClient();
    virtual void serverIsReady();
private:
    QTimer timerGift;
};
}

#endif
