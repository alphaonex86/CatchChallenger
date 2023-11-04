#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../bot/simple/SimpleAction.h"

#include <QNetworkProxy>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<std::vector<char> >("std::vector<char>");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");
    qRegisterMetaType<QList<QList<CatchChallenger::CharacterEntry> > >("QList<QList<CatchChallenger::CharacterEntry> >");
    qRegisterMetaType<QList<CatchChallenger::ServerFromPoolForDisplay*> >("QList<CatchChallenger::ServerFromPoolForDisplay*>");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");

    qRegisterMetaType<CatchChallenger::Player_type>("std::vector<CatchChallenger::ServerFromPoolForDisplay>");
    qRegisterMetaType<std::vector<std::vector<CatchChallenger::CharacterEntry> > >("std::vector<std::vector<CatchChallenger::CharacterEntry> >");

    ui->setupUi(this);
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::loggedDone,this,&MainWindow::logged,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::datapackIsReady,this,&MainWindow::datapackIsReady,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::datapackMainSubIsReady,this,&MainWindow::datapackMainSubIsReady,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::statusError,this,&MainWindow::statusError,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfSelectedCharacter,this,&MainWindow::display_numberOfSelectedCharacter,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfBotConnected,this,&MainWindow::display_numberOfBotConnected))
        abort();//,Qt::QueuedConnection
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_detectSlowDown,this,&MainWindow::detectSlowDown,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_all_player_connected,this,&MainWindow::all_player_connected,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_all_player_on_map,this,&MainWindow::all_player_on_map,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnection::emit_lastReplyTime,this,&MainWindow::lastReplyTime,Qt::QueuedConnection))
        abort();
    if(!connect(&slowDownTimer,&QTimer::timeout,&multipleBotConnexion,&MultipleBotConnectionImplForGui::detectSlowDown,Qt::QueuedConnection))
        abort();
    slowDownTimer.start(200);

    multipleBotConnexion.botInterface=new SimpleAction();

    if(settings.contains("login"))
        ui->login->setText(settings.value("login").toString());
    if(settings.contains("pass"))
        ui->pass->setText(settings.value("pass").toString());
    if(settings.contains("host"))
        ui->host->setText(settings.value("host").toString());
    if(settings.contains("port"))
        ui->port->setValue(settings.value("port").toUInt());
    if(settings.contains("proxy"))
        ui->proxy->setText(settings.value("proxy").toString());
    if(settings.contains("proxyport"))
        ui->proxyport->setValue(settings.value("proxyport").toUInt());
    if(settings.contains("multipleConnexion"))
    {
        if(settings.value("multipleConnexion").toUInt()<2)
            ui->multipleConnexion->setChecked(false);
        else
        {
            ui->multipleConnexion->setChecked(true);
            ui->connexionCountTarget->setValue(settings.value("multipleConnexion").toUInt());
        }
        if(settings.contains("connectBySeconds"))
            ui->connectBySeconds->setValue(settings.value("connectBySeconds").toUInt());
        if(settings.contains("maxDiffConnectedSelected"))
            ui->maxDiffConnectedSelected->setValue(settings.value("maxDiffConnectedSelected").toUInt());
    }
    if(settings.contains("autoCreateCharacter"))
        ui->autoCreateCharacter->setChecked(settings.value("autoCreateCharacter").toBool());
    ui->serverList->header()->setSectionResizeMode(QHeaderView::Fixed);
    ui->serverList->header()->resizeSection(0,680);
    ui->groupBox_char->setEnabled(false);
    ui->autoCreateCharacter->setEnabled(false);
    ui->groupBox_Server->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete multipleBotConnexion.botInterface;
    delete ui;
}

void MainWindow::lastReplyTime(const quint32 &time)
{
    ui->labelLastReplyTime->setText(tr("Last reply time: %1ms").arg(time));
}


void MainWindow::detectSlowDown(uint32_t queryCount,uint32_t worseTime)
{
    if(worseTime>3600*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2h").arg(queryCount).arg(worseTime/(3600*1000)));
    else if(worseTime>2*60*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2min").arg(queryCount).arg(worseTime/(60*1000)));
    else if(worseTime>5*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2s").arg(queryCount).arg(worseTime/(1000)));
    else
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2ms").arg(queryCount).arg(worseTime));
}

void MainWindow::logged(CatchChallenger::Api_client_real *senderObject,
                        const std::vector<CatchChallenger::ServerFromPoolForDisplay> serverOrdenedList,
                        const std::vector<std::vector<CatchChallenger::CharacterEntry> > characterEntryList,
                        bool haveTheDatapack)
{
    Q_UNUSED(haveTheDatapack);
    if(senderObject==NULL)
    {
        qDebug() << "MainWindow::logged(): qobject_cast<CatchChallenger::Api_client_real *>(sender())==NULL";
        return;
    }

    ui->characterList->clear();
    this->serverOrdenedList=serverOrdenedList;
    this->characterEntryList=characterEntryList;

    ui->characterList->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());

    ui->serverList->header()->setSectionResizeMode(QHeaderView::Fixed);
    ui->serverList->header()->resizeSection(0,400);
    updateServerList(senderObject);
}

void MainWindow::updateServerList(CatchChallenger::Api_client_real *senderObject)
{
    //do the grouping for characterGroup count
    {
        serverByCharacterGroup.clear();
        unsigned int index=0;
        int serverByCharacterGroupTempIndexToDisplay=1;
        while(index<serverOrdenedList.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=serverOrdenedList.at(index);
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

    //clear and determine what kind of view
    ui->serverList->clear();

    CatchChallenger::LogicialGroup logicialGroup=senderObject->getLogicialGroup();
    bool fullView=true;
    if(serverOrdenedList.size()>10)
        fullView=false;
    const uint64_t &current__date=QDateTime::currentDateTime().toTime_t();

    addToServerList(logicialGroup,ui->serverList->invisibleRootItem(),current__date,fullView);
    ui->serverList->expandAll();
}

void MainWindow::statusError(QString error)
{
    ui->statusBar->showMessage(error);
}

void MainWindow::on_connect_clicked()
{
    if(!ui->connect->isEnabled())
        return;
    if(ui->pass->text().size()<6)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your password need to be at minimum of 6 characters"));
        return;
    }
    if(ui->login->text().size()<3)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your login need to be at minimum of 3 characters"));
        return;
    }
    ui->groupBox_MultipleConnexion->setEnabled(false);
    ui->groupBox_Proxy->setEnabled(false);
    settings.setValue("login",ui->login->text());
    settings.setValue("pass",ui->pass->text());
    settings.setValue("host",ui->host->text());
    settings.setValue("port",ui->port->value());
    settings.setValue("proxy",ui->proxy->text());
    settings.setValue("proxyport",ui->proxyport->value());
    if(ui->multipleConnexion->isChecked())
        settings.setValue("multipleConnexion",ui->connexionCountTarget->value());
    else
        settings.setValue("multipleConnexion",0);
    settings.setValue("connectBySeconds",ui->connectBySeconds->value());
    settings.setValue("maxDiffConnectedSelected",ui->maxDiffConnectedSelected->value());
    settings.setValue("autoCreateCharacter",ui->autoCreateCharacter->isChecked());

    multipleBotConnexion.mLogin=ui->login->text();
    multipleBotConnexion.mPass=ui->pass->text();
    multipleBotConnexion.mMultipleConnexion=ui->multipleConnexion->isChecked();
    multipleBotConnexion.mAutoCreateCharacter=ui->autoCreateCharacter->isChecked();
    multipleBotConnexion.mConnectBySeconds=ui->connectBySeconds->value();
    multipleBotConnexion.mConnexionCountTarget=ui->connexionCountTarget->value();
    multipleBotConnexion.mMaxDiffConnectedSelected=ui->maxDiffConnectedSelected->value();
    multipleBotConnexion.mProxy=ui->proxy->text();
    multipleBotConnexion.mProxyport=ui->proxyport->value();
    multipleBotConnexion.mHost=ui->host->text();
    multipleBotConnexion.mPort=ui->port->value();

    if(ui->autoCreateCharacter->isChecked())
    {
        ui->characterSelect->setEnabled(false);
        ui->characterList->setEnabled(false);
    }
    if(ui->multipleConnexion->isChecked())
    {
        //for the other client
        ui->characterSelect->setEnabled(false);
        ui->characterList->setEnabled(false);
        ui->groupBox_MultipleConnexion->setEnabled(false);
    }
    ui->connect->setEnabled(false);

    //do only the first client to download the datapack
    multipleBotConnexion.createClient();
}

void MainWindow::on_characterSelect_clicked()
{
    if(ui->characterList->count()<=0 || !ui->characterSelect->isEnabled())
        return;
    multipleBotConnexion.characterSelectForFirstCharacter(ui->characterList->currentData().toUInt());
    ui->groupBox_char->setEnabled(false);
}

void MainWindow::display_numberOfBotConnected(quint16 numberOfBotConnected)
{
    ui->numberOfBotConnected->setText(tr("Number of bot connected: %1").arg(numberOfBotConnected));
}

void MainWindow::display_numberOfSelectedCharacter(quint16 numberOfSelectedCharacter)
{
    ui->numberOfSelectedCharacter->setText(tr("Selected character: %1").arg(numberOfSelectedCharacter));
}

void MainWindow::on_move_toggled(bool checked)
{
    multipleBotConnexion.botInterface->setValue("move",checked);
}

void MainWindow::on_randomText_toggled(bool checked)
{
    multipleBotConnexion.botInterface->setValue("randomText",checked);
}

void MainWindow::on_chatRandomReply_toggled(bool checked)
{
    multipleBotConnexion.botInterface->setValue("globalChatRandomReply",checked);
}

void MainWindow::on_bugInDirection_toggled(bool checked)
{
    multipleBotConnexion.botInterface->setValue("bugInDirection",checked);
}

void MainWindow::on_serverList_activated(const QModelIndex &index)
{
    Q_UNUSED(index);
    on_serverListSelect_clicked();
}

void MainWindow::on_serverListSelect_clicked()
{
    const QList<QTreeWidgetItem *> &selectedItems=ui->serverList->selectedItems();
    if(selectedItems.size()!=1)
        return;

    const QTreeWidgetItem * const selectedItem=selectedItems.at(0);
    unsigned int serverSelectedIndex=selectedItem->data(99,99).toUInt();

    CatchChallenger::ServerFromPoolForDisplay * const serverSelected=&serverOrdenedList[serverSelectedIndex];
    const uint8_t &charactersGroupIndex=serverSelected->charactersGroupIndex;
    if(charactersGroupIndex>=characterEntryList.size())
    {
        ui->groupBox_Server->setEnabled(false);
        QMessageBox::critical(this,tr("Error"),tr("The group index is wrong, maybe the server list have change"));
        return;
    }
    const quint32 &connectionTargetCount=ui->connexionCountTarget->value();
    if(ui->multipleConnexion->isChecked())
        if(serverSelected->maxPlayer<65533 && serverSelected->maxPlayer>=serverSelected->currentPlayer)
        {
            const quint32 &freeSlot=serverSelected->maxPlayer-serverSelected->currentPlayer;
            if(connectionTargetCount>freeSlot)
            {
                ui->groupBox_Server->setEnabled(false);
                QMessageBox::critical(this,tr("Error"),tr("Too many connexion count target for the free slot"));
                return;
            }
        }
    multipleBotConnexion.serverSelect(serverSelected->charactersGroupIndex,serverSelected->uniqueKey);

    ui->groupBox_Server->setEnabled(false);
    if(!ui->multipleConnexion->isChecked())
    {
        ui->groupBox_char->setEnabled(true);
        if(characterEntryList.at(charactersGroupIndex).empty())
        {
            if(ui->autoCreateCharacter->isChecked())
            {
                qDebug() << "MainWindow::on_serverListSelect_clicked(): ui->characterList->count()==0 auto create";
                //auto create do into multipleBotConnexion.serverSelect()
                ui->groupBox_char->setEnabled(false);
            }
            else
            {
                ui->characterList->setStyleSheet("background-color:rgb(255,0,0);");
                ui->groupBox_char->setEnabled(false);
                ui->characterList->addItem("No character exists AND auto create disabled");
                QMessageBox::warning(this,"Error","No character exists AND auto create disabled");
            }
            return;
        }
        else
        {
            unsigned int index=0;
            while(index<characterEntryList.at(charactersGroupIndex).size())
            {
                const CatchChallenger::CharacterEntry &character=characterEntryList.at(charactersGroupIndex).at(index);
                ui->characterList->addItem(QString::fromStdString(character.pseudo),character.character_id);
                index++;
            }
            ui->characterList->setEnabled(true);
        }
    }
    else
    {
        ui->groupBox_char->setEnabled(false);
        if(characterEntryList.size()>0)
        {
            if(characterEntryList.at(charactersGroupIndex).empty())
            {
                qDebug() << "MainWindow::on_serverListSelect_clicked(): ui->characterList->count()==0";
                return;
            }
            const CatchChallenger::CharacterEntry &character=characterEntryList.at(charactersGroupIndex).front();
            multipleBotConnexion.characterSelectForFirstCharacter(character.character_id);
        }
        else
        {
            qDebug() << "MainWindow::on_serverListSelect_clicked(): ui->characterList->count()==0";
        }
    }
    ui->characterSelect->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());
}

void MainWindow::addToServerList(CatchChallenger::LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView)
{
    item->setText(0,QString::fromStdString(logicialGroup.name));
    {
        //to order the group
        std::vector<std::string> keys;
        for (const auto &n : logicialGroup.logicialGroupList)
            keys.push_back(n.first);
        qSort(keys);
        //list the group
        unsigned int index=0;
        while(index<keys.size())
        {
            QTreeWidgetItem * const itemGroup=new QTreeWidgetItem(item);
            addToServerList(*logicialGroup.logicialGroupList[keys.at(index)],
                    itemGroup,currentDate,fullView);
            index++;
        }
    }
    {
        qSort(logicialGroup.servers);
        //list the server
        unsigned int index=0;
        while(index<logicialGroup.servers.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=logicialGroup.servers.at(index);
            QTreeWidgetItem *itemServer=new QTreeWidgetItem(item);
            QString text;
            QString groupText;
            if(serverByCharacterGroup.size()>1)
                groupText=QStringLiteral(" (%1)").arg(serverByCharacterGroup.value(server.charactersGroupIndex).second);
            QString name=QString::fromStdString(server.name);
            if(name.isEmpty())
                name=tr("Default server");
            if(fullView)
            {
                text=name+groupText;
                if(server.playedTime>0)
                {
                    /*if(!server.description.empty())
                        text+=" "+tr("%1 played").arg(QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(server.playedTime)));
                    else
                        text+="\n"+tr("%1 played").arg(QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(server.playedTime)));*/
                }
                if(!server.description.empty())
                    text+="\n"+QString::fromStdString(server.description);
            }
            else
            {
                if(server.description.empty())
                    text=name+groupText;
                else
                    text=name+groupText+" - "+QString::fromStdString(server.description);
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

void MainWindow::datapackIsReady()
{
    ui->groupBox_Server->setEnabled(true);
    if(!ui->multipleConnexion->isChecked())
    {
        ui->autoCreateCharacter->setEnabled(true);
        ui->groupBox_char->setEnabled(true);
    }
}

void MainWindow::datapackMainSubIsReady()
{
    if(!CommonSettingsServer::commonSettingsServer.chat_allow_all && !CommonSettingsServer::commonSettingsServer.chat_allow_local)
    {
        ui->randomText->setEnabled(false);
        ui->randomText->setChecked(false);
        ui->chatRandomReply->setEnabled(false);
        ui->chatRandomReply->setChecked(false);
    }
}

void MainWindow::on_autoCreateCharacter_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    settings.setValue("autoCreateCharacter",ui->autoCreateCharacter->isChecked());
}

void MainWindow::all_player_connected()
{
    qDebug() << "MainWindow::all_player_connected()";
}

void MainWindow::all_player_on_map()
{
    qDebug() << "MainWindow::all_player_on_map()";
}
