#include <QString>
#include <QTimer>
#include <QSettings>

#include "QtServer.hpp"

#ifndef CATCHCHALLENGER_INTERNALSERVER_H
#define CATCHCHALLENGER_INTERNALSERVER_H

namespace CatchChallenger {
class InternalServer : public QtServer
{
    Q_OBJECT
public:
    /// \param settings ref is destroyed after this call
    explicit InternalServer();
    virtual ~InternalServer();
    virtual void run();
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
    QSettings settings;
};
}

#endif
