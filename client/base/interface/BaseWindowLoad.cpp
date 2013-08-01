#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/CommonDatapack.h"
#include "../ClientVariable.h"
#include "DatapackClientLoader.h"
#include "Chat.h"
#include "../fight/interface/ClientFightEngine.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>

using namespace CatchChallenger;

void BaseWindow::resetAll()
{
    ui->frame_main_display_interface_player->hide();
    ui->label_interface_number_of_player->setText("?/?");
    ui->stackedWidget->setCurrentWidget(ui->page_init);
    Chat::chat->resetAll();
    MapController::mapController->resetAll();
    haveDatapack=false;
    havePlayerInformations=false;
    DatapackClientLoader::datapackLoader.resetAll();
    haveInventory=false;
    datapackIsParsed=false;
    ui->inventory->clear();
    ui->shopItemList->clear();
    items_graphical.clear();
    items_to_graphical.clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    plants_items_graphical.clear();
    plants_items_to_graphical.clear();
    crafting_recipes_items_graphical.clear();
    crafting_recipes_items_to_graphical.clear();
    quests.clear();
    ui->tip->setVisible(false);
    ui->persistant_tip->setVisible(false);
    ui->gain->setVisible(false);
    ui->IG_dialog->setVisible(false);
    seedWait=false;
    collectWait=false;
    inSelection=false;
    isInQuest=false;
    displayAttackProgression=0;
    fightTimerFinish=false;
    queryList.clear();
    ui->inventoryInformation->setVisible(false);
    ui->inventoryUse->setVisible(false);
    ui->inventoryDestroy->setVisible(false);
    previousRXSize=0;
    previousTXSize=0;
    ui->plantUse->setVisible(false);
    ui->craftingUse->setVisible(false);
    waitToSell=false;
    fightTimerFinish=false;
    ui->tabWidgetTrainerCard->setCurrentWidget(ui->tabWidgetTrainerCardPage1);
    ui->selectMonster->setVisible(false);
    doNextActionStep=DoNextActionStep_Start;

    CatchChallenger::ClientFightEngine::fightEngine.resetAll();
}

void BaseWindow::serverIsLoading()
{
    ui->label_connecting_status->setText(tr("Preparing the game data"));
    resetAll();
}

void BaseWindow::serverIsReady()
{
    ui->label_connecting_status->setText(tr("Game data is ready"));
}

void BaseWindow::setMultiPlayer(bool multiplayer)
{
    emit sendsetMultiPlayer(multiplayer);
    //frame_main_display_right->setVisible(multiplayer);
    ui->frame_main_display_interface_player->setVisible(multiplayer);
}

void BaseWindow::disconnected(QString reason)
{
    if(haveShowDisconnectionReason)
        return;
    haveShowDisconnectionReason=true;
    QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(reason));
    resetAll();
}

void BaseWindow::notLogged(QString reason)
{
    QMessageBox::information(this,tr("Unable to login"),tr("Unable to login: %1").arg(reason));
}

void BaseWindow::logged()
{
    updateConnectingStatus();
    CatchChallenger::Api_client_real::client->sendDatapackContent();
}

void BaseWindow::protocol_is_good()
{
    ui->label_connecting_status->setText(tr("Try login..."));
}

void BaseWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(this->socketState==socketState)
        return;
    this->socketState=socketState;

    if(socketState!=QAbstractSocket::UnconnectedState)
    {
        if(socketState==QAbstractSocket::ConnectedState)
        {
            haveShowDisconnectionReason=false;
            ui->label_connecting_status->setText(tr("Try initialise the protocol..."));
            CatchChallenger::Api_client_real::client->sendProtocol();
        }
        else
            ui->label_connecting_status->setText(tr("Connecting to the server..."));
    }
}

void BaseWindow::have_current_player_info()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::have_current_player_info()";
    #endif
    if(havePlayerInformations)
        return;
    havePlayerInformations=true;
    Player_private_and_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations();
    cash=informations.cash;
    warehouse_cash=informations.warehouse_cash;
    quests=informations.quests;
    ui->player_informations_pseudo->setText(informations.public_informations.pseudo);
    ui->tradePlayerPseudo->setText(informations.public_informations.pseudo);
    ui->warehousePlayerPseudo->setText(informations.public_informations.pseudo);
    ui->player_informations_cash->setText(QString("%1$").arg(informations.cash));
    CatchChallenger::ClientFightEngine::fightEngine.setVariable(CatchChallenger::Api_client_real::client->get_player_informations());
    DebugClass::debugConsole(QString("%1 is logged with id: %2, cash: %3").arg(informations.public_informations.pseudo).arg(informations.public_informations.simplifiedId).arg(informations.cash));
    updatePlayerImage();
    updateConnectingStatus();
}

void BaseWindow::haveTheDatapack()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveTheDatapack()";
    #endif
    if(haveDatapack)
        return;
    haveDatapack=true;

    emit parseDatapack(CatchChallenger::Api_client_real::client->get_datapack_base_name());
}

void BaseWindow::have_inventory(const QHash<quint32,quint32> &items, const QHash<quint32, quint32> &warehouse_items)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::have_inventory()";
    #endif
    this->items=items;
    this->warehouse_items=warehouse_items;
    haveInventory=true;
    updateConnectingStatus();
}

void BaseWindow::load_inventory()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::load_inventory()";
    #endif
    if(!haveInventory || !datapackIsParsed)
        return;
    ui->inventoryInformation->setVisible(false);
    ui->inventoryUse->setVisible(false);
    ui->inventoryDestroy->setVisible(false);
    ui->inventory->clear();
    items_graphical.clear();
    items_to_graphical.clear();
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();

        bool show=!inSelection;
        if(inSelection)
        {
            switch(waitedObjectType)
            {
                case ObjectType_Seed:
                    if(DatapackClientLoader::datapackLoader.itemToPlants.contains(i.key()))
                        show=true;
                break;
                case ObjectType_UseInFight:
                    if(CatchChallenger::ClientFightEngine::fightEngine.isInFightWithWild())
                        if(CommonDatapack::commonDatapack.items.trap.contains(i.key()))
                            show=true;
                break;
                default:
                qDebug() << "waitedObjectType is unknow into load_inventory()";
                break;
            }
        }
        if(show)
        {
            QListWidgetItem *item=new QListWidgetItem();
            items_to_graphical[i.key()]=item;
            items_graphical[item]=i.key();
            if(DatapackClientLoader::datapackLoader.itemsExtra.contains(i.key()))
            {
                item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[i.key()].image);
                if(i.value()>1)
                    item->setText(QString::number(i.value()));
                item->setToolTip(DatapackClientLoader::datapackLoader.itemsExtra[i.key()].name);
            }
            else
            {
                item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
                if(i.value()>1)
                    item->setText(QString("id: %1 (x%2)").arg(i.key()).arg(i.value()));
                else
                    item->setText(QString("id: %1").arg(i.key()));
            }
            ui->inventory->addItem(item);
        }
    }
}

void BaseWindow::datapackParsed()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::datapackParsed()";
    #endif
    datapackIsParsed=true;
    updateConnectingStatus();
    updatePlayerImage();
}

void BaseWindow::updateConnectingStatus()
{
    QStringList waitedData;
    if(!haveInventory || !havePlayerInformations)
        waitedData << tr("loading the player informations");
    if(!haveDatapack)
        waitedData << tr("loading the datapack");
    else if(!datapackIsParsed)
        waitedData << tr("opening the datapack");
    if(waitedData.isEmpty())
    {
        Player_private_and_public_informations player_private_and_public_informations=CatchChallenger::Api_client_real::client->get_player_informations();
        warehouse_playerMonster=player_private_and_public_informations.warehouse_playerMonster;
        MapController::mapController->setBotsAlreadyBeaten(player_private_and_public_informations.bot_already_beaten);
        load_inventory();
        load_plant_inventory();
        load_crafting_inventory();
        updateDisplayedQuests();
        if(!check_senddata())
            return;
        load_monsters();
        show_reputation();
        this->setWindowTitle(tr("CatchChallenger - %1").arg(CatchChallenger::Api_client_real::client->getPseudo()));
        ui->stackedWidget->setCurrentWidget(ui->page_map);
        showTip(tr("Welcome <b><i>%1</i></b> on catchchallenger").arg(CatchChallenger::Api_client_real::client->getPseudo()));
        return;
    }
    ui->label_connecting_status->setText(tr("Waiting: %1").arg(waitedData.join(", ")));
}

//with the datapack content
bool BaseWindow::check_senddata()
{
    if(!check_monsters())
        return false;
    //check the reputation here
    QHashIterator<QString,PlayerReputation> i(CatchChallenger::Api_client_real::client->player_informations.reputation);
    while(i.hasNext())
    {
        i.next();
        if(i.value().level<-100 || i.value().level>100)
        {
            error(QString("level is <100 or >100, skip"));
            return false;
        }
        if(!CatchChallenger::CommonDatapack::commonDatapack.reputation.contains(i.key()))
        {
            error(QString("The reputation: %1 don't exist").arg(i.key()));
            return false;
        }
        if(i.value().level>=0)
        {
            if(i.value().level>=CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_positive.size())
            {
                error(QString("The reputation level %1 is wrong because is out of range (reputation level: %2 > max level: %3)").arg(i.key()).arg(i.value().level).arg(CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_positive.size()));
                return false;
            }
        }
        else
        {
            if((-i.value().level)>CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_negative.size())
            {
                error(QString("The reputation level %1 is wrong because is out of range (reputation level: %2 < max level: %3)").arg(i.key()).arg(i.value().level).arg(CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_negative.size()));
                return false;
            }
        }
        if(i.value().point>0)
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_positive.size()==i.value().level)//start at level 0 in positive
            {
                emit message(QString("The reputation level is already at max, drop point"));
                return false;
            }
            if(i.value().point>=CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_positive.at(i.value().level+1))//start at level 0 in positive
            {
                error(QString("The reputation point %1 is greater than max %2").arg(i.value().point).arg(CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_positive.at(i.value().level)));
                return false;
            }
        }
        else if(i.value().point<0)
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_negative.size()==-i.value().level)//start at level -1 in negative
            {
                error(QString("The reputation level is already at min, drop point"));
                return false;
            }
            if(i.value().point<CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_negative.at(-i.value().level))//start at level -1 in negative
            {
                error(QString("The reputation point %1 is greater than max %2").arg(i.value().point).arg(CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_negative.at(i.value().level)));
                return false;
            }
        }
    }
    return true;
}

void BaseWindow::show_reputation()
{
    QString html="<ul>";
    QHashIterator<QString,PlayerReputation> i(CatchChallenger::Api_client_real::client->player_informations.reputation);
    while(i.hasNext())
    {
        i.next();
        if(i.value().level>=0)
        {
            if((i.value().level+1)==CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_positive.size())
                html+=QString("<li>100% %1</li>")
                    .arg(CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].text_positive.last());
            else
            {
                qint32 next_level_xp=CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_positive.at(i.value().level+1);
                if(next_level_xp==0)
                {
                    error("Next level can't be need 0 xp");
                    return;
                }
                QString text=CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].text_positive.at(i.value().level);
                html+=QString("<li>%1% %2</li>").arg((i.value().point*100)/next_level_xp).arg(text);
            }
        }
        else
        {
            if((-i.value().level)==CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_negative.size())
                html+=QString("<li>100% %1</li>")
                    .arg(CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].text_negative.last());
            else
            {
                qint32 next_level_xp=CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].reputation_negative.at(-i.value().level);
                if(next_level_xp==0)
                {
                    error("Next level can't be need 0 xp");
                    return;
                }
                QString text=CatchChallenger::CommonDatapack::commonDatapack.reputation[i.key()].text_negative.at(-i.value().level-1);
                html+=QString("<li>%1% %2</li>")
                    .arg((i.value().point*100)/next_level_xp)
                    .arg(text);
            }
        }
    }
    html+="</ul>";
    ui->labelReputation->setText(html);
}

QPixmap BaseWindow::getFrontSkin(const QString &skinName) const
{
    QPixmap skin=QPixmap(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinName+"/front.png");
    if(skin.isNull())
    {
        skin=QPixmap(":/images/player_default/front.png");
        qDebug() << "Unable to load the player image: "+CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinName+"/front.png";
    }
    return skin;
}

QPixmap BaseWindow::getFrontSkin(const quint32 &skinId) const
{
    QPixmap skin;
    QStringList skinFolderList=DatapackClientLoader::datapackLoader.skins;
    //front image
    if(skinId<(quint32)skinFolderList.size())
        skin=getFrontSkin(skinFolderList.at(skinId));
    else
    {
        skin=QPixmap(":/images/player_default/front.png");
        qDebug() << "The skin id: "+QString::number(skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into BaseWindow::updatePlayerImage()";
    }
    return skin;
}

QPixmap BaseWindow::getBackSkin(const quint32 &skinId) const
{
    QPixmap skin;
    QStringList skinFolderList=DatapackClientLoader::datapackLoader.skins;
    //back image
    if(skinId<(quint32)skinFolderList.size())
    {
        skin=QPixmap(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(skinId)+"/back.png");
        if(skin.isNull())
        {
            skin=QPixmap(":/images/player_default/back.png");
            qDebug() << "Unable to load the player image: "+CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(skinId)+"/back.png";
        }
    }
    else
    {
        skin=QPixmap(":/images/player_default/back.png");
        qDebug() << "The skin id: "+QString::number(skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into BaseWindow::updatePlayerImage()";
    }
    return skin;
}

void BaseWindow::updatePlayerImage()
{
    if(havePlayerInformations && haveDatapack)
    {
        Player_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations().public_informations;
        playerFrontImage=getFrontSkin(informations.skinId);
        playerBackImage=getBackSkin(informations.skinId);

        //load into the UI
        ui->player_informations_front->setPixmap(playerFrontImage);
        ui->warehousePlayerImage->setPixmap(playerFrontImage);
        ui->tradePlayerImage->setPixmap(playerFrontImage);
    }
}

void BaseWindow::updateDisplayedQuests()
{
    QString html="<ul>";
    ui->questsList->clear();
    quests_to_id_graphical.clear();
    QHashIterator<quint32, PlayerQuest> i(quests);
    while (i.hasNext()) {
        i.next();
        if(DatapackClientLoader::datapackLoader.questsExtra.contains(i.key()) && i.value().step>0)
        {
            QListWidgetItem * item=new QListWidgetItem(DatapackClientLoader::datapackLoader.questsExtra[i.key()].name);
            quests_to_id_graphical[item]=i.key();
            ui->questsList->addItem(item);
        }
        else
            html+="<li>"+DatapackClientLoader::datapackLoader.questsExtra[i.key()].name+"</li>";
    }
    html+="</ul>";
    if(html=="<ul></ul>")
        html="<i>None</i>";
    ui->finishedQuests->setHtml(html);
    on_questsList_itemSelectionChanged();
}

void BaseWindow::on_questsList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=ui->questsList->selectedItems();
    if(items.size()!=1)
    {
        ui->questDetails->setText(tr("Select a quest"));
        return;
    }
    if(!quests_to_id_graphical.contains(items.first()))
    {
        qDebug() << "Selected quest have not id";
        ui->questDetails->setText(tr("Select a quest"));
        return;
    }
    quint32 questId=quests_to_id_graphical[items.first()];
    if(!quests.contains(questId))
    {
        qDebug() << "Selected quest is not into the player list";
        ui->questDetails->setText(tr("Select a quest"));
        return;
    }
    if(quests[questId].step==0 || quests[questId].step>DatapackClientLoader::datapackLoader.questsExtra[questId].steps.size())
    {
        qDebug() << "Selected quest step is out of range";
        ui->questDetails->setText(tr("Select a quest"));
        return;
    }
    ui->questDetails->setText(DatapackClientLoader::datapackLoader.questsExtra[questId].steps[quests[questId].step-1]);
}

QListWidgetItem * BaseWindow::itemToGraphic(const quint32 &id,const quint32 &quantity)
{
    QListWidgetItem *item=new QListWidgetItem();
    item->setData(99,id);
    if(DatapackClientLoader::datapackLoader.itemsExtra.contains(id))
    {
        item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[id].image);
        if(quantity>1)
            item->setText(QString::number(quantity));
        item->setToolTip(DatapackClientLoader::datapackLoader.itemsExtra[id].name);
    }
    else
    {
        item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        if(quantity>1)
            item->setText(QString("id: %1 (x%2)").arg(id).arg(quantity));
        else
            item->setText(QString("id: %1").arg(id));
    }
    return item;
}

void BaseWindow::updateTheWareHouseContent()
{
    if(!haveInventory || !datapackIsParsed)
        return;

    //inventory
    {
        ui->warehousePlayerInventory->clear();
        QHashIterator<quint32,quint32> i(items);
        while (i.hasNext()) {
            i.next();
            qint64 quantity=i.value();
            if(change_warehouse_items.contains(i.key()))
                quantity+=change_warehouse_items[i.key()];
            if(quantity>0)
                ui->warehousePlayerInventory->addItem(itemToGraphic(i.key(),quantity));
        }
        QHashIterator<quint32,qint32> j(change_warehouse_items);
        while (j.hasNext()) {
            j.next();
            if(!items.contains(j.key()) && j.value()>0)
                ui->warehousePlayerInventory->addItem(itemToGraphic(j.key(),j.value()));
        }
    }

    //inventory warehouse
    {
        ui->warehousePlayerStoredInventory->clear();
        QHashIterator<quint32,quint32> i(warehouse_items);
        while (i.hasNext()) {
            i.next();
            qint64 quantity=i.value();
            if(change_warehouse_items.contains(i.key()))
                quantity-=change_warehouse_items[i.key()];
            if(quantity>0)
                ui->warehousePlayerStoredInventory->addItem(itemToGraphic(i.key(),quantity));
        }
        QHashIterator<quint32,qint32> j(change_warehouse_items);
        while (j.hasNext()) {
            j.next();
            if(!warehouse_items.contains(j.key()) && j.value()<0)
                ui->warehousePlayerStoredInventory->addItem(itemToGraphic(j.key(),-j.value()));
        }
    }

    //cash
    ui->warehousePlayerCash->setText(tr("Cash: %1").arg(cash+temp_warehouse_cash));
    ui->warehousePlayerStoredCash->setText(tr("Cash: %1").arg(warehouse_cash-temp_warehouse_cash));

    //monster
    {
        const QList<PlayerMonster> &playerMonster=CatchChallenger::ClientFightEngine::fightEngine.getPlayerMonster();
        ui->warehousePlayerMonster->clear();
        int index=0;
        int size=playerMonster.size();
        while(index<size)
        {
            const PlayerMonster &monster=playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monster.monster))
            {
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(tr("%1, level: %2")
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].name)
                        .arg(monster.level)
                        );
                item->setToolTip(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].description);
                item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].front);
                item->setData(99,monster.id);
                if(!monster_to_deposit.contains(monster.id))
                    ui->warehousePlayerMonster->addItem(item);
                else
                    ui->warehousePlayerStoredMonster->addItem(item);
            }
            index++;
        }
    }

    //monster warehouse
    {
        ui->warehousePlayerStoredMonster->clear();
        int index=0;
        int size=warehouse_playerMonster.size();
        while(index<size)
        {
            const PlayerMonster &monster=warehouse_playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monster.monster))
            {
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(tr("%1, level: %2")
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].name)
                        .arg(monster.level)
                        );
                item->setToolTip(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].description);
                item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].front);
                item->setData(99,monster.id);
                if(!monster_to_withdraw.contains(monster.id))
                    ui->warehousePlayerStoredMonster->addItem(item);
                else
                    ui->warehousePlayerMonster->addItem(item);
            }
            index++;
        }
    }

    //set the button enabled
    ui->warehouseWithdrawCash->setEnabled((warehouse_cash-temp_warehouse_cash)>0);
    ui->warehouseDepositCash->setEnabled((cash+temp_warehouse_cash)>0);
    ui->warehouseDepositItem->setEnabled(ui->warehousePlayerInventory->count()>0);
    ui->warehouseWithdrawItem->setEnabled(ui->warehousePlayerStoredInventory->count()>0);
    ui->warehouseDepositMonster->setEnabled(ui->warehousePlayerMonster->count()>1);
    ui->warehouseWithdrawMonster->setEnabled(ui->warehousePlayerStoredMonster->count()>0);
}
