#ifndef CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H
#define CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H

#include <QObject>

namespace CatchChallenger {
class LocalClientHandlerWithoutSender
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
public:
    explicit LocalClientHandlerWithoutSender();
    virtual ~LocalClientHandlerWithoutSender();
    static LocalClientHandlerWithoutSender localClientHandlerWithoutSender;
    QList<void*> allClient;
public:
    void doAllAction();
    void doDDOSAction();
};
}

#endif
