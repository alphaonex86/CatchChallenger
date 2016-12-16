#ifndef BOTTARGETLIST_H
#define BOTTARGETLIST_H

#include <QDialog>
#include <QHash>

#include "../bot/MultipleBotConnection.h"
#include "../bot/actions/ActionsAction.h"

namespace Ui {
class BotTargetList;
}

class BotTargetList : public QDialog
{
    Q_OBJECT

public:
    explicit BotTargetList(QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient,
    QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient,
    QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient,
    ActionsAction *actionsAction);
    ~BotTargetList();
    void loadAllBotsInformation();

private:
    Ui::BotTargetList *ui;
    QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient;
    QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient;
    QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient;
    ActionsAction *actionsAction;

    bool botsInformationLoaded;
};

#endif // BOTTARGETLIST_H
