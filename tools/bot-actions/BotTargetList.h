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
    void updateLayerElements();
private slots:
    void on_bots_itemSelectionChanged();
    void on_comboBox_Layer_activated(int index);
private:
    Ui::BotTargetList *ui;
    QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient;
    QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient;
    QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient;
    ActionsAction *actionsAction;
    QHash<QString,MultipleBotConnection::CatchChallengerClient *> pseudoToBot;
    QHash<QString,int> zoneHash;
    QList<QString> layerList;

    bool botsInformationLoaded;
};

#endif // BOTTARGETLIST_H
