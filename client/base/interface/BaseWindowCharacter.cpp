#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/base/FacilityLibGeneral.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "../FacilityLibClient.h"
#include "NewGame.h"

using namespace CatchChallenger;

void BaseWindow::on_character_add_clicked()
{
    if((characterEntryListInWaiting.size()+characterListForSelection.at(serverOrdenedList.at(serverSelected)->charactersGroupIndex).size())>=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        QMessageBox::warning(this,tr("Error"),tr("You have already the max characters count"));
        return;
    }
    if(newProfile!=NULL)
        delete newProfile;
    newProfile=new NewProfile(client->datapackPathBase(),this);
    connect(newProfile,&NewProfile::finished,this,&BaseWindow::newProfileFinished);
    newProfile->show();
}

void BaseWindow::newProfileFinished()
{
    const QString &datapackPath=client->datapackPathBase();
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.size()>1)
        if(!newProfile->ok())
        {
            if(characterListForSelection.at(serverOrdenedList.at(serverSelected)->charactersGroupIndex).isEmpty() && CommonSettingsCommon::commonSettingsCommon.min_character>0)
                client->tryDisconnect();
            return;
        }
    unsigned int profileIndex=0;
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.size()>1)
        profileIndex=newProfile->getProfileIndex();
    if(profileIndex>=CatchChallenger::CommonDatapack::commonDatapack.profileList.size())
        return;
    Profile profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
    newProfile->deleteLater();
    newProfile=NULL;
    NewGame nameGame(datapackPath+DATAPACK_BASE_PATH_SKIN,datapackPath+DATAPACK_BASE_PATH_MONSTERS,profile.monstergroup,profile.forcedskin,this);
    if(!nameGame.haveSkin())
    {
        if(characterListForSelection.at(serverOrdenedList.at(serverSelected)->charactersGroupIndex).isEmpty() && CommonSettingsCommon::commonSettingsCommon.min_character>0)
            client->tryDisconnect();
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Sorry but no skin found into: %1").arg(QFileInfo(datapackPath+DATAPACK_BASE_PATH_SKIN).absoluteFilePath()));
        return;
    }
    nameGame.exec();
    if(!nameGame.haveTheInformation())
    {
        if(characterListForSelection.at(serverOrdenedList.at(serverSelected)->charactersGroupIndex).isEmpty() && CommonSettingsCommon::commonSettingsCommon.min_character>0)
            client->tryDisconnect();
        return;
    }
    CharacterEntry characterEntry;
    characterEntry.character_id=0;
    characterEntry.delete_time_left=0;
    characterEntry.last_connect=QDateTime::currentMSecsSinceEpoch()/1000;
    //characterEntry.mapId=DatapackClientLoader::datapackLoader.mapToId.value(profile.map);
    characterEntry.played_time=0;
    characterEntry.pseudo=nameGame.pseudo().toStdString();
    if(characterEntry.pseudo.find(" ")!=std::string::npos)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your psuedo can't contains space"));
        client->tryDisconnect();
        return;
    }
    characterEntry.charactersGroupIndex=nameGame.monsterGroupId();
    characterEntry.skinId=nameGame.skinId();
    client->addCharacter(serverOrdenedList.at(serverSelected)->charactersGroupIndex,
                         static_cast<uint8_t>(profileIndex),QString::fromStdString(characterEntry.pseudo),
                         characterEntry.charactersGroupIndex,characterEntry.skinId);
    characterEntryListInWaiting << characterEntry;
    if((characterEntryListInWaiting.size()+characterListForSelection.at(serverOrdenedList.at(serverSelected)->charactersGroupIndex).size())>=CommonSettingsCommon::commonSettingsCommon.max_character)
        ui->character_add->setEnabled(false);
    ui->stackedWidget->setCurrentWidget(ui->page_init);
    ui->label_connecting_status->setText(tr("Creating your new character"));
}

void BaseWindow::newCharacterId(const uint8_t &returnCode, const uint32_t &characterId)
{
    CharacterEntry characterEntry=characterEntryListInWaiting.first();
    characterEntryListInWaiting.removeFirst();
    if(returnCode==0x00)
    {
        characterEntry.character_id=characterId;
        characterListForSelection[serverOrdenedList.at(serverSelected)->charactersGroupIndex] << characterEntry;
        updateCharacterList();
        ui->characterEntryList->item(ui->characterEntryList->count()-1)->setSelected(true);
        //if(characterEntryList.size().size()>=CommonSettings::commonSettings.min_character && characterEntryList.size().size()<=CommonSettings::commonSettings.max_character)
            on_character_select_clicked();
    /*    else
            ui->stackedWidget->setCurrentWidget(ui->page_character);*/
    }
    else
    {
        if(returnCode==0x01)
            QMessageBox::warning(this,tr("Error"),tr("This pseudo is already taken"));
        else if(returnCode==0x02)
            QMessageBox::warning(this,tr("Error"),tr("Have already the max caraters taken"));
        else
            QMessageBox::warning(this,tr("Error"),tr("Unable to create the character"));
        on_character_back_clicked();
    }
}

void BaseWindow::updateCharacterList()
{
    ui->characterEntryList->clear();
    int index=0;
    while(index<characterListForSelection.at(serverOrdenedList.at(serverSelected)->charactersGroupIndex).size())
    {
        const CharacterEntry &characterEntry=characterListForSelection.at(serverOrdenedList.at(serverSelected)->charactersGroupIndex).at(index);
        QListWidgetItem * item=new QListWidgetItem();
        item->setData(99,characterEntry.character_id);
        item->setData(98,characterEntry.delete_time_left);
        //item->setData(97,characterEntry.mapId);
        QString text=QString::fromStdString(characterEntry.pseudo+"\n");
        if(characterEntry.played_time>0)
            text+=tr("%1 played").arg(FacilityLibClient::timeToString(characterEntry.played_time));
        else
            text+=tr("Never played");
        if(characterEntry.delete_time_left>0)
            text+="\n"+tr("%1 to be deleted").arg(FacilityLibClient::timeToString(characterEntry.delete_time_left));
        /*if(characterEntry.mapId==-1)
            text+="\n"+tr("Map missing, can't play");*/
        item->setText(text);
        if(characterEntry.skinId<DatapackClientLoader::datapackLoader.skins.size())
            item->setIcon(QIcon(client->datapackPathBase()+DATAPACK_BASE_PATH_SKIN+DatapackClientLoader::datapackLoader.skins.at(characterEntry.skinId)+"/front.png"));
        else
            item->setIcon(QIcon(QStringLiteral(":/images/player_default/front.png")));
        ui->characterEntryList->addItem(item);
        index++;
    }
}

void BaseWindow::on_character_back_clicked()
{
    if(multiplayer)
        ui->stackedWidget->setCurrentWidget(ui->page_serverList);
    else
        client->tryDisconnect();
}

void BaseWindow::on_character_select_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->characterEntryList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    on_characterEntryList_itemDoubleClicked(selectedItems.first());
}

void BaseWindow::on_character_remove_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->characterEntryList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const uint32_t &character_id=selectedItems.first()->data(99).toUInt();
    const uint32_t &delete_time_left=selectedItems.first()->data(98).toUInt();
    if(delete_time_left>0)
    {
        QMessageBox::warning(this,tr("Error"),tr("Deleting already planned"));
        return;
    }
    client->removeCharacter(serverOrdenedList.at(serverSelected)->charactersGroupIndex,character_id);
    int index=0;
    while(index<characterListForSelection.at(serverOrdenedList.at(serverSelected)->charactersGroupIndex).size())
    {
        const CharacterEntry &characterEntry=characterListForSelection.at(serverOrdenedList.at(serverSelected)->charactersGroupIndex).at(index);
        if(characterEntry.character_id==character_id)
        {
            characterListForSelection[serverOrdenedList.at(serverSelected)->charactersGroupIndex][index].delete_time_left=CommonSettingsCommon::commonSettingsCommon.character_delete_time;
            break;
        }
        index++;
    }
    updateCharacterList();
    QMessageBox::information(this,tr("Information"),tr("Your charater will be deleted into %1")
                             .arg(FacilityLibClient::timeToString(CommonSettingsCommon::commonSettingsCommon.character_delete_time))
                             );
}

void BaseWindow::on_characterEntryList_itemDoubleClicked(QListWidgetItem *item)
{
    /*if(item->data(97).toInt()==-1)
    {
        QMessageBox::warning(this,tr("Error"),tr("You can't play with this buggy charater"));
        return;
    }*/
    client->selectCharacter(serverOrdenedList.at(serverSelected)->charactersGroupIndex,serverOrdenedList.at(serverSelected)->uniqueKey,item->data(99).toUInt(),serverSelected);
    ui->stackedWidget->setCurrentWidget(ui->page_init);
    ui->label_connecting_status->setText(tr("Selecting your character"));
}


void BaseWindow::updateServerList()
{
    //do the grouping for characterGroup count
    {
        serverByCharacterGroup.clear();
        int index=0;
        int serverByCharacterGroupTempIndexToDisplay=1;
        while(index<serverOrdenedList.size())
        {
            const ServerFromPoolForDisplay &server=*serverOrdenedList.at(index);
            if(serverByCharacterGroup.contains(server.charactersGroupIndex))
                serverByCharacterGroup[server.charactersGroupIndex].first++;
            else
            {
                serverByCharacterGroup[server.charactersGroupIndex].first=1;
                serverByCharacterGroup[server.charactersGroupIndex].second=static_cast<uint8_t>(serverByCharacterGroupTempIndexToDisplay);
                serverByCharacterGroupTempIndexToDisplay++;
            }
            index++;
        }
    }

    //clear and determine what kind of view
    ui->serverList->clear();
    LogicialGroup logicialGroup=client->getLogicialGroup();
    bool fullView=true;
    if(serverOrdenedList.size()>10)
        fullView=false;
    const uint64_t &current__date=QDateTime::currentDateTime().toTime_t();

    //reload, bug if before init
    if(icon_server_list_star1.isNull())
    {
        BaseWindow::icon_server_list_star1=QIcon(":/images/interface/server_list/star1.png");
        if(BaseWindow::icon_server_list_star1.isNull())
            abort();
        BaseWindow::icon_server_list_star2=QIcon(":/images/interface/server_list/star2.png");
        BaseWindow::icon_server_list_star3=QIcon(":/images/interface/server_list/star3.png");
        BaseWindow::icon_server_list_star4=QIcon(":/images/interface/server_list/star4.png");
        BaseWindow::icon_server_list_star5=QIcon(":/images/interface/server_list/star5.png");
        BaseWindow::icon_server_list_star6=QIcon(":/images/interface/server_list/star6.png");
        BaseWindow::icon_server_list_stat1=QIcon(":/images/interface/server_list/stat1.png");
        BaseWindow::icon_server_list_stat2=QIcon(":/images/interface/server_list/stat2.png");
        BaseWindow::icon_server_list_stat3=QIcon(":/images/interface/server_list/stat3.png");
        BaseWindow::icon_server_list_stat4=QIcon(":/images/interface/server_list/stat4.png");
        BaseWindow::icon_server_list_bug=QIcon(":/images/interface/server_list/bug.png");
        if(BaseWindow::icon_server_list_bug.isNull())
            abort();
        icon_server_list_color.push_back(QIcon(":/images/colorflags/0.png"));
        icon_server_list_color.push_back(QIcon(":/images/colorflags/1.png"));
        icon_server_list_color.push_back(QIcon(":/images/colorflags/2.png"));
        icon_server_list_color.push_back(QIcon(":/images/colorflags/3.png"));
    }
    //do the average value
    {
        averagePlayedTime=0;
        averageLastConnect=0;
        int entryCount=0;
        int index=0;
        while(index<serverOrdenedList.size())
        {
            const ServerFromPoolForDisplay &server=*serverOrdenedList.at(index);
            if(server.playedTime>0 && server.lastConnect<=current__date)
            {
                averagePlayedTime+=server.playedTime;
                averageLastConnect+=server.lastConnect;
                entryCount++;
            }
            index++;
        }
        if(entryCount>0)
        {
            averagePlayedTime/=entryCount;
            averageLastConnect/=entryCount;
        }
    }
    addToServerList(logicialGroup,ui->serverList->invisibleRootItem(),current__date,fullView);
    ui->serverList->expandAll();
}

bool ServerFromPoolForDisplay::operator<(const ServerFromPoolForDisplay &serverFromPoolForDisplay) const
{
    if(serverFromPoolForDisplay.uniqueKey<this->uniqueKey)
        return true;
    else
        return false;
}

void BaseWindow::addToServerList(LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView)
{
    item->setText(0,logicialGroup.name);
    {
        //to order the group
        QStringList keys=logicialGroup.logicialGroupList.keys();
        keys.sort();
        //list the group
        int index=0;
        while(index<keys.size())
        {
            QTreeWidgetItem * const itemGroup=new QTreeWidgetItem(item);
            addToServerList(logicialGroup.logicialGroupList[keys.value(index)],itemGroup,currentDate,fullView);
            index++;
        }
    }
    {
        qSort(logicialGroup.servers);
        //list the server
        int index=0;
        while(index<logicialGroup.servers.size())
        {
            const ServerFromPoolForDisplay &server=logicialGroup.servers.at(index);
            QTreeWidgetItem *itemServer=new QTreeWidgetItem(item);
            QString text;
            QString groupText;
            if(characterListForSelection.size()>1 && serverByCharacterGroup.size()>1)
            {
                const uint8_t groupInt=serverByCharacterGroup.value(server.charactersGroupIndex).second;
                if(groupInt>=icon_server_list_color.size())
                    groupText=QStringLiteral(" (%1)").arg(groupInt);
                if(!icon_server_list_color.isEmpty())
                    itemServer->setIcon(0,icon_server_list_color.at(groupInt%icon_server_list_color.size()));
            }
            QString name=server.name;
            if(name.isEmpty())
                name=tr("Default server");
            if(fullView)
            {
                text=name+groupText;
                if(server.playedTime>0)
                {
                    if(!server.description.isEmpty())
                        text+=" "+tr("%1 played").arg(FacilityLibClient::timeToString(server.playedTime));
                    else
                        text+="\n"+tr("%1 played").arg(FacilityLibClient::timeToString(server.playedTime));
                }
                if(!server.description.isEmpty())
                    text+="\n"+server.description;
            }
            else
            {
                if(server.description.isEmpty())
                    text=name+groupText;
                else
                    text=name+groupText+" - "+server.description;
            }
            itemServer->setText(0,text);

            //do the icon here
            if(server.playedTime>5*365*24*3600)
            {
                itemServer->setIcon(0,BaseWindow::icon_server_list_bug);
                itemServer->setToolTip(0,tr("Played time greater than 5y, bug?"));
            }
            else if(server.lastConnect>0 && server.lastConnect<1420070400)
            {
                itemServer->setIcon(0,BaseWindow::icon_server_list_bug);
                itemServer->setToolTip(0,tr("Played before 2015, bug?"));
            }
            else if(server.maxPlayer<=65533 && (server.maxPlayer<server.currentPlayer || server.maxPlayer==0))
            {
                itemServer->setIcon(0,BaseWindow::icon_server_list_bug);
                if(server.maxPlayer<server.currentPlayer)
                    itemServer->setToolTip(0,tr("maxPlayer<currentPlayer"));
                else
                    itemServer->setToolTip(0,tr("maxPlayer==0"));
            }
            else if(server.playedTime>0 || server.lastConnect>0)
            {
                uint64_t dateDiff=0;
                if(currentDate>server.lastConnect)
                    dateDiff=currentDate-server.lastConnect;
                if(server.playedTime>24*3600*31)
                {
                    if(dateDiff<24*3600)
                    {
                        itemServer->setIcon(0,BaseWindow::icon_server_list_star1);
                        itemServer->setToolTip(0,tr("Played time greater than 24h, last connect in this last 24h"));
                    }
                    else
                    {
                        itemServer->setIcon(0,BaseWindow::icon_server_list_star2);
                        itemServer->setToolTip(0,tr("Played time greater than 24h, last connect not in this last 24h"));
                    }
                }
                else if(server.lastConnect<averageLastConnect)
                {
                    if(server.playedTime<averagePlayedTime)
                    {
                        itemServer->setIcon(0,BaseWindow::icon_server_list_star3);
                        itemServer->setToolTip(0,tr("Into the more recent server used, out of the most used server"));
                    }
                    else
                    {
                        itemServer->setIcon(0,BaseWindow::icon_server_list_star4);
                        itemServer->setToolTip(0,tr("Into the more recent server used, into the most used server"));
                    }
                }
                else
                {
                    if(server.playedTime<averagePlayedTime)
                    {
                        itemServer->setIcon(0,BaseWindow::icon_server_list_star5);
                        itemServer->setToolTip(0,tr("Out of the more recent server used, out of the most used server"));
                    }
                    else
                    {
                        itemServer->setIcon(0,BaseWindow::icon_server_list_star6);
                        itemServer->setToolTip(0,tr("Out of the more recent server used, into the most used server"));
                    }
                }

            }
            if(server.maxPlayer<=65533)
            {
                //do server.currentPlayer/server.maxPlayer icon
                if(server.maxPlayer<=0 || server.currentPlayer>server.maxPlayer)
                    itemServer->setIcon(1,BaseWindow::icon_server_list_bug);
                else
                {
                    //to be very sure
                    if(server.maxPlayer>0)
                    {
                        int percent=(server.currentPlayer*100)/server.maxPlayer;
                        if(server.currentPlayer==server.maxPlayer || (server.maxPlayer>50 && percent>98))
                            itemServer->setIcon(1,BaseWindow::icon_server_list_stat4);
                        else if(server.currentPlayer>30 && percent>50)
                            itemServer->setIcon(1,BaseWindow::icon_server_list_stat3);
                        else if(server.currentPlayer>5 && percent>20)
                            itemServer->setIcon(1,BaseWindow::icon_server_list_stat2);
                        else
                            itemServer->setIcon(1,BaseWindow::icon_server_list_stat1);
                    }
                }
                itemServer->setText(1,QStringLiteral("%1/%2").arg(server.currentPlayer).arg(server.maxPlayer));
            }
            itemServer->setData(99,99,server.serverOrdenedListIndex);
            index++;
        }
    }
}

void BaseWindow::on_serverListBack_clicked()
{
    client->tryDisconnect();
}
