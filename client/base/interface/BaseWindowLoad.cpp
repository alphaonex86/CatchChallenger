#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "../ClientVariable.h"
#include "DatapackClientLoader.h"
#include "Chat.h"

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
    ui->tip->setVisible(false);
    ui->persistant_tip->setVisible(false);
    ui->gain->setVisible(false);
    ui->IG_dialog->setVisible(false);
    seedWait=false;
    collectWait=false;
    inSelection=false;
    queryList.clear();
    ui->inventoryInformation->setVisible(false);
    ui->inventoryUse->setVisible(false);
    ui->inventoryDestroy->setVisible(false);
    previousRXSize=0;
    previousTXSize=0;
    ui->plantUse->setVisible(false);
    ui->craftingUse->setVisible(false);
    waitToSell=false;
    ui->tabWidgetTrainerCard->setCurrentWidget(ui->tabWidgetTrainerCardPage1);

    CatchChallenger::FightEngine::fightEngine.resetAll();
}

void BaseWindow::serverIsLoading()
{
    ui->label_connecting_status->setText(tr("Preparing the game data"));
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
    ui->player_informations_pseudo->setText(informations.public_informations.pseudo);
    ui->player_informations_cash->setText(QString("%1$").arg(informations.cash));
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

void BaseWindow::have_inventory(const QHash<quint32,quint32> &items)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::have_inventory()";
    #endif
    this->items=items;
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
                default:
                qDebug() << "waitedObjectType is unknow into load_inventory()";
                break;
            }
        }
        if(show)
        {
            QListWidgetItem *item;
            item=new QListWidgetItem();
            items_to_graphical[i.key()]=item;
            items_graphical[item]=i.key();
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
        load_inventory();
        load_plant_inventory();
        load_crafting_inventory();
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
        if(!DatapackClientLoader::datapackLoader.reputation.contains(i.key()))
        {
            error(QString("The reputation: %1 don't exist").arg(i.key()));
            return false;
        }
        if(i.value().level>=0)
        {
            if(i.value().level>=DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_positive.size())
            {
                error(QString("The reputation level %1 is wrong because is out of range (reputation level: %2 > max level: %3)").arg(i.key()).arg(i.value().level).arg(DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_positive.size()));
                return false;
            }
        }
        else
        {
            if((-i.value().level)>DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_negative.size())
            {
                error(QString("The reputation level %1 is wrong because is out of range (reputation level: %2 < max level: %3)").arg(i.key()).arg(i.value().level).arg(DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_negative.size()));
                return false;
            }
        }
        if(i.value().point>0)
        {
            if(DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_positive.size()==i.value().level)//start at level 0 in positive
            {
                emit message(QString("The reputation level is already at max, drop point"));
                return false;
            }
            if(i.value().point>=DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_positive.at(i.value().level+1))//start at level 0 in positive
            {
                error(QString("The reputation point %1 is greater than max %2").arg(i.value().point).arg(DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_positive.at(i.value().level)));
                return false;
            }
        }
        else if(i.value().point<0)
        {
            if(DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_negative.size()==-i.value().level)//start at level -1 in negative
            {
                error(QString("The reputation level is already at min, drop point"));
                return false;
            }
            if(i.value().point<DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_negative.at(-i.value().level))//start at level -1 in negative
            {
                error(QString("The reputation point %1 is greater than max %2").arg(i.value().point).arg(DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_negative.at(i.value().level)));
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
            if((i.value().level+1)==DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_positive.size())
                html+=QString("<li>100% %1</li>")
                    .arg(DatapackClientLoader::datapackLoader.reputation[i.key()].text_positive.last());
            else
            {
                qint32 next_level_xp=DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_positive.at(i.value().level+1);
                if(next_level_xp==0)
                {
                    error("Next level can't be need 0 xp");
                    return;
                }
                QString text=DatapackClientLoader::datapackLoader.reputation[i.key()].text_positive.at(i.value().level);
                html+=QString("<li>%1% %2</li>").arg((i.value().point*100)/next_level_xp).arg(text);
            }
        }
        else
        {
            if((-i.value().level)==DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_negative.size())
                html+=QString("<li>100% %1</li>")
                    .arg(DatapackClientLoader::datapackLoader.reputation[i.key()].text_negative.last());
            else
            {
                qint32 next_level_xp=DatapackClientLoader::datapackLoader.reputation[i.key()].reputation_negative.at(-i.value().level);
                if(next_level_xp==0)
                {
                    error("Next level can't be need 0 xp");
                    return;
                }
                QString text=DatapackClientLoader::datapackLoader.reputation[i.key()].text_negative.at(-i.value().level-1);
                html+=QString("<li>%1% %2</li>")
                    .arg((i.value().point*100)/next_level_xp)
                    .arg(text);
            }
        }
    }
    html+="</ul>";
    ui->labelReputation->setText(html);
}

void BaseWindow::updatePlayerImage()
{
    if(havePlayerInformations && haveDatapack)
    {
        Player_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations().public_informations;
        skinFolderList=CatchChallenger::FacilityLib::skinIdList(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN);

        //front image
        if(informations.skinId<skinFolderList.size())
        {
            playerFrontImage=QPixmap(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(informations.skinId)+"/front.png");
            if(playerFrontImage.isNull())
            {
                playerFrontImage=QPixmap(":/images/player_default/front.png");
                qDebug() << "Unable to load the player image: "+CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(informations.skinId)+"/front.png";
            }
        }
        else
        {
            playerFrontImage=QPixmap(":/images/player_default/front.png");
            qDebug() << "The skin id: "+QString::number(informations.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into BaseWindow::updatePlayerImage()";
        }

        //back image
        if(informations.skinId<skinFolderList.size())
        {
            playerBackImage=QPixmap(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(informations.skinId)+"/back.png");
            if(playerBackImage.isNull())
            {
                playerBackImage=QPixmap(":/images/player_default/back.png");
                qDebug() << "Unable to load the player image: "+CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(informations.skinId)+"/back.png";
            }
        }
        else
        {
            playerBackImage=QPixmap(":/images/player_default/back.png");
            qDebug() << "The skin id: "+QString::number(informations.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into BaseWindow::updatePlayerImage()";
        }

        //load into the UI
        ui->player_informations_front->setPixmap(playerFrontImage);
    }
}
