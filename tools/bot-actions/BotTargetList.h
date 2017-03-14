#ifndef BOTTARGETLIST_H
#define BOTTARGETLIST_H

#include <QDialog>
#include <QHash>
#include <QListWidgetItem>

#include "../bot/MultipleBotConnection.h"
#include "../bot/actions/ActionsAction.h"
#include "WaitScreen.h"

namespace Ui {
class BotTargetList;
}

bool operator==(const CatchChallenger::MapCondition& lhs, const CatchChallenger::MapCondition& rhs);

class BotTargetList : public QDialog
{
    Q_OBJECT

public:
    explicit BotTargetList(QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient,
    QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient,
    QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient,
    ActionsAction *actionsAction);
    ~BotTargetList();

    struct SimplifiedMapForPathFinding
    {
        struct PathToGo
        {
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > left;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > right;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > top;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > bottom;
        };
        std::unordered_map<std::pair<uint8_t,uint8_t>,PathToGo,pairhash> pathToGo;
        std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> pointQueued;
    };
    struct DestinationForPath
    {
        CatchChallenger::Orientation destination_orientation;
        uint8_t destination_x;
        uint8_t destination_y;
    };

    void loadAllBotsInformation();
    void loadAllBotsInformation2();
    void updateLayerElements();
    void updateMapInformation();
    void updateMapContent();
    void updatePlayerInformation();
    void updatePlayerMapSlot();
    void updatePlayerMap(const bool &force=false);
    void updatePlayerStep();
    void startPlayerMove();
    static bool nextZoneIsAccessible(const CatchChallenger::Api_protocol *api, const MapServerMini::BlockObject * const blockObject);
    std::vector<std::string> contentToGUI(const MapServerMini::BlockObject * const blockObject,const MultipleBotConnection::CatchChallengerClient * const client, QListWidget *listGUI=NULL);
    std::vector<std::string> contentToGUI(const MultipleBotConnection::CatchChallengerClient * const client, QListWidget *listGUI, const std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> &resolvedBlockList, const bool &displayTooHard, bool dirt, bool itemOnMap, bool fight, bool shop, bool heal, bool wildMonster);
    std::string graphStepNearMap(const MultipleBotConnection::CatchChallengerClient * const client,const MapServerMini::BlockObject * const currentNearBlock, const unsigned int &depth=2);
    std::string graphLocalMap();
    std::pair<uint8_t,uint8_t> getNextPosition(const MapServerMini::BlockObject * const blockObject,ActionsBotInterface::GlobalTarget &target);
    /*if normal, then just go
     * else if dirt like: is linked as idenpendant tile */
    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > pathFinding(const MapServerMini::BlockObject * const source_blockObject,
            const CatchChallenger::Orientation &source_orientation, const uint8_t &source_x, const uint8_t &source_y,
            /*const MapServerMini::BlockObject * const destination_blockObject,the block link to the multi-map change*/
            const std::vector<DestinationForPath> &destinations,
            unsigned int &destinationIndexSelected,
            bool *ok);
    static std::string stepToString(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath);
    static uint32_t getSeedToPlant(CatchChallenger::Api_protocol * api, bool *haveSeedToPlant=NULL);
signals:
    void start_preload_the_map();
private slots:
    void on_bots_itemSelectionChanged();
    void on_comboBox_Layer_activated(int index);
    void on_localTargets_itemActivated(QListWidgetItem *item);
    void on_comboBoxStep_currentIndexChanged(int index);
    void on_browseMap_clicked();
    void on_searchDeep_editingFinished();
    void on_globalTargets_itemActivated(QListWidgetItem *item);
    void on_tooHard_clicked();
    void on_trackThePlayer_clicked();
    void on_autoSelectTarget_toggled(bool checked);
    void on_autoSelectFilter_clicked();
    void on_hideTooHard_toggled(bool checked);

private:
    Ui::BotTargetList *ui;
    QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient;
    QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient;
    QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient;
    ActionsAction *actionsAction;
    QHash<QString,MultipleBotConnection::CatchChallengerClient *> pseudoToBot;

    bool botsInformationLoaded;
    uint32_t mapId;
    WaitScreen waitScreen;

    uint8_t updateMapContentX;
    uint8_t updateMapContentY;
    uint32_t updateMapContentMapId;
    CatchChallenger::Direction updateMapContentDirection;

    bool dirt,itemOnMap,fight,shop,heal,wildMonster;

    std::vector<uint32_t> mapIdListLocalTarget;
    std::vector<ActionsBotInterface::GlobalTarget> targetListGlobalTarget;
    bool alternateColor;
    static std::string pathFindingToString(const MapServerMini::BlockObjectPathFinding &resolvedBlock, unsigned int points=0);
    static bool isSame(const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monstersCollisionContentA,const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monstersCollisionContentB);
    static bool newPathIsBetterPath(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &oldPath,const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &newPath);
    static uint16_t pathTileCount(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &path);
    static std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > selectTheBetterPathToDestination(std::unordered_map<unsigned int,
                                                 std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> >
                                                 > pathToEachDestinations,unsigned int &destinationIndexSelected);

    //std::unodered_map<const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent,un> monsterCollisionContentDuplicate;
};

#endif // BOTTARGETLIST_H
