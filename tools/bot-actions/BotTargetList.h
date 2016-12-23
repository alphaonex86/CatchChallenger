#ifndef BOTTARGETLIST_H
#define BOTTARGETLIST_H

#include <QDialog>
#include <QHash>
#include <QListWidgetItem>

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
    void updateMapInformation();
private slots:
    void on_bots_itemSelectionChanged();
    void on_comboBox_Layer_activated(int index);
    void on_localTargets_itemActivated(QListWidgetItem *item);

    void on_comboBoxStep_currentIndexChanged(int index);

private:
    Ui::BotTargetList *ui;
    QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient;
    QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient;
    QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient;
    ActionsAction *actionsAction;
    QHash<QString,MultipleBotConnection::CatchChallengerClient *> pseudoToBot;

    bool botsInformationLoaded;
    uint32_t mapId;
    std::vector<uint32_t> mapIdList;
};

#endif // BOTTARGETLIST_H
