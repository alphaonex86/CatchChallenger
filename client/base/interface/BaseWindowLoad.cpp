#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "../ClientVariable.h"
#include "DatapackClientLoader.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>

using namespace Pokecraft;

void BaseWindow::resetAll()
{
    ui->frame_main_display_interface_player->hide();
    ui->label_interface_number_of_player->setText("?/?");
    ui->stackedWidget->setCurrentIndex(0);
    chat->resetAll();
    mapController->resetAll();
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
    canFight=false;
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
    client->sendDatapackContent();
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
            client->sendProtocol();
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
    Player_private_and_public_informations informations=client->get_player_informations();
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

    emit parseDatapack(client->get_datapack_base_name());
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
        if(!check_monsters())
            return;
        check_fight();
        load_monsters();
        this->setWindowTitle(tr("Pokecraft - %1").arg(client->getPseudo()));
        ui->stackedWidget->setCurrentIndex(1);
        showTip(tr("Welcome <b><i>%1</i></b> on pokecraft").arg(client->getPseudo()));
        return;
    }
    ui->label_connecting_status->setText(tr("Waiting: %1").arg(waitedData.join(", ")));
}

void BaseWindow::updatePlayerImage()
{
    if(havePlayerInformations && haveDatapack)
    {
        Player_public_informations informations=client->get_player_informations().public_informations;
        skinFolderList=Pokecraft::FacilityLib::skinIdList(client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN);
        QPixmap playerImage;
        if(informations.skinId<skinFolderList.size())
        {
            playerImage=QPixmap(client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(informations.skinId)+"/front.png");
            if(!playerImage.isNull())
                ui->player_informations_front->setPixmap(playerImage);
            else
            {
                ui->player_informations_front->setPixmap(QPixmap(":/images/player_default/front.png"));
                qDebug() << "Unable to load the player image: "+client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(informations.skinId)+"/front.png";
            }
        }
        else
        {
            ui->player_informations_front->setPixmap(QPixmap(":/images/player_default/front.png"));
            qDebug() << "The skin id: "+QString::number(informations.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into BaseWindow::updatePlayerImage()";
        }
    }
}

void BaseWindow::check_fight()
{
    canFight=false;
    int index=0;
    int size=client->player_informations.playerMonster.size();
    while(index<size)
    {
        if(client->player_informations.playerMonster.at(index).hp>0 && client->player_informations.playerMonster.at(index).egg_step==0)
        {
            canFight=true;
            break;
        }
        index++;
    }
    mapController->setCanGoToGrass(canFight);
}
