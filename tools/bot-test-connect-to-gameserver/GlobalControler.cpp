#include "GlobalControler.h"

GlobalControler::GlobalControler(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<std::vector<char> >("std::vector<char>");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");
    qRegisterMetaType<QList<QList<CatchChallenger::CharacterEntry> > >("QList<QList<CatchChallenger::CharacterEntry> >");
    qRegisterMetaType<QList<CatchChallenger::ServerFromPoolForDisplay*> >("QList<CatchChallenger::ServerFromPoolForDisplay*>");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::loggedDone,this,&GlobalControler::logged,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::datapackIsReady,this,&GlobalControler::datapackIsReady,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::datapackMainSubIsReady,this,&GlobalControler::datapackMainSubIsReady,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::statusError,this,&GlobalControler::statusError,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfSelectedCharacter,this,&GlobalControler::display_numberOfSelectedCharacter,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfBotConnected,this,&GlobalControler::display_numberOfBotConnected);//,Qt::QueuedConnection
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_detectSlowDown,this,&GlobalControler::detectSlowDown,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_all_player_connected,this,&GlobalControler::all_player_connected,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_all_player_on_map,this,&GlobalControler::all_player_on_map,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnection::emit_lastReplyTime,this,&GlobalControler::lastReplyTime,Qt::QueuedConnection);
    connect(&slowDownTimer,&QTimer::timeout,&multipleBotConnexion,&MultipleBotConnectionImplForGui::detectSlowDown,Qt::QueuedConnection);
    slowDownTimer.start(200);
    connect(&autoConnect,&QTimer::timeout,&multipleBotConnexion,&MultipleBotConnectionImplForGui::on_connect_clicked,Qt::QueuedConnection);
    autoConnect.setSingleShot(true);
    autoConnect.start(0);

    multipleBotConnexion.botInterface=NULL;

    if(settings.contains("login"))
        login=settings.value("login").toString();
    else
        settings.setValue("login","login");
    if(settings.contains("pass"))
        pass=settings.value("pass").toString();
    else
        settings.setValue("pass","pass");
    if(settings.contains("host"))
        host=settings.value("host").toString();
    else
        settings.setValue("host","localhost");
    if(settings.contains("port"))
        port=settings.value("port").toUInt();
    else
        settings.setValue("port","9999");
    if(settings.contains("proxy"))
        proxy=settings.value("proxy").toString();
    else
        settings.setValue("proxy","");
    if(settings.contains("proxyport"))
        proxyport=settings.value("proxyport").toUInt();
    else
        settings.setValue("proxyport","9999");

    if(settings.contains("charactersGroupIndex"))
        charactersGroupIndex=settings.value("charactersGroupIndex").toUInt();
    else
        settings.setValue("charactersGroupIndex","0");
    if(settings.contains("serverUniqueKey"))
        serverUniqueKey=settings.value("serverUniqueKey").toUInt();
    else
        settings.setValue("serverUniqueKey","9999");
    if(settings.contains("character"))
        character=settings.value("character").toUInt();
    else
        settings.setValue("character","botTest");
}

void GlobalControler::detectSlowDown(QString text)
{
//    ui->labelQueryList->setText(text);
}

void GlobalControler::logged(CatchChallenger::Api_client_real *senderObject,const QList<CatchChallenger::ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CatchChallenger::CharacterEntry> > &characterEntryList,bool haveTheDatapack)
{
    Q_UNUSED(haveTheDatapack);
    if(senderObject==NULL)
    {
        qDebug() << "GlobalControler::logged(): qobject_cast<CatchChallenger::Api_client_real *>(sender())==NULL";
        return;
    }

    this->serverOrdenedList=serverOrdenedList;
    this->characterEntryList=characterEntryList;
    if(characterEntryList.size()>=charactersGroupIndex)
    {
        QCoreApplication::exit(24);
        return;
    }
    updateServerList(senderObject);
    {
        int index=0;
        QList<CatchChallenger::CharacterEntry> characterEntryListNew=characterEntryList.at(charactersGroupIndex);
        while(index<characterEntryListNew.size())
        {
            const CatchChallenger::CharacterEntry &characterEntry=characterEntryListNew.at(index);
            if(characterEntry.pseudo==character)
            {
                multipleBotConnexion.characterSelectForFirstCharacter(characterEntry.character_id);
                return;
            }
            index++;
        }
        {
            QCoreApplication::exit(25);
            return;
        }
    }
}

void GlobalControler::updateServerList(CatchChallenger::Api_client_real *senderObject)
{
    //do the grouping for characterGroup count
    {
        serverByCharacterGroup.clear();
        int index=0;
        int serverByCharacterGroupTempIndexToDisplay=1;
        while(index<serverOrdenedList.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=*serverOrdenedList.at(index);
            if(serverByCharacterGroup.contains(server.charactersGroupIndex))
                serverByCharacterGroup[server.charactersGroupIndex].first++;
            else
            {
                serverByCharacterGroup[server.charactersGroupIndex].first=1;
                serverByCharacterGroup[server.charactersGroupIndex].second=serverByCharacterGroupTempIndexToDisplay;
                serverByCharacterGroupTempIndexToDisplay++;
            }
            index++;
        }
    }
    {
        int index=0;
        while(index<serverOrdenedList.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=*serverOrdenedList.at(index);
            if(server.charactersGroupIndex==charactersGroupIndex && server.uniqueKey==serverUniqueKey)
                return;
            index++;
        }
    }
    QCoreApplication::exit(23);
}

void GlobalControler::statusError(QString error)
{
    QCoreApplication::exit(26);
    ui->statusBar->showMessage(error);
}

void GlobalControler::on_connect_clicked()
{
    if(pass.size()<6)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your password need to be at minimum of 6 characters"));
        return;
    }
    if(login.size()<3)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your login need to be at minimum of 3 characters"));
        return;
    }

    multipleBotConnexion.mLogin=login;
    multipleBotConnexion.mPass=pass;
    multipleBotConnexion.mMultipleConnexion=false;
    multipleBotConnexion.mAutoCreateCharacter=false;
    multipleBotConnexion.mConnectBySeconds=9;
    multipleBotConnexion.mConnexionCountTarget=9;
    multipleBotConnexion.mMaxDiffConnectedSelected=9;
    multipleBotConnexion.mProxy=proxy;
    multipleBotConnexion.mProxyport=proxyport;
    multipleBotConnexion.mHost=host;
    multipleBotConnexion.mPort=port;

    //do only the first client to download the datapack
    multipleBotConnexion.createClient();
}

void GlobalControler::on_serverList_activated(const QModelIndex &index)
{
    Q_UNUSED(index);
    on_serverListSelect_clicked();
}

void GlobalControler::on_serverListSelect_clicked()
{
    const QList<QTreeWidgetItem *> &selectedItems=ui->serverList->selectedItems();
    if(selectedItems.size()!=1)
        return;

    const QTreeWidgetItem * const selectedItem=selectedItems.at(0);
    unsigned int serverSelectedIndex=selectedItem->data(99,99).toUInt();

    CatchChallenger::ServerFromPoolForDisplay * const serverSelected=serverOrdenedList.at(serverSelectedIndex);
    const uint8_t &charactersGroupIndex=serverSelected->charactersGroupIndex;
    if(charactersGroupIndex>=characterEntryList.size())
    {
        ui->groupBox_Server->setEnabled(false);
        QMessageBox::critical(this,tr("Error"),tr("The group index is wrong, maybe the server list have change"));
        return;
    }
    multipleBotConnexion.serverSelect(serverSelected->charactersGroupIndex,serverSelected->uniqueKey);

    ui->groupBox_Server->setEnabled(false);
    if(!ui->multipleConnexion->isChecked())
    {
        ui->groupBox_char->setEnabled(true);
        int index=0;
        while(index<characterEntryList.size())
        {
            const CatchChallenger::CharacterEntry &character=characterEntryList.at(charactersGroupIndex).at(index);
            ui->characterList->addItem(QString::fromStdString(character.pseudo),character.character_id);
            index++;
        }
    }
    else
    {
        ui->groupBox_char->setEnabled(false);
        if(characterEntryList.size()>0)
        {
            if(characterEntryList.at(charactersGroupIndex).empty())
            {
                qDebug() << "GlobalControler::on_serverListSelect_clicked(): ui->characterList->count()==0";
                return;
            }
            const CatchChallenger::CharacterEntry &character=characterEntryList.at(charactersGroupIndex).first();
            multipleBotConnexion.characterSelectForFirstCharacter(character.character_id);
        }
        else
        {
            qDebug() << "GlobalControler::on_serverListSelect_clicked(): ui->characterList->count()==0";
        }
    }
    ui->characterSelect->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());
}

void GlobalControler::addToServerList(CatchChallenger::LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView)
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
            const CatchChallenger::ServerFromPoolForDisplay &server=logicialGroup.servers.at(index);
            QTreeWidgetItem *itemServer=new QTreeWidgetItem(item);
            QString text;
            QString groupText;
            if(serverByCharacterGroup.size()>1)
                groupText=QStringLiteral(" (%1)").arg(serverByCharacterGroup.value(server.charactersGroupIndex).second);
            QString name=server.name;
            if(name.isEmpty())
                name=tr("Default server");
            if(fullView)
            {
                text=name+groupText;
                if(server.playedTime>0)
                {
                    if(!server.description.isEmpty())
                        text+=" "+tr("%1 played").arg(CatchChallenger::FacilityLibClient::timeToString(server.playedTime));
                    else
                        text+="\n"+tr("%1 played").arg(CatchChallenger::FacilityLibClient::timeToString(server.playedTime));
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
                itemServer->setToolTip(0,tr("Played time greater than 5y, bug?"));
            else if(server.lastConnect>0 && server.lastConnect<1420070400)
                itemServer->setToolTip(0,tr("Played before 2015, bug?"));
            else if(server.maxPlayer<=65533 && (server.maxPlayer<server.currentPlayer || server.maxPlayer==0))
            {
                if(server.maxPlayer<server.currentPlayer)
                    itemServer->setToolTip(0,tr("maxPlayer<currentPlayer"));
                else
                    itemServer->setToolTip(0,tr("maxPlayer==0"));
            }
            if(server.maxPlayer<=65533)
                itemServer->setText(1,QStringLiteral("%1/%2").arg(server.currentPlayer).arg(server.maxPlayer));
            itemServer->setData(99,99,server.serverOrdenedListIndex);
            index++;
        }
    }
}

void GlobalControler::datapackIsReady()
{
    ui->groupBox_Server->setEnabled(true);
    select the server
}

void GlobalControler::datapackMainSubIsReady()
{
}

void GlobalControler::on_autoCreateCharacter_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    settings.setValue("autoCreateCharacter",ui->autoCreateCharacter->isChecked());
}

void GlobalControler::all_player_connected()
{
    qDebug() << "GlobalControler::all_player_connected()";
}

void GlobalControler::all_player_on_map()
{
    qDebug() << "GlobalControler::all_player_on_map()";
    QCoreApplication::exit(0);
}

