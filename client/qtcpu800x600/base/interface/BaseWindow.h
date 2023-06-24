//to have winsock2 include before Windows.h, prevent mingw warn
#include "../../../libqtcatchchallenger/Api_client_real.hpp"

#include "../../../libqtcatchchallenger/ClientVariableAudio.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
#include <QAudioOutput>
#include "../QInfiniteBuffer.h"
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
#include <QBuffer>

#include "../../crafting/interface/QmlInterface/CraftingAnimation.h"
#include "../../../../general/base/GeneralStructures.hpp"
#include "../../../libcatchchallenger/Api_protocol.hpp"
#include "../../../qtmaprender/MapController.hpp"
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
    void setMultiPlayer(bool multiplayer, Api_protocol_Qt *client);
    void resetAll();
    void serverIsLoading();
    void serverIsReady();
    std::string lastLocation() const;
    std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, PlayerQuest> getQuests() const;
    uint16_t getActualBotId() const;
    bool haveNextStepQuestRequirements(const Quest &quest) const;
    bool haveStartQuestRequirement(const Quest &quest) const;
    enum ObjectType
    {
        ObjectType_All,
        ObjectType_Seed,
        ObjectType_Sell,
        ObjectType_Trade,
        ObjectType_MonsterToTrade,
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
        std::string pseudo;
        uint8_t skinId;
        std::vector<uint8_t> stat;
        uint8_t monsterPlace;
        PublicPlayerMonster publicPlayerMonster;
    };
    MapController *mapController;
protected:
    void changeEvent(QEvent *e);
    //static void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const std::string &msg);
    static QFile debugFile;
    static uint8_t debugFileStatus;
public slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
    void selectObject(const ObjectType &objectType);
    void objectSelection(const bool &ok, const uint16_t &itemId=0, const uint32_t &quantity=1);
    void connectAllSignals();
    #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
    void receiveLanPort(uint16_t port);
    #endif
private slots:
    void message(std::string message) const;
    void stdmessage(std::string message) const;
    void disconnected(std::string reason);
    void notLogged(std::string reason);
    void protocol_is_good();
    void error(std::string error);
    void stderror(const std::string &error);
    void errorWithTheCurrentMap();
    void repelEffectIsOver();
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void setEvents(const std::vector<std::pair<uint8_t, uint8_t> > &events);
    void newEvent(const uint8_t &event,const uint8_t &event_value);
    void forcedEvent(const uint8_t &event,const uint8_t &event_value);

    //player UI
    void on_pushButton_interface_bag_clicked();
    void on_toolButton_quit_inventory_clicked();
    void on_inventory_itemSelectionChanged();
    void on_listPlantList_itemSelectionChanged();
    void tipTimeout();
    void gainTimeout();
    void showTip(const std::string &tip);
    void showPlace(const std::string &place);
    void showGain();
    void composeAndDisplayGain();
    void addQuery(const QueryType &queryType);
    void removeQuery(const QueryType &queryType);
    void updateQueryList();
    void loadSettings();
    void loadSettingsWithDatapack();
    void updateTheWareHouseContent();
    QListWidgetItem * itemToGraphic(const uint16_t &itemid, const uint32_t &quantity);
    //player
    void logged(const std::vector<std::vector<CharacterEntry> > &characterEntryList);
    void updatePlayerImage();
    void have_character_position();
    void haveCharacter();
    void sendDatapackContentMainSub();
    void have_main_and_sub_datapack_loaded();
    void have_inventory(const std::unordered_map<uint16_t, uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items);
    void add_to_inventory(const uint16_t &item, const uint32_t &quantity=1, const bool &showGain=true);
    void add_to_inventory(const std::vector<std::pair<uint16_t,uint32_t> > &items,const bool &showGain=true);
    void add_to_inventory(const std::unordered_map<uint16_t, uint32_t> &items, const bool &showGain=true);
    void add_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items);
    void remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items);
    void remove_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items);
    void remove_to_inventory(const uint16_t &itemId, const uint32_t &quantity=1);
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
    QPixmap getFrontSkin(const std::string &skinName);
    QPixmap getFrontSkin(const uint32_t &skinId);
    std::string getFrontSkinPath(const std::string &skin);
    std::string getFrontSkinPath(const uint32_t &skinId);
    QPixmap getBackSkin(const uint32_t &skinId) const;
    std::string getBackSkinPath(const uint32_t &skinId) const;
    std::string getSkinPath(const std::string &skinName, const std::string &type) const;
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
    void pathFindingInternalError();
    void updateFactoryStatProduction(const IndustryStatus &industryStatus,const Industry &industry);
    void factoryToProductItem(QListWidgetItem *item);
    void factoryToResourceItem(QListWidgetItem *item);
    void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
    void animationFinished();
    void evolutionCanceled();
    void teleportConditionNotRespected(const std::string &text);
    static std::string reputationRequirementsToText(const ReputationRequirements &reputationRequirements);

    //datapack
    void haveTheDatapack();
    void haveTheDatapackMainSub();
    void haveDatapackMainSubCode();
    void newDatapackFile(const uint32_t &size);
    void progressingDatapackFile(const uint32_t &size);
    void datapackSize(const uint32_t &datapackFileNumber,const uint32_t &datapackFileSize);
    void gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression);

    //inventory
    void on_inventory_itemActivated(QListWidgetItem *item);
    void objectUsed(const ObjectUsage &objectUsage);
    uint32_t itemQuantity(const uint16_t &itemId) const;
    //trade
    void tradeRequested(const std::string &pseudo, const uint8_t &skinInt);
    void tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt);
    void tradeCanceledByOther();
    void tradeFinishedByOther();
    void tradeValidatedByTheServer();
    void tradeAddTradeCash(const uint64_t &cash);
    void tradeAddTradeObject(const uint16_t &item, const uint32_t &quantity);
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster);
    void tradeUpdateCurrentObject();
    std::vector<PlayerMonster> warehouseMonsterOnPlayer() const;
    //battle
    void battleRequested(const std::string &pseudo, const uint8_t &skinInt);
    void battleAcceptedByOther(const std::string &pseudo, const uint8_t &skinId, const std::vector<uint8_t> &stat, const uint8_t &monsterPlace, const PublicPlayerMonster &publicPlayerMonster);
    void battleAcceptedByOtherFull(const BattleInformations &battleInformations);
    void battleCanceledByOther();
    void sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn);

    //shop
    void haveShopList(const std::vector<ItemToSellOrBuy> &items);
    void haveBuyObject(const BuyStat &stat,const uint32_t &newPrice);
    void haveSellObject(const SoldStat &stat,const uint32_t &newPrice);
    void displaySellList();

    //shop
    void haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice);
    void haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice);
    void haveFactoryList(const uint32_t &remainingProductionTime, const std::vector<ItemToSellOrBuy> &resources, const std::vector<ItemToSellOrBuy> &products);

    //plant
    void insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature);
    void remove_plant(const uint32_t &mapId, const uint8_t &x, const uint8_t &y);
    void cancelAllPlantQuery(const std::string map, const uint8_t x, const uint8_t y);//without ref because after reset them self will failed all reset
    void seed_planted(const bool &ok);
    void plant_collected(const CatchChallenger::Plant_collect &stat);
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage);

    //network
    void updateRXTX();

    //bot
    void goToBotStep(const uint8_t &step);
    std::string parseHtmlToDisplay(const std::string &htmlContent);

    //fight
    void wildFightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void prepareFight();
    void botFight(const uint16_t &fightId);
    void botFightFull(const uint16_t &fightId);
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
    void displayText(const std::string &text);
    void resetPosition(bool outOfScreen=false, bool topMonster=true, bool bottomMonster=true);
    void init_environement_display(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void init_current_monster_display(PlayerMonster *fightMonster=NULL);
    void init_other_monster_display();
    void useTrap(const uint16_t &itemId);
    void displayTrap();
    void displayExperienceGain();
    void checkEvolution();

    //clan/city
    void cityCapture(const uint32_t &remainingTime,const uint8_t &type);
    void cityCaptureUpdateTime();
    void updatePageZoneCatch();

    //learn
    bool showLearnSkillByPosition(const uint8_t &monsterPosition);
    bool learnSkillByPosition(const uint8_t &monsterPosition, const uint16_t &skill);

    //quest
    bool haveReputationRequirements(const std::vector<ReputationRequirements> &reputationList) const;
    void appendReputationRewards(const std::vector<ReputationRewards> &reputationList);
    void showQuestText(const uint32_t &textId);
    bool tryValidateQuestStep(const uint16_t &questId, const uint16_t &botId, bool silent=false);
    bool nextStepQuest(const Quest &quest);
    bool startQuest(const Quest &quest);
    bool botHaveQuest(const uint16_t &botId) const;
    std::vector<std::pair<uint16_t, std::string> > getQuestList(const uint16_t &botId) const;
    void updateDisplayedQuests();
    void appendReputationPoint(const std::string &type,const int32_t &point);

    //clan
    void clanActionSuccess(const uint32_t &clanId);
    void clanActionFailed();
    void clanDissolved();
    void updateClanDisplay();
    void clanInformations(const std::string &name);
    void clanInvite(const uint32_t &clanId, const std::string &name);

    //city
    void captureCityYourAreNotLeader();
    void captureCityYourLeaderHaveStartInOtherCity(const std::string &zone);
    void captureCityPreviousNotFinished();
    void captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityStartBotFight(const uint16_t &player_count, const uint16_t &clan_count, const uint16_t &fightId);
    void captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityWin();

    void IG_dialog_text_linkActivated(const std::string &rawlink);

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
    void on_checkBoxEncyclopediaMonsterKnown_toggled(bool checked);
    void on_checkBoxEncyclopediaItemKnown_toggled(bool checked);
    void on_toolButtonAdmin_clicked();
    void on_toolButton_quit_admin_clicked();
    void on_itemFilterAdmin_returnPressed();
    void on_playerGiveAdmin_clicked();
    void on_listNearPlayer_itemActivated(QListWidgetItem *item);
    void on_listAllItem_itemActivated(QListWidgetItem *item);
    void on_openToLan_clicked();
    void on_toolButtonLan_triggered(QAction *arg1);

    void on_toolButtonLan_clicked();

protected slots:
    //datapack
    void datapackParsed();
    void datapackParsedMainSub();
    void datapackChecksumError();
    void loadSoundSettings();
    //UI
    void updateConnectingStatus();
    char my_toupper(char ch);
    std::string str_toupper(std::string s);
private:
    Ui::BaseWindowUI *ui;
    QFrame *renderFrame;
    std::string imagesInterfaceRepeatableString,imagesInterfaceInProgressString;
    QTimer tip_timeout;
    QTimer gain_timeout;
    std::vector<QueryType> queryList;
    uint16_t shopId;/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
    std::unordered_map<uint32_t,ItemToSellOrBuy> itemsIntoTheShop;
    int64_t temp_warehouse_cash;// if >0 then Withdraw
    //selection of quantity
    uint32_t tempQuantityForSell;
    bool waitToSell;
    std::vector<ItemToSellOrBuy> itemsToSell,itemsToBuy;
    QPixmap playerFrontImage,playerBackImage;
    std::string playerFrontImagePath,playerBackImagePath;
    std::vector<ActionClan> actionClan;
    std::string clanName;
    bool haveClanInformations;
    uint16_t factoryId;
    IndustryStatus industryStatus;
    bool factoryInProduction;
    ObjectType waitedObjectType;
    uint32_t datapackDownloadedCount;
    uint32_t datapackDownloadedSize;
    uint32_t progressingDatapackFileSize;
    std::unordered_map<uint8_t,uint8_t> datapackGatewayProgression;
    bool protocolIsGood;

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
    uint8_t currentMonsterLevel;
    std::unordered_set<std::string> supportedImageFormats;
    std::string lastPlaceDisplayed;
    std::vector<uint8_t> events;
    std::string visualCategory;
    QTimer botFightTimer;
    std::vector<std::string> add_to_inventoryGainList;
    std::vector<QElapsedTimer> add_to_inventoryGainTime;
    std::vector<std::string> add_to_inventoryGainExtraList;
    std::vector<QElapsedTimer> add_to_inventoryGainExtraTime;

    //cache
    QFont disableIntoListFont;
    QBrush disableIntoListBrush;

    //for server/character selection
    bool isLogged;
    uint32_t averagePlayedTime,averageLastConnect;
    std::unordered_map<uint8_t/*character group index*/,std::pair<uint8_t/*server count*/,uint8_t/*temp Index to display*/> > serverByCharacterGroup;
    std::vector<std::vector<CharacterEntry> > characterListForSelection;
    std::vector<CharacterEntry> characterEntryListInWaiting;
    int serverSelected;

    //plant seed in waiting
    struct SeedInWaiting
    {
        uint16_t seedItemId;
        uint8_t x,y;
        std::string map;
    };
    std::vector<SeedInWaiting> seed_in_waiting;
    struct ClientPlantInCollecting
    {
        std::string map;
        uint8_t x,y;
        uint8_t plant_id;
        uint16_t seconds_to_mature;
    };
    std::vector<ClientPlantInCollecting> plant_collect_in_waiting;
    //bool seedWait,collectWait;

    QElapsedTimer updateRXTXTime;
    QTimer updateRXTXTimer;
    uint64_t previousRXSize,previousTXSize;
    std::string toHtmlEntities(std::string text);
    QSettings settings;
    bool haveShowDisconnectionReason;
    std::string toSmilies(std::string text);
    std::vector<std::string> server_list;
    QAbstractSocket::SocketState socketState;
    bool haveDatapack,haveDatapackMainSub,haveCharacterPosition,haveCharacterInformation,haveInventory,datapackIsParsed,mainSubDatapackIsParsed;
    bool characterSelected;
    uint16_t fightId;

    //player items
    std::unordered_map<uint16_t,int32_t> change_warehouse_items;//negative = deposite, positive = withdraw
    std::unordered_map<QListWidgetItem *,uint16_t> items_graphical;
    std::unordered_map<uint16_t,QListWidgetItem *> items_to_graphical;
    std::unordered_map<QListWidgetItem *,uint16_t> shop_items_graphical;
    std::unordered_map<uint16_t,QListWidgetItem *> shop_items_to_graphical;
    std::unordered_map<QListWidgetItem *,uint8_t> plants_items_graphical;
    std::unordered_map<uint8_t,QListWidgetItem *> plants_items_to_graphical;
    std::unordered_map<QListWidgetItem *,uint16_t> crafting_recipes_items_graphical;
    std::unordered_map<uint16_t,QListWidgetItem *> crafting_recipes_items_to_graphical;
    std::unordered_map<QListWidgetItem *,uint16_t> fight_attacks_graphical;
    std::unordered_map<QListWidgetItem *,uint8_t> monsterspos_items_graphical;
    std::unordered_map<QListWidgetItem *,uint16_t> attack_to_learn_graphical;
    std::unordered_map<QListWidgetItem *,uint16_t> quests_to_id_graphical;
    bool inSelection;
    std::vector<uint16_t> objectInUsing;

    //crafting
    std::vector<std::vector<std::pair<uint16_t,uint32_t> > > materialOfRecipeInUsing;
    std::vector<std::pair<uint16_t,uint32_t> > productOfRecipeInUsing;

    //bot
    Bot actualBot;

    //fight
    QTimer moveFightMonsterBottomTimer;
    QTimer moveFightMonsterTopTimer;
    QTimer moveFightMonsterBothTimer;
    QTimer displayAttackTimer;
    QTimer displayExpTimer;
    QTimer doNextActionTimer;
    QElapsedTimer updateAttackTime;
    QElapsedTimer updateTrapTime;
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
    uint32_t trapItemId;
    int displayTrapProgression;
    QTimer displayTrapTimer;
    bool trapSuccess;
    int32_t attack_quantity_changed;
    std::vector<BattleInformations> battleInformationsList;
    std::vector<uint16_t> botFightList;
    std::unordered_map<uint8_t,QListWidgetItem *> buffToGraphicalItemTop,buffToGraphicalItemBottom;
    bool useTheRescueSkill;

    //city
    QTimer nextCityCatchTimer;
    City city;
    QTimer updater_page_zonecatch;
    QDateTime nextCatch,nextCatchOnScreen;
    std::string zonecatchName;
    bool zonecatch;

    //trade
    TradeOtherStat tradeOtherStat;
    std::unordered_map<uint16_t,uint32_t> tradeOtherObjects,tradeCurrentObjects;
    std::vector<CatchChallenger::PlayerMonster> tradeOtherMonsters,tradeCurrentMonsters;
    std::vector<uint8_t> tradeCurrentMonstersPosition;
    std::vector<uint8_t> tradeEvolutionMonsters;

    //learn
    uint8_t monsterPositionToLearn;

    //quest
    bool isInQuest;
    uint16_t questId;

    //battle
    BattleStep battleStep;

    struct Ambiance
    {
        #ifndef CATCHCHALLENGER_NOAUDIO
        QAudioOutput *player;
        QInfiniteBuffer *buffer;
        QByteArray *data;
        #endif
        std::string file;
    };
    Ambiance currentAmbiance;

    static std::string text_type;
    static std::string text_lang;
    static std::string text_en;
    static std::string text_text;

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
    static std::vector<QIcon> icon_server_list_color;

    bool monsterBeforeMoveForChangeInWaiting;
    QTimer checkQueryTime;
    int lastReplyTimeValue;
    QElapsedTimer lastReplyTimeSince;
    uint32_t worseQueryTime;
    bool multiplayer;

    CatchChallenger::Api_protocol_Qt * client;
    Chat *chat;

    //skin
    std::unordered_map<std::string,QPixmap> frontSkinCacheString;
signals:
    void newError(std::string error,std::string detailedError);
    //datapack
    void parseDatapack(const std::string &datapackPath);
    void parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode);
    void datapackParsedMainSubMap();
    void teleportDone();
    //plant, can do action only if the previous is finish
    void useSeed(const uint8_t &plant_id);
    void collectMaturePlant();
    //inventory
    void destroyObject(uint32_t object,uint32_t quantity=1);
    void gameIsLoaded();
    #ifndef CATCHCHALLENGER_NOAUDIO
    void audioLoopRestart(void *vlcPlayer);
    #endif
    #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
    void emitOpenToLan(QString name, bool allowInternet);
    #endif
};
}

#endif
