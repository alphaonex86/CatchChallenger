//to have winsock2 include before Windows.h, prevent mingw warn
#include "../Api_client_real.h"

#ifndef CATCHCHALLENGER_NOAUDIO
#include <vlc/vlc.h>
#endif
#include <QWidget>
#include <QMessageBox>
#include <QAbstractSocket>
#include <QSettings>
#include <QTimer>
#include <QTime>
#include <QListWidgetItem>
#include <QFrame>
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QMovie>
#include <QQuickView>
#include <QTreeWidgetItem>
#include <unordered_set>
#include <set>
#include <map>

#include "../../crafting/interface/QmlInterface/CraftingAnimation.h"
#include "../../../general/base/ChatParsing.h"
#include "../../../general/base/GeneralStructures.h"
#include "../Api_protocol.h"
#include "MapController.h"
#include "Chat.h"
#include "NewProfile.h"
#include "../QmlInterface/AnimationControl.h"
#include "../../fight/interface/QmlInterface/QmlMonsterGeneralInformations.h"
#include "../../fight/interface/QmlInterface/EvolutionControl.h"

#ifndef CATCHCHALLENGER_BASEWINDOW_H
#define CATCHCHALLENGER_BASEWINDOW_H

namespace Ui {
    class BaseWindowUI;
}

namespace CatchChallenger {
class BaseWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BaseWindow();
    ~BaseWindow();
    static BaseWindow* baseWindow;
    void setMultiPlayer(bool multiplayer);
    void resetAll();
    void serverIsLoading();
    void serverIsReady();
    QString lastLocation() const;
    std::unordered_map<uint16_t, PlayerQuest> getQuests() const;
    uint8_t getActualBotId() const;
    bool haveNextStepQuestRequirements(const Quest &quest) const;
    bool haveStartQuestRequirement(const Quest &quest) const;
    enum ObjectType
    {
        ObjectType_All,
        ObjectType_Seed,
        ObjectType_Sell,
        ObjectType_SellToMarket,
        ObjectType_Trade,
        ObjectType_MonsterToTrade,
        ObjectType_MonsterToTradeToMarket,
        ObjectType_MonsterToLearn,
        ObjectType_MonsterToFight,
        ObjectType_MonsterToFightKO,
        ObjectType_ItemOnMonsterOutOfFight,
        ObjectType_ItemOnMonster,
        ObjectType_ItemEvolutionOnMonster,
        ObjectType_ItemLearnOnMonster,
        ObjectType_UseInFight
    };
    enum ActionClan
    {
        ActionClan_Create,
        ActionClan_Leave,
        ActionClan_Dissolve,
        ActionClan_Invite,
        ActionClan_Eject
    };
    enum QueryType
    {
        QueryType_Seed,
        QueryType_CollectPlant
    };
    enum MoveType
    {
        MoveType_Enter,
        MoveType_Leave,
        MoveType_Dead
    };
    enum TradeOtherStat
    {
        TradeOtherStat_InWait,
        TradeOtherStat_Accepted
    };
    enum BattleType
    {
        BattleType_Wild,
        BattleType_Bot,
        BattleType_OtherPlayer
    };
    enum BattleStep
    {
        BattleStep_Presentation,
        BattleStep_PresentationMonster,
        BattleStep_Middle,
        BattleStep_Final
    };
    struct BattleInformations
    {
        QString pseudo;
        uint8_t skinId;
        QList<uint8_t> stat;
        uint8_t monsterPlace;
        PublicPlayerMonster publicPlayerMonster;
    };
protected:
    void changeEvent(QEvent *e);
    static void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static QFile debugFile;
    static uint8_t debugFileStatus;
public slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
    void selectObject(const ObjectType &objectType);
    void objectSelection(const bool &ok, const uint16_t &itemId=0, const uint32_t &quantity=1);
    void connectAllSignals();
private slots:
    void message(QString message) const;
    void stdmessage(std::string message) const;
    void disconnected(QString reason);
    void notLogged(QString reason);
    void protocol_is_good();
    void error(QString error);
    void stderror(const std::string &error);
    void errorWithTheCurrentMap();
    void repelEffectIsOver();
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void setEvents(const QList<QPair<uint8_t, uint8_t> > &events);
    void newEvent(const uint8_t &event,const uint8_t &event_value);

    //player UI
    void on_pushButton_interface_bag_clicked();
    void on_toolButton_quit_inventory_clicked();
    void on_inventory_itemSelectionChanged();
    void on_listPlantList_itemSelectionChanged();
    void tipTimeout();
    void gainTimeout();
    void showTip(const QString &tip);
    void showPlace(const QString &place);
    void showGain();
    void composeAndDisplayGain();
    void addQuery(const QueryType &queryType);
    void removeQuery(const QueryType &queryType);
    void updateQueryList();
    void loadSettings();
    void loadSettingsWithDatapack();
    void updateTheWareHouseContent();
    QListWidgetItem * itemToGraphic(const uint32_t &id, const uint32_t &quantity);
    //player
    void logged(const QList<ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CharacterEntry> > &characterEntryList);
    void updatePlayerImage();
    void have_character_position();
    void haveCharacter();
    void sendDatapackContentMainSub();
    void have_main_and_sub_datapack_loaded();
    void have_inventory(const std::unordered_map<uint16_t, uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items);
    void add_to_inventory(const uint32_t &item,const uint32_t &quantity=1,const bool &showGain=true);
    void add_to_inventory(const QList<QPair<uint16_t,uint32_t> > &items,const bool &showGain=true);
    void add_to_inventory(const QHash<uint16_t,uint32_t> &items, const bool &showGain=true);
    void add_to_inventory_slot(const QHash<uint16_t,uint32_t> &items);
    void remove_to_inventory(const QHash<uint16_t,uint32_t> &items);
    void remove_to_inventory_slot(const QHash<uint16_t,uint32_t> &items);
    void remove_to_inventory(const uint32_t &itemId,const uint32_t &quantity=1);
    void load_inventory();
    void load_plant_inventory();
    void load_crafting_inventory();
    void load_monsters();
    void load_event();
    bool check_monsters();
    bool check_senddata();//with the datapack content
    void show_reputation();
    void addCash(const uint32_t &cash);
    void removeCash(const uint32_t &cash);
    void updatePlayerType();
    QPixmap getFrontSkin(const QString &skinName) const;
    QPixmap getFrontSkin(const uint32_t &skinId) const;
    QPixmap getBackSkin(const uint32_t &skinId) const;
    QString getSkinPath(const QString &skinName, const QString &type) const;
    QString getFrontSkinPath(const uint32_t &skinId) const;
    QString getFrontSkinPath(const QString &skin) const;
    QString getBackSkinPath(const uint32_t &skinId) const;
    void monsterCatch(const bool &success);
    //render
    void stopped_in_front_of(CatchChallenger::Map_client *map, uint8_t x, uint8_t y);
    bool stopped_in_front_of_check_bot(CatchChallenger::Map_client *map, uint8_t x, uint8_t y);
    int32_t havePlant(CatchChallenger::Map_client *map, uint8_t x, uint8_t y) const;//return -1 if not found, else the index
    void actionOnNothing();
    void actionOn(CatchChallenger::Map_client *map, uint8_t x, uint8_t y);
    bool actionOnCheckBot(CatchChallenger::Map_client *map, uint8_t x, uint8_t y);
    void botFightCollision(CatchChallenger::Map_client *map, uint8_t x, uint8_t y);
    void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
    void currentMapLoaded();
    void pathFindingNotFound();
    void updateFactoryStatProduction(const IndustryStatus &industryStatus,const Industry &industry);
    void factoryToProductItem(QListWidgetItem *item);
    void factoryToResourceItem(QListWidgetItem *item);
    void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
    void animationFinished();
    void evolutionCanceled();
    void teleportConditionNotRespected(const QString &text);
    static QString reputationRequirementsToText(const ReputationRequirements &reputationRequirements);

    //datapack
    void haveTheDatapack();
    void haveTheDatapackMainSub();
    void newDatapackFile(const uint32_t &size);
    void progressingDatapackFile(const uint32_t &size);
    void datapackSize(const uint32_t &datapackFileNumber,const uint32_t &datapackFileSize);

    //inventory
    void on_inventory_itemActivated(QListWidgetItem *item);
    void objectUsed(const ObjectUsage &objectUsage);
    uint32_t itemQuantity(const uint32_t &itemId) const;
    //trade
    void tradeRequested(const QString &pseudo, const uint8_t &skinInt);
    void tradeAcceptedByOther(const QString &pseudo,const uint8_t &skinInt);
    void tradeCanceledByOther();
    void tradeFinishedByOther();
    void tradeValidatedByTheServer();
    void tradeAddTradeCash(const uint64_t &cash);
    void tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity);
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster);
    void tradeUpdateCurrentObject();
    QList<PlayerMonster> warehouseMonsterOnPlayer() const;
    //battle
    void battleRequested(const QString &pseudo, const uint8_t &skinInt);
    void battleAcceptedByOther(const QString &pseudo, const uint8_t &skinId, const QList<uint8_t> &stat, const uint8_t &monsterPlace, const PublicPlayerMonster &publicPlayerMonster);
    void battleAcceptedByOtherFull(const BattleInformations &battleInformations);
    void battleCanceledByOther();
    void sendBattleReturn(const QList<Skill::AttackReturn> &attackReturn);

    //shop
    void haveShopList(const QList<ItemToSellOrBuy> &items);
    void haveBuyObject(const BuyStat &stat,const uint32_t &newPrice);
    void haveSellObject(const SoldStat &stat,const uint32_t &newPrice);
    void displaySellList();

    //shop
    void haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice);
    void haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice);
    void haveFactoryList(const uint32_t &remainingProductionTime, const QList<ItemToSellOrBuy> &resources, const QList<ItemToSellOrBuy> &products);

    //plant
    void insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature);
    void remove_plant(const uint32_t &mapId, const uint8_t &x, const uint8_t &y);
    void cancelAllPlantQuery(const QString map, const uint8_t x, const uint8_t y);//without ref because after reset them self will failed all reset
    void seed_planted(const bool &ok);
    void plant_collected(const CatchChallenger::Plant_collect &stat);
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage);

    //network
    void updateRXTX();

    //bot
    void goToBotStep(const uint8_t &step);
    QString parseHtmlToDisplay(const QString &htmlContent);

    //fight
    void wildFightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void prepareFight();
    void botFight(const uint32_t &fightId);
    void botFightFull(const uint32_t &fightId);
    void botFightFullDiffered();
    bool fightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void on_pushButtonFightEnterNext_clicked();
    void moveFightMonsterBottom();
    void updateCurrentMonsterInformation();
    void updateCurrentMonsterInformationXp();
    void updateAttackList();
    void moveFightMonsterTop();
    void moveFightMonsterBoth();
    void updateOtherMonsterInformation();
    void on_toolButtonFightQuit_clicked();
    void on_pushButtonFightAttack_clicked();
    void on_pushButtonFightMonster_clicked();
    void on_pushButtonFightAttackConfirmed_clicked();
    void on_pushButtonFightReturn_clicked();
    void on_listWidgetFightAttack_itemSelectionChanged();
    void finalFightTextQuit();
    void teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction);
    void doNextAction();
    void pageChanged();
    void displayAttack();
    bool displayFirstAttackText(bool firstText);
    void displayText(const QString &text);
    void resetPosition(bool outOfScreen=false, bool topMonster=true, bool bottomMonster=true);
    void init_environement_display(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void init_current_monster_display(PlayerMonster *fightMonster=NULL);
    void init_other_monster_display();
    void useTrap(const uint32_t &itemId);
    void displayTrap();
    void displayExperienceGain();
    void checkEvolution();

    //clan/city
    void cityCapture(const uint32_t &remainingTime,const uint8_t &type);
    void cityCaptureUpdateTime();
    void updatePageZoneCatch();

    //learn
    bool showLearnSkillByPosition(const uint8_t &monsterPosition);
    bool learnSkillByPosition(const uint8_t &monsterPosition,const uint32_t &skill);

    //quest
    bool haveReputationRequirements(const QList<ReputationRequirements> &reputationList) const;
    bool haveReputationRequirements(const std::vector<ReputationRequirements> &reputationList) const;
    void appendReputationRewards(const QList<ReputationRewards> &reputationList);
    void appendReputationRewards(const std::vector<ReputationRewards> &reputationList);
    void getTextEntryPoint();
    void showQuestText(const uint32_t &textId);
    bool tryValidateQuestStep(bool silent=false);
    bool nextStepQuest(const Quest &quest);
    bool startQuest(const Quest &quest);
    bool botHaveQuest(const uint32_t &botId);
    QList<QPair<uint32_t,QString> > getQuestList(const uint32_t &botId);
    void updateDisplayedQuests();
    void appendReputationPoint(const QString &type,const int32_t &point);

    //clan
    void clanActionSuccess(const uint32_t &clanId);
    void clanActionFailed();
    void clanDissolved();
    void updateClanDisplay();
    void clanInformations(const QString &name);
    void clanInvite(const uint32_t &clanId, const QString &name);

    //city
    void captureCityYourAreNotLeader();
    void captureCityYourLeaderHaveStartInOtherCity(const QString &zone);
    void captureCityPreviousNotFinished();
    void captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId);
    void captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityWin();

    //market
    void marketList(const uint64_t &price,const QList<MarketObject> &marketObjectList,const QList<MarketMonster> &marketMonsterList,const QList<MarketObject> &marketOwnObjectList,const QList<MarketMonster> &marketOwnMonsterList);
    void addOwnMonster(const MarketMonster &marketMonster);
    void marketBuy(const bool &success);
    void marketBuyMonster(const PlayerMonster &playerMonster);
    void marketPut(const bool &success);
    void marketGetCash(const uint64_t &cash);
    void marketWithdrawCanceled();
    void marketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity);
    void marketWithdrawMonster(const PlayerMonster &playerMonster);

    //autoconnect
    void number_of_player(uint16_t number,uint16_t max);
    void on_toolButton_interface_quit_clicked();
    void on_toolButton_quit_interface_clicked();
    void on_pushButton_interface_trainer_clicked();
    void on_toolButton_quit_options_clicked();
    void on_inventoryDestroy_clicked();
    void on_inventoryUse_clicked();
    void on_toolButton_quit_plants_clicked();
    void on_inventoryInformation_clicked();
    void on_plantUse_clicked();
    void on_listPlantList_itemActivated(QListWidgetItem *item);
    void on_pushButton_interface_crafting_clicked();
    void on_toolButton_quit_crafting_clicked();
    void on_listCraftingList_itemSelectionChanged();
    void on_craftingUse_clicked();
    void on_listCraftingList_itemActivated(QListWidgetItem *);
    void on_toolButtonOptions_clicked();
    void on_checkBoxLimitFPS_toggled(bool checked);
    void on_spinBoxMaxFPS_editingFinished();
    void on_IG_dialog_text_linkActivated(const QString &rawlink);
    void on_toolButton_quit_shop_clicked();
    void on_shopItemList_itemActivated(QListWidgetItem *item);
    void on_shopItemList_itemSelectionChanged();
    void on_shopBuy_clicked();
    void on_pushButton_interface_monsters_clicked();
    void on_toolButton_monster_list_quit_clicked();
    void on_tradePlayerCash_editingFinished();
    void on_tradeCancel_clicked();
    void on_tradeValidate_clicked();
    void on_tradeAddItem_clicked();
    void on_tradeAddMonster_clicked();
    void on_selectMonster_clicked();
    void on_monsterList_itemActivated(QListWidgetItem *item);
    void on_learnQuit_clicked();
    void on_learnValidate_clicked();
    void on_learnAttackList_itemActivated(QListWidgetItem *item);
    void on_close_IG_dialog_clicked();
    void on_questsList_itemSelectionChanged();
    void on_listWidgetFightAttack_itemActivated(QListWidgetItem *item);
    void on_warehouseWithdrawCash_clicked();
    void on_warehouseDepositCash_clicked();
    void on_warehouseWithdrawItem_clicked();
    void on_warehouseDepositItem_clicked();
    void on_warehouseWithdrawMonster_clicked();
    void on_warehouseDepositMonster_clicked();
    void on_warehousePlayerInventory_itemActivated(QListWidgetItem *item);
    void on_warehousePlayerStoredInventory_itemActivated(QListWidgetItem *item);
    void on_warehousePlayerMonster_itemActivated(QListWidgetItem *item);
    void on_warehousePlayerStoredMonster_itemActivated(QListWidgetItem *item);
    void on_toolButton_quit_warehouse_clicked();
    void on_warehouseValidate_clicked();
    void on_pushButtonFightBag_clicked();
    void on_clanActionLeave_clicked();
    void on_clanActionDissolve_clicked();
    void on_clanActionInvite_clicked();
    void on_clanActionEject_clicked();
    void on_zonecaptureCancel_clicked();
    void on_factoryBuy_clicked();
    void on_factoryProducts_itemActivated(QListWidgetItem *item);
    void on_factorySell_clicked();
    void on_factoryResources_itemActivated(QListWidgetItem *item);
    void on_factoryQuit_clicked();
    void on_monsterDetailsQuit_clicked();
    void on_monsterListMoveUp_clicked();
    void on_monsterListMoveDown_clicked();
    void on_monsterList_itemSelectionChanged();
    void on_marketQuit_clicked();
    void on_marketWithdraw_clicked();
    void updateMarketObject(QListWidgetItem *item,const MarketObject &marketObject);
    void on_marketObject_itemActivated(QListWidgetItem *item);
    void on_marketOwnObject_itemActivated(QListWidgetItem *item);
    void on_marketMonster_itemActivated(QListWidgetItem *item);
    void on_marketOwnMonster_itemActivated(QListWidgetItem *item);
    void on_marketPutObject_clicked();
    void on_marketPutMonster_clicked();
    void on_audioVolume_valueChanged(int value);
    void newProfileFinished();
    void updateCharacterList();
    void newCharacterId(const uint8_t &returnCode,const uint32_t &characterId);
    void on_character_add_clicked();
    void on_character_back_clicked();
    void on_character_select_clicked();
    void on_character_remove_clicked();
    void on_characterEntryList_itemDoubleClicked(QListWidgetItem *item);
    void on_listCraftingMaterials_itemActivated(QListWidgetItem *item);
    void on_forceZoom_toggled(bool checked);
    void on_zoom_valueChanged(int value);
    void changeDeviceIndex(int device);
    void lastReplyTime(const uint32_t &time);
    void detectSlowDown();
    void updateTheTurtle();
    void on_serverListBack_clicked();
    void updateServerList();
    void addToServerList(LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView=true);
    void on_serverList_activated(const QModelIndex &index);
    void on_serverListSelect_clicked();
    void on_toolButtonEncyclopedia_clicked();
    void on_toolButton_quit_encyclopedia_clicked();
    void on_listWidgetEncyclopediaMonster_itemActivated(QListWidgetItem *item);
    void on_listWidgetEncyclopediaItem_itemActivated(QListWidgetItem *item);
    void on_listWidgetEncyclopediaMonster_itemChanged(QListWidgetItem *item);
    void on_listWidgetEncyclopediaItem_itemChanged(QListWidgetItem *item);
    void on_listWidgetEncyclopediaItem_itemSelectionChanged();
    void on_listWidgetEncyclopediaMonster_itemSelectionChanged();
protected slots:
    //datapack
    void datapackParsed();
    void datapackParsedMainSub();
    void datapackChecksumError();
    void loadSoundSettings();
    //UI
    void updateConnectingStatus();
private:
    Ui::BaseWindowUI *ui;
    QFrame *renderFrame;
    QString imagesInterfaceRepeatableString,imagesInterfaceInProgressString;
    QTimer tip_timeout;
    QTimer gain_timeout;
    QList<QueryType> queryList;
    uint32_t shopId;
    QHash<uint32_t,ItemToSellOrBuy> itemsIntoTheShop;
    uint64_t cash,warehouse_cash;
    int64_t temp_warehouse_cash;// if >0 then Withdraw
    //selection of quantity
    uint32_t tempQuantityForSell;
    bool waitToSell;
    QList<ItemToSellOrBuy> itemsToSell,itemsToBuy;
    QPixmap playerFrontImage,playerBackImage;
    QString playerFrontImagePath,playerBackImagePath;
    uint32_t clan;
    bool clan_leader;
    std::unordered_set<ActionAllow,std::hash<uint8_t> > allow;
    QList<ActionClan> actionClan;
    QString clanName;
    bool haveClanInformations;
    uint32_t factoryId;
    IndustryStatus industryStatus;
    bool factoryInProduction;
    ObjectType waitedObjectType;
    uint32_t datapackDownloadedCount;
    uint32_t datapackDownloadedSize;
    uint32_t progressingDatapackFileSize;

    NewProfile *newProfile;
    uint32_t datapackFileNumber;
    int32_t datapackFileSize;
    AnimationControl animationControl;
    EvolutionControl *evolutionControl;
    QWidget *previousAnimationWidget;
    QQuickView *animationWidget;
    CraftingAnimation *craftingAnimationObject;
    QWidget *qQuickViewContainer;
    QmlMonsterGeneralInformations *baseMonsterEvolution;
    QmlMonsterGeneralInformations *targetMonsterEvolution;
    uint8_t monsterEvolutionPostion;
    uint32_t mLastGivenXP;
    uint32_t currentMonsterLevel;
    QSet<QString> supportedImageFormats;
    QString lastPlaceDisplayed;
    std::vector<uint8_t> events;
    QString visualCategory;
    QTimer botFightTimer;
    QStringList add_to_inventoryGainList;
    QList<QTime> add_to_inventoryGainTime;
    QStringList add_to_inventoryGainExtraList;
    QList<QTime> add_to_inventoryGainExtraTime;

    //cache
    QFont disableIntoListFont;
    QBrush disableIntoListBrush;

    //for server/character selection
    bool isLogged;
    uint32_t averagePlayedTime,averageLastConnect;
    QList<ServerFromPoolForDisplay *> serverOrdenedList;
    QHash<uint8_t/*character group index*/,QPair<uint8_t/*server count*/,uint8_t/*temp Index to display*/> > serverByCharacterGroup;
    QList<QList<CharacterEntry> > characterListForSelection;
    QList<CharacterEntry> characterEntryListInWaiting;
    int serverSelected;

    //plant seed in waiting
    struct SeedInWaiting
    {
        uint32_t seedItemId;
        uint8_t x,y;
        QString map;
    };
    QList<SeedInWaiting> seed_in_waiting;
    struct ClientPlantInCollecting
    {
        QString map;
        uint8_t x,y;
        uint8_t plant_id;
        uint16_t seconds_to_mature;
    };
    QList<ClientPlantInCollecting> plant_collect_in_waiting;
    //bool seedWait,collectWait;

    QTime updateRXTXTime;
    QTimer updateRXTXTimer;
    uint64_t previousRXSize,previousTXSize;
    QString toHtmlEntities(QString text);
    QSettings settings;
    bool haveShowDisconnectionReason;
    QString toSmilies(QString text);
    QStringList server_list;
    QAbstractSocket::SocketState socketState;
    bool haveDatapack,haveDatapackMainSub,haveCharacterPosition,haveCharacterInformation,haveInventory,datapackIsParsed,mainSubDatapackIsParsed;
    bool characterSelected;
    uint32_t fightId;

    //market buy
    QList<QPair<uint32_t,uint32_t> > marketBuyObjectList;
    uint32_t marketBuyCashInSuspend;
    bool marketBuyInSuspend;
    //market put
    uint32_t marketPutCashInSuspend;
    QList<QPair<uint16_t,uint32_t> > marketPutObjectInSuspendList;
    QList<CatchChallenger::PlayerMonster> marketPutMonsterList;
    QList<uint8_t> marketPutMonsterPlaceList;
    bool marketPutInSuspend;
    //market withdraw
    bool marketWithdrawInSuspend;
    QList<MarketObject> marketWithdrawObjectList;
    QList<MarketMonster> marketWithdrawMonsterList;

    //player items
    std::unordered_set<uint16_t> itemOnMap;
    std::unordered_map<uint16_t/*dirtOnMap*/,PlayerPlant> plantOnMap;
    QHash<uint16_t,int32_t> change_warehouse_items;//negative = deposite, positive = withdraw
    std::unordered_map<uint16_t,uint32_t> items,warehouse_items;
    QHash<QListWidgetItem *,uint32_t> items_graphical;
    QHash<uint32_t,QListWidgetItem *> items_to_graphical;
    QHash<QListWidgetItem *,uint32_t> shop_items_graphical;
    QHash<uint32_t,QListWidgetItem *> shop_items_to_graphical;
    QHash<QListWidgetItem *,uint32_t> plants_items_graphical;
    QHash<uint32_t,QListWidgetItem *> plants_items_to_graphical;
    QHash<QListWidgetItem *,uint32_t> crafting_recipes_items_graphical;
    QHash<uint32_t,QListWidgetItem *> crafting_recipes_items_to_graphical;
    QHash<QListWidgetItem *,uint32_t> fight_attacks_graphical;
    QHash<QListWidgetItem *,uint8_t> monsters_items_graphical;
    QHash<QListWidgetItem *,uint32_t> attack_to_learn_graphical;
    QHash<QListWidgetItem *,uint32_t> quests_to_id_graphical;
    bool inSelection;
    QList<uint32_t> objectInUsing;
    QList<uint32_t> monster_to_deposit,monster_to_withdraw;

    //crafting
    QList<QList<QPair<uint32_t,uint32_t> > > materialOfRecipeInUsing;
    QList<QPair<uint32_t,uint32_t> > productOfRecipeInUsing;

    //bot
    Bot actualBot;

    //fight
    QTimer moveFightMonsterBottomTimer;
    QTimer moveFightMonsterTopTimer;
    QTimer moveFightMonsterBothTimer;
    QTimer displayAttackTimer;
    QTimer displayExpTimer;
    QTimer doNextActionTimer;
    QTime updateAttackTime;
    QTime updateTrapTime;
    MoveType moveType;
    bool fightTimerFinish;
    int displayAttackProgression;
    uint8_t lastStepUsed;
    enum DoNextActionStep
    {
        DoNextActionStep_Start,
        DoNextActionStep_Loose,
        DoNextActionStep_Win
    };
    DoNextActionStep doNextActionStep;
    void loose();
    void win();
    bool escape,escapeSuccess;
    bool haveDisplayCurrentAttackSuccess;
    bool haveDisplayOtherAttackSuccess;
    BattleType battleType;
    QMovie *movie;
    QList<PlayerMonster> warehouse_playerMonster;
    uint32_t trapItemId;
    int displayTrapProgression;
    QTimer displayTrapTimer;
    bool trapSuccess;
    int32_t attack_quantity_changed;
    QList<BattleInformations> battleInformationsList;
    QList<uint32_t> botFightList;
    QHash<uint32_t,QListWidgetItem *> buffToGraphicalItemTop,buffToGraphicalItemBottom;
    bool useTheRescueSkill;

    //city
    QTimer nextCityCatchTimer;
    City city;
    QTimer updater_page_zonecatch;
    QDateTime nextCatch,nextCatchOnScreen;
    QString zonecatchName;
    bool zonecatch;

    //trade
    TradeOtherStat tradeOtherStat;
    QHash<uint16_t,uint32_t> tradeOtherObjects,tradeCurrentObjects;
    QList<CatchChallenger::PlayerMonster> tradeEvolutionMonsters,tradeOtherMonsters,tradeCurrentMonsters;
    QList<uint8_t> tradeCurrentMonstersPosition;

    //learn
    uint8_t monsterPositionToLearn;

    //quest
    bool isInQuest;
    uint16_t questId;
    std::unordered_map<uint16_t, PlayerQuest> quests;

    //battle
    BattleStep battleStep;

    struct Ambiance
    {
        #ifndef CATCHCHALLENGER_NOAUDIO
        libvlc_media_player_t *player;
        #endif
        QString file;
    };
    QList<Ambiance> ambianceList;

    static QString text_type;
    static QString text_lang;
    static QString text_en;
    static QString text_text;

    static QIcon icon_server_list_star1;
    static QIcon icon_server_list_star2;
    static QIcon icon_server_list_star3;
    static QIcon icon_server_list_star4;
    static QIcon icon_server_list_star5;
    static QIcon icon_server_list_star6;
    static QIcon icon_server_list_stat1;
    static QIcon icon_server_list_stat2;
    static QIcon icon_server_list_stat3;
    static QIcon icon_server_list_stat4;
    static QIcon icon_server_list_bug;

    bool monsterBeforeMoveForChangeInWaiting;
    QTimer checkQueryTime;
    int lastReplyTimeValue;
    QTime lastReplyTimeSince;
    uint32_t worseQueryTime;
    bool multiplayer;
signals:
    void newError(QString error,QString detailedError);
    //datapack
    void parseDatapack(const QString &datapackPath);
    void parseDatapackMainSub(const QString &mainDatapackCode, const QString &subDatapackCode);
    void datapackParsedMainSubMap();
    void sendsetMultiPlayer(const bool & multiplayer);
    void teleportDone();
    //plant, can do action only if the previous is finish
    void useSeed(const uint8_t &plant_id);
    void collectMaturePlant();
    //inventory
    void destroyObject(uint32_t object,uint32_t quantity=1);
    void gameIsLoaded();
};
}

#endif
