#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "../../general/base/CommonSettings.h"
#include "../../general/base/FacilityLib.h"

#include <QNetworkProxy>
#include <QMessageBox>

#define CATCHCHALLENGER_BOTCONNECTION_VERSION "0.0.0.2"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    ui->setupUi(this);
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    connect(&moveTimer,&QTimer::timeout,this,&MainWindow::doMove);
    connect(&moveTimer,&QTimer::timeout,this,&MainWindow::doText);
    connect(&moveTimer,&QTimer::timeout,this,&MainWindow::detectSlowDown);
    moveTimer.start(1000);
    textTimer.start(1000);
    slowDownTimer.start(200);
    number=1;
    numberOfBotConnected=0;
    numberOfSelectedCharacter=0;
    haveEnError=false;

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
            ui->connexionCount->setValue(settings.value("multipleConnexion").toUInt());
        }
        if(settings.contains("connectBySeconds"))
            ui->connectBySeconds->setValue(settings.value("connectBySeconds").toUInt());
        if(settings.contains("maxDiffConnectedSelected"))
            ui->maxDiffConnectedSelected->setValue(settings.value("maxDiffConnectedSelected").toUInt());
    }
    if(settings.contains("autoCreateCharacter"))
        ui->autoCreateCharacter->setChecked(settings.value("autoCreateCharacter").toBool());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::disconnected()
{
    haveEnError=true;
    numberOfBotConnected--;
    ui->numberOfBotConnected->setText(tr("Number of bot connected: %1").arg(numberOfBotConnected));

    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(sender());
    if(senderObject==NULL)
        return;

    if(!connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason)
    {
        ui->statusBar->showMessage(tr("Disconnected by the host"));
        ui->connect->setEnabled(true);
    }
    connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=false;
    if(connectedSocketToCatchChallengerClient[senderObject]->selectedCharacter==true)
    {
        connectedSocketToCatchChallengerClient[senderObject]->selectedCharacter=false;
        numberOfSelectedCharacter--;
        ui->numberOfSelectedCharacter->setText(tr("Selected character: %1").arg(numberOfSelectedCharacter));
    }
}

void MainWindow::lastReplyTime(const quint32 &time)
{
    ui->labelLastReplyTime->setText(tr("Last reply time: %1ms").arg(time));
}

void MainWindow::tryLink(CatchChallengerClient * client)
{
    numberOfBotConnected++;
    ui->numberOfBotConnected->setText(tr("Number of bot connected: %1").arg(numberOfBotConnected));

    connect(client->api,&CatchChallenger::Api_client_real::protocol_is_good,this,&MainWindow::protocol_is_good);
    if(!ui->multipleConnexion->isChecked())
    {
        client->login=ui->login->text();
        client->pass=ui->pass->text();
        client->api->sendProtocol();
    }
    else
    {
        QString login=ui->login->text();
        QString pass=ui->pass->text();
        login.replace(QLatin1Literal("%NUMBER%"),QString::number(client->number));
        pass.replace(QLatin1Literal("%NUMBER%"),QString::number(client->number));
        client->login=login;
        client->pass=login;
        client->api->sendProtocol();
    }
}

void MainWindow::protocol_is_good()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    senderObject->tryLogin(apiToCatchChallengerClient.value(senderObject)->login,apiToCatchChallengerClient.value(senderObject)->pass);
}

void MainWindow::doMove()
{
    if(!ui->move->isChecked())
        return;

    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
        if(i.value()->api->getCaracterSelected() && i.value()->have_informations && i.value()->socket->isValid() && i.value()->socket->state()==QAbstractSocket::ConnectedState)
        {
            if(ui->bugInDirection->isChecked())
                i.value()->api->send_player_move(0,i.value()->direction);
            else
            {
                if(i.value()->direction==CatchChallenger::Direction_look_at_bottom)
                {
                    i.value()->direction=CatchChallenger::Direction_look_at_left;
                    i.value()->api->send_player_move(0,i.value()->direction);
                }
                else if(i.value()->direction==CatchChallenger::Direction_look_at_left)
                {
                    i.value()->direction=CatchChallenger::Direction_look_at_top;
                    i.value()->api->send_player_move(0,i.value()->direction);
                }
                else if(i.value()->direction==CatchChallenger::Direction_look_at_top)
                {
                    i.value()->direction=CatchChallenger::Direction_look_at_right;
                    i.value()->api->send_player_move(0,i.value()->direction);
                }
                else if(i.value()->direction==CatchChallenger::Direction_look_at_right)
                {
                    i.value()->direction=CatchChallenger::Direction_look_at_bottom;
                    i.value()->api->send_player_move(0,i.value()->direction);
                }
                else
                {
                    qDebug() << "Out of direction scope";
                    abort();
                }
            }
        }
    }
}

void MainWindow::doText()
{
    if(apiToCatchChallengerClient.isEmpty())
        return;
    if(!ui->randomText->isEnabled())
        return;
    if(!ui->randomText->isChecked())
        return;

    QList<CatchChallengerClient *> clientList;
    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        clientList << i.value();
    }
    CatchChallengerClient *client=clientList.at(rand()%clientList.size());
    //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
    if(client->api->getCaracterSelected() && client->have_informations && ui->randomText->isChecked() && client->socket->isValid() && client->socket->state()==QAbstractSocket::ConnectedState)
    {
        if(CommonSettings::commonSettings.chat_allow_local && rand()%10==0)
        {
            switch(rand()%3)
            {
                case 0:
                    client->api->sendChatText(CatchChallenger::Chat_type_local,"What's up?");
                break;
                case 1:
                    client->api->sendChatText(CatchChallenger::Chat_type_local,"Have good day!");
                break;
                case 2:
                    client->api->sendChatText(CatchChallenger::Chat_type_local,"... and now, what I have win :)");
                break;
            }
        }
        else
        {
            if(CommonSettings::commonSettings.chat_allow_all && rand()%100==0)
            {
                switch(rand()%4)
                {
                    case 0:
                        client->api->sendChatText(CatchChallenger::Chat_type_all,"Hello world! :)");
                    break;
                    case 1:
                        client->api->sendChatText(CatchChallenger::Chat_type_all,"It's so good game!");
                    break;
                    case 2:
                        client->api->sendChatText(CatchChallenger::Chat_type_all,"This game have reason to ask donations!");
                    break;
                    case 3:
                        client->api->sendChatText(CatchChallenger::Chat_type_all,"Donate if you can!");
                    break;
                }
            }
        }
    }
}

void MainWindow::detectSlowDown()
{
    quint32 queryCount=0;
    quint32 worseTime=0;
    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        const QMap<quint8,QTime> &values=i.key()->getQuerySendTimeList();
        queryCount+=values.size();
        QMapIterator<quint8,QTime> i(values);
        while (i.hasNext()) {
            i.next();
            const quint32 &time=i.value().elapsed();
            if(time>worseTime)
                worseTime=time;
        }
    }
    if(worseTime>3600*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2h").arg(queryCount).arg(worseTime/(3600*1000)));
    else if(worseTime>2*60*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2min").arg(queryCount).arg(worseTime/(60*1000)));
    else if(worseTime>5*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2s").arg(queryCount).arg(worseTime/(1000)));
    else
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2ms").arg(queryCount).arg(worseTime));
}

//quint32,QString,quint16,quint16,quint8,quint16
void MainWindow::insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    ui->statusBar->showMessage(tr("On the map"));
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    if(player.simplifiedId==apiToCatchChallengerClient[senderObject]->api->getId())
        apiToCatchChallengerClient[senderObject]->direction=direction;
    apiToCatchChallengerClient[senderObject]->have_informations=true;
}

void MainWindow::haveCharacter()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;
    ui->statusBar->showMessage(QStringLiteral("Now on the map"));
}

void MainWindow::logged(const QList<CatchChallenger::CharacterEntry> &characterEntryList)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    apiToCatchChallengerClient[senderObject]->charactersList=characterEntryList;

    ui->characterList->clear();
    if(!ui->multipleConnexion->isChecked())
    {
        int index=0;
        while(index<characterEntryList.size())
        {
            const CatchChallenger::CharacterEntry &character=characterEntryList.at(index);
            ui->characterList->addItem(character.pseudo,character.character_id);
            index++;
        }
    }
    ui->characterList->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());
    ui->characterSelect->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());
    if(apiToCatchChallengerClient.size()==1)
    {
        if(!CommonSettings::commonSettings.chat_allow_all && !CommonSettings::commonSettings.chat_allow_local)
        {
            ui->randomText->setEnabled(false);
            ui->randomText->setChecked(false);
            ui->chatRandomReply->setEnabled(false);
            ui->chatRandomReply->setChecked(false);
        }
        //get the datapack
        apiToCatchChallengerClient[senderObject]->api->sendDatapackContent();
        return;
    }
    if(apiToCatchChallengerClient[senderObject]->charactersList.count()<=0)
    {
        qDebug() << apiToCatchChallengerClient[senderObject]->login << "have not character";
        if(ui->autoCreateCharacter->isChecked())
        {
            qDebug() << apiToCatchChallengerClient[senderObject]->login << "create new character";
            quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
            QString pseudo="bot"+CatchChallenger::FacilityLib::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettings::commonSettings.max_pseudo_size-3);
            quint32 skinId;
            const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
            if(!profile.forcedskin.isEmpty())
                skinId=profile.forcedskin.at(rand()%profile.forcedskin.size());
            else
                skinId=rand()%skinsList.size();
            apiToCatchChallengerClient[senderObject]->api->addCharacter(profileIndex,pseudo,skinId);
        }
        return;
    }
    if(ui->multipleConnexion->isChecked())
    {
        const quint32 &character_id=apiToCatchChallengerClient[senderObject]->charactersList.at(rand()%apiToCatchChallengerClient[senderObject]->charactersList.size()).character_id;
        if(!characterOnMap.contains(character_id))
        {
            characterOnMap << character_id;
            if(!apiToCatchChallengerClient[senderObject]->api->selectCharacter(character_id))
                qDebug() << "Unable to do automatic character selection:" << character_id;
            else
                qDebug() << "Automatic select character:" << character_id;
        }
        return;
    }
}

void MainWindow::haveTheDatapack()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    //load the datapack
    {
        CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/"));
        //load the skins list
        QDir dir(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/skin/fighter/"));
        QFileInfoList entryList=dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
        int index=0;
        while(index<entryList.size())
        {
            skinsList << entryList.at(index);
            index++;
        }
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.isEmpty())
    {
        qDebug() << "Profile list is empty";
        return;
    }

    if(apiToCatchChallengerClient[senderObject]->charactersList.count()<=0)
    {
        qDebug() << apiToCatchChallengerClient[senderObject]->login << "have not character";
        if(ui->autoCreateCharacter->isChecked())
        {
            qDebug() << apiToCatchChallengerClient[senderObject]->login << "create new character";
            ui->characterSelect->setEnabled(false);
            ui->characterList->setEnabled(false);
            quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
            QString pseudo="bot"+CatchChallenger::FacilityLib::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettings::commonSettings.max_pseudo_size-3);
            quint32 skinId;
            const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
            if(!profile.forcedskin.isEmpty())
                skinId=profile.forcedskin.at(rand()%profile.forcedskin.size());
            else
                skinId=rand()%skinsList.size();
            apiToCatchChallengerClient[senderObject]->api->addCharacter(profileIndex,pseudo,skinId);
        }
        return;
    }
    ifMultipleConnexionStartCreation();
    //the actual client
    const quint32 &character_id=apiToCatchChallengerClient[senderObject]->charactersList.at(rand()%apiToCatchChallengerClient[senderObject]->charactersList.size()).character_id;
    if(!characterOnMap.contains(character_id))
    {
        characterOnMap << character_id;
        if(!apiToCatchChallengerClient[senderObject]->api->selectCharacter(character_id))
            qDebug() << "Unable to select character after datpack loading:" << character_id;
        else
            qDebug() << "Select character after datpack loading:" << character_id;
    }
}

void MainWindow::ifMultipleConnexionStartCreation()
{
    if(ui->multipleConnexion->isChecked() && !connectTimer.isActive())
    {
        connect(&connectTimer,&QTimer::timeout,this,&MainWindow::connectTimerSlot);
        connectTimer.start(1000/ui->connectBySeconds->value());

        //for the other client
        ui->characterSelect->setEnabled(false);
        ui->characterList->setEnabled(false);
        ui->groupBox_MultipleConnexion->setEnabled(false);

        return;
    }
}

void MainWindow::connectTimerSlot()
{
    if(apiToCatchChallengerClient.size()<ui->connexionCount->value())
    {
        const quint32 &diff=numberOfBotConnected-numberOfSelectedCharacter;
        if(diff<=(quint32)ui->maxDiffConnectedSelected->value())
            createClient();
    }
    else
        connectTimer.stop();
}

void MainWindow::newCharacterId(const quint8 &returnCode, const quint32 &characterId)
{
    if(returnCode!=0x00)
    {
        qDebug() << "new character not created, server have returned a failed: " << returnCode;
        return;
    }
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
    {
        qDebug() << apiToCatchChallengerClient[senderObject]->login << "new character is created but unable to locate the sender";
        return;
    }
    else
        qDebug() << apiToCatchChallengerClient[senderObject]->login << "new character is created";

    if(!characterOnMap.contains(characterId))
    {
        characterOnMap << characterId;
        if(!apiToCatchChallengerClient[senderObject]->api->selectCharacter(characterId))
            qDebug() << "Unable to select character after creation:" << characterId;
        else
        {
            qDebug() << "Select new character created after creation:" << characterId;
            ifMultipleConnexionStartCreation();
        }
    }
    else
        qDebug() << apiToCatchChallengerClient[senderObject]->login << "new character is already on map";
}

void MainWindow::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;
    apiToCatchChallengerClient[senderObject]->selectedCharacter=true;
    numberOfSelectedCharacter++;
    ui->numberOfSelectedCharacter->setText(tr("Selected character: %1").arg(numberOfSelectedCharacter));

    Q_UNUSED(informations);
//    DebugClass::debugConsole(QStringLiteral("MainWindow::have_current_player_info() pseudo: %1").arg(informations.public_informations.pseudo));
}

void MainWindow::newError(QString error,QString detailedError)
{
    haveEnError=true;

    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    apiToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
    ui->statusBar->showMessage(QStringLiteral("Error: %1, detailedError: %2").arg(error).arg(detailedError));
    CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() error: %1, detailedError: %2").arg(error).arg(detailedError));
    apiToCatchChallengerClient[senderObject]->socket->disconnectFromHost();
}

void MainWindow::newSocketError(QAbstractSocket::SocketError error)
{
    haveEnError=true;

    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(sender());
    if(senderObject==NULL)
        return;

    connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
    if(error==0)
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() Connection refused"));
        ui->statusBar->showMessage(QStringLiteral("Connection refused"));
    }
    else if(error==13)
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() SslHandshakeFailedError"));
        ui->statusBar->showMessage(QStringLiteral("SslHandshakeFailedError"));
    }
    else
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() error: %1").arg(error));
        ui->statusBar->showMessage(QStringLiteral("Error: %1").arg(error));
    }

}

void MainWindow::new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type)
{
    if(!ui->chatRandomReply->isChecked() && chat_type!=CatchChallenger::Chat_type_pm)
        return;

    Q_UNUSED(text);
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    Q_UNUSED(type);
    switch(chat_type)
    {
        case CatchChallenger::Chat_type_all:
        if(CommonSettings::commonSettings.chat_allow_all)
            switch(rand()%(100*apiToCatchChallengerClient.size()))
            {
                case 0:
                    apiToCatchChallengerClient[senderObject]->api->sendChatText(CatchChallenger::Chat_type_local,"I'm according "+pseudo);
                break;
                default:
                break;
            }
        break;
        case CatchChallenger::Chat_type_local:
        if(CommonSettings::commonSettings.chat_allow_local)
            switch(rand()%(3*apiToCatchChallengerClient.size()))
            {
                case 0:
                    apiToCatchChallengerClient[senderObject]->api->sendChatText(CatchChallenger::Chat_type_local,"You are in right "+pseudo);
                break;
            }
        break;
        case CatchChallenger::Chat_type_pm:
        if(CommonSettings::commonSettings.chat_allow_private)
        {
            if(text=="version")
                apiToCatchChallengerClient[senderObject]->api->sendPM(QStringLiteral("Version %1").arg(CATCHCHALLENGER_BOTCONNECTION_VERSION),pseudo);
            else
                apiToCatchChallengerClient[senderObject]->api->sendPM(QStringLiteral("Hello %1, I'm few bit busy for now").arg(pseudo),pseudo);
        }
        break;
        default:
        break;
    }
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
    haveEnError=false;
    ui->groupBox_MultipleConnexion->setEnabled(false);
    ui->groupBox_Proxy->setEnabled(false);
    settings.setValue("login",ui->login->text());
    settings.setValue("pass",ui->pass->text());
    settings.setValue("host",ui->host->text());
    settings.setValue("port",ui->port->value());
    settings.setValue("proxy",ui->proxy->text());
    settings.setValue("proxyport",ui->proxyport->value());
    if(ui->multipleConnexion->isChecked())
        settings.setValue("multipleConnexion",ui->connexionCount->value());
    else
        settings.setValue("multipleConnexion",0);
    settings.setValue("connectBySeconds",ui->connectBySeconds->value());
    settings.setValue("maxDiffConnectedSelected",ui->maxDiffConnectedSelected->value());
    settings.setValue("autoCreateCharacter",ui->autoCreateCharacter->isChecked());

    if(!ui->connect->isEnabled())
        return;
    ui->connect->setEnabled(false);


    //do only the first client to download the datapack
    createClient();
}

void MainWindow::createClient()
{
    if(haveEnError)
        return;
    CatchChallengerClient * client=new CatchChallengerClient;

    QSslSocket *sslSocket=new QSslSocket();

    QNetworkProxy proxy;
    if(!ui->proxy->text().isEmpty())
    {
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(ui->proxy->text());
        proxy.setPort(ui->proxyport->value());
        sslSocket->setProxy(proxy);
    }

    client->haveFirstHeader=false;
    client->sslSocket=sslSocket;
    sslSocketToCatchChallengerClient[client->sslSocket]=client;

    connect(sslSocket,&QSslSocket::readyRead,this,&MainWindow::readForFirstHeader,Qt::DirectConnection);
    connect(sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MainWindow::sslErrors,Qt::QueuedConnection);
    sslSocket->connectToHost(ui->host->text(),ui->port->value());
}

void MainWindow::connectTheExternalSocket(CatchChallengerClient * client)
{
    client->socket=new CatchChallenger::ConnectedSocket(client->sslSocket);
    client->api=new CatchChallenger::Api_client_real(client->socket,false);
    client->api->setDatapackPath(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/"));
    connect(client->api,&CatchChallenger::Api_client_real::insert_player,            this,&MainWindow::insert_player);
    connect(client->api,&CatchChallenger::Api_client_real::new_chat_text,            this,&MainWindow::new_chat_text,Qt::QueuedConnection);
    connect(client->api,&CatchChallenger::Api_client_real::haveCharacter,            this,&MainWindow::haveCharacter);
    connect(client->api,&CatchChallenger::Api_client_real::logged,                   this,&MainWindow::logged);
    connect(client->api,&CatchChallenger::Api_client_real::have_current_player_info, this,&MainWindow::have_current_player_info);
    connect(client->api,&CatchChallenger::Api_client_real::newError,                 this,&MainWindow::newError);
    connect(client->api,&CatchChallenger::Api_client_real::newCharacterId,           this,&MainWindow::newCharacterId);
    connect(client->api,&CatchChallenger::Api_client_real::lastReplyTime,            this,&MainWindow::lastReplyTime);
    connect(client->socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),                    this,&MainWindow::newSocketError);
    connect(client->socket,&CatchChallenger::ConnectedSocket::disconnected,          this,&MainWindow::disconnected);
    if(apiToCatchChallengerClient.isEmpty())
        connect(client->api,&CatchChallenger::Api_client_real::haveTheDatapack,      this,&MainWindow::haveTheDatapack);
    client->haveShowDisconnectionReason=false;
    client->have_informations=false;
    client->number=number;
    client->selectedCharacter=false;
    number++;
    apiToCatchChallengerClient[client->api]=client;
    connectedSocketToCatchChallengerClient[client->socket]=client;
    tryLink(client);
}

void MainWindow::sslErrors(const QList<QSslError> &errors)
{
    QSslSocket *senderObject = qobject_cast<QSslSocket *>(sender());
    if(senderObject==NULL)
        return;

    QStringList sslErrors;
    int index=0;
    while(index<errors.size())
    {
        qDebug() << "Ssl error:" << errors.at(index).errorString();
        sslErrors << errors.at(index).errorString();
        index++;
    }
    /*QMessageBox::warning(this,tr("Ssl error"),sslErrors.join("\n"));
    realSocket->disconnectFromHost();*/
}

void MainWindow::on_characterSelect_clicked()
{
    if(ui->characterList->count()<=0 || !ui->characterSelect->isEnabled())
        return;
    const quint32 &charId=ui->characterList->currentData().toUInt();
    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        if(!i.value()->api->selectCharacter(charId))
            qDebug() << "Unable to manual select character:" << charId;
        else
        {
            characterOnMap << charId;
            qDebug() << "Manual select character:" << charId;
        }
    }
    ui->characterSelect->setEnabled(false);
    ui->characterList->setEnabled(false);
}

void MainWindow::sslHandcheckIsFinished()
{
    QSslSocket *socket=qobject_cast<QSslSocket *>(sender());
    if(socket==NULL)
        return;
    connectTheExternalSocket(sslSocketToCatchChallengerClient[socket]);
}

void MainWindow::readForFirstHeader()
{
    QSslSocket *socket=qobject_cast<QSslSocket *>(sender());
    if(socket==NULL)
        return;
    CatchChallengerClient * client=sslSocketToCatchChallengerClient[socket];
    if(client->haveFirstHeader)
        return;
    quint8 value;
    if(socket->read((char*)&value,sizeof(value))==sizeof(value))
    {
        client->haveFirstHeader=true;
        if(value==0x01)
        {
            socket->setPeerVerifyMode(QSslSocket::VerifyNone);
            socket->ignoreSslErrors();
            socket->startClientEncryption();
            connect(socket,&QSslSocket::encrypted,this,&MainWindow::sslHandcheckIsFinished);
        }
        else
            connectTheExternalSocket(client);
    }
}
