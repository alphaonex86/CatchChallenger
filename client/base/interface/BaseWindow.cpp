#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "../ClientVariable.h"
#include "../fight/interface/FightEngine.h"
#include "DatapackClientLoader.h"
#include "MapController.h"
#include "Chat.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QRegExp>
#include <QScriptValue>
#include <QScriptEngine>

//do buy queue
//do sell queue

using namespace CatchChallenger;

BaseWindow* BaseWindow::baseWindow=NULL;

BaseWindow::BaseWindow() :
    ui(new Ui::BaseWindowUI)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<QHash<quint32,quint32> >("QHash<quint32,quint32>");
    qRegisterMetaType<QHash<quint32,quint32> >("CatchChallenger::Plant_collect");
    qRegisterMetaType<QList<ItemToSellOrBuy> >("QList<ItemToSell>");

    socketState=QAbstractSocket::UnconnectedState;

    MapController::mapController=new MapController(true,false,true,false);
    ProtocolParsing::initialiseTheVariable();
    ui->setupUi(this);
    Chat::chat=new Chat(ui->page_map);

    updateRXTXTimer.start(1000);
    updateRXTXTime.restart();

    tip_timeout.setInterval(TIMETODISPLAY_TIP);
    gain_timeout.setInterval(TIMETODISPLAY_GAIN);
    tip_timeout.setSingleShot(true);
    gain_timeout.setSingleShot(true);

    moveFightMonsterBottomTimer.setSingleShot(true);
    moveFightMonsterBottomTimer.setInterval(20);
    moveFightMonsterTopTimer.setSingleShot(true);
    moveFightMonsterTopTimer.setInterval(20);
    displayAttackTimer.setSingleShot(true);
    displayAttackTimer.setInterval(50);
    doNextActionTimer.setSingleShot(true);
    doNextActionTimer.setInterval(3000);

    connect(CatchChallenger::Api_client_real::client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(error(QString)),this,SLOT(error(QString)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(message(QString)),this,SLOT(message(QString)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(notLogged(QString)),this,SLOT(notLogged(QString)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(logged()),this,SLOT(logged()),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(haveTheDatapack()),this,SLOT(haveTheDatapack()),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(newError(QString,QString)),this,SLOT(newError(QString,QString)),Qt::QueuedConnection);

    //connect the map controler
    connect(CatchChallenger::Api_client_real::client,SIGNAL(have_current_player_info(CatchChallenger::Player_private_and_public_informations)),this,SLOT(have_current_player_info()),Qt::QueuedConnection);

    //inventory
    connect(CatchChallenger::Api_client_real::client,SIGNAL(have_inventory(QHash<quint32,quint32>)),this,SLOT(have_inventory(QHash<quint32,quint32>)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(add_to_inventory(QHash<quint32,quint32>)),this,SLOT(add_to_inventory(QHash<quint32,quint32>)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(remove_to_inventory(QHash<quint32,quint32>)),this,SLOT(remove_to_inventory(QHash<quint32,quint32>)));

    //chat
    connect(CatchChallenger::Api_client_real::client,SIGNAL(new_chat_text(CatchChallenger::Chat_type,QString,QString,CatchChallenger::Player_type)),Chat::chat,SLOT(new_chat_text(CatchChallenger::Chat_type,QString,QString,CatchChallenger::Player_type)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(new_system_text(CatchChallenger::Chat_type,QString)),Chat::chat,SLOT(new_system_text(CatchChallenger::Chat_type,QString)));
    connect(this,SIGNAL(sendsetMultiPlayer(bool)),Chat::chat,SLOT(setVisible(bool)),Qt::QueuedConnection);

    connect(CatchChallenger::Api_client_real::client,SIGNAL(number_of_player(quint16,quint16)),this,SLOT(number_of_player(quint16,quint16)));
    //connect(CatchChallenger::Api_client_real::client,SIGNAL(new_player_info()),this,SLOT(update_chat()),Qt::QueuedConnection);

    //connect the datapack loader
    connect(&DatapackClientLoader::datapackLoader,SIGNAL(datapackParsed()),this,SLOT(datapackParsed()),Qt::QueuedConnection);
    connect(this,SIGNAL(parseDatapack(QString)),&DatapackClientLoader::datapackLoader,SLOT(parseDatapack(QString)),Qt::QueuedConnection);
    connect(&DatapackClientLoader::datapackLoader,SIGNAL(datapackParsed()),MapController::mapController,SLOT(datapackParsed()),Qt::QueuedConnection);

    //render, logical part into Map_Client
    connect(MapController::mapController,SIGNAL(stopped_in_front_of(CatchChallenger::Map_client*,quint8,quint8)),this,SLOT(stopped_in_front_of(CatchChallenger::Map_client*,quint8,quint8)));
    connect(MapController::mapController,SIGNAL(actionOn(CatchChallenger::Map_client*,quint8,quint8)),this,SLOT(actionOn(CatchChallenger::Map_client*,quint8,quint8)));
    connect(MapController::mapController,SIGNAL(blockedOn(MapVisualiserPlayer::BlockedOn)),this,SLOT(blockedOn(MapVisualiserPlayer::BlockedOn)));
    connect(MapController::mapController,SIGNAL(error(QString)),this,SLOT(error(QString)));

    //fight
    connect(CatchChallenger::Api_client_real::client,SIGNAL(random_seeds(QByteArray)),&FightEngine::fightEngine,SLOT(appendRandomSeeds(QByteArray)));
    connect(MapController::mapController,SIGNAL(fightCollision(CatchChallenger::Map_client*,quint8,quint8)),this,SLOT(fightCollision(CatchChallenger::Map_client*,quint8,quint8)));
    connect(&moveFightMonsterBottomTimer,SIGNAL(timeout()),this,SLOT(moveFightMonsterBottom()));
    connect(&moveFightMonsterTopTimer,SIGNAL(timeout()),this,SLOT(moveFightMonsterTop()));
    connect(&displayAttackTimer,SIGNAL(timeout()),this,SLOT(displayAttack()));
    connect(&doNextActionTimer,SIGNAL(timeout()),this,SLOT(doNextAction()));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(teleportTo(quint32,quint16,quint16,CatchChallenger::Direction)),this,SLOT(teleportTo(quint32,quint16,quint16,CatchChallenger::Direction)),Qt::QueuedConnection);

    //plants
    connect(this,SIGNAL(useSeed(quint8)),CatchChallenger::Api_client_real::client,SLOT(useSeed(quint8)));
    connect(this,SIGNAL(collectMaturePlant()),CatchChallenger::Api_client_real::client,SLOT(collectMaturePlant()));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(seed_planted(bool)),this,SLOT(seed_planted(bool)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(plant_collected(CatchChallenger::Plant_collect)),this,SLOT(plant_collected(CatchChallenger::Plant_collect)));
    //crafting
    connect(CatchChallenger::Api_client_real::client,SIGNAL(recipeUsed(RecipeUsage)),this,SLOT(recipeUsed(RecipeUsage)));
    //trade
    connect(CatchChallenger::Api_client_real::client,SIGNAL(tradeRequested(QString,quint8)),this,SLOT(tradeRequested(QString,quint8)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(tradeAcceptedByOther(QString,quint8)),this,SLOT(tradeAcceptedByOther(QString,quint8)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(tradeCanceledByOther()),this,SLOT(tradeCanceledByOther()));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(tradeFinishedByOther()),this,SLOT(tradeFinishedByOther()));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(tradeValidatedByTheServer()),this,SLOT(tradeValidatedByTheServer()));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(tradeAddTradeCash(quint64)),this,SLOT(tradeAddTradeCash(quint64)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(tradeAddTradeObject(quint32,quint32)),this,SLOT(tradeAddTradeObject(quint32,quint32)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(tradeAddTradeMonster(CatchChallenger::PlayerMonster)),this,SLOT(tradeAddTradeMonster(CatchChallenger::PlayerMonster)));
    //inventory
    connect(CatchChallenger::Api_client_real::client,SIGNAL(objectUsed(ObjectUsage)),this,SLOT(objectUsed(ObjectUsage)));
    //shop
    connect(CatchChallenger::Api_client_real::client,SIGNAL(haveShopList(QList<ItemToSellOrBuy>)),this,SLOT(haveShopList(QList<ItemToSellOrBuy>)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(haveSellObject(SoldStat,quint32)),this,SLOT(haveSellObject(SoldStat,quint32)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(haveBuyObject(BuyStat,quint32)),this,SLOT(haveBuyObject(BuyStat,quint32)));

    connect(this,SIGNAL(destroyObject(quint32,quint32)),CatchChallenger::Api_client_real::client,SLOT(destroyObject(quint32,quint32)));
    connect(&updateRXTXTimer,SIGNAL(timeout()),this,SLOT(updateRXTX()));

    connect(&tip_timeout,SIGNAL(timeout()),this,SLOT(tipTimeout()));
    connect(&gain_timeout,SIGNAL(timeout()),this,SLOT(gainTimeout()));

    MapController::mapController->setDatapackPath(CatchChallenger::Api_client_real::client->get_datapack_base_name());

    renderFrame = new QFrame(ui->page_map);
    renderFrame->setObjectName(QString::fromUtf8("renderFrame"));
    renderFrame->setMinimumSize(QSize(600, 572));
    QVBoxLayout *renderLayout = new QVBoxLayout(renderFrame);
    renderLayout->setSpacing(0);
    renderLayout->setContentsMargins(0, 0, 0, 0);
    renderLayout->setObjectName(QString::fromUtf8("renderLayout"));
    renderLayout->addWidget(MapController::mapController);
    renderFrame->setGeometry(QRect(0, 0, 800, 516));
    renderFrame->lower();
    renderFrame->lower();
    renderFrame->lower();

    Chat::chat->setGeometry(QRect(0, 0, 300, 400));

    resetAll();
    loadSettings();

    MapController::mapController->setFocus();

    CatchChallenger::Api_client_real::client->startReadData();
}

BaseWindow::~BaseWindow()
{
    delete ui;
    delete MapController::mapController;
    delete Chat::chat;
}

void BaseWindow::tradeRequested(const QString &pseudo,const quint8 &skinInt)
{
    QMessageBox::StandardButton button=QMessageBox::question(this,tr("Trade request"),tr("Do you accept the trade with <b>%1</b>?").arg(pseudo),QMessageBox::Yes|QMessageBox::No);
    if(button!=QMessageBox::Yes)
    {
        CatchChallenger::Api_client_real::client->tradeRefused();
        return;
    }
    CatchChallenger::Api_client_real::client->tradeAccepted();
    tradeAcceptedByOther(pseudo,skinInt);
}

void BaseWindow::tradeAcceptedByOther(const QString &pseudo,const quint8 &skinInt)
{
    ui->stackedWidget->setCurrentWidget(ui->page_trade);
    tradeOtherStat=TradeOtherStat_InWait;
    //reset the current player info
    ui->tradePlayerCash->setMinimum(0);
    ui->tradePlayerCash->setValue(0);
    ui->tradePlayerItems->clear();
    ui->tradePlayerMonsters->clear();
    ui->tradePlayerCash->setEnabled(true);
    ui->tradePlayerItems->setEnabled(true);
    ui->tradePlayerMonsters->setEnabled(true);
    ui->tradeAddItem->setEnabled(true);
    ui->tradeAddMonster->setEnabled(true);
    ui->tradeValidate->setEnabled(true);

    skinFolderList=CatchChallenger::FacilityLib::skinIdList(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN);
    QPixmap otherFrontImage;
    //front image
    if(skinInt<skinFolderList.size())
    {
        otherFrontImage=QPixmap(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(skinInt)+"/front.png");
        if(otherFrontImage.isNull())
        {
            otherFrontImage=QPixmap(":/images/player_default/front.png");
            qDebug() << "Unable to load the player image: "+CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(skinInt)+"/front.png";
        }
    }
    else
    {
        otherFrontImage=QPixmap(":/images/player_default/front.png");
        qDebug() << "The skin id: "+QString::number(skinInt)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into tradeRequested()";
    }

    //reset the other player info
    ui->tradeOtherImage->setPixmap(otherFrontImage);
    ui->tradeOtherPseudo->setText(pseudo);
    ui->tradeOtherCash->setValue(0);
    ui->tradeOtherItems->clear();
    ui->tradeOtherMonsters->clear();
    ui->tradeOtherStat->setText(tr("The other player have not validation their selection"));
}

void BaseWindow::tradeCanceledByOther()
{
    if(ui->stackedWidget->currentWidget()!=ui->page_trade)
        return;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("The other player have canceled your trade request"));
    addCash(ui->tradePlayerCash->value());
    add_to_inventory(tradeCurrentObjects,false);
    CatchChallenger::FightEngine::fightEngine.addPlayerMonster(tradeCurrentMonsters);
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
}

void BaseWindow::tradeFinishedByOther()
{
    tradeOtherStat=TradeOtherStat_Accepted;
    if(!ui->tradeValidate->isEnabled())
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    else
        ui->tradeOtherStat->setText(tr("The other player have validated the selection"));
}

void BaseWindow::tradeValidatedByTheServer()
{
    if(ui->stackedWidget->currentWidget()==ui->page_trade)
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("Your trade is successfull"));
    add_to_inventory(tradeOtherObjects);
    addCash(ui->tradeOtherCash->value());
    CatchChallenger::FightEngine::fightEngine.addPlayerMonster(tradeOtherMonsters);
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
}

void BaseWindow::tradeAddTradeCash(const quint64 &cash)
{
    ui->tradeOtherCash->setValue(ui->tradeOtherCash->value()+cash);
}

void BaseWindow::tradeAddTradeObject(const quint32 &item,const quint32 &quantity)
{
    if(tradeOtherObjects.contains(item))
        tradeOtherObjects[item]+=quantity;
    else
        tradeOtherObjects[item]=quantity;
    ui->tradeOtherItems->clear();
    QHashIterator<quint32,quint32> i(tradeOtherObjects);
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item=new QListWidgetItem();
        if(DatapackClientLoader::datapackLoader.items.contains(i.key()))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.items[i.key()].image);
            if(i.value()>1)
                item->setText(QString::number(i.value()));
            item->setToolTip(DatapackClientLoader::datapackLoader.items[i.key()].name);
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(i.value()>1)
                item->setText(QString("id: %1 (x%2)").arg(i.key()).arg(i.value()));
            else
                item->setText(QString("id: %1").arg(i.key()));
        }
        ui->tradeOtherItems->addItem(item);
    }
}

void BaseWindow::tradeUpdateCurrentObject()
{
    ui->tradePlayerItems->clear();
    QHashIterator<quint32,quint32> i(tradeCurrentObjects);
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item=new QListWidgetItem();
        if(DatapackClientLoader::datapackLoader.items.contains(i.key()))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.items[i.key()].image);
            if(i.value()>1)
                item->setText(QString::number(i.value()));
            item->setToolTip(DatapackClientLoader::datapackLoader.items[i.key()].name);
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(i.value()>1)
                item->setText(QString("id: %1 (x%2)").arg(i.key()).arg(i.value()));
            else
                item->setText(QString("id: %1").arg(i.key()));
        }
        ui->tradePlayerItems->addItem(item);
    }
}

QString BaseWindow::lastLocation() const
{
    return MapController::mapController->lastLocation();
}

void BaseWindow::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
    break;
    default:
    break;
    }
}

void BaseWindow::message(QString message)
{
    qDebug() << message;
}

void BaseWindow::number_of_player(quint16 number,quint16 max)
{
    ui->frame_main_display_interface_player->show();
    ui->label_interface_number_of_player->setText(QString("%1/%2").arg(number).arg(max));
}

void BaseWindow::on_toolButton_interface_quit_clicked()
{
    CatchChallenger::Api_client_real::client->tryDisconnect();
}

void BaseWindow::on_toolButton_quit_interface_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_pushButton_interface_trainer_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_player);
}

//return ok, itemId
void BaseWindow::selectObject(const ObjectType &objectType)
{
    inSelection=true;
    waitedObjectType=objectType;
    switch(objectType)
    {
        case ObjectType_Seed:
            ui->stackedWidget->setCurrentWidget(ui->page_plants);
            on_listPlantList_itemSelectionChanged();
        break;
        case ObjectType_Sell:
            ui->stackedWidget->setCurrentWidget(ui->page_shop);
            displaySellList();
        break;
        case ObjectType_MonsterToTrade:
        case ObjectType_MonsterToLearn:
            ui->selectMonster->setVisible(true);
            ui->stackedWidget->setCurrentWidget(ui->page_monster);
        break;
        case ObjectType_All:
        case ObjectType_Trade:
        default:
            ui->inventoryUse->setText(tr("Select"));
            ui->inventoryUse->setVisible(true);
            ui->stackedWidget->setCurrentWidget(ui->page_inventory);
            on_listCraftingList_itemSelectionChanged();
        break;
    }
}

void BaseWindow::objectSelection(const bool &ok, const quint32 &itemId, const quint32 &quantity)
{
    inSelection=false;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    ui->inventoryUse->setText(tr("Select"));
    switch(waitedObjectType)
    {
        case ObjectType_Sell:
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items[itemId]<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            items[itemId]-=quantity;
            if(items[itemId]==0)
                items.remove(itemId);
            ItemToSellOrBuy tempItem;
            tempItem.object=itemId;
            tempItem.quantity=quantity;
            tempItem.price=DatapackClientLoader::datapackLoader.items[itemId].price/2;
            itemsToSell << tempItem;
            CatchChallenger::Api_client_real::client->sellObject(shopId,tempItem.object,tempItem.quantity,tempItem.price);
            load_inventory();
            load_plant_inventory();
        break;
        case ObjectType_Trade:
            ui->stackedWidget->setCurrentWidget(ui->page_trade);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items[itemId]<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            CatchChallenger::Api_client_real::client->addObject(itemId,quantity);
            items[itemId]-=quantity;
            if(items[itemId]==0)
                items.remove(itemId);
            if(tradeCurrentObjects.contains(itemId))
                tradeCurrentObjects[itemId]+=quantity;
            else
                tradeCurrentObjects[itemId]=quantity;
            load_inventory();
            load_plant_inventory();
            tradeUpdateCurrentObject();
        break;
        case ObjectType_MonsterToLearn:
        {
            if(!ok)
            {
                ui->stackedWidget->setCurrentWidget(ui->page_map);
                return;
            }
            ui->stackedWidget->setCurrentWidget(ui->page_learn);
            monsterToLearn=itemId;
            if(!showLearnSkill(monsterToLearn))
            {
                newError(tr("Internal error"),"Unable to load the right monster");
                return;
            }
        }
        break;
        case ObjectType_MonsterToTrade:
        {
            if(waitedObjectType==ObjectType_MonsterToLearn)
            {
                ui->stackedWidget->setCurrentWidget(ui->page_learn);
                monsterToLearn=itemId;
                return;
            }
            ui->stackedWidget->setCurrentWidget(ui->page_trade);
            if(!ok)
                break;
            QList<PlayerMonster> playerMonster=FightEngine::fightEngine.getPlayerMonster();
            if(playerMonster.size()<=1)
            {
                QMessageBox::warning(this,tr("Warning"),tr("You can't trade your last monster"));
                break;
            }
            if(!FightEngine::fightEngine.remainMonstersToFight(itemId))
            {
                QMessageBox::warning(this,tr("Warning"),tr("You don't have more monster valid"));
                break;
            }
            //get the right monster
            int index=0;
            while(index<playerMonster.size())
            {
                if(playerMonster.at(index).id==itemId)
                {
                    tradeCurrentMonsters << playerMonster.at(index);
                    FightEngine::fightEngine.removeMonster(itemId);
                    CatchChallenger::Api_client_real::client->addMonster(itemId);
                    QListWidgetItem *item=new QListWidgetItem();
                    item->setText(CatchChallenger::FightEngine::fightEngine.monsterExtra[tradeCurrentMonsters.last().monster].name);
                    item->setToolTip(tr("Level: %1").arg(tradeCurrentMonsters.last().level));
                    item->setIcon(CatchChallenger::FightEngine::fightEngine.monsterExtra[tradeCurrentMonsters.last().monster].front);
                    ui->tradePlayerMonsters->addItem(item);
                    break;
                }
                index++;
            }
            load_monsters();
        }
        break;
        case ObjectType_Seed:
            ui->plantUse->setVisible(false);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            items[itemId]--;
            if(items[itemId]==0)
                items.remove(itemId);
            seed_in_waiting=itemId;
            addQuery(QueryType_Seed);
            seedWait=true;
            if(DatapackClientLoader::datapackLoader.itemToPlants.contains(itemId))
            {
                load_plant_inventory();
                load_inventory();
                qDebug() << QString("send seed for: %1").arg(DatapackClientLoader::datapackLoader.itemToPlants[itemId]);
                emit useSeed(DatapackClientLoader::datapackLoader.itemToPlants[itemId]);
            }
            else
                qDebug() << QString("seed not found for item: %1").arg(itemId);
        break;
        default:
        qDebug() << "waitedObjectType is unknow";
        return;
    }
    waitedObjectType=ObjectType_All;
}

void BaseWindow::add_to_inventory(const QHash<quint32,quint32> &items,const bool &showGain)
{
    if(items.empty())
        return;
    if(showGain)
    {
        QString html=tr("You have obtained: ");
        QStringList objects;
        QHashIterator<quint32,quint32> i(items);
        while (i.hasNext()) {
            i.next();

            //add really to the list
            if(this->items.contains(i.key()))
                this->items[i.key()]+=i.value();
            else
                this->items[i.key()]=i.value();

            QPixmap image;
            QString name;
            if(DatapackClientLoader::datapackLoader.items.contains(i.key()))
            {
                image=DatapackClientLoader::datapackLoader.items[i.key()].image;
                name=DatapackClientLoader::datapackLoader.items[i.key()].name;
            }
            else
            {
                image=DatapackClientLoader::datapackLoader.defaultInventoryImage();
                name=QString("id: %1").arg(i.key());
            }

            image=image.scaled(24,24);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            image.save(&buffer, "PNG");
            if(objects.size()<16)
            {
                if(i.value()>1)
                    objects << QString("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(i.value()).arg(name);
                else
                    objects << QString("%2 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(name);
            }
        }
        if(objects.size()==16)
            objects << "...";
        html+=objects.join(", ");
        BaseWindow::showGain(html);
    }

    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
}

void BaseWindow::remove_to_inventory(const QHash<quint32,quint32> &items)
{
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();

        //add really to the list
        if(this->items.contains(i.key()))
        {
            if(this->items[i.key()]<=i.value())
                this->items.remove(i.key());
            else
                this->items[i.key()]-=i.value();
        }
    }
    load_inventory();
    load_plant_inventory();
}

void BaseWindow::newError(QString error,QString detailedError)
{
    qDebug() << detailedError.toLocal8Bit();
    if(socketState!=QAbstractSocket::ConnectedState)
        return;
    CatchChallenger::Api_client_real::client->tryDisconnect();
    resetAll();
    QMessageBox::critical(this,tr("Error"),error);
}

void BaseWindow::error(QString error)
{
    newError(tr("Error with the protocol"),error);
}

void BaseWindow::on_pushButton_interface_bag_clicked()
{
    if(inSelection)
    {
        qDebug() << "BaseWindow::on_pushButton_interface_bag_clicked() in selection, can't click here";
        return;
    }
    ui->stackedWidget->setCurrentWidget(ui->page_inventory);
}

void BaseWindow::on_toolButton_quit_inventory_clicked()
{
    ui->inventory->reset();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(inSelection)
        objectSelection(false);
    on_inventory_itemSelectionChanged();
}

void BaseWindow::on_inventory_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
    {
        ui->inventory_image->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText("");
        ui->inventory_description->setText(tr("Select an object"));
        ui->inventoryInformation->setVisible(false);
        ui->inventoryUse->setVisible(false);
        ui->inventoryDestroy->setVisible(false);
        return;
    }
    QListWidgetItem *item=items.first();
    if(!DatapackClientLoader::datapackLoader.items.contains(items_graphical[item]))
    {
        ui->inventoryInformation->setVisible(false);
        ui->inventoryUse->setVisible(false);
        ui->inventoryDestroy->setVisible(!inSelection);
        ui->inventory_image->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText(tr("Unknow name"));
        ui->inventory_description->setText(tr("Unknow description"));
        return;
    }

    const DatapackClientLoader::Item &content=DatapackClientLoader::datapackLoader.items[items_graphical[item]];
    ui->inventoryInformation->setVisible(!inSelection &&
                                         /* is a plant */
                                         DatapackClientLoader::datapackLoader.itemToPlants.contains(items_graphical[item])
                                         );
    ui->inventoryUse->setVisible(inSelection ||
                                         /* is a recipe */
                                         DatapackClientLoader::datapackLoader.itemToCrafingRecipes.contains(items_graphical[item])
                                         );
    ui->inventoryDestroy->setVisible(!inSelection);
    ui->inventory_image->setPixmap(content.image);
    ui->inventory_name->setText(content.name);
    ui->inventory_description->setText(content.description);
}

void BaseWindow::tipTimeout()
{
    ui->tip->setVisible(false);
}

void BaseWindow::gainTimeout()
{
    ui->gain->setVisible(false);
}

void BaseWindow::showTip(const QString &tip)
{
    ui->tip->setVisible(true);
    ui->tip->setText(tip);
    tip_timeout.start();
}

void BaseWindow::showGain(const QString &gain)
{
    ui->gain->setVisible(true);
    ui->gain->setText(gain);
    gain_timeout.start();
}

void BaseWindow::addQuery(const QueryType &queryType)
{
    if(queryList.size()==0)
        ui->persistant_tip->setVisible(true);
    queryList << queryType;
    updateQueryList();
}

void BaseWindow::removeQuery(const QueryType &queryType)
{
    queryList.removeOne(queryType);
    if(queryList.size()==0)
        ui->persistant_tip->setVisible(false);
    else
        updateQueryList();
}

void BaseWindow::updateQueryList()
{
    QStringList queryStringList;
    int index=0;
    while(index<queryList.size() && index<5)
    {
        switch(queryList.at(index))
        {
            case QueryType_Seed:
                queryStringList << tr("Planting seed...");
            break;
            case QueryType_CollectPlant:
                queryStringList << tr("Collecting plant...");
            break;
            default:
                queryStringList << tr("Unknow action...");
            break;
        }
        index++;
    }
    ui->persistant_tip->setText(queryStringList.join("<br />"));
}

void BaseWindow::on_toolButton_quit_options_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::stopped_in_front_of(CatchChallenger::Map_client *map, quint8 x, quint8 y)
{
    if(stopped_in_front_of_check_bot(map,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index).x==x && map->plantList.at(index).y==y)
            {
                quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map->plantList.at(index).mature_at<current_time)
                    showTip(tr("To recolt the plant press <i>Enter</i>"));
                else
                    showTip(tr("This plant is growing and can't be collected"));
                return;
            }
            index++;
        }
        showTip(tr("To plant a seed press <i>Enter</i>"));
        return;
    }
    else
    {
        //check bot with border
        CatchChallenger::Map * current_map=map;
        switch(MapController::mapController->getDirection())
        {
            case CatchChallenger::Direction_look_at_left:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_left,&current_map,&x,&y,false))
                    stopped_in_front_of_check_bot(map,x,y);
            break;
            case CatchChallenger::Direction_look_at_right:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_right,&current_map,&x,&y,false))
                    stopped_in_front_of_check_bot(map,x,y);
            break;
            case CatchChallenger::Direction_look_at_top:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_top,&current_map,&x,&y,false))
                    stopped_in_front_of_check_bot(map,x,y);
            break;
            case CatchChallenger::Direction_look_at_bottom:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_bottom,&current_map,&x,&y,false))
                    stopped_in_front_of_check_bot(map,x,y);
            break;
            default:
            break;
        }
    }
}

bool BaseWindow::stopped_in_front_of_check_bot(CatchChallenger::Map_client *map, quint8 x, quint8 y)
{
    if(!map->bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    showTip(tr("To interact with the bot press <i>Enter</i>"));
    return true;
}

void BaseWindow::actionOn(Map_client *map, quint8 x, quint8 y)
{
    if(actionOnCheckBot(map,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index).x==x && map->plantList.at(index).y==y)
            {
                if(collectWait)
                {
                    showTip(tr("Wait to finish to collect the previous plant"));
                    return;
                }
                quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map->plantList.at(index).mature_at<current_time)
                {
                    collectWait=true;
                    addQuery(QueryType_CollectPlant);
                    emit collectMaturePlant();
                }
                else
                    showTip(tr("This plant is growing and can't be collected"));
                return;
            }
            index++;
        }
        if(seedWait)
        {
            showTip(tr("Wait to finish to plant the previous seed"));
            return;
        }
        selectObject(ObjectType_Seed);
        return;
    }
    else
    {
        //check bot with border
        CatchChallenger::Map * current_map=map;
        switch(MapController::mapController->getDirection())
        {
            case CatchChallenger::Direction_look_at_left:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_left,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            case CatchChallenger::Direction_look_at_right:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_right,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            case CatchChallenger::Direction_look_at_top:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_top,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            case CatchChallenger::Direction_look_at_bottom:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_bottom,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            default:
            break;
        }
    }
}

bool BaseWindow::actionOnCheckBot(CatchChallenger::Map_client *map, quint8 x, quint8 y)
{
    if(!map->bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    actualBot=map->bots[QPair<quint8,quint8>(x,y)];
    isInQuest=false;
    goToBotStep(1);
    return true;
}

void BaseWindow::blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar)
{
    switch(blockOnVar)
    {
        case MapVisualiserPlayer::BlockedOn_Grass:
        case MapVisualiserPlayer::BlockedOn_Wather:
        case MapVisualiserPlayer::BlockedOn_Cave:
            qDebug() << "You can't enter to the fight zone if you are not able to fight";
            showTip(tr("You can't enter to the fight zone if you are not able to fight"));
        break;
        case MapVisualiserPlayer::BlockedOn_RandomNumber:
            qDebug() << "You can't enter to the fight zone, because have not random number";
            showTip(tr("You can't enter to the fight zone, because have not random number"));
        break;
    }
}

//network
void BaseWindow::updateRXTX()
{
    quint64 RXSize=CatchChallenger::Api_client_real::client->getRXSize();
    quint64 TXSize=CatchChallenger::Api_client_real::client->getTXSize();
    if(previousRXSize>RXSize)
        previousRXSize=RXSize;
    if(previousTXSize>TXSize)
        previousTXSize=TXSize;
    double RXSpeed=(RXSize-previousRXSize)*1000/updateRXTXTime.elapsed();
    double TXSpeed=(TXSize-previousTXSize)*1000/updateRXTXTime.elapsed();
    if(RXSpeed<1024)
        ui->labelInput->setText(QString("%1B/s").arg(RXSpeed,0,'g',0));
    else if(RXSpeed<10240)
        ui->labelInput->setText(QString("%1KB/s").arg(RXSpeed/1024,0,'g',1));
    else
        ui->labelInput->setText(QString("%1KB/s").arg(RXSpeed/1024,0,'g',0));
    if(TXSpeed<1024)
        ui->labelOutput->setText(QString("%1B/s").arg(TXSpeed,0,'g',0));
    else if(TXSpeed<10240)
        ui->labelOutput->setText(QString("%1KB/s").arg(TXSpeed/1024,0,'g',1));
    else
        ui->labelOutput->setText(QString("%1KB/s").arg(TXSpeed/1024,0,'g',0));
    if(RXSpeed>0 && TXSpeed>0)
        qDebug() << QString("received: %1B/s, transmited: %2B/s").arg(RXSpeed).arg(TXSpeed);
    else
    {
        if(RXSpeed>0)
            qDebug() << QString("received: %1B/s").arg(RXSpeed);
        if(TXSpeed>0)
            qDebug() << QString("transmited: %1B/s").arg(TXSpeed);
    }
    updateRXTXTime.restart();
    previousRXSize=RXSize;
    previousTXSize=TXSize;
}

bool BaseWindow::haveNextStepQuestRequirements(const CatchChallenger::Quest &quest)
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check quest step requirement for: " << quest.id;
    #endif
    if(!quests.contains(quest.id))
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    quint8 step=quests[quest.id].step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        if(itemQuantity(item.item)<item.quantity)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "have not the quantity for the item: " << item.item;
            #endif
            return false;
        }
        index++;
    }
    return true;
}

bool BaseWindow::haveStartQuestRequirement(const CatchChallenger::Quest &quest)
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check quest requirement for: " << quest.id;
    #endif
    Player_private_and_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations();
    if(informations.quests.contains(quest.id))
    {
        if(informations.quests[quest.id].step!=0)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "can start the quest because is already running: " << questId;
            #endif
            return false;
        }
        if(informations.quests[quest.id].finish_one_time && !quest.repeatable)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "done one time and no repeatable: " << questId;
            #endif
            return false;
        }
    }
    int index=0;
    while(index<quest.requirements.quests.size())
    {
        const quint32 &questId=quest.requirements.quests.at(index);
        if(!informations.quests.contains(questId))
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "have never started the quest: " << questId;
            #endif
            return false;
        }
        if(!informations.quests[questId].finish_one_time)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest never finished: " << questId;
            #endif
            return false;
        }
        index++;
    }
    index=0;
    while(index<quest.requirements.reputation.size())
    {
        const CatchChallenger::Quest::ReputationRequirements &reputation=quest.requirements.reputation.at(index);
        if(informations.reputation.contains(reputation.type))
        {
            const PlayerReputation &playerReputation=informations.reputation[reputation.type];
            if(!reputation.positif)
            {
                if(-reputation.level<playerReputation.level)
                {
                    #ifdef DEBUG_CLIENT_QUEST
                    qDebug() << "reputation.level(" << reputation.level << ")<playerReputation.level(" << playerReputation.level << ")";
                    #endif
                    return false;
                }
            }
            else
            {
                if(reputation.level>playerReputation.level || playerReputation.point<0)
                {
                    #ifdef DEBUG_CLIENT_QUEST
                    qDebug() << "reputation.level(" << reputation.level << ")>playerReputation.level(" << playerReputation.level << ") || playerReputation.point(" << playerReputation.point << ")<0";
                    #endif
                    return false;
                }
            }
        }
        else
            if(!reputation.positif)//default level is 0, but required level is negative
            {
                #ifdef DEBUG_CLIENT_QUEST
                qDebug() << "reputation.level(" << reputation.level << ")<0 and no reputation.type=" << reputation.type;
                #endif
                return false;
            }
        index++;
    }
    return true;
}

bool BaseWindow::nextStepQuest(const Quest &quest)
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "drop quest step requirement for: " << quest.id;
    #endif
    if(!quests.contains(quest.id))
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    quint8 step=quests[quest.id].step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        QHash<quint32,quint32> items;
        items[item.item]=item.quantity;
        remove_to_inventory(items);
        index++;
    }
    quests[quest.id].step++;
    if(quests[quest.id].step==quest.steps.size())
    {
        #ifdef DEBUG_CLIENT_QUEST
        qDebug() << "finish the quest: " << quest.id;
        #endif
        then have finish the quest
    }
    return true;
}

bool BaseWindow::startQuest(const Quest &quest)
{
    if(!quests.contains(quest.id))
    {
        quests[quest.id].step=1;
        quests[quest.id].finish_one_time=false;
    }
    else
        quests[quest.id].step=1;
    return true;
}

bool BaseWindow::botHaveQuest(const quint32 &botId)
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check bot quest for: " << botId;
    #endif
    Player_private_and_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations();
    //do the not started quest here
    QList<quint32> botQuests=DatapackClientLoader::datapackLoader.botToQuestStart.values(botId);
    int index=0;
    while(index<botQuests.size())
    {
        const quint8 &questId=botQuests.at(index);
        const CatchChallenger::Quest &currentQuest=DatapackClientLoader::datapackLoader.quests[questId];
        if(!informations.quests.contains(botQuests.at(index)))
        {
            //quest not started
            if(haveStartQuestRequirement(currentQuest))
                return true;
            else
                {}//have not the requirement
        }
        else
        {
            if(!DatapackClientLoader::datapackLoader.quests.contains(botQuests.at(index)))
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(informations.quests[botQuests.at(index)].step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(informations.quests[botQuests.at(index)].finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(currentQuest))
                                return true;
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    QList<quint32> bots=currentQuest.steps.at(informations.quests[questId].step-1).bots;
                    if(bots.contains(botId))
                        return true;//in progress
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    QHashIterator<quint32, PlayerQuest> i(informations.quests);
    while (i.hasNext()) {
        i.next();
        if(!botQuests.contains(i.key()) && i.value().step>0)
        {
            CatchChallenger::Quest currentQuest=DatapackClientLoader::datapackLoader.quests[i.key()];
            QList<quint32> bots=currentQuest.steps.at(i.value().step-1).bots;
            if(bots.contains(botId))
                return true;//in progress, but not the starting bot
            else
                {}//it's another bot
        }
    }
    return false;
}

QList<QPair<quint32,QString> > BaseWindow::getQuestList(const quint32 &botId)
{
    QList<QPair<quint32,QString> > entryList;
    QPair<quint32,QString> oneEntry;
    Player_private_and_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations();
    //do the not started quest here
    QList<quint32> botQuests=DatapackClientLoader::datapackLoader.botToQuestStart.values(botId);
    int index=0;
    while(index<botQuests.size())
    {
        const quint8 &questId=botQuests.at(index);
        const CatchChallenger::Quest &currentQuest=DatapackClientLoader::datapackLoader.quests[questId];
        if(!informations.quests.contains(botQuests.at(index)))
        {
            //quest not started
            if(haveStartQuestRequirement(currentQuest))
            {
                oneEntry.first=questId;
                if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                    oneEntry.second=DatapackClientLoader::datapackLoader.questsExtra[questId].name;
                else
                {
                    qDebug() << "internal bug: quest extra not found";
                    oneEntry.second="???";
                }
                entryList << oneEntry;
            }
            else
                {}//have not the requirement
        }
        else
        {
            if(!DatapackClientLoader::datapackLoader.quests.contains(botQuests.at(index)))
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(informations.quests[botQuests.at(index)].step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(informations.quests[botQuests.at(index)].finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(currentQuest))
                            {
                                oneEntry.first=questId;
                                if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                                    oneEntry.second=DatapackClientLoader::datapackLoader.questsExtra[questId].name;
                                else
                                {
                                    qDebug() << "internal bug: quest extra not found";
                                    oneEntry.second="???";
                                }
                                entryList << oneEntry;
                            }
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    QList<quint32> bots=currentQuest.steps.at(informations.quests[questId].step-1).bots;
                    if(bots.contains(botId))
                    {
                        oneEntry.first=questId;
                        if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                            oneEntry.second=tr("%1 (in progress)").arg(DatapackClientLoader::datapackLoader.questsExtra[questId].name);
                        else
                        {
                            qDebug() << "internal bug: quest extra not found";
                            oneEntry.second=tr("??? (in progress)");
                        }
                        entryList << oneEntry;
                    }
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    QHashIterator<quint32, PlayerQuest> i(informations.quests);
    while (i.hasNext()) {
        i.next();
        if(!botQuests.contains(i.key()) && i.value().step>0)
        {
            CatchChallenger::Quest currentQuest=DatapackClientLoader::datapackLoader.quests[i.key()];
            QList<quint32> bots=currentQuest.steps.at(i.value().step-1).bots;
            if(bots.contains(botId))
            {
                //in progress, but not the starting bot
                oneEntry.first=i.key();
                oneEntry.second=tr("%1 (in progress)").arg(DatapackClientLoader::datapackLoader.questsExtra[i.key()].name);
                entryList << oneEntry;
            }
            else
                {}//it's another bot
        }
    }
    return entryList;
}

//bot
void BaseWindow::goToBotStep(const quint8 &step)
{
    isInQuest=false;
    if(!actualBot.step.contains(step))
    {
        showTip(tr("Error into the bot, repport this error please"));
        return;
    }
    if(actualBot.step[step].attribute("type")=="text")
    {
        QDomElement text = actualBot.step[step].firstChildElement("text");
        while(!text.isNull())
        {
            if(text.hasAttribute("lang") && text.attribute("lang")!="en")
            {
                //not enlish, skip for now
            }
            else
            {
                QString textToShow=text.text();
                textToShow=parseHtmlToDisplay(textToShow);
                ui->IG_dialog_text->setText(textToShow);
                ui->IG_dialog->setVisible(true);
                return;
            }
            text = actualBot.step[step].nextSiblingElement("text");
        }
        showTip(tr("Bot text not found, repport this error please"));
        return;
    }
    else if(actualBot.step[step].attribute("type")=="shop")
    {
        if(CatchChallenger::Api_client_real::client->getHaveShopAction())
        {
            showTip(tr("Already in shop action"));
            return;
        }
        if(!actualBot.step[step].hasAttribute("shop"))
        {
            showTip(tr("The shop call, but missing informations"));
            return;
        }
        bool ok;
        shopId=actualBot.step[step].attribute("shop").toUInt(&ok);
        if(!ok)
        {
            showTip(tr("The shop call, but wrong shop id"));
            return;
        }
        QPixmap pixmap;
        if(actualBot.properties.contains("skin"))
        {
            pixmap=QPixmap(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+"/"+actualBot.properties["skin"]+"/front.png");
            if(pixmap.isNull())
            {
                qDebug() << QString("Unable to load seller skin: %1").arg(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+"/"+actualBot.properties["skin"]+"/front.png");
                pixmap=QPixmap(":/images/player_default/front.png");
            }
        }
        else
            pixmap=QPixmap(":/images/player_default/front.png");
        pixmap=pixmap.scaled(160,160);
        ui->shopSellerImage->setPixmap(pixmap);
        ui->stackedWidget->setCurrentWidget(ui->page_shop);
        ui->shopItemList->clear();
        on_shopItemList_itemSelectionChanged();
        ui->shopDescription->setText(tr("Waiting the shop content"));
        ui->shopBuy->setVisible(false);
        qDebug() << "goToBotStep(), client->getShopList(shopId): " << shopId;
        CatchChallenger::Api_client_real::client->getShopList(shopId);
        ui->shopCash->setText(tr("Cash: %1").arg(cash));
        return;
    }
    else if(actualBot.step[step].attribute("type")=="sell")
    {
        if(!actualBot.step[step].hasAttribute("shop"))
        {
            showTip(tr("The shop call, but missing informations"));
            return;
        }
        bool ok;
        shopId=actualBot.step[step].attribute("shop").toUInt(&ok);
        if(!ok)
        {
            showTip(tr("The shop call, but wrong shop id"));
            return;
        }
        QPixmap pixmap;
        if(actualBot.properties.contains("skin"))
        {
            pixmap=QPixmap(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+"/"+actualBot.properties["skin"]+"/front.png");
            if(pixmap.isNull())
            {
                qDebug() << QString("Unable to load seller skin: %1").arg(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+"/"+actualBot.properties["skin"]+"/front.png");
                pixmap=QPixmap(":/images/player_default/front.png");
            }
        }
        else
            pixmap=QPixmap(":/images/player_default/front.png");
        pixmap=pixmap.scaled(160,160);
        ui->shopSellerImage->setPixmap(pixmap);
        if(CatchChallenger::Api_client_real::client->getHaveShopAction())
        {
            showTip(tr("Already in shop action"));
            return;
        }
        waitToSell=true;
        selectObject(ObjectType_Sell);
        return;
    }
    else if(actualBot.step[step].attribute("type")=="learn")
    {
        selectObject(ObjectType_MonsterToLearn);
        return;
    }
    else if(actualBot.step[step].attribute("type")=="quests")
    {
        QString textToShow;
        textToShow+="<ul>";
        QList<QPair<quint32,QString> > quests=BaseWindow::getQuestList(actualBot.botId);
        int index=0;
        while(index<quests.size())
        {
            QPair<quint32,QString> quest=quests.at(index);
            textToShow+=QString("<a href=\"quest_%1\">%2</a>").arg(quest.first).arg(quest.second);
            index++;
        }
        textToShow+="</ul>";
        ui->IG_dialog_text->setText(textToShow);
        ui->IG_dialog->setVisible(true);
        return;
    }
    else
    {
        showTip(tr("Bot step type error, repport this error please"));
        return;
    }
}

QString BaseWindow::parseHtmlToDisplay(const QString &htmlContent)
{
    QString newContent=htmlContent;
    #ifdef NOREMOTE
    QRegExp remote(QRegExp::escape("<span class=\"remote\">")+".*"+QRegExp::escape("</span>"));
    remote.setMinimal(true);
    newContent.remove(remote);
    #endif
    if(!botHaveQuest(actualBot.botId))//if have not quest
    {
        QRegExp quest(QRegExp::escape("<span class=\"quest\">")+".*"+QRegExp::escape("</span>"));
        quest.setMinimal(true);
        newContent.remove(quest);
    }
    newContent.replace("href=\"http","style=\"color:#BB9900;\" href=\"http",Qt::CaseInsensitive);
    newContent.replace(QRegExp("(href=\"http[^>]+>[^<]+)</a>"),"\\1 <img src=\":/images/link.png\" alt=\"\" /></a>");
    return newContent;
}

void BaseWindow::on_inventory_itemActivated(QListWidgetItem *item)
{
    if(!items_graphical.contains(item))
    {
        qDebug() << "BaseWindow::on_inventory_itemActivated(): activated item not found";
        return;
    }
    if(inSelection)
    {
        quint32 tempQuantitySelected;
        bool ok=true;
        if(items[items_graphical[item]]>1)
            tempQuantitySelected=QInputDialog::getInt(this,tr("Quantity"),tr("Select a quantity"),1,1,items[items_graphical[item]],1,&ok);
        else
            tempQuantitySelected=1;
        if(!ok)
        {
            objectSelection(false);
            return;
        }
        if(!items.contains(items_graphical[item]))
        {
            objectSelection(false);
            return;
        }
        if(items[items_graphical[item]]<tempQuantitySelected)
        {
            objectSelection(false);
            return;
        }
        objectSelection(true,items_graphical[item],tempQuantitySelected);
        return;
    }

    //is crafting recipe
    if(DatapackClientLoader::datapackLoader.itemToCrafingRecipes.contains(items_graphical[item]))
    {
        Player_private_and_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations();
        if(informations.recipes.contains(DatapackClientLoader::datapackLoader.itemToCrafingRecipes[items_graphical[item]]))
        {
            QMessageBox::information(this,tr("Information"),tr("You already know this recipe"));
            return;
        }
        objectInUsing << items_graphical[item];
        items[items_graphical[item]]--;
        if(items[items_graphical[item]]==0)
            items.remove(items_graphical[item]);
        CatchChallenger::Api_client_real::client->useObject(items_graphical[item]);
        load_inventory();
    }
    else
        qDebug() << "BaseWindow::on_inventory_itemActivated(): unknow object type";
}

void BaseWindow::objectUsed(const ObjectUsage &objectUsage)
{
    switch(objectUsage)
    {
        case ObjectUsage_correctlyUsed:
        //is crafting recipe
        if(DatapackClientLoader::datapackLoader.itemToCrafingRecipes.contains(objectInUsing.first()))
        {
            CatchChallenger::Api_client_real::client->addRecipe(DatapackClientLoader::datapackLoader.itemToCrafingRecipes[objectInUsing.first()]);
            load_crafting_inventory();
        }
        else
            qDebug() << "BaseWindow::objectUsed(): unknow object type";
        break;
        case ObjectUsage_failed:
        break;
        case ObjectUsage_impossible:
            if(items.contains(objectInUsing.first()))
                items[objectInUsing.first()]++;
            else
                items[objectInUsing.first()]=1;
            load_inventory();
        break;
        default:
        break;
    }
    objectInUsing.removeFirst();
}

void BaseWindow::on_inventoryDestroy_clicked()
{
    qDebug() << "on_inventoryDestroy_clicked()";
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
        return;
    quint32 itemId=items_graphical[items.first()];
    if(!this->items.contains(itemId))
        return;
    quint32 quantity=this->items[itemId];
    if(quantity>1)
    {
        bool ok;
        quint32 quantity_temp=QInputDialog::getInt(this,tr("Destroy"),tr("Quantity to destroy"),quantity,1,quantity,1,&ok);
        if(!ok)
            return;
        quantity=quantity_temp;
    }
    QMessageBox::StandardButton button;
    if(DatapackClientLoader::datapackLoader.items.contains(itemId))
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 %2?").arg(quantity).arg(DatapackClientLoader::datapackLoader.items[itemId].name),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    else
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 unknow item (id: %2)?").arg(quantity).arg(itemId),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    if(button!=QMessageBox::Yes)
        return;
    if(!this->items.contains(itemId))
        return;
    if(this->items[itemId]<quantity)
        quantity=this->items[itemId];
    emit destroyObject(itemId,quantity);
    if(this->items[itemId]<=quantity)
        this->items.remove(itemId);
    else
        this->items[itemId]-=quantity;
    load_inventory();
    load_plant_inventory();
}

quint32 BaseWindow::itemQuantity(const quint32 &itemId)
{
    if(items.contains(itemId))
        return items[itemId];
    return 0;
}

void BaseWindow::on_inventoryUse_clicked()
{
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
        return;
    on_inventory_itemActivated(items.first());
}

void BaseWindow::on_inventoryInformation_clicked()
{
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
    {
        qDebug() << "on_inventoryInformation_clicked() should not be accessible here";
        return;
    }
    QListWidgetItem *item=items.first();
    if(!items_graphical.contains(item))
    {
        qDebug() << "on_inventoryInformation_clicked() item not found here";
        return;
    }
    if(DatapackClientLoader::datapackLoader.itemToPlants.contains(items_graphical[item]))
    {
        if(!plants_items_to_graphical.contains(DatapackClientLoader::datapackLoader.itemToPlants[items_graphical[item]]))
        {
            qDebug() << QString("on_inventoryInformation_clicked() is not into plant list: item: %1, plant: %2").arg(items_graphical[item]).arg(DatapackClientLoader::datapackLoader.itemToPlants[items_graphical[item]]);
            return;
        }
        ui->listPlantList->reset();
        ui->stackedWidget->setCurrentWidget(ui->page_plants);
        plants_items_to_graphical[DatapackClientLoader::datapackLoader.itemToPlants[items_graphical[item]]]->setSelected(true);
        on_listPlantList_itemSelectionChanged();
    }
    else
    {
        qDebug() << "on_inventoryInformation_clicked() information on unknow object type";
        return;
    }
}

void BaseWindow::on_toolButtonOptions_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_options);
}

void BaseWindow::on_IG_dialog_text_linkActivated(const QString &link)
{
    ui->IG_dialog->setVisible(false);
    if(link.startsWith("http://") || link.startsWith("https://"))
    {
        if(!QDesktopServices::openUrl(QUrl(link)))
            showTip(QString("Unable to open the url: %1").arg(link));
        return;
    }
    bool ok;
    if(link.startsWith("quest_"))
    {
        QString tempLink=link;
        tempLink.remove("quest_");
        quint32 questId=tempLink.toUShort(&ok);
        if(!ok)
        {
            showTip(QString("Unable to open the link: %1").arg(link));
            return;
        }
        if(!DatapackClientLoader::datapackLoader.quests.contains(questId))
        {
            showTip(tr("Quest not found"));
            return;
        }
        isInQuest=true;
        this->questId=questId;
        getTextEntryPoint();
        return;
    }
    if(link=="close")
        return;
    if(link=="next_quest_step" && isInQuest)
    {
        nextQuestStep();
        return;
    }
    quint8 step=link.toUShort(&ok);
    if(!ok)
    {
        showTip(QString("Unable to open the link: %1").arg(link));
        return;
    }
    if(isInQuest)
    {
        showQuestText(step);
        return;
    }
    goToBotStep(step);
}

void BaseWindow::nextQuestStep()
{
    if(!DatapackClientLoader::datapackLoader.quests.contains(questId))
    {
        showTip(tr("Quest not found"));
        return;
    }

    if(!quests.contains(questId))
    {
        if(haveStartQuestRequirement(DatapackClientLoader::datapackLoader.quests[questId]))
        {
            CatchChallenger::Api_client_real::client->startQuest(questId);
            startQuest(DatapackClientLoader::datapackLoader.quests[questId]);
            CatchChallenger::PlayerQuest quest;
            quest.step=1;
            quest.finish_one_time=false;
            quests[questId]=quest;
            updateDisplayedQuests();
        }
        else
            showTip(tr("You don't have the requirement to start this quest"));
        return;
    }
    else if(quests[questId].step==0)
    {
        if(haveStartQuestRequirement(DatapackClientLoader::datapackLoader.quests[questId]))
        {
            CatchChallenger::Api_client_real::client->startQuest(questId);
            startQuest(DatapackClientLoader::datapackLoader.quests[questId]);
            quests[questId].step=1;
            updateDisplayedQuests();
        }
        else
            showTip(tr("You don't have the requirement to start this quest"));
        return;
    }
    if(quests[questId].step>=(DatapackClientLoader::datapackLoader.quests[questId].steps.size()-1))
    {
        showTip(QString("finish quest %1").arg(questId));
        return;
    }
    showTip(QString("next_quest_step quest %1").arg(questId));
}

void BaseWindow::getTextEntryPoint()
{
    if(!isInQuest)
    {
        showTip(QString("Internal error: Is not in quest"));
        return;
    }
    QScriptEngine engine;

    QString client_logic=CatchChallenger::Api_client_real::client->get_datapack_base_name()+"/"+DATAPACK_BASE_PATH_QUESTS+"/"+QString::number(questId)+"/client_logic.js";
    if(!QFile(client_logic).exists())
    {
        showTip(tr("Client file missing"));
        qDebug() << "client_logic file is missing:" << client_logic;
        return;
    }

    QFile scriptFile(client_logic);
    scriptFile.open(QIODevice::ReadOnly);
    QTextStream stream(&scriptFile);
    QString contents = stream.readAll();
    scriptFile.close();
    if(!quests.contains(questId))
    {
        contents.replace("currentQuestStep()","0");
        contents.replace("finishOneTime()","false");
        contents.replace("haveNextStepQuestRequirements()","false");//bug if use that's
    }
    else
    {
        PlayerQuest quest=quests[questId];
        contents.replace("currentQuestStep()",QString::number(quest.step));
        if(quest.finish_one_time)
            contents.replace("finishOneTime()","true");
        else
            contents.replace("finishOneTime()","false");
        if(quest.step<=0)
            contents.replace("haveNextStepQuestRequirements()","false");
        else if(haveNextStepQuestRequirements(DatapackClientLoader::datapackLoader.quests[questId]))
            contents.replace("haveNextStepQuestRequirements()","true");
        else
            contents.replace("haveNextStepQuestRequirements()","false");
    }

    QScriptValue result = engine.evaluate(contents, client_logic);
    if (result.isError()) {
        qDebug() << "script error:" << QString::fromLatin1("%0:%1: %2")
                    .arg(client_logic)
                    .arg(result.property("lineNumber").toInt32())
                    .arg(result.toString());
        showTip(QString::fromLatin1("%0:%1: %2")
        .arg(client_logic)
        .arg(result.property("lineNumber").toInt32())
        .arg(result.toString()));
        return;
    }

    QScriptValue getTextEntryPoint = engine.globalObject().property("getTextEntryPoint");
    quint32 textEntryPoint=getTextEntryPoint.call().toNumber();
    qDebug() << "textEntryPoint:" << textEntryPoint;
    showQuestText(textEntryPoint);
}

void BaseWindow::showQuestText(const quint32 &textId)
{
    if(!DatapackClientLoader::datapackLoader.questsText.contains(questId))
    {
        qDebug() << QString("No quest text for this quest: %1").arg(questId);
        showTip(tr("No quest text for this quest"));
        return;
    }
    if(!DatapackClientLoader::datapackLoader.questsText[questId].text.contains(textId))
    {
        qDebug() << "No quest text entry point";
        showTip(tr("No quest text entry point"));
        return;
    }

    QString textToShow=parseHtmlToDisplay(DatapackClientLoader::datapackLoader.questsText[questId].text[textId]);
    ui->IG_dialog_text->setText(textToShow);
    ui->IG_dialog->setVisible(true);
}

void BaseWindow::on_toolButton_quit_shop_clicked()
{
    waitToSell=false;
    inSelection=false;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_shopItemList_itemActivated(QListWidgetItem *item)
{
    if(CatchChallenger::Api_client_real::client->getHaveShopAction())
        return;
    if(!waitToSell)
    {
        if(cash<itemsIntoTheShop[shop_items_graphical[item]].price)
        {
            QMessageBox::information(this,tr("Buy"),tr("You have not the cash to buy this item"));
            return;
        }
        bool ok=true;
        if(cash/itemsIntoTheShop[shop_items_graphical[item]].price>1)
            tempQuantityForBuy=QInputDialog::getInt(this,tr("Buy"),tr("Quantity to buy"),1,1,cash/itemsIntoTheShop[shop_items_graphical[item]].price,1,&ok);
        else
            tempQuantityForBuy=1;
        if(!ok)
            return;
        tempItemForBuy=shop_items_graphical[item];
        CatchChallenger::Api_client_real::client->buyObject(shopId,tempItemForBuy,tempQuantityForBuy,itemsIntoTheShop[tempItemForBuy].price);
        ui->stackedWidget->setCurrentWidget(ui->page_map);
        tempCashForBuy=itemsIntoTheShop[tempItemForBuy].price*tempQuantityForBuy;
        removeCash(tempCashForBuy);
        showTip(tr("Buying the object..."));
    }
    else
    {
        if(!items.contains(shop_items_graphical[item]))
            return;
        bool ok=true;
        if(items[shop_items_graphical[item]]>1)
            tempQuantityForSell=QInputDialog::getInt(this,tr("Sell"),tr("Quantity to sell"),1,1,items[shop_items_graphical[item]],1,&ok);
        else
            tempQuantityForSell=1;
        if(!ok)
            return;
        if(!items.contains(shop_items_graphical[item]))
            return;
        if(items[shop_items_graphical[item]]<tempQuantityForSell)
            return;
        objectSelection(true,shop_items_graphical[item],tempQuantityForSell);
        ui->stackedWidget->setCurrentWidget(ui->page_map);
        showTip(tr("Selling the object..."));
    }
}

void BaseWindow::on_shopItemList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=ui->shopItemList->selectedItems();
    if(items.size()!=1)
    {
        ui->shopName->setText("");
        ui->shopDescription->setText(tr("Select an object"));
        ui->shopBuy->setVisible(false);
        ui->shopImage->setPixmap(QPixmap(":/images/inventory/unknow-object.png"));
        return;
    }
    ui->shopBuy->setVisible(true);
    QListWidgetItem *item=items.first();
    if(!DatapackClientLoader::datapackLoader.items.contains(shop_items_graphical[item]))
    {
        ui->shopImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->shopName->setText(tr("Unknow name"));
        ui->shopDescription->setText(tr("Unknow description"));
        return;
    }
    const DatapackClientLoader::Item &content=DatapackClientLoader::datapackLoader.items[shop_items_graphical[item]];

    ui->shopImage->setPixmap(content.image);
    ui->shopName->setText(content.name);
    ui->shopDescription->setText(content.description);
}

void BaseWindow::on_shopBuy_clicked()
{
    QList<QListWidgetItem *> items=ui->shopItemList->selectedItems();
    if(items.size()!=1)
    {
        qDebug() << "on_shopBuy_clicked() no correct selection";
        return;
    }
    on_shopItemList_itemActivated(items.first());
}

void BaseWindow::haveShopList(const QList<ItemToSellOrBuy> &items)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveShopList()";
    #endif
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    ui->shopItemList->clear();
    itemsIntoTheShop.clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    int index=0;
    while(index<items.size())
    {
        itemsIntoTheShop[items.at(index).object]=items.at(index);
        QListWidgetItem *item=new QListWidgetItem();
        shop_items_to_graphical[items.at(index).object]=item;
        shop_items_graphical[item]=items.at(index).object;
        if(DatapackClientLoader::datapackLoader.items.contains(items.at(index).object))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.items[items.at(index).object].image);
            if(items.at(index).quantity==0)
                item->setText(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.items[items.at(index).object].name).arg(items.at(index).price));
            else
                item->setText(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.items[items.at(index).object].name).arg(items.at(index).price).arg(items.at(index).quantity));
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(items.at(index).quantity==0)
                item->setText(tr("Item %1\nPrice: %2$").arg(items.at(index).object).arg(items.at(index).price));
            else
                item->setText(tr("Item %1 at %2$\nQuantity: %3").arg(items.at(index).object).arg(items.at(index).price).arg(items.at(index).quantity));
        }
        if(items.at(index).price>cash)
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
        }
        ui->shopItemList->addItem(item);
        index++;
    }
    ui->shopBuy->setText(tr("Buy"));
    on_shopItemList_itemSelectionChanged();
}

void BaseWindow::displaySellList()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::displaySellList()";
    #endif
    ui->shopItemList->clear();
    itemsIntoTheShop.clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();
        if(DatapackClientLoader::datapackLoader.items.contains(i.key()))
        {
            QListWidgetItem *item=new QListWidgetItem();
            shop_items_to_graphical[i.key()]=item;
            shop_items_graphical[item]=i.key();
            item->setIcon(DatapackClientLoader::datapackLoader.items[i.key()].image);
            if(i.value()>1)
                item->setText(tr("%1\nPrice: %2$, quantity: %3").arg(DatapackClientLoader::datapackLoader.items[i.key()].name).arg(DatapackClientLoader::datapackLoader.items[i.key()].price/2).arg(i.value()));
            else
                item->setText(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.items[i.key()].name).arg(DatapackClientLoader::datapackLoader.items[i.key()].price/2));
            ui->shopItemList->addItem(item);
        }
    }
    ui->shopBuy->setText(tr("Sell"));
    on_shopItemList_itemSelectionChanged();
}

void BaseWindow::haveBuyObject(const BuyStat &stat,const quint32 &newPrice)
{
    QHash<quint32,quint32> items;
    switch(stat)
    {
        case BuyStat_Done:
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() Can't buy at 0$!";
                return;
            }
            addCash(tempCashForBuy);
            removeCash(newPrice*tempQuantityForBuy);
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_HaveNotQuantity:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case BuyStat_PriceHaveChanged:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
}

void BaseWindow::haveSellObject(const SoldStat &stat,const quint32 &newPrice)
{
    waitToSell=false;
    switch(stat)
    {
        case SoldStat_Done:
            addCash(itemsToSell.first().price*itemsToSell.first().quantity);
            showTip(tr("Item sold"));
        break;
        case SoldStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() the price 0$ can't be better price!";
                return;
            }
            addCash(newPrice*itemsToSell.first().quantity);
            showTip(tr("Item sold at better price"));
        break;
        case SoldStat_WrongQuantity:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case SoldStat_PriceHaveChanged:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
    itemsToSell.removeFirst();
}

void BaseWindow::addCash(const quint32 &cash)
{
    this->cash+=cash;
    ui->player_informations_cash->setText(QString("%1$").arg(this->cash));
    ui->tradePlayerCash->setMaximum(this->cash);
}

void BaseWindow::removeCash(const quint32 &cash)
{
    this->cash-=cash;
    ui->player_informations_cash->setText(QString("%1$").arg(this->cash));
    ui->tradePlayerCash->setMaximum(this->cash);
}

void BaseWindow::on_pushButton_interface_monsters_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_monster);
}

void BaseWindow::on_toolButton_monster_list_quit_clicked()
{
    if(waitedObjectType==ObjectType_MonsterToTrade || waitedObjectType==ObjectType_MonsterToLearn)
    {
        if(inSelection)
        {
            objectSelection(false);
            return;
        }
    }
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_tradePlayerCash_editingFinished()
{
    if(ui->tradePlayerCash->value()==ui->tradePlayerCash->minimum())
        return;
    CatchChallenger::Api_client_real::client->addTradeCash(ui->tradePlayerCash->value()-ui->tradePlayerCash->minimum());
    ui->tradePlayerCash->setMinimum(ui->tradePlayerCash->value());
}

void BaseWindow::on_toolButton_bioscan_quit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_tradeCancel_clicked()
{
    CatchChallenger::Api_client_real::client->tradeCanceled();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_tradeValidate_clicked()
{
    CatchChallenger::Api_client_real::client->tradeFinish();
    if(tradeOtherStat==TradeOtherStat_Accepted)
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    else
    {
        ui->tradePlayerCash->setEnabled(false);
        ui->tradePlayerItems->setEnabled(false);
        ui->tradePlayerMonsters->setEnabled(false);
        ui->tradeAddItem->setEnabled(false);
        ui->tradeAddMonster->setEnabled(false);
        ui->tradeValidate->setEnabled(false);
    }
}

void BaseWindow::on_tradeAddItem_clicked()
{
    selectObject(ObjectType_Trade);
}

void CatchChallenger::BaseWindow::on_tradeAddMonster_clicked()
{
    selectObject(ObjectType_MonsterToTrade);
}

void CatchChallenger::BaseWindow::on_selectMonster_clicked()
{
    QList<QListWidgetItem *> selectedMonsters=ui->monsterList->selectedItems();
    if(selectedMonsters.size()!=1)
        return;
    on_monsterList_itemActivated(selectedMonsters.first());
}

void CatchChallenger::BaseWindow::on_monsterList_itemActivated(QListWidgetItem *item)
{
    if(!monsters_items_graphical.contains(item))
        return;
    objectSelection(true,monsters_items_graphical[item]);
}

void CatchChallenger::BaseWindow::on_close_IG_dialog_clicked()
{
    isInQuest=false;
}
