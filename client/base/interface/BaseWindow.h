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

#include "../../crafting/interface/QmlInterface/CraftingAnimation.h"
#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../Api_client_real.h"
#include "../../general/base/Api_protocol.h"
#include "../audio/QOggSimplePlayer.h"
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
    QHash<quint32, PlayerQuest> getQuests() const;
    quint8 getActualBotId() const;
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
        quint8 skinId;
        QList<quint8> stat;
        quint8 monsterPlace;
        PublicPlayerMonster publicPlayerMonster;
    };
protected:
    void changeEvent(QEvent *e);
public slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
    void selectObject(const ObjectType &objectType);
    void objectSelection(const bool &ok,const quint32 &itemId=0,const quint32 &quantity=1);
    void connectAllSignals();
private slots:
    void message(QString message) const;
    void disconnected(QString reason);
    void notLogged(QString reason);
    void protocol_is_good();
    void error(QString error);
    void errorWithTheCurrentMap();
    void repelEffectIsOver();
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void setEvents(const QList<QPair<quint8,quint8> > events);

    //player UI
    void on_pushButton_interface_bag_clicked();
    void on_toolButton_quit_inventory_clicked();
    void on_inventory_itemSelectionChanged();
    void on_listPlantList_itemSelectionChanged();
    void tipTimeout();
    void gainTimeout();
    void showTip(const QString &tip);
    void showPlace(const QString &place);
    void showGain(const QString &gain);
    void addQuery(const QueryType &queryType);
    void removeQuery(const QueryType &queryType);
    void updateQueryList();
    void loadSettings();
    void loadSettingsWithDatapack();
    void updateTheWareHouseContent();
    QListWidgetItem * itemToGraphic(const quint32 &id, const quint32 &quantity);
    //player
    void logged(const QList<CharacterEntry> &characterEntryList);
    void updatePlayerImage();
    void have_current_player_info();
    void have_inventory(const QHash<quint32,quint32> &items,const QHash<quint32,quint32> &warehouse_items);
    void add_to_inventory(const quint32 &item,const quint32 &quantity=1,const bool &showGain=true);
    void add_to_inventory(const QList<QPair<quint32,quint32> > &items,const bool &showGain=true);
    void add_to_inventory(const QHash<quint32,quint32> &items, const bool &showGain=true);
    void add_to_inventory_slot(const QHash<quint32,quint32> &items);
    void remove_to_inventory(const QHash<quint32,quint32> &items);
    void remove_to_inventory_slot(const QHash<quint32,quint32> &items);
    void remove_to_inventory(const quint32 &itemId,const quint32 &quantity=1);
    void load_inventory();
    void load_plant_inventory();
    void load_crafting_inventory();
    void load_monsters();
    void load_event();
    bool check_monsters();
    bool check_senddata();//with the datapack content
    void show_reputation();
    void addCash(const quint32 &cash);
    void removeCash(const quint32 &cash);
    void addBitcoin(const double &bitcoin);
    void removeBitcoin(const double &bitcoin);
    QPixmap getFrontSkin(const QString &skinName) const;
    QPixmap getFrontSkin(const quint32 &skinId) const;
    QPixmap getBackSkin(const quint32 &skinId) const;
    QString getSkinPath(const QString &skinName, const QString &type) const;
    QString getFrontSkinPath(const quint32 &skinId) const;
    QString getFrontSkinPath(const QString &skin) const;
    QString getBackSkinPath(const quint32 &skinId) const;
    void monsterCatch(const quint32 &newMonsterId);
    //render
    void stopped_in_front_of(CatchChallenger::Map_client *map, quint8 x, quint8 y);
    bool stopped_in_front_of_check_bot(CatchChallenger::Map_client *map, quint8 x, quint8 y);
    qint32 havePlant(CatchChallenger::Map_client *map, quint8 x, quint8 y) const;//return -1 if not found, else the index
    void actionOnNothing();
    void actionOn(CatchChallenger::Map_client *map, quint8 x, quint8 y);
    bool actionOnCheckBot(CatchChallenger::Map_client *map, quint8 x, quint8 y);
    void botFightCollision(CatchChallenger::Map_client *map, quint8 x, quint8 y);
    void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
    void currentMapLoaded();
    void updateFactoryStatProduction(const IndustryStatus &industryStatus,const Industry &industry);
    void factoryToProductItem(QListWidgetItem *item);
    void factoryToResourceItem(QListWidgetItem *item);
    void insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint8 &x,const quint8 &y,const CatchChallenger::Direction &direction);
    void animationFinished();
    void evolutionCanceled();
    void teleportConditionNotRespected(const QString &text);

    //datapack
    void haveTheDatapack();
    void newDatapackFile(const quint32 &size);
    void datapackSize(const quint32 &datapckFileNumber,const quint32 &datapckFileSize);

    //inventory
    void on_inventory_itemActivated(QListWidgetItem *item);
    void objectUsed(const ObjectUsage &objectUsage);
    quint32 itemQuantity(const quint32 &itemId) const;
    //trade
    void tradeRequested(const QString &pseudo, const quint8 &skinInt);
    void tradeAcceptedByOther(const QString &pseudo,const quint8 &skinInt);
    void tradeCanceledByOther();
    void tradeFinishedByOther();
    void tradeValidatedByTheServer();
    void tradeAddTradeCash(const quint64 &cash);
    void tradeAddTradeObject(const quint32 &item,const quint32 &quantity);
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster);
    void tradeUpdateCurrentObject();
    QList<PlayerMonster> warehouseMonsterOnPlayer() const;
    //battle
    void battleRequested(const QString &pseudo, const quint8 &skinInt);
    void battleAcceptedByOther(const QString &pseudo, const quint8 &skinId, const QList<quint8> &stat, const quint8 &monsterPlace, const PublicPlayerMonster &publicPlayerMonster);
    void battleAcceptedByOtherFull(const BattleInformations &battleInformations);
    void battleCanceledByOther();
    void sendBattleReturn(const QList<Skill::AttackReturn> &attackReturn);

    //shop
    void haveShopList(const QList<ItemToSellOrBuy> &items);
    void haveBuyObject(const BuyStat &stat,const quint32 &newPrice);
    void haveSellObject(const SoldStat &stat,const quint32 &newPrice);
    void displaySellList();

    //shop
    void haveBuyFactoryObject(const BuyStat &stat,const quint32 &newPrice);
    void haveSellFactoryObject(const SoldStat &stat,const quint32 &newPrice);
    void haveFactoryList(const quint32 &remainingProductionTime, const QList<ItemToSellOrBuy> &resources, const QList<ItemToSellOrBuy> &products);

    //plant
    void insert_plant(const quint32 &mapId,const quint8 &x,const quint8 &y,const quint8 &plant_id,const quint16 &seconds_to_mature);
    void remove_plant(const quint32 &mapId, const quint8 &x, const quint8 &y);
    void cancelAllPlantQuery(const QString map, const quint8 x, const quint8 y);//without ref because after reset them self will failed all reset
    void seed_planted(const bool &ok);
    void plant_collected(const CatchChallenger::Plant_collect &stat);
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage);

    //network
    void updateRXTX();

    //bot
    void goToBotStep(const quint8 &step);
    QString parseHtmlToDisplay(const QString &htmlContent);

    //fight
    void wildFightCollision(CatchChallenger::Map_client *map, const quint8 &x, const quint8 &y);
    void prepareFight();
    void botFight(const quint32 &fightId);
    void botFightFull(const quint32 &fightId);
    bool fightCollision(CatchChallenger::Map_client *map, const quint8 &x, const quint8 &y);
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
    void teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    void doNextAction();
    void displayAttack();
    bool displayFirstAttackText(bool firstText);
    void displayText(const QString &text);
    void resetPosition(bool outOfScreen=false, bool topMonster=true, bool bottomMonster=true);
    void init_environement_display(CatchChallenger::Map_client *map, const quint8 &x, const quint8 &y);
    void init_current_monster_display();
    void init_other_monster_display();
    void useTrap(const quint32 &itemId);
    void displayTrap();
    void displayExperienceGain();
    void checkEvolution();

    //clan/city
    void cityCapture(const quint32 &remainingTime,const quint8 &type);
    void cityCaptureUpdateTime();
    void updatePageZoneCatch();

    //learn
    bool showLearnSkill(const quint32 &monsterId);
    bool learnSkill(const quint32 &monsterId,const quint32 &skill);

    //quest
    bool haveReputationRequirements(const QList<ReputationRequirements> &reputationList) const;
    void appendReputationRewards(const QList<ReputationRewards> &reputationList);
    void getTextEntryPoint();
    void showQuestText(const quint32 &textId);
    void nextQuestStep();
    bool nextStepQuest(const Quest &quest);
    bool startQuest(const Quest &quest);
    bool botHaveQuest(const quint32 &botId);
    QList<QPair<quint32,QString> > getQuestList(const quint32 &botId);
    void updateDisplayedQuests();
    void appendReputationPoint(const QString &type,const qint32 &point);

    //clan
    void clanActionSuccess(const quint32 &clanId);
    void clanActionFailed();
    void clanDissolved();
    void updateClanDisplay();
    void clanInformations(const QString &name);
    void clanInvite(const quint32 &clanId, const QString &name);

    //city
    void captureCityYourAreNotLeader();
    void captureCityYourLeaderHaveStartInOtherCity(const QString &zone);
    void captureCityPreviousNotFinished();
    void captureCityStartBattle(const quint16 &player_count,const quint16 &clan_count);
    void captureCityStartBotFight(const quint16 &player_count,const quint16 &clan_count,const quint32 &fightId);
    void captureCityDelayedStart(const quint16 &player_count,const quint16 &clan_count);
    void captureCityWin();

    //market
    void marketList(const quint64 &price,const double &bitcoin,const QList<MarketObject> &marketObjectList,const QList<MarketMonster> &marketMonsterList,const QList<MarketObject> &marketOwnObjectList,const QList<MarketMonster> &marketOwnMonsterList);
    void addOwnMonster(const MarketMonster &marketMonster);
    void marketBuy(const bool &success);
    void marketBuyMonster(const PlayerMonster &playerMonster);
    void marketPut(const bool &success);
    void marketGetCash(const quint64 &cash,const double &bitcoin);
    void marketWithdrawCanceled();
    void marketWithdrawObject(const quint32 &objectId,const quint32 &quantity);
    void marketWithdrawMonster(const PlayerMonster &playerMonster);

    //autoconnect
    void number_of_player(quint16 number,quint16 max);
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
    void newCharacterId(const quint32 &characterId);
    void on_character_add_clicked();
    void on_character_back_clicked();
    void on_character_select_clicked();
    void on_character_remove_clicked();
    void on_characterEntryList_itemDoubleClicked(QListWidgetItem *item);
    void on_listCraftingMaterials_itemActivated(QListWidgetItem *item);
    void on_forceZoom_toggled(bool checked);
    void on_zoom_valueChanged(int value);
protected slots:
    //datapack
    void datapackParsed();
    //UI
    void updateConnectingStatus();
private:
    Ui::BaseWindowUI *ui;
    QFrame *renderFrame;
    QTimer tip_timeout;
    QTimer gain_timeout;
    QList<QueryType> queryList;
    quint32 shopId;
    QHash<quint32,ItemToSellOrBuy> itemsIntoTheShop;
    quint64 cash,warehouse_cash;
    double bitcoin;
    qint64 temp_warehouse_cash;// if >0 then Withdraw
    //selection of quantity
    quint32 tempQuantityForSell;
    bool waitToSell;
    QList<ItemToSellOrBuy> itemsToSell,itemsToBuy;
    QPixmap playerFrontImage,playerBackImage;
    QString playerFrontImagePath,playerBackImagePath;
    quint32 clan;
    bool clan_leader;
    QSet<ActionAllow> allow;
    QList<ActionClan> actionClan;
    QString clanName;
    bool haveClanInformations;
    quint32 factoryId;
    IndustryStatus industryStatus;
    bool factoryInProduction;
    ObjectType waitedObjectType;
    quint32 datapackDownloadedCount;
    quint32 datapackDownloadedSize;
    QList<CharacterEntry> characterEntryList,characterEntryListInWaiting;
    bool isLogged;
    NewProfile *newProfile;
    quint32 datapckFileNumber;
    quint32 datapckFileSize;
    AnimationControl animationControl;
    EvolutionControl *evolutionControl;
    QWidget *previousAnimationWidget;
    QQuickView *animationWidget;
    CraftingAnimation *craftingAnimationObject;
    QWidget *qQuickViewContainer;
    QmlMonsterGeneralInformations *baseMonsterEvolution;
    QmlMonsterGeneralInformations *targetMonsterEvolution;
    quint32 idMonsterEvolution;
    quint32 mLastGivenXP;
    quint32 currentMonsterLevel;
    QSet<QString> supportedImageFormats;
    QString lastPlaceDisplayed;
    QList<quint32> events;
    QString visualCategory;

    //plant seed in waiting
    struct SeedInWaiting
    {
        quint32 seed;
        quint8 x,y;
        QString map;
    };
    QList<SeedInWaiting> seed_in_waiting;
    struct ClientPlantInCollecting
    {
        QString map;
        quint8 x,y;
        quint8 plant_id;
        quint16 seconds_to_mature;
    };
    QList<ClientPlantInCollecting> plant_collect_in_waiting;
    //bool seedWait,collectWait;

    QTime updateRXTXTime;
    QTimer updateRXTXTimer;
    quint64 previousRXSize,previousTXSize;
    QString toHtmlEntities(QString text);
    QSettings settings;
    bool haveShowDisconnectionReason;
    QString toSmilies(QString text);
    QStringList server_list;
    QAbstractSocket::SocketState socketState;
    bool haveDatapack,havePlayerInformations,haveInventory,datapackIsParsed;

    //market buy
    QList<QPair<quint32,quint32> > marketBuyObjectList;
    quint32 marketBuyCashInSuspend;
    double marketBuyBitcoinInSuspend;
    bool marketBuyInSuspend;
    //market put
    quint32 marketPutCashInSuspend;
    double marketPutBitcoinInSuspend;
    QList<QPair<quint32,quint32> > marketPutObjectInSuspendList;
    QList<CatchChallenger::PlayerMonster> marketPutMonsterList;
    QList<quint8> marketPutMonsterPlaceList;
    bool marketPutInSuspend;
    //market withdraw
    bool marketWithdrawInSuspend;
    QList<MarketObject> marketWithdrawObjectList;
    QList<MarketMonster> marketWithdrawMonsterList;

    //player items
    QHash<quint32,quint32> warehouse_items;
    QHash<quint32,qint32> change_warehouse_items;
    QHash<quint32,quint32> items;
    QHash<QListWidgetItem *,quint32> items_graphical;
    QHash<quint32,QListWidgetItem *> items_to_graphical;
    QHash<QListWidgetItem *,quint32> shop_items_graphical;
    QHash<quint32,QListWidgetItem *> shop_items_to_graphical;
    QHash<QListWidgetItem *,quint32> plants_items_graphical;
    QHash<quint32,QListWidgetItem *> plants_items_to_graphical;
    QHash<QListWidgetItem *,quint32> crafting_recipes_items_graphical;
    QHash<quint32,QListWidgetItem *> crafting_recipes_items_to_graphical;
    QHash<QListWidgetItem *,quint32> fight_attacks_graphical;
    QHash<QListWidgetItem *,quint32> monsters_items_graphical;
    QHash<QListWidgetItem *,quint32> attack_to_learn_graphical;
    QHash<QListWidgetItem *,quint32> quests_to_id_graphical;
    bool inSelection;
    QList<quint32> objectInUsing;
    QList<quint32> monster_to_deposit,monster_to_withdraw;

    //crafting
    QList<QList<QPair<quint32,quint32> > > materialOfRecipeInUsing;
    QList<QPair<quint32,quint32> > productOfRecipeInUsing;

    //bot
    Bot actualBot;
    quint32 fightId;

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
    quint8 lastStepUsed;
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
    quint32 trapItemId;
    int displayTrapProgression;
    QTimer displayTrapTimer;
    bool trapSuccess;
    qint32 attack_quantity_changed;
    QList<BattleInformations> battleInformationsList;
    QList<quint32> botFightList;
    QHash<quint32,QListWidgetItem *> buffToGraphicalItemTop,buffToGraphicalItemBottom;
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
    QHash<quint32,quint32> tradeOtherObjects,tradeCurrentObjects;
    QList<CatchChallenger::PlayerMonster> tradeEvolutionMonsters,tradeOtherMonsters,tradeCurrentMonsters;

    //learn
    quint32 monsterToLearn;

    //quest
    bool isInQuest;
    quint32 questId;
    QHash<quint32, PlayerQuest> quests;

    //battle
    BattleStep battleStep;

    QList<QOggSimplePlayer*> ambiance;
    QThread audioReadThread;

    static QString text_type;
    static QString text_lang;
    static QString text_en;
    static QString text_text;
signals:
    void newError(QString error,QString detailedError);
    //datapack
    void parseDatapack(const QString &datapackPath);
    void sendsetMultiPlayer(const bool & multiplayer);
    void teleportDone();
    //plant, can do action only if the previous is finish
    void useSeed(const quint8 &plant_id);
    void collectMaturePlant();
    //inventory
    void destroyObject(quint32 object,quint32 quantity=1);
};
}

#endif
