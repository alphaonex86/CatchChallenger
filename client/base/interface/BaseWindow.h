#include <QWidget>
#include <QMessageBox>
#include <QRegExp>
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

#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../base/Api_client_real.h"
#include "../base/Api_protocol.h"
#include "MapController.h"
#include "Chat.h"

#ifndef POKECRAFT_BASEWINDOW_H
#define POKECRAFT_BASEWINDOW_H

namespace Ui {
    class BaseWindowUI;
}

namespace Pokecraft {
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
    enum ObjectType
    {
        ObjectType_All,
        ObjectType_Seed,
        ObjectType_Sell
    };
    ObjectType waitedObjectType;
    enum QueryType
    {
        QueryType_Seed,
        QueryType_CollectPlant
    };
    enum MoveType
    {
        MoveType_Enter,
        MoveType_Dead
    };
protected:
    void changeEvent(QEvent *e);
public slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
    void selectObject(const ObjectType &objectType);
    void objectSelection(const bool &ok,const quint32 &itemId,const quint32 &quantity=1);
private slots:
    void message(QString message);
    void disconnected(QString reason);
    void notLogged(QString reason);
    void protocol_is_good();
    void newError(QString error,QString detailedError);
    void error(QString error);

    //player UI
    void on_pushButton_interface_bag_clicked();
    void on_toolButton_quit_inventory_clicked();
    void on_inventory_itemSelectionChanged();
    void on_listPlantList_itemSelectionChanged();
    void tipTimeout();
    void gainTimeout();
    void showTip(const QString &tip);
    void showGain(const QString &gain);
    void addQuery(const QueryType &queryType);
    void removeQuery(const QueryType &queryType);
    void updateQueryList();
    void loadSettings();
    //player
    void logged();
    void updatePlayerImage();
    void have_current_player_info();
    void have_inventory(const QHash<quint32,quint32> &items);
    void add_to_inventory(const QHash<quint32,quint32> &items);
    void remove_to_inventory(const QHash<quint32,quint32> &items);
    void load_inventory();
    void load_plant_inventory();
    void load_crafting_inventory();
    void load_monsters();
    bool check_monsters();
    void addCash(const quint32 &cash);
    void removeCash(const quint32 &cash);
    //render
    void stopped_in_front_of(Pokecraft::Map_client *map, quint8 x, quint8 y);
    bool stopped_in_front_of_check_bot(Pokecraft::Map_client *map, quint8 x, quint8 y);
    void actionOn(Pokecraft::Map_client *map, quint8 x, quint8 y);
    bool actionOnCheckBot(Pokecraft::Map_client *map, quint8 x, quint8 y);
    void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);

    //datapack
    void haveTheDatapack();

    //inventory
    void on_inventory_itemActivated(QListWidgetItem *item);
    void objectUsed(const ObjectUsage &objectUsage);

    //shop
    void haveShopList(const QList<ItemToSellOrBuy> &items);
    void haveBuyObject(const BuyStat &stat,const quint32 &newPrice);
    void haveSellObject(const SoldStat &stat,const quint32 &newPrice);
    void displaySellList();

    //plant
    void seed_planted(const bool &ok);
    void plant_collected(const Pokecraft::Plant_collect &stat);
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage);

    //network
    void updateRXTX();

    //bot
    void goToBotStep(const quint8 &step);

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
    void on_checkBoxZoom_toggled(bool checked);
    void on_checkBoxLimitFPS_toggled(bool checked);
    void on_spinBoxMaxFPS_editingFinished();
    void on_IG_dialog_text_linkActivated(const QString &link);
    void on_toolButton_quit_shop_clicked();
    void on_shopItemList_itemActivated(QListWidgetItem *item);
    void on_shopItemList_itemSelectionChanged();
    void on_shopBuy_clicked();
    void on_pushButton_interface_monsters_clicked();
    void on_toolButton_monster_list_quit_clicked();

    //fight
    void fightCollision(Pokecraft::Map_client *map, const quint8 &x, const quint8 &y);
    void on_pushButtonFightEnterNext_clicked();
    void moveFightMonsterBottom();
    void updateCurrentMonsterInformation();
    void moveFightMonsterTop();
    void updateOtherMonsterInformation();
    void on_toolButtonFightQuit_clicked();
    void on_pushButtonFightAttack_clicked();
    void on_pushButtonFightMonster_clicked();
    void on_pushButtonFightAttackConfirmed_clicked();
    void on_pushButtonFightReturn_clicked();
    void on_listWidgetFightAttack_itemSelectionChanged();
    void finalFightTextQuit();
    void teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const Pokecraft::Direction &direction);
    void doNextAction();
    void displayAttack();
    void displayText(const QString &text);
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
    quint64 tempCashForBuy,cash;
    quint32 tempQuantityForBuy,tempItemForBuy;
    //selection of quantity
    quint32 tempQuantityForSell;
    bool waitToSell;
    QList<ItemToSellOrBuy> itemsToSell;
    QPixmap playerFrontImage,playerBackImage;

    //plant seed in waiting
    quint32 seed_in_waiting;
    bool seedWait,collectWait;

    QTime updateRXTXTime;
    QTimer updateRXTXTimer;
    quint64 previousRXSize,previousTXSize;
    QString toHtmlEntities(QString text);
    QSettings settings;
    bool haveShowDisconnectionReason;
    QString toSmilies(QString text);
    QStringList server_list;
    QAbstractSocket::SocketState socketState;
    QStringList skinFolderList;
    bool haveDatapack,havePlayerInformations,haveInventory,datapackIsParsed;

    //player items
    QHash<quint32,quint32> items;
    QHash<QListWidgetItem *,quint32> items_graphical;
    QHash<quint32,QListWidgetItem *> items_to_graphical;
    QHash<QListWidgetItem *,quint32> shop_items_graphical;
    QHash<quint32,QListWidgetItem *> shop_items_to_graphical;
    QHash<QListWidgetItem *,quint32> plants_items_graphical;
    QHash<quint32,QListWidgetItem *> plants_items_to_graphical;
    QHash<QListWidgetItem *,quint32> crafting_recipes_items_graphical;
    QHash<quint32,QListWidgetItem *> crafting_recipes_items_to_graphical;
    QHash<QListWidgetItem *,PlayerMonster::Skill> fight_attacks_graphical;
    bool inSelection;
    QList<quint32> objectInUsing;

    //crafting
    QList<QList<QPair<quint32,quint32> > > materialOfRecipeInUsing;
    QList<QPair<quint32,quint32> > productOfRecipeInUsing;

    //bot
    Bot actualBot;

    //fight
    QTimer moveFightMonsterBottomTimer;
    QTimer moveFightMonsterTopTimer;
    QTimer displayAttackTimer;
    QTimer doNextActionTimer;
    QTime updateAttackTime;
    MoveType moveType;
    bool fightTimerFinish;
    int displayAttackProgression;
    enum DoNextActionStep
    {
        DoNextActionStep_Start,
        DoNextActionStep_Lose,
        DoNextActionStep_Win
    };
    DoNextActionStep doNextActionStep;
    void lose();
    void win();
    bool escape;
signals:
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
