#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../Options.h"
#include "../FacilityLibClient.h"
#include "../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../general/base/CommonSettingsServer.hpp"
#include "../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../Ultimate.h"

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../../../libqtcatchchallenger/ClientVariableAudio.hpp"
#include "../../../libqtcatchchallenger/Audio.hpp"
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
    QtDatapackClientLoader::datapackLoader->resetAll();
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
    if(client->public_and_private_informations.encyclopedia_item!=NULL)
    {
        delete client->public_and_private_informations.encyclopedia_item;
        client->public_and_private_informations.encyclopedia_item=NULL;
    }
    if(client->public_and_private_informations.encyclopedia_monster!=NULL)
    {
        delete client->public_and_private_informations.encyclopedia_monster;
        client->public_and_private_informations.encyclopedia_monster=NULL;
    }*/

    #ifndef CATCHCHALLENGER_NOAUDIO
    if(currentAmbiance.player!=NULL)
    {
        Audio::audio->removePlayer(currentAmbiance.player);
        currentAmbiance.player->stop();
        currentAmbiance.buffer->close();
        currentAmbiance.player->deleteLater();
        currentAmbiance.buffer->deleteLater();
        currentAmbiance.data->clear();//memleak but to prevent one crash
        currentAmbiance.player=NULL;
        currentAmbiance.buffer=NULL;
        currentAmbiance.data=NULL;
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

    if(client!=nullptr)
        client->resetAll();
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

void BaseWindow::setMultiPlayer(bool multiplayer, Api_protocol_Qt *client)
{
    this->multiplayer=multiplayer;
    this->client=client;
    if(client!=NULL)
        mapController->setDatapackPath(client->datapackPathBase(),client->mainDatapackCode());
    else
        abort();
    chat->setClient(client);
    chat->setMultiPlayer(multiplayer);
    ui->openToLan->setEnabled(true);
    ui->openToLan->setVisible(!multiplayer);
    chat->show();
    chat->raise();
    //client->setClient(client);
    /*if(!multiplayer)
        mapController->setOpenGl(true);*/
    //frame_main_display_right->setVisible(multiplayer);
    //ui->frame_main_display_interface_player->setVisible(multiplayer);//displayed when have the player connected (if have)

    if(!connect(static_cast<Api_protocol_Qt *>(client),&Api_protocol_Qt::QtnewError,  this,&BaseWindow::newError))
        abort();
    /*if(!connect(static_cast<Api_protocol_Qt *>(client),&Api_protocol_Qt::Qterror,     this,&BaseWindow::error))
        abort();*/
    if(!connect(static_cast<Api_protocol_Qt *>(client),&Api_protocol_Qt::Qtmessage,     this,&BaseWindow::message))
        abort();
    /*if(!connect(client,&ClientFightEngine::errorFightEngine,     this,&BaseWindow::stderror))
        abort();*/
    /*if(!connect(client,&CatchChallenger::Api_client_real::Qtrandom_seeds,client,&ClientFightEngine::newRandomNumber))
       abort();*/

    mapController->fightEngine=this->client;
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

    QDir finalDatapackFolder(QString::fromStdString(client->datapackPathBase()));
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here

    if(!entryList.empty() && settings.contains("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase())))
    {
        const QString str=settings.value("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase())).toString();
        const QByteArray &data=QByteArray::fromHex(str.toUtf8());
        client->sendDatapackContentBase(std::string(data.constData(),data.size()));
    }
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
            ui->label_connecting_status->setText(tr("Start the protocol..."));
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

    //client->public_and_private_informations.playerMonster=client->get_player_informations().playerMonster;
    //client->setVariableContent(client->get_player_informations());

    haveCharacterInformation=true;

    datapackGatewayProgression.clear();
    updateConnectingStatus();
}

void BaseWindow::sendDatapackContentMainSub()
{
    if(client==nullptr)
    {
        std::cerr << "sendDatapackContentMainSub(): client==nullptr" << std::endl;
        abort();
    }
    if(settings.contains("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain())) &&
            settings.contains("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub())))
    {
        const QString strmain=settings.value("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain())).toString();
        const QByteArray &datamain=QByteArray::fromHex(strmain.toUtf8());

        const QString strsub=settings.value("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub())).toString();
        const QByteArray &datasub=QByteArray::fromHex(strsub.toUtf8());

        client->sendDatapackContentMainSub(std::string(datamain.constData(),datamain.size()),
                std::string(datasub.constData(),datasub.size()));
    }
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
        for (const auto &n : QtDatapackClientLoader::datapackLoader->get_itemsExtra())
        {
            const uint16_t &itemId=n.first;
            const DatapackClientLoader::ItemExtra &itemsExtra=n.second;
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString::fromStdString(itemsExtra.name));
            item->setData(99,itemId);
            item->setIcon(QIcon(QtDatapackClientLoader::datapackLoader->getItemExtra(itemId).image));
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
                      QString(QByteArray(
                          CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),
                          static_cast<int>(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size())
                                  ).toHex())
                      );

    if(client!=NULL)
        emit parseDatapack(client->datapackPathBase());
}

void BaseWindow::haveDatapackMainSubCode()
{
    if(client==nullptr)
    {
        std::cerr << "sendDatapackContentMainSub(): client==nullptr" << std::endl;
        return;
    }
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
                      QString(QByteArray(
                          CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data(),
                          static_cast<int>(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size())
                                  ).toHex())
                      );
    settings.setValue("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub()),
                      QString(QByteArray(
                          CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data(),
                          static_cast<int>(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size())
                                  ).toHex())
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
    (void)items;
    (void)warehouse_items;
    //const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
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
                    if(QtDatapackClientLoader::datapackLoader->get_itemToPlants().find(i->first)!=QtDatapackClientLoader::datapackLoader->get_itemToPlants().cend())
                        show=true;
                break;
                case ObjectType_UseInFight:
                    if(client->isInFightWithWild() && CommonDatapack::commonDatapack.get_items().trap.find(i->first)!=CommonDatapack::commonDatapack.get_items().trap.cend())
                        show=true;
                    else if(CommonDatapack::commonDatapack.get_items().monsterItemEffect.find(i->first)!=CommonDatapack::commonDatapack.get_items().monsterItemEffect.cend())
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
            if(QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(i->first)!=QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend())
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->getItemExtra(i->first).image);
                if(i->second>1)
                    item->setText(QString::number(i->second));
                item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(i->first).name));
            }
            else
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
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
    if(client==nullptr)
    {
        std::cerr << "client==nullptr into BaseWindow::datapackParsed() (abort)" << std::endl;
        //just ignore because caused by:
        //Datapack on mirror is corrupted: The checksum sended by the server is not the same than have on the mirror
        return;
    }
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
    if(!client->setMapNumber(QtDatapackClientLoader::datapackLoader->get_mapToId().size()))
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
    const QStringList &outputDeviceNames=Audio::audio->output_list();
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
        if(event.first>=CommonDatapack::commonDatapack.get_events().size())
        {
            emit error("BaseWindow::setEvents() event out of range");
            break;
        }
        if(event.second>=CommonDatapack::commonDatapack.get_events().at(event.first).values.size())
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
        while((uint32_t)events.size()<CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
            events.push_back(0);
    }
    if((uint32_t)events.size()>CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
    {
        while((uint32_t)events.size()>CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
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
        if(Ultimate::ultimate.isUltimate())
            ui->label_ultimate->setVisible(false);
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
        if(i->first>=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().size())
        {
            error(QStringLiteral("The reputation: %1 don't exist").arg(i->first).toStdString());
            return false;
        }
        if(i->second.level>=0)
        {
            if(i->second.level>=(int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_positive.size())
            {
                error(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 > max level: %3)")
                      .arg(i->first).arg(i->second.level)
                      .arg(CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first)
                           .reputation_positive.size()).toStdString());
                return false;
            }
        }
        else
        {
            if((-i->second.level)>(int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_negative.size())
            {
                error(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 < max level: %3)")
                      .arg(i->first).arg(i->second.level)
                      .arg(CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first)
                           .reputation_negative.size()).toStdString());
                return false;
            }
        }
        if(i->second.point>0)
        {
            if((int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_positive.size()==i->second.level)//start at level 0 in positive
            {
                emit message("The reputation level is already at max, drop point");
                return false;
            }
            if(i->second.point>=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_positive.at(i->second.level+1))//start at level 0 in positive
            {
                error(QStringLiteral("The reputation point %1 is greater than max %2")
                      .arg(i->second.point)
                      .arg(CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first)
                           .reputation_positive.at(i->second.level)).toStdString());
                return false;
            }
        }
        else if(i->second.point<0)
        {
            if((int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_negative.size()==-i->second.level)//start at level -1 in negative
            {
                error("The reputation level is already at min, drop point");
                return false;
            }
            if(i->second.point<CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_negative.at(-i->second.level))//start at level -1 in negative
            {
                error(QStringLiteral("The reputation point %1 is greater than max %2")
                      .arg(i->second.point)
                      .arg(CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first)
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
            if((i->second.level+1)==(int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_positive.size())
                html+=QStringLiteral("<li>100% %1</li>")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(
                             CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).name
                             ).reputation_positive.back())).toStdString();
            else
            {
                int32_t next_level_xp=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_positive.at(i->second.level+1);
                if(next_level_xp==0)
                {
                    error("Next level can't be need 0 xp");
                    return;
                }
                std::string text=QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(
                            CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).name
                            ).reputation_positive.at(i->second.level);
                html+=QStringLiteral("<li>%1% %2</li>").arg((i->second.point*100)/next_level_xp).arg(QString::fromStdString(text)).toStdString();
            }
        }
        else
        {
            if((-i->second.level)==(int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_negative.size())
                html+=QStringLiteral("<li>100% %1</li>")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(
                             CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).name
                             ).reputation_negative.back())).toStdString();
            else
            {
                int32_t next_level_xp=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_negative.at(-i->second.level);
                if(next_level_xp==0)
                {
                    error("Next level can't be need 0 xp");
                    return;
                }
                std::string text=QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(
                            CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).name
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
    if(skinId<(uint32_t)QtDatapackClientLoader::datapackLoader->get_skins().size())
        return getFrontSkin(QtDatapackClientLoader::datapackLoader->get_skins().at(skinId));
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
    const std::vector<std::string> &skinFolderList=QtDatapackClientLoader::datapackLoader->get_skins();
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
    const std::vector<std::string> &skinFolderList=QtDatapackClientLoader::datapackLoader->get_skins();
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
        if(QtDatapackClientLoader::datapackLoader->get_questsExtra().find(i->first)!=QtDatapackClientLoader::datapackLoader->get_questsExtra().cend())
        {
            if(i->second.step>0)
            {
                QListWidgetItem * item=new QListWidgetItem(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_questsExtra().at(i->first).name));
                quests_to_id_graphical[item]=i->first;
                ui->questsList->addItem(item);
            }
            if(i->second.step==0 || i->second.finish_one_time)
            {
                if(Ultimate::ultimate.isUltimate())
                {
                    html+="<li>";
                    if(CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(i->first).repeatable)
                        html+=imagesInterfaceRepeatableString;
                    if(i->second.step>0)
                        html+=imagesInterfaceInProgressString;
                    html+=QtDatapackClientLoader::datapackLoader->get_questsExtra().at(i->first).name+"</li>";
                }
                else
                    html+="<li>"+QtDatapackClientLoader::datapackLoader->get_questsExtra().at(i->first).name+"</li>";
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
    const DatapackClientLoader::QuestExtra &questExtra=QtDatapackClientLoader::datapackLoader->get_questsExtra().at(questId);
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
        std::vector<Quest::Item> items=CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId)
                .steps.at(playerInformations.quests.at(questId).step-1).requirements.items;
        std::vector<std::string> objects;
        unsigned int index=0;
        while(index<items.size())
        {
            QPixmap image;
            std::string name;
            if(QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(items.at(index).item)!=QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend())
            {
                image=QtDatapackClientLoader::datapackLoader->getItemExtra(items.at(index).item).image;
                name=QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(items.at(index).item).name;
            }
            else
            {
                image=QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
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
            std::vector<Quest::Item> items=CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId).rewards.items;
            std::vector<std::string> objects;
            unsigned int index=0;
            while(index<items.size())
            {
                QPixmap image;
                std::string name;
                if(QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(items.at(index).item)!=
                        QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend())
                {
                    image=QtDatapackClientLoader::datapackLoader->getItemExtra(items.at(index).item).image;
                    name=QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(items.at(index).item).name;
                }
                else
                {
                    image=QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
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
            std::vector<ReputationRewards> reputationRewards=CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId).rewards.reputation;
            std::vector<std::string> reputations;
            unsigned int index=0;
            while(index<reputationRewards.size())
            {
                if(QtDatapackClientLoader::datapackLoader->get_reputationExtra().find(
                            CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(
                                                       reputationRewards.at(index).reputationId).name
                            )!=
                        QtDatapackClientLoader::datapackLoader->get_reputationExtra().cend())
                {
                    const std::string &reputationName=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(
                                reputationRewards.at(index).reputationId).name;
                    if(reputationRewards.at(index).point<0)
                        reputations.push_back(tr("Less reputation for %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(reputationName).name)).toStdString());
                    if(reputationRewards.at(index).point>0)
                        reputations.push_back(tr("More reputation for %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(reputationName).name)).toStdString());
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
            std::vector<ActionAllow> allowRewards=CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId).rewards.allow;
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
    if(QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(itemid)!=
            QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend())
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->getItemExtra(itemid).image);
        if(quantity>1)
            item->setText(QString::number(quantity));
        item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(itemid).name));
    }
    else
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        if(quantity>1)
            item->setText(QStringLiteral("id: %1 (x%2)").arg(itemid).arg(quantity));
        else
            item->setText(QStringLiteral("id: %1").arg(itemid));
    }
    return item;
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
        client->confirmEvolutionByPosition(monsterEvolutionPostion);
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

    const Monster &monsterCommon=CommonDatapack::commonDatapack.get_monsters().at(static_cast<uint16_t>(item->data(99).toUInt()));
    const DatapackClientLoader::MonsterExtra &monsterExtra=
            QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(static_cast<uint16_t>(item->data(99).toUInt()));
    ui->labelEncyclopediaMonster->setText("");
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    QPixmap image=QtDatapackClientLoader::datapackLoader->getMonsterExtra(static_cast<uint16_t>(item->data(99).toUInt())).front;
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
                if(QtDatapackClientLoader::datapackLoader->get_typeExtra().find(typeSub)!=QtDatapackClientLoader::datapackLoader->get_typeExtra().cend())
                {
                    const DatapackClientLoader::TypeExtra &typeExtra=QtDatapackClientLoader::datapackLoader->get_typeExtra().at(typeSub);
                    if(!typeExtra.name.empty())
                    {
                        std::vector<char> data;
                        data.push_back(typeExtra.color.r);
                        data.push_back(typeExtra.color.g);
                        data.push_back(typeExtra.color.b);
                        //if(typeExtra.color.isValid())
                            typeList.push_back("<span style=\"background-color:#"+binarytoHexa(data)+";\">"+typeExtra.name+"</span>");
                        /*else
                            typeList.push_back(typeExtra.name);*/
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

    const Item &itemCommon=CommonDatapack::commonDatapack.get_items().item.at(static_cast<uint16_t>(item->data(99).toUInt()));
    const DatapackClientLoader::ItemExtra &itemExtra=
            QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(static_cast<uint16_t>(item->data(99).toUInt()));
    ui->labelEncyclopediaItem->setText("");
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    QPixmap image=QtDatapackClientLoader::datapackLoader->getItemExtra(static_cast<uint16_t>(item->data(99).toUInt())).image;
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
