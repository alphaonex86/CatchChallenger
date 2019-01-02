#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../Options.h"
#include "../FacilityLibClient.h"
#include "../../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "../../../general/base/CommonSettingsServer.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../Ultimate.h"

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../Audio.h"
#endif

#include <QBuffer>
#include <QStandardPaths>
#include <iostream>

using namespace CatchChallenger;

void BaseWindow::resetAll()
{
    if(!Ultimate::ultimate.isUltimate())
    {
        ui->labelLastReplyTime->hide();
        ui->labelQueryList->hide();
    }
    else
    {
        ui->labelLastReplyTime->setText(tr("Last reply time: ?"));
        ui->labelQueryList->setText(tr("Running query: ? Query with worse time: ?"));
    }
    datapackGatewayProgression.clear();
    ui->labelSlow->hide();
    ui->frame_main_display_interface_player->hide();
    ui->label_interface_number_of_player->setText("?/?");
    ui->frameLoading->setStyleSheet("#frameLoading {border-image: url(:/images/empty.png);border-width: 0px;}");
    ui->stackedWidget->setCurrentWidget(ui->page_init);
    chat->resetAll();
    ui->itemFilterAdmin->clear();
    mapController->resetAll();
    waitedObjectType=ObjectType_All;
    lastReplyTimeValue=-1;
    protocolIsGood=false;
    monsterBeforeMoveForChangeInWaiting=false;
    worseQueryTime=0;
    progressingDatapackFileSize=0;
    haveDatapack=false;
    haveDatapackMainSub=false;
    characterSelected=false;
    haveCharacterPosition=false;
    haveCharacterInformation=false;
    DatapackClientLoader::datapackLoader.resetAll();
    haveInventory=false;
    isLogged=false;
    datapackIsParsed=false;
    mainSubDatapackIsParsed=false;
    characterListForSelection.clear();
    characterEntryListInWaiting.clear();
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
    inSelection=false;
    isInQuest=false;
    displayAttackProgression=0;
    mLastGivenXP=0;
    serverSelected=-1;
    fightTimerFinish=false;
    queryList.clear();
    ui->inventoryInformation->setVisible(false);
    ui->inventoryUse->setVisible(false);
    ui->inventoryDestroy->setVisible(false);
    ui->toolButtonAdmin->setVisible(false);
    previousRXSize=0;
    previousTXSize=0;
    attack_quantity_changed=0;
    previousAnimationWidget=NULL;
    ui->plantUse->setVisible(false);
    ui->craftingUse->setVisible(false);
    waitToSell=false;
    fightTimerFinish=false;
    ui->tabWidgetTrainerCard->setCurrentWidget(ui->tabWidgetTrainerCardPage1);
    ui->selectMonster->setVisible(false);
    doNextActionStep=DoNextActionStep_Start;
    actionClan.clear();
    clanName.clear();
    haveClanInformations=false;
    nextCityCatchTimer.stop();
    battleInformationsList.clear();
    botFightList.clear();
    buffToGraphicalItemTop.clear();
    buffToGraphicalItemBottom.clear();
    zonecatch=false;
    marketBuyInSuspend=false;
    marketBuyObjectList.clear();
    marketBuyCashInSuspend=0;
    marketPutMonsterList.clear();
    marketPutMonsterPlaceList.clear();
    marketPutInSuspend=false;
    marketPutCashInSuspend=0;
    marketWithdrawInSuspend=false;
    marketWithdrawObjectList.clear();
    marketWithdrawMonsterList.clear();
    datapackDownloadedCount=0;
    datapackDownloadedSize=0;
    escape=false;
    escapeSuccess=false;
    datapackFileNumber=0;
    datapackFileSize=0;
    baseMonsterEvolution=NULL;
    targetMonsterEvolution=NULL;
    monsterEvolutionPostion=0;
    evolutionControl=NULL;
    lastPlaceDisplayed.clear();
    events.clear();
    visualCategory.clear();
    add_to_inventoryGainList.clear();
    add_to_inventoryGainTime.clear();
    add_to_inventoryGainExtraList.clear();
    add_to_inventoryGainExtraTime.clear();
    client=NULL;
    frontSkinCacheString.clear();
    /*this is only mirror, drop into Api_protocol::resetAll()
    if(fightEngine.public_and_private_informations.encyclopedia_item!=NULL)
    {
        delete fightEngine.public_and_private_informations.encyclopedia_item;
        fightEngine.public_and_private_informations.encyclopedia_item=NULL;
    }
    if(fightEngine.public_and_private_informations.encyclopedia_monster!=NULL)
    {
        delete fightEngine.public_and_private_informations.encyclopedia_monster;
        fightEngine.public_and_private_informations.encyclopedia_monster=NULL;
    }*/

    #ifndef CATCHCHALLENGER_NOAUDIO
    if(currentAmbiance.manager!=NULL)
    {
        libvlc_event_detach(currentAmbiance.manager,libvlc_MediaPlayerEncounteredError,BaseWindow::vlceventStatic,currentAmbiance.player);
        libvlc_event_detach(currentAmbiance.manager,libvlc_MediaPlayerEndReached,BaseWindow::vlceventStatic,currentAmbiance.player);
        libvlc_media_player_stop(currentAmbiance.player);
        libvlc_media_player_release(currentAmbiance.player);
        Audio::audio.removePlayer(currentAmbiance.player);
        currentAmbiance.manager=NULL;
        currentAmbiance.player=NULL;
        currentAmbiance.file.clear();
    }
    #endif

    industryStatus.products.clear();
    industryStatus.resources.clear();
    if(newProfile!=NULL)
    {
        delete newProfile;
        newProfile=NULL;
    }

    fightEngine.resetAll();
}

void BaseWindow::serverIsLoading()
{
    ui->label_connecting_status->setText(tr("Preparing the game data"));
    //resetAll();->do bug because it reset this->client
}

void BaseWindow::serverIsReady()
{
    ui->label_connecting_status->setText(tr("Game data is ready"));
}

void BaseWindow::setMultiPlayer(bool multiplayer, CatchChallenger::Api_client_real *client)
{
    this->multiplayer=multiplayer;
    this->client=client;
    if(client!=NULL)
        mapController->setDatapackPath(client->datapackPathBase(),client->mainDatapackCode());
    else
        abort();
    chat->setClient(client);
    chat->setMultiPlayer(multiplayer);
    chat->show();
    chat->raise();
    fightEngine.setClient(client);
    /*if(!multiplayer)
        mapController->setOpenGl(true);*/
    //frame_main_display_right->setVisible(multiplayer);
    //ui->frame_main_display_interface_player->setVisible(multiplayer);//displayed when have the player connected (if have)
}

void BaseWindow::disconnected(std::string reason)
{
    if(haveShowDisconnectionReason)
        return;
    haveShowDisconnectionReason=true;
    QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(QString::fromStdString(reason)));
    newError("Disconnected","Disconnected by the reason: "+reason);
    resetAll();
}

void BaseWindow::notLogged(std::string reason)
{
    QMessageBox::information(this,tr("Unable to login"),tr("Unable to login: %1").arg(QString::fromStdString(reason)));
    newError("Unable to login","Unable to login: "+reason);
    resetAll();
}

void BaseWindow::logged(const std::vector<std::vector<CharacterEntry> > &characterEntryList)
{
    this->characterListForSelection=characterEntryList;
    if(settings.contains("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase())))
        client->sendDatapackContentBase(settings.value("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase())).toString().toStdString());
    else
        if(client==NULL)
        {
            std::cerr << "BaseWindow::logged() client==NULL" << std::endl;
            abort();
        }
    client->sendDatapackContentBase();
    isLogged=true;
    datapackGatewayProgression.clear();
    updateConnectingStatus();
    std::cout << "wait: haveTheDatapack()" << std::endl;
}

void BaseWindow::protocol_is_good()
{
    std::cout << "BaseWindow::protocol_is_good()" << std::endl;
    protocolIsGood=true;
    updateConnectingStatus();
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
        }
        else
            ui->label_connecting_status->setText(tr("Connecting to the server..."));
    }
}

void BaseWindow::haveCharacter()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveCharacter()";
    #endif
    if(haveCharacterInformation)
        return;

    fightEngine.public_and_private_informations.playerMonster=client->player_informations.playerMonster;
    fightEngine.setVariableContent(client->get_player_informations());

    haveCharacterInformation=true;

    datapackGatewayProgression.clear();
    updateConnectingStatus();
}

void BaseWindow::sendDatapackContentMainSub()
{
    if(settings.contains("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain())) &&
            settings.contains("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub())))
        client->sendDatapackContentMainSub(settings.value("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain())).toString().toStdString(),
                settings.value("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub())).toString().toStdString());
    else
        client->sendDatapackContentMainSub();
}

void BaseWindow::have_character_position()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::have_character_position()";
    #endif
    if(haveCharacterPosition)
        return;

    haveCharacterPosition=true;

    updateConnectingStatus();
    updatePlayerType();
}

void BaseWindow::have_main_and_sub_datapack_loaded()
{
    client->have_main_and_sub_datapack_loaded();
    if(!client->getCaracterSelected())
    {
        error("BaseWindow::have_main_and_sub_datapack_loaded(): don't have player info, need to code this delay part");
        return;
    }
    const Player_private_and_public_informations &informations=client->get_player_informations();

    ui->player_informations_pseudo->setText(QString::fromStdString(informations.public_informations.pseudo));
    ui->tradePlayerPseudo->setText(QString::fromStdString(informations.public_informations.pseudo));
    ui->warehousePlayerPseudo->setText(QString::fromStdString(informations.public_informations.pseudo));
    ui->player_informations_cash->setText(QStringLiteral("%1$").arg(informations.cash));
    ui->shopCash->setText(tr("Cash: %1$").arg(informations.cash));

    //always after monster load on CatchChallenger::ClientFightEngine::fightEngine
    mapController->have_current_player_info(informations);

    qDebug() << (QStringLiteral("%1 is logged with id: %2, cash: %3")
                 .arg(QString::fromStdString(informations.public_informations.pseudo))
                 .arg(informations.public_informations.simplifiedId)
                 .arg(informations.cash)
                 );
    updateConnectingStatus();
    updateClanDisplay();
}

void BaseWindow::updatePlayerType()
{
    const Player_private_and_public_informations &informations=client->get_player_informations();
    ui->player_informations_type->setText(QString());
    ui->player_informations_type->setPixmap(QPixmap());
    switch(informations.public_informations.type)
    {
        default:
        case Player_type_normal:
            ui->player_informations_type->setText(tr("Normal player"));
        break;
        case Player_type_premium:
            ui->player_informations_type->setPixmap(QPixmap(":/images/chat/premium.png"));
        break;
        case Player_type_dev:
            ui->player_informations_type->setPixmap(QPixmap(":/images/chat/developer.png"));
        break;
        case Player_type_gm:
            ui->player_informations_type->setPixmap(QPixmap(":/images/chat/admin.png"));
        break;
    }
    const CatchChallenger::Player_type &type=informations.public_informations.type;
    const bool isAdmin=type==CatchChallenger::Player_type_dev || type==CatchChallenger::Player_type_gm;
    ui->toolButtonAdmin->setVisible(isAdmin);
    if(isAdmin)
    {
        ui->listAllItem->clear();
        for (const auto &n : DatapackClientLoader::datapackLoader.itemsExtra)
        {
            const uint16_t &itemId=n.first;
            const DatapackClientLoader::ItemExtra &itemsExtra=n.second;
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString::fromStdString(itemsExtra.name));
            item->setData(99,itemId);
            item->setIcon(QIcon(itemsExtra.image));
            ui->listAllItem->addItem(item);
        }
    }
}

void BaseWindow::insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(player);
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);
    const Player_public_informations &informations=client->get_player_informations().public_informations;
    if(informations.simplifiedId==player.simplifiedId)
        updatePlayerImage();
}

void BaseWindow::haveTheDatapack()
{
    if(client==NULL)
        return;
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveTheDatapack()";
    #endif
    if(haveDatapack)
        return;
    haveDatapack=true;
    settings.setValue("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase()),
                      QByteArray(
                          CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),
                          static_cast<int>(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size())
                                  )
                      );

    if(client!=NULL)
        emit parseDatapack(client->datapackPathBase());
}

void BaseWindow::haveDatapackMainSubCode()
{
    sendDatapackContentMainSub();
    updateConnectingStatus();
}

void BaseWindow::haveTheDatapackMainSub()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveTheDatapackMainSub()";
    #endif
    if(haveDatapackMainSub)
        return;
    haveDatapackMainSub=true;
    settings.setValue("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain()),
                      QByteArray(
                          CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data(),
                          static_cast<int>(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size())
                                  )
                      );
    settings.setValue("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub()),
                      QByteArray(
                          CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data(),
                          static_cast<int>(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size())
                                  )
                      );

    if(client!=NULL)
        emit parseDatapackMainSub(client->mainDatapackCode(),client->subDatapackCode());
}

void BaseWindow::datapackSize(const uint32_t &datapackFileNumber,const uint32_t &datapackFileSize)
{
    this->datapackFileNumber=datapackFileNumber;
    this->datapackFileSize=datapackFileSize;
    updateConnectingStatus();
}

void BaseWindow::gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression)
{
    /*if(progression==100)
        datapackGatewayProgression.remove(gateway);
    else*/
        datapackGatewayProgression[gateway]=progression;
    updateConnectingStatus();
}

void BaseWindow::newDatapackFile(const uint32_t &size)
{
    progressingDatapackFileSize=0;
    datapackDownloadedCount++;
    datapackDownloadedSize+=size;
    updateConnectingStatus();
}

void BaseWindow::progressingDatapackFile(const uint32_t &size)
{
    progressingDatapackFileSize=size;
    updateConnectingStatus();
}

void BaseWindow::have_inventory(const std::unordered_map<uint16_t,uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items)
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations();
    playerInformations.items=items;
    playerInformations.warehouse_items=warehouse_items;
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::have_inventory()";
    #endif
    haveInventory=true;
    updateConnectingStatus();
}

void BaseWindow::load_inventory()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
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
    auto i=playerInformations.items.begin();
    while (i!=playerInformations.items.cend())
    {
        bool show=!inSelection;
        if(inSelection)
        {
            switch(waitedObjectType)
            {
                case ObjectType_Seed:
                    //reputation requierement control is into load_plant_inventory() NOT: on_listPlantList_itemSelectionChanged()
                    if(DatapackClientLoader::datapackLoader.itemToPlants.find(i->first)!=DatapackClientLoader::datapackLoader.itemToPlants.cend())
                        show=true;
                break;
                case ObjectType_UseInFight:
                    if(fightEngine.isInFightWithWild() && CommonDatapack::commonDatapack.items.trap.find(i->first)!=CommonDatapack::commonDatapack.items.trap.cend())
                        show=true;
                    else if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(i->first)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
                        show=true;
                    else
                        show=false;
                break;
                default:
                qDebug() << "waitedObjectType is unknow into load_inventory()";
                break;
            }
        }
        if(show)
        {
            QListWidgetItem *item=new QListWidgetItem();
            items_to_graphical[i->first]=item;
            items_graphical[item]=i->first;
            if(DatapackClientLoader::datapackLoader.itemsExtra.find(i->first)!=DatapackClientLoader::datapackLoader.itemsExtra.cend())
            {
                item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.at(i->first).image);
                if(i->second>1)
                    item->setText(QString::number(i->second));
                item->setToolTip(QString::fromStdString(DatapackClientLoader::datapackLoader.itemsExtra.at(i->first).name));
            }
            else
            {
                item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
                if(i->second>1)
                    item->setText(QStringLiteral("id: %1 (x%2)").arg(i->first).arg(i->second));
                else
                    item->setText(QStringLiteral("id: %1").arg(i->first));
            }
            ui->inventory->addItem(item);
        }
        ++i;
    }
}

void BaseWindow::datapackParsed()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::datapackParsed()";
    #endif
    datapackIsParsed=true;
    updateConnectingStatus();
    loadSettingsWithDatapack();
    {
        if(QFile(QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelBottom.png")).exists())
            ui->frameFightBottom->setStyleSheet(QStringLiteral("#frameFightBottom{background-image: url('")+QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelBottom.png');padding:6px 6px 6px 14px;}"));
        else
            ui->frameFightBottom->setStyleSheet(QStringLiteral("#frameFightBottom{background-image: url(:/images/interface/fight/labelBottom.png);padding:6px 6px 6px 14px;}"));
        if(QFile(QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelTop.png")).exists())
            ui->frameFightTop->setStyleSheet(QStringLiteral("#frameFightTop{background-image: url('")+QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelTop.png');padding:6px 14px 6px 6px;}"));
        else
            ui->frameFightTop->setStyleSheet(QStringLiteral("#frameFightTop{background-image: url(:/images/interface/fight/labelTop.png);padding:6px 14px 6px 6px;}"));
    }
    //updatePlayerImage();
}

void BaseWindow::datapackParsedMainSub()
{
    if(client==NULL)
        return;
    if(mapController==NULL)
        return;
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::datapackParsedMainSub()";
    #endif
    mainSubDatapackIsParsed=true;

    //always after monster load on CatchChallenger::ClientFightEngine::fightEngine
    mapController->setDatapackPath(client->datapackPathBase(),client->mainDatapackCode());
    if(!client->setMapNumber(DatapackClientLoader::datapackLoader.mapToId.size()))
        emit newError(tr("Internal error").toStdString(),"No map loaded to call client->setMapNumber()");

    have_main_and_sub_datapack_loaded();

    emit datapackParsedMainSubMap();

    updateConnectingStatus();
}

void BaseWindow::datapackChecksumError()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::datapackChecksumError()";
    #endif
    datapackIsParsed=false;
    //reset all the cached hash
    settings.remove("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase()));
    settings.remove("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain()));
    settings.remove("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub()));
    emit newError(tr("Datapack on mirror is corrupted").toStdString(),
                  "The checksum sended by the server is not the same than have on the mirror");
}

void BaseWindow::loadSoundSettings()
{
    #ifndef CATCHCHALLENGER_NOAUDIO
    const QStringList &outputDeviceNames=Audio::audio.output_list();
    #else
    const QStringList outputDeviceNames;
    #endif
    Options::options.setAudioDeviceList(outputDeviceNames);
    ui->audiodevice->clear();
    if(outputDeviceNames.isEmpty())
        {}//soundEngine.setOutputDeviceName(QString());
    else
    {
        const int &indexDevice=Options::options.getIndexDevice();
        ui->audiodevice->addItems(outputDeviceNames);
        if(indexDevice!=-1)
        {
            ui->audiodevice->setCurrentIndex(indexDevice);
            {}//soundEngine.setOutputDeviceName(outputDeviceNames.at(indexDevice));
        }
        if(!connect(ui->audiodevice,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&BaseWindow::changeDeviceIndex))
            abort();
    }
}

void BaseWindow::setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events)
{
    this->events.clear();
    unsigned int index=0;
    while(index<events.size())
    {
        const std::pair<uint8_t,uint8_t> event=events.at(index);
        if(event.first>=CommonDatapack::commonDatapack.events.size())
        {
            emit error("BaseWindow::setEvents() event out of range");
            break;
        }
        if(event.second>=CommonDatapack::commonDatapack.events.at(event.first).values.size())
        {
            emit error("BaseWindow::setEvents() event value out of range");
            break;
        }
        while(this->events.size()<=event.first)
            this->events.push_back(0);
        this->events[event.first]=event.second;
        index++;
    }
    load_event();
}

void BaseWindow::load_event()
{
    if(datapackIsParsed)
    {
        while((uint32_t)events.size()<CatchChallenger::CommonDatapack::commonDatapack.events.size())
            events.push_back(0);
    }
    if((uint32_t)events.size()>CatchChallenger::CommonDatapack::commonDatapack.events.size())
    {
        while((uint32_t)events.size()>CatchChallenger::CommonDatapack::commonDatapack.events.size())
            events.pop_back();
        emit error("BaseWindow::load_event() event list biger than it should");
    }
    //set the event
    {
        uint8_t index=0;
        while(index<events.size())
        {
            forcedEvent(index,events.at(index));
            index++;
        }
    }
}

void BaseWindow::updateConnectingStatus()
{
    if(isLogged && datapackIsParsed)
    {
        const std::vector<ServerFromPoolForDisplay> &serverOrdenedList=client->getServerOrdenedList();
        if(serverSelected==-1)
        {
            if(ui->stackedWidget->currentWidget()!=ui->page_serverList)
            {
                if(multiplayer)
                {
                    ui->stackedWidget->setCurrentWidget(ui->page_serverList);
                    ui->serverList->header()->setSectionResizeMode(QHeaderView::Fixed);
                    ui->serverList->header()->resizeSection(0,680);
                    updateServerList();
                }
                else
                {
                    /*updateServerList();
                    bool ok;

                    const QTreeWidgetItem * const selectedItem=ui->serverList->itemAt(0,0);
                    if(selectedItem==NULL)
                    {
                        error(tr("Internal Error: No internal server detected").toStdString());
                        return;
                    }

                    serverSelected=selectedItem->data(99,99).toInt(&ok);
                    if(!ok)
                    {
                        error("BaseWindow::updateConnectingStatus(): serverSelected=selectedItem->data(99,99).toUInt() convert to int wrong: "+selectedItem->data(99,99).toString().toStdString());
                        return;
                    }*/
                    serverSelected=0;
                    if(serverSelected<0 || serverSelected>=(int)serverOrdenedList.size())
                    {
                        error("BaseWindow::updateConnectingStatus(): serverSelected=selectedItem->data(99,99).toUInt() corrupted value");
                        return;
                    }
                    updateConnectingStatus();
                }
                return;
            }
        }
        else if(!haveCharacterPosition && !haveCharacterInformation && !client->character_select_is_send() &&
                (unsigned int)serverSelected<serverOrdenedList.size())
        {
            if(ui->stackedWidget->currentWidget()!=ui->page_character)
            {
                if(multiplayer)
                    ui->stackedWidget->setCurrentWidget(ui->page_character);
                const uint8_t &charactersGroupIndex=serverOrdenedList.at(serverSelected).charactersGroupIndex;
                const std::vector<CharacterEntry> &characterEntryList=characterListForSelection.at(charactersGroupIndex);
                ui->character_add->setEnabled(characterEntryList.size()<CommonSettingsCommon::commonSettingsCommon.max_character);
                ui->character_remove->setEnabled(characterEntryList.size()>CommonSettingsCommon::commonSettingsCommon.min_character);
                if(characterEntryList.empty())
                {
                    if(CommonSettingsCommon::commonSettingsCommon.max_character==0)
                        emit message("Can't create character but the list is empty");
                }
                updateCharacterList();
                if((characterListForSelection.empty() ||
                    characterListForSelection.at(charactersGroupIndex).empty()) &&
                        CommonSettingsCommon::commonSettingsCommon.max_character>0)
                {
                    if(CommonSettingsCommon::commonSettingsCommon.min_character>0)
                    {
                        ui->frameLoading->setStyleSheet("#frameLoading {border-image: url(:/images/empty.png);border-width: 0px;}");
                        ui->stackedWidget->setCurrentWidget(ui->page_init);
                        ui->label_connecting_status->setText(QString());
                    }
                    on_character_add_clicked();
                    return;
                }
                if(characterListForSelection.size()==1 && CommonSettingsCommon::commonSettingsCommon.min_character>=characterListForSelection.size() &&
                        CommonSettingsCommon::commonSettingsCommon.max_character<=characterListForSelection.size())
                {
                    if(characterListForSelection.at(charactersGroupIndex).size()==1)
                    {
                        characterSelected=true;
                        ui->characterEntryList->item(ui->characterEntryList->count()-1)->setSelected(true);
                        on_character_select_clicked();
                        return;
                    }
                    else
                        emit message("BaseWindow::updateConnectingStatus(): characterListForSelection.at(charactersGroupIndex).size()!=1, bug");
                }
                return;
            }
        }
    }
    QStringList waitedData;
    if((haveCharacterPosition && haveCharacterInformation) && !mainSubDatapackIsParsed)
        waitedData << tr("Loading of the specific datapack part");
    if(haveDatapack && (!haveInventory || !haveCharacterPosition || !haveCharacterInformation))
    {
        if(!haveCharacterPosition || !haveCharacterInformation)
            waitedData << tr("Loading of the player informations");
        else
            waitedData << tr("Loading of the inventory");
    }
    if(!haveDatapack)
    {
        if(!protocolIsGood)
            waitedData << tr("Try send the protocol...");
        else if(!isLogged)
        {
            if(datapackGatewayProgression.empty())
                waitedData << tr("Try login...");
            else if(datapackGatewayProgression.size()<2)
                waitedData << tr("Updating the gateway cache...");
            else
                waitedData << tr("Updating the %1 gateways cache...").arg(datapackGatewayProgression.size());
        }
        else
        {
            if(datapackFileSize==0)
                waitedData << tr("Loading of the datapack");
            else if(datapackFileSize<0)
                waitedData << tr("Loaded datapack size: %1KB").arg((datapackDownloadedSize+progressingDatapackFileSize)/1000);//when the http server don't send the size
            else if((datapackDownloadedSize+progressingDatapackFileSize)>=(uint32_t)datapackFileSize)
                waitedData << tr("Loaded datapack file: 100%");
            else
                waitedData << tr("Loaded datapack file: %1%").arg(((datapackDownloadedSize+progressingDatapackFileSize)*100)/datapackFileSize);
        }
    }
    else if(!datapackIsParsed)
        waitedData << tr("Opening the datapack");
    if(waitedData.isEmpty())
    {
        Player_private_and_public_informations &playerInformations=client->get_player_informations();
        if(playerInformations.bot_already_beaten==NULL)
        {
            std::cerr << "void BaseWindow::updateConnectingStatus(): waitedData.isEmpty(), playerInformations.bot_already_beaten==NULL" << std::endl;
            abort();
        }
        mapController->setBotsAlreadyBeaten(playerInformations.bot_already_beaten);
        mapController->setInformations(&playerInformations.items,&playerInformations.quests,&events,&playerInformations.itemOnMap,&playerInformations.plantOnMap);
        client->unloadSelection();
        load_inventory();
        load_plant_inventory();
        load_crafting_inventory();
        updateDisplayedQuests();
        if(!check_senddata())
            return;
        load_monsters();
        show_reputation();
        load_event();
        emit gameIsLoaded();
        this->setWindowTitle(QStringLiteral("CatchChallenger - %1").arg(QString::fromStdString(client->getPseudo())));
        ui->stackedWidget->setCurrentWidget(ui->page_map);
        showTip(tr("Welcome <b><i>%1</i></b> on <i>CatchChallenger</i>").arg(QString::fromStdString(client->getPseudo())).toStdString());
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
    auto i=client->player_informations.reputation.begin();
    while(i!=client->player_informations.reputation.cend())
    {
        if(i->second.level<-100 || i->second.level>100)
        {
            error("level is <100 or >100, skip");
            return false;
        }
        if(i->first>=CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
        {
            error(QStringLiteral("The reputation: %1 don't exist").arg(i->first).toStdString());
            return false;
        }
        if(i->second.level>=0)
        {
            if(i->second.level>=(int32_t)CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_positive.size())
            {
                error(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 > max level: %3)")
                      .arg(i->first).arg(i->second.level)
                      .arg(CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first)
                           .reputation_positive.size()).toStdString());
                return false;
            }
        }
        else
        {
            if((-i->second.level)>(int32_t)CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_negative.size())
            {
                error(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 < max level: %3)")
                      .arg(i->first).arg(i->second.level)
                      .arg(CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first)
                           .reputation_negative.size()).toStdString());
                return false;
            }
        }
        if(i->second.point>0)
        {
            if((int32_t)CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_positive.size()==i->second.level)//start at level 0 in positive
            {
                emit message("The reputation level is already at max, drop point");
                return false;
            }
            if(i->second.point>=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_positive.at(i->second.level+1))//start at level 0 in positive
            {
                error(QStringLiteral("The reputation point %1 is greater than max %2")
                      .arg(i->second.point)
                      .arg(CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first)
                           .reputation_positive.at(i->second.level)).toStdString());
                return false;
            }
        }
        else if(i->second.point<0)
        {
            if((int32_t)CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_negative.size()==-i->second.level)//start at level -1 in negative
            {
                error("The reputation level is already at min, drop point");
                return false;
            }
            if(i->second.point<CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_negative.at(-i->second.level))//start at level -1 in negative
            {
                error(QStringLiteral("The reputation point %1 is greater than max %2")
                      .arg(i->second.point)
                      .arg(CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first)
                           .reputation_negative.at(i->second.level)).toStdString());
                return false;
            }
        }
        ++i;
    }
    return true;
}

void BaseWindow::show_reputation()
{
    std::string html="<ul>";
    auto i=client->player_informations.reputation.begin();
    while(i!=client->player_informations.reputation.cend())
    {
        if(i->second.level>=0)
        {
            if((i->second.level+1)==(int32_t)CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_positive.size())
                html+=QStringLiteral("<li>100% %1</li>")
                    .arg(QString::fromStdString(DatapackClientLoader::datapackLoader.reputationExtra.at(
                             CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).name
                             ).reputation_positive.back())).toStdString();
            else
            {
                int32_t next_level_xp=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_positive.at(i->second.level+1);
                if(next_level_xp==0)
                {
                    error("Next level can't be need 0 xp");
                    return;
                }
                std::string text=DatapackClientLoader::datapackLoader.reputationExtra.at(
                            CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).name
                            ).reputation_positive.at(i->second.level);
                html+=QStringLiteral("<li>%1% %2</li>").arg((i->second.point*100)/next_level_xp).arg(QString::fromStdString(text)).toStdString();
            }
        }
        else
        {
            if((-i->second.level)==(int32_t)CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_negative.size())
                html+=QStringLiteral("<li>100% %1</li>")
                    .arg(QString::fromStdString(DatapackClientLoader::datapackLoader.reputationExtra.at(
                             CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).name
                             ).reputation_negative.back())).toStdString();
            else
            {
                int32_t next_level_xp=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).reputation_negative.at(-i->second.level);
                if(next_level_xp==0)
                {
                    error("Next level can't be need 0 xp");
                    return;
                }
                std::string text=DatapackClientLoader::datapackLoader.reputationExtra.at(
                            CatchChallenger::CommonDatapack::commonDatapack.reputation.at(i->first).name
                            ).reputation_negative.at(-i->second.level-1);
                html+=QStringLiteral("<li>%1% %2</li>")
                    .arg((i->second.point*100)/next_level_xp)
                    .arg(QString::fromStdString(text)).toStdString();
            }
        }
        ++i;
    }
    html+="</ul>";
    ui->labelReputation->setText(QString::fromStdString(html));
}

QPixmap BaseWindow::getFrontSkin(const std::string &skinName)
{
    if(frontSkinCacheString.find(skinName)!=frontSkinCacheString.cend())
        return frontSkinCacheString.at(skinName);
    const QPixmap skin(QString::fromStdString(getFrontSkinPath(skinName)));
    if(!skin.isNull())
        frontSkinCacheString[skinName]=skin;
    else
        frontSkinCacheString[skinName]=QPixmap(":/images/player_default/front.png");
    return frontSkinCacheString.at(skinName);
}

QPixmap BaseWindow::getFrontSkin(const uint32_t &skinId)
{
    if(skinId<(uint32_t)DatapackClientLoader::datapackLoader.skins.size())
        return getFrontSkin(DatapackClientLoader::datapackLoader.skins.at(skinId));
    else
        return getFrontSkin(std::string());
}

QPixmap BaseWindow::getBackSkin(const uint32_t &skinId) const
{
    const QPixmap skin(QString::fromStdString(getBackSkinPath(skinId)));
    if(!skin.isNull())
        return skin;
    return QPixmap(":/images/player_default/back.png");
}

std::string BaseWindow::getSkinPath(const std::string &skinName,const std::string &type) const
{
    {
        QFileInfo pngFile(QString::fromStdString(client->datapackPathBase())+
                          DATAPACK_BASE_PATH_SKIN+QString::fromStdString(skinName)+
                          QStringLiteral("/%1.png").arg(QString::fromStdString(type)));
        if(pngFile.exists())
            return pngFile.absoluteFilePath().toStdString();
    }
    {
        QFileInfo gifFile(QString::fromStdString(client->datapackPathBase())+
                          DATAPACK_BASE_PATH_SKIN+QString::fromStdString(skinName)+
                          QStringLiteral("/%1.gif").arg(QString::fromStdString(type)));
        if(gifFile.exists())
            return gifFile.absoluteFilePath().toStdString();
    }
    QDir folderList(QString::fromStdString(client->datapackPathBase())+DATAPACK_BASE_PATH_SKINBASE);
    const QStringList &entryList=folderList.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    int entryListIndex=0;
    while(entryListIndex<entryList.size())
    {
        {
            QFileInfo pngFile(QStringLiteral("%1/skin/%2/%3/%4.png")
                              .arg(QString::fromStdString(client->datapackPathBase()))
                              .arg(entryList.at(entryListIndex))
                              .arg(QString::fromStdString(skinName))
                              .arg(QString::fromStdString(type)));
            if(pngFile.exists())
                return pngFile.absoluteFilePath().toStdString();
        }
        {
            QFileInfo gifFile(QStringLiteral("%1/skin/%2/%3/%4.gif")
                              .arg(QString::fromStdString(client->datapackPathBase()))
                              .arg(entryList.at(entryListIndex))
                              .arg(QString::fromStdString(skinName))
                              .arg(QString::fromStdString(type)));
            if(gifFile.exists())
                return gifFile.absoluteFilePath().toStdString();
        }
        entryListIndex++;
    }
    return std::string();
}

std::string BaseWindow::getFrontSkinPath(const uint32_t &skinId)
{
    /// \todo merge it cache string + id
    const std::vector<std::string> &skinFolderList=DatapackClientLoader::datapackLoader.skins;
    //front image
    if(skinId<(uint32_t)skinFolderList.size())
        return getSkinPath(skinFolderList.at(skinId),"front");
    else
        qDebug() << "The skin id: "+QString::number(skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into BaseWindow::updatePlayerImage()";
    return ":/images/player_default/front.png";
}

std::string BaseWindow::getFrontSkinPath(const std::string &skin)
{
    return getSkinPath(skin,"front");
}

std::string BaseWindow::getBackSkinPath(const uint32_t &skinId) const
{
    const std::vector<std::string> &skinFolderList=DatapackClientLoader::datapackLoader.skins;
    //front image
    if(skinId<(uint32_t)skinFolderList.size())
        return getSkinPath(skinFolderList.at(skinId),"back");
    else
        qDebug() << "The skin id: "+QString::number(skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into BaseWindow::updatePlayerImage()";
    return ":/images/player_default/back.png";
}

void BaseWindow::updatePlayerImage()
{
    if(haveCharacterPosition && haveDatapack)
    {
        const Player_public_informations &informations=client->get_player_informations().public_informations;
        playerFrontImage=getFrontSkin(informations.skinId);
        playerBackImage=getBackSkin(informations.skinId);
        playerFrontImagePath=getFrontSkinPath(informations.skinId);
        playerBackImagePath=getBackSkinPath(informations.skinId);

        //load into the UI
        ui->player_informations_front->setPixmap(playerFrontImage);
        ui->warehousePlayerImage->setPixmap(playerFrontImage);
        ui->tradePlayerImage->setPixmap(playerFrontImage);
    }
}

void BaseWindow::updateDisplayedQuests()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    std::string html("<ul>");
    ui->questsList->clear();
    quests_to_id_graphical.clear();
    auto i=playerInformations.quests.begin();
    while(i!=playerInformations.quests.cend())
    {
        if(DatapackClientLoader::datapackLoader.questsExtra.find(i->first)!=DatapackClientLoader::datapackLoader.questsExtra.cend())
        {
            if(i->second.step>0)
            {
                QListWidgetItem * item=new QListWidgetItem(QString::fromStdString(DatapackClientLoader::datapackLoader.questsExtra.at(i->first).name));
                quests_to_id_graphical[item]=i->first;
                ui->questsList->addItem(item);
            }
            if(i->second.step==0 || i->second.finish_one_time)
            {
                if(Ultimate::ultimate.isUltimate())
                {
                    html+="<li>";
                    if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(i->first).repeatable)
                        html+=imagesInterfaceRepeatableString;
                    if(i->second.step>0)
                        html+=imagesInterfaceInProgressString;
                    html+=DatapackClientLoader::datapackLoader.questsExtra.at(i->first).name+"</li>";
                }
                else
                    html+="<li>"+DatapackClientLoader::datapackLoader.questsExtra.at(i->first).name+"</li>";
            }
        }
        ++i;
    }
    html+="</ul>";
    if(html=="<ul></ul>")
        html="<i>None</i>";
    ui->finishedQuests->setHtml(QString::fromStdString(html));
    on_questsList_itemSelectionChanged();
}

void BaseWindow::on_questsList_itemSelectionChanged()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    QList<QListWidgetItem *> items=ui->questsList->selectedItems();
    if(items.size()!=1)
    {
        ui->questDetails->setText(tr("Select a quest"));
        return;
    }
    if(quests_to_id_graphical.find(items.front())==quests_to_id_graphical.cend())
    {
        qDebug() << "Selected quest have not id";
        ui->questDetails->setText(tr("Select a quest"));
        return;
    }
    const uint16_t &questId=quests_to_id_graphical.at(items.first());
    if(playerInformations.quests.find(questId)==playerInformations.quests.cend())
    {
        qDebug() << "Selected quest is not into the player list";
        ui->questDetails->setText(tr("Select a quest"));
        return;
    }
    const DatapackClientLoader::QuestExtra &questExtra=DatapackClientLoader::datapackLoader.questsExtra.at(questId);
    if(playerInformations.quests.at(questId).step==0 ||
            playerInformations.quests.at(questId).step>questExtra.steps.size())
    {
        qDebug() << "Selected quest step is out of range";
        ui->questDetails->setText(tr("Select a quest"));
        return;
    }
    const uint8_t stepQuest=playerInformations.quests.at(questId).step-1;
    std::string stepDescription;
    {
        if(stepQuest>=questExtra.steps.size())
        {
            qDebug() << "no condition match into stepDescriptionList";
            ui->questDetails->setText(tr("Select a quest"));
            return;
        }
        else
            stepDescription=questExtra.steps.at(stepQuest);
    }
    stepDescription+="<br />";
    std::string stepRequirements;
    {
        std::vector<Quest::Item> items=CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId)
                .steps.at(playerInformations.quests.at(questId).step-1).requirements.items;
        std::vector<std::string> objects;
        unsigned int index=0;
        while(index<items.size())
        {
            QPixmap image;
            std::string name;
            if(DatapackClientLoader::datapackLoader.itemsExtra.find(items.at(index).item)!=DatapackClientLoader::datapackLoader.itemsExtra.cend())
            {
                image=DatapackClientLoader::datapackLoader.itemsExtra.at(items.at(index).item).image;
                name=DatapackClientLoader::datapackLoader.itemsExtra.at(items.at(index).item).name;
            }
            else
            {
                image=DatapackClientLoader::datapackLoader.defaultInventoryImage();
                name="id: "+std::to_string(items.at(index).item);
            }

            image=image.scaled(24,24);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            image.save(&buffer, "PNG");
            if(objects.size()<16)
            {
                if(items.at(index).quantity>1)
                    objects.push_back(QStringLiteral("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />")
                                      .arg(QString(byteArray.toBase64()))
                                      .arg(items.at(index).quantity).arg(QString::fromStdString(name))
                                      .toStdString());
                else
                    objects.push_back(QStringLiteral("%2 <img src=\"data:image/png;base64,%1\" />")
                                      .arg(QString(byteArray.toBase64()))
                                      .arg(QString::fromStdString(name)).toStdString());
            }
            index++;
        }
        if(objects.size()==16)
            objects.push_back("...");
        stepRequirements+=tr("Step requirements: ").toStdString()+"<br />"+stringimplode(objects,", ");
    }
    std::string finalRewards;
    if(questExtra.showRewards || Ultimate::ultimate.isUltimate())
    {
        finalRewards+=tr("Final rewards: ").toStdString();
        {
            std::vector<Quest::Item> items=CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId).rewards.items;
            std::vector<std::string> objects;
            unsigned int index=0;
            while(index<items.size())
            {
                QPixmap image;
                std::string name;
                if(DatapackClientLoader::datapackLoader.itemsExtra.find(items.at(index).item)!=
                        DatapackClientLoader::datapackLoader.itemsExtra.cend())
                {
                    image=DatapackClientLoader::datapackLoader.itemsExtra.at(items.at(index).item).image;
                    name=DatapackClientLoader::datapackLoader.itemsExtra.at(items.at(index).item).name;
                }
                else
                {
                    image=DatapackClientLoader::datapackLoader.defaultInventoryImage();
                    name=QStringLiteral("id: %1").arg(items.at(index).item).toStdString();
                }
                image=image.scaled(24,24);
                QByteArray byteArray;
                QBuffer buffer(&byteArray);
                image.save(&buffer, "PNG");
                if(objects.size()<16)
                {
                    if(items.at(index).quantity>1)
                        objects.push_back(QStringLiteral("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />")
                                          .arg(QString(byteArray.toBase64()))
                                          .arg(items.at(index).quantity)
                                          .arg(QString::fromStdString(name))
                                          .toStdString());
                    else
                        objects.push_back(QStringLiteral("%2 <img src=\"data:image/png;base64,%1\" />")
                                          .arg(QString(byteArray.toBase64()))
                                          .arg(QString::fromStdString(name))
                                          .toStdString());
                }
                index++;
            }
            if(objects.size()==16)
                objects.push_back("...");
            finalRewards+=stringimplode(objects,", ")+"<br />";
        }
        {
            std::vector<ReputationRewards> reputationRewards=CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId).rewards.reputation;
            std::vector<std::string> reputations;
            unsigned int index=0;
            while(index<reputationRewards.size())
            {
                if(DatapackClientLoader::datapackLoader.reputationExtra.find(
                            CatchChallenger::CommonDatapack::commonDatapack.reputation.at(
                                                       reputationRewards.at(index).reputationId).name
                            )!=
                        DatapackClientLoader::datapackLoader.reputationExtra.cend())
                {
                    const std::string &reputationName=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(
                                reputationRewards.at(index).reputationId).name;
                    if(reputationRewards.at(index).point<0)
                        reputations.push_back(tr("Less reputation for %1").arg(QString::fromStdString(DatapackClientLoader::datapackLoader.reputationExtra.at(reputationName).name)).toStdString());
                    if(reputationRewards.at(index).point>0)
                        reputations.push_back(tr("More reputation for %1").arg(QString::fromStdString(DatapackClientLoader::datapackLoader.reputationExtra.at(reputationName).name)).toStdString());
                }
                index++;
            }
            if(reputations.size()>16)
            {
                while(reputations.size()>15)
                    reputations.pop_back();
                reputations.push_back("...");
            }
            finalRewards+=stringimplode(reputations,", ")+"<br />";
        }
        {
            std::vector<ActionAllow> allowRewards=CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId).rewards.allow;
            std::vector<std::string> allows;
            unsigned int index=0;
            while(index<allowRewards.size())
            {
                if(vectorcontainsAtLeastOne(allowRewards,ActionAllow_Clan))
                    allows.push_back(tr("Add permission to create clan").toStdString());
                index++;
            }
            if(allows.size()>16)
            {
                while(allows.size()>15)
                    allows.pop_back();
                allows.push_back("...");
            }
            finalRewards+=stringimplode(allows,", ")+"<br />";
        }
    }

    ui->questDetails->setText(QString::fromStdString(stepDescription+stepRequirements+finalRewards));
}

QListWidgetItem * BaseWindow::itemToGraphic(const uint16_t &itemid, const uint32_t &quantity)
{
    QListWidgetItem *item=new QListWidgetItem();
    item->setData(99,itemid);
    if(DatapackClientLoader::datapackLoader.itemsExtra.find(itemid)!=
            DatapackClientLoader::datapackLoader.itemsExtra.cend())
    {
        item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.at(itemid).image);
        if(quantity>1)
            item->setText(QString::number(quantity));
        item->setToolTip(QString::fromStdString(DatapackClientLoader::datapackLoader.itemsExtra.at(itemid).name));
    }
    else
    {
        item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        if(quantity>1)
            item->setText(QStringLiteral("id: %1 (x%2)").arg(itemid).arg(quantity));
        else
            item->setText(QStringLiteral("id: %1").arg(itemid));
    }
    return item;
}

void BaseWindow::updateTheWareHouseContent()
{
    if(!haveInventory || !datapackIsParsed)
        return;
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();

    //inventory
    {
        ui->warehousePlayerInventory->clear();
        auto i=playerInformations.items.begin();
        while(i!=playerInformations.items.cend())
        {
            int32_t quantity=i->second;
            if(change_warehouse_items.find(i->first)!=change_warehouse_items.cend())
                quantity+=change_warehouse_items.at(i->first);
            if(quantity>0)
                ui->warehousePlayerInventory->addItem(itemToGraphic(i->first,quantity));
            ++i;
        }
        for (const auto &j : change_warehouse_items) {
            if(playerInformations.items.find(j.first)==playerInformations.items.cend() && j.second>0)
                ui->warehousePlayerInventory->addItem(itemToGraphic(j.first,j.second));
        }
    }

    qDebug() << QStringLiteral("ui->warehousePlayerInventory icon size: %1x%2").arg(ui->warehousePlayerInventory->iconSize().width()).arg(ui->warehousePlayerInventory->iconSize().height());

    //inventory warehouse
    {
        ui->warehousePlayerStoredInventory->clear();
        auto i=playerInformations.warehouse_items.begin();
        while(i!=playerInformations.warehouse_items.cend())
        {
            int32_t quantity=i->second;
            if(change_warehouse_items.find(i->first)!=change_warehouse_items.cend())
                quantity-=change_warehouse_items.at(i->first);
            if(quantity>0)
                ui->warehousePlayerStoredInventory->addItem(itemToGraphic(i->first,quantity));
            ++i;
        }
        for (const auto &j : change_warehouse_items) {
            if(playerInformations.warehouse_items.find(j.first)==playerInformations.warehouse_items.cend() && j.second<0)
                ui->warehousePlayerStoredInventory->addItem(itemToGraphic(j.first,-j.second));
        }
    }

    //cash
    ui->warehousePlayerCash->setText(tr("Cash: %1").arg(playerInformations.cash+temp_warehouse_cash));
    ui->warehousePlayerStoredCash->setText(tr("Cash: %1").arg(playerInformations.warehouse_cash-temp_warehouse_cash));

    //do before because the dispatch put into random of it
    ui->warehousePlayerStoredMonster->clear();
    ui->warehousePlayerMonster->clear();

    //monster
    {
        const std::vector<PlayerMonster> &playerMonster=fightEngine.getPlayerMonster();
        unsigned int index=0;
        while(index<playerMonster.size())
        {
            const PlayerMonster &monster=playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
            {
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(tr("%1, level: %2")
                        .arg(QString::fromStdString(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).name))
                        .arg(monster.level)
                        );
                item->setToolTip(QString::fromStdString(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).description));
                item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).front);
                //item->setData(99,monster.id);
                if(!vectorcontainsAtLeastOne(monster_to_deposit,index) || vectorcontainsAtLeastOne(monster_to_withdraw,index))
                    ui->warehousePlayerMonster->addItem(item);
                else
                    ui->warehousePlayerStoredMonster->addItem(item);
            }
            index++;
        }
    }

    //monster warehouse
    {
        unsigned int index=0;
        while(index<playerInformations.warehouse_playerMonster.size())
        {
            const PlayerMonster &monster=playerInformations.warehouse_playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
            {
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(tr("%1, level: %2")
                        .arg(QString::fromStdString(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).name))
                        .arg(monster.level)
                        );
                item->setToolTip(QString::fromStdString(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).description));
                item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).front);
                //item->setData(99,monster.id);
                if(!vectorcontainsAtLeastOne(monster_to_withdraw,index) || vectorcontainsAtLeastOne(monster_to_deposit,index))
                    ui->warehousePlayerStoredMonster->addItem(item);
                else
                    ui->warehousePlayerMonster->addItem(item);
            }
            index++;
        }
    }

    //set the button enabled
    ui->warehouseWithdrawCash->setEnabled((playerInformations.warehouse_cash-temp_warehouse_cash)>0);
    ui->warehouseDepositCash->setEnabled((playerInformations.cash+temp_warehouse_cash)>0);
    ui->warehouseDepositItem->setEnabled(ui->warehousePlayerInventory->count()>0);
    ui->warehouseWithdrawItem->setEnabled(ui->warehousePlayerStoredInventory->count()>0);
    ui->warehouseDepositMonster->setEnabled(ui->warehousePlayerMonster->count()>1);
    ui->warehouseWithdrawMonster->setEnabled(ui->warehousePlayerStoredMonster->count()>0);
}

void BaseWindow::cityCapture(const uint32_t &remainingTime,const uint8_t &type)
{
    if(remainingTime==0)
    {
        nextCityCatchTimer.stop();
        std::cout << "City capture disabled" << std::endl;
        return;//disabled
    }
    nextCityCatchTimer.start(remainingTime*1000);
    nextCatch=QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()+remainingTime*1000);
    city.capture.frenquency=(City::Capture::Frequency)type;
    city.capture.day=(City::Capture::Day)QDateTime::currentDateTime().addSecs(remainingTime).date().dayOfWeek();
    city.capture.hour=static_cast<uint8_t>(QDateTime::currentDateTime().addSecs(remainingTime).time().hour());
    city.capture.minute=static_cast<uint8_t>(QDateTime::currentDateTime().addSecs(remainingTime).time().minute());
}

void BaseWindow::animationFinished()
{
    if(animationWidget!=NULL)
    {
        delete animationWidget;
        animationWidget=NULL;
    }
    if(qQuickViewContainer!=NULL)
    {
        delete qQuickViewContainer;
        qQuickViewContainer=NULL;
    }
    if(previousAnimationWidget==ui->page_crafting)
    {
        ui->stackedWidget->setCurrentWidget(previousAnimationWidget);
        craftingAnimationObject->deleteLater();
        craftingAnimationObject=NULL;
    }
    else if(previousAnimationWidget==ui->page_map)
    {
        ui->stackedWidget->setCurrentWidget(previousAnimationWidget);
        if(baseMonsterEvolution!=NULL)
        {
            delete baseMonsterEvolution;
            baseMonsterEvolution=NULL;
        }
        if(targetMonsterEvolution!=NULL)
        {
            delete targetMonsterEvolution;
            targetMonsterEvolution=NULL;
        }
        fightEngine.confirmEvolutionByPosition(monsterEvolutionPostion);
        monsterEvolutionPostion=0;
        load_monsters();
    }
    else
        qDebug() << "Unknown animation quit";
}

void BaseWindow::evolutionCanceled()
{
    if(animationWidget!=NULL)
    {
        delete animationWidget;
        animationWidget=NULL;
    }
    if(qQuickViewContainer!=NULL)
    {
        delete qQuickViewContainer;
        qQuickViewContainer=NULL;
    }
    ui->stackedWidget->setCurrentWidget(previousAnimationWidget);
    if(baseMonsterEvolution!=NULL)
    {
        delete baseMonsterEvolution;
        baseMonsterEvolution=NULL;
    }
    if(targetMonsterEvolution!=NULL)
    {
        delete targetMonsterEvolution;
        targetMonsterEvolution=NULL;
    }
    if(evolutionControl!=NULL)
    {
        delete evolutionControl;
        evolutionControl=NULL;
    }
    monsterEvolutionPostion=0;
    checkEvolution();
}

/*void BaseWindow::customMessageHandler(QtMsgType type, const QMessageLogContext &context, const std::string &msg)
{
    Q_UNUSED(context);

    //std::string dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    //std::string txt = QString("[%1] ").arg(dt);
    std::string txt;

    switch (type)
    {
        default:
        case QtDebugMsg:
            txt = QStringLiteral("%1\n").arg(msg);
        break;
        case QtWarningMsg:
            txt = QStringLiteral("[Warning] %1\n").arg(msg);
        break;
        case QtCriticalMsg:
            txt = QStringLiteral("[Critical] %1\n").arg(msg);
        break;
        case QtFatalMsg:
            txt = QStringLiteral("[Fatal] %1\n").arg(msg);
            std::cout << txt.toUtf8().data();
        break;
    }
    std::cout << msg.toUtf8().data() << std::endl;

    if(BaseWindow::debugFileStatus==0)
    {
        if(!debugFile.isOpen())
        {
            QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
            debugFile.setFileName(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/LogFile.log");
            if(!debugFile.open(QIODevice::WriteOnly))
            {
                qDebug() << debugFile.errorString();
                BaseWindow::debugFileStatus=2;
                std::cout << static_cast<const char *>(msg.toLocal8Bit().constData()) << std::endl;
                return;
            }
            debugFile.resize(0);
        }
        BaseWindow::debugFileStatus=1;
    }
    if(debugFile.isOpen())
    {
        debugFile.write(txt.toUtf8());
        debugFile.flush();
    }
}*/

void CatchChallenger::BaseWindow::on_toolButtonEncyclopedia_clicked()
{
    on_checkBoxEncyclopediaItemKnown_toggled(ui->checkBoxEncyclopediaItemKnown->isChecked());
    on_checkBoxEncyclopediaMonsterKnown_toggled(ui->checkBoxEncyclopediaMonsterKnown->isChecked());

    ui->tabWidgetEncyclopedia->setCurrentIndex(0);
    ui->stackedWidget->setCurrentWidget(ui->page_encyclopedia);
}

void CatchChallenger::BaseWindow::on_listWidgetEncyclopediaMonster_itemActivated(QListWidgetItem *item)
{
    if(item==NULL || item->data(99)==0)
    {
        ui->labelEncyclopediaMonster->setText("");
        return;
    }

    const Monster &monsterCommon=CommonDatapack::commonDatapack.monsters.at(static_cast<uint16_t>(item->data(99).toUInt()));
    const DatapackClientLoader::MonsterExtra &monsterExtra=
            DatapackClientLoader::datapackLoader.monsterExtra.at(static_cast<uint16_t>(item->data(99).toUInt()));
    ui->labelEncyclopediaMonster->setText("");
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    QPixmap image=monsterExtra.front;
    image.scaled(image.width()*3,image.height()*3).save(&buffer, "PNG");
    ui->labelEncyclopediaMonster->setText(
                QStringLiteral("<center><img src=\"data:image/png;base64,%1\" /></center><br />").arg(QString(byteArray.toBase64()))+
                tr("Name: <b>%1</b><br />Description: %2")
                .arg(QString::fromStdString(monsterExtra.name))
                .arg(QString::fromStdString(monsterExtra.description))
                );
    if(!monsterExtra.kind.empty())
        ui->labelEncyclopediaMonster->setText(ui->labelEncyclopediaMonster->text()+"<br />"+tr("Kind: %1").arg(QString::fromStdString(monsterExtra.kind)));
    if(!monsterExtra.habitat.empty())
        ui->labelEncyclopediaMonster->setText(ui->labelEncyclopediaMonster->text()+"<br />"+tr("Habitat: %1").arg(QString::fromStdString(monsterExtra.habitat)));
    if(monsterCommon.ratio_gender<0 || monsterCommon.ratio_gender>100)
        ui->labelEncyclopediaMonster->setText(ui->labelEncyclopediaMonster->text()+"<br />"+tr("Gender: Unknown"));
    else if(monsterCommon.ratio_gender==0)
        ui->labelEncyclopediaMonster->setText(ui->labelEncyclopediaMonster->text()+"<br />"+tr("Gender: <span style=\"color:#3068C2\">Male</span>"));
    else if(monsterCommon.ratio_gender==100)
        ui->labelEncyclopediaMonster->setText(ui->labelEncyclopediaMonster->text()+"<br />"+tr("Gender: <span style=\"color:#C25254\">Female</span>"));
    else
        ui->labelEncyclopediaMonster->setText(ui->labelEncyclopediaMonster->text()+"<br />"+tr("Gender: %1% <span style=\"color:#3068C2\">Male</span> and %2% <span style=\"color:#C25254\">Female</span>")
                                              .arg(100-monsterCommon.ratio_gender)
                                              .arg(monsterCommon.ratio_gender)
                                              );
    {
        const std::vector<uint8_t> &types=monsterCommon.type;
        if(!types.empty())
        {
            std::vector<std::string> typeList;
            unsigned int sub_index=0;
            while(sub_index<types.size())
            {
                const auto &typeSub=types.at(sub_index);
                if(DatapackClientLoader::datapackLoader.typeExtra.find(typeSub)!=DatapackClientLoader::datapackLoader.typeExtra.cend())
                {
                    const DatapackClientLoader::TypeExtra &typeExtra=DatapackClientLoader::datapackLoader.typeExtra.at(typeSub);
                    if(!typeExtra.name.empty())
                    {
                        if(typeExtra.color.isValid())
                            typeList.push_back("<span style=\"background-color:"+typeExtra.color.name().toStdString()+";\">"+typeExtra.name+"</span>");
                        else
                            typeList.push_back(typeExtra.name);
                    }
                }
                sub_index++;
            }
            ui->labelEncyclopediaMonster->setText(ui->labelEncyclopediaMonster->text()+"<br />"+tr("Type:")+QString::fromStdString(stringimplode(typeList,", ")));
        }
    }
}

void CatchChallenger::BaseWindow::on_listWidgetEncyclopediaItem_itemActivated(QListWidgetItem *item)
{
    if(item==NULL || item->data(99)==0)
    {
        ui->labelEncyclopediaItem->setText("");
        return;
    }

    const Item &itemCommon=CommonDatapack::commonDatapack.items.item.at(static_cast<uint16_t>(item->data(99).toUInt()));
    const DatapackClientLoader::ItemExtra &itemExtra=
            DatapackClientLoader::datapackLoader.itemsExtra.at(static_cast<uint16_t>(item->data(99).toUInt()));
    ui->labelEncyclopediaItem->setText("");
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    QPixmap image=itemExtra.image;
    image.scaled(image.width()*3,image.height()*3).save(&buffer, "PNG");
    image.save(&buffer, "PNG");
    ui->labelEncyclopediaItem->setText(
                QStringLiteral("<center><img src=\"data:image/png;base64,%1\" /></center><br />").arg(QString(byteArray.toBase64()))+
                tr("Name: <b>%1</b><br />Description: %2")
                .arg(QString::fromStdString(itemExtra.name))
                .arg(QString::fromStdString(itemExtra.description))
                );
    if(itemCommon.price>0)
        ui->labelEncyclopediaItem->setText(ui->labelEncyclopediaItem->text()+"<br />"+tr("Price: %1$").arg(itemCommon.price));
    else
        ui->labelEncyclopediaItem->setText(ui->labelEncyclopediaItem->text()+"<br />"+tr("Can't be sold"));
    if(!itemCommon.consumeAtUse)
        ui->labelEncyclopediaItem->setText(ui->labelEncyclopediaItem->text()+"<br />"+tr("<b>Infinity use</b>"));
}

void CatchChallenger::BaseWindow::on_listWidgetEncyclopediaMonster_itemChanged(QListWidgetItem *item)
{
    on_listWidgetEncyclopediaMonster_itemActivated(item);
}

void CatchChallenger::BaseWindow::on_listWidgetEncyclopediaItem_itemChanged(QListWidgetItem *item)
{
    on_listWidgetEncyclopediaItem_itemActivated(item);
}

void CatchChallenger::BaseWindow::on_listWidgetEncyclopediaItem_itemSelectionChanged()
{
    on_listWidgetEncyclopediaItem_itemActivated(ui->listWidgetEncyclopediaItem->currentItem());
}

void CatchChallenger::BaseWindow::on_listWidgetEncyclopediaMonster_itemSelectionChanged()
{
    on_listWidgetEncyclopediaMonster_itemActivated(ui->listWidgetEncyclopediaMonster->currentItem());
}
