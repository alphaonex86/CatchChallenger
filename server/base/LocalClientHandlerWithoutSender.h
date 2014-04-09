#ifndef CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H
#define CATCHCHALLENGER_LOCALCLIENBTHANDLERWITHOUTSENDER_H

#include <QObject>

namespace CatchChallenger {
class LocalClientHandlerWithoutSender : public QObject
{
    Q_OBJECT
public:
    explicit LocalClientHandlerWithoutSender();
    virtual ~LocalClientHandlerWithoutSender();
    static LocalClientHandlerWithoutSender localClientHandlerWithoutSender;
    QList<void*> allClient;
public slots:
    void doAllAction();
};
}

#endif
