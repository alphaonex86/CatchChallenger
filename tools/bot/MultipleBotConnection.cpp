#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "../../general/base/CommonSettings.h"
#include "../../general/base/FacilityLib.h"

#include <QNetworkProxy>
#include <QMessageBox>

#define CATCHCHALLENGER_BOTCONNECTION_VERSION "0.0.0.3"

MultipleBotConnection::MultipleBotConnection()
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    number=1;
    numberOfBotConnected=0;
    numberOfSelectedCharacter=0;
    haveEnError=false;
}

MultipleBotConnection::~MultipleBotConnection()
{
}

void MultipleBotConnection::disconnected()
{
    qDebug() << "disconnected()";
    haveEnError=true;
    numberOfBotConnected--;
}

void MultipleBotConnection::lastReplyTime(const quint32 &time)
{
    Q_UNUSED(time);
}

void MultipleBotConnection::tryLink(CatchChallengerClient * client)
{
    numberOfBotConnected++;

    QObject::connect(client->api,&CatchChallenger::Api_client_real::protocol_is_good,this,&MultipleBotConnection::protocol_is_good);
    if(!multipleConnexion())
    {
        client->login=login();
        client->pass=pass();
        client->api->sendProtocol();
    }
    else
    {
        QString loginString=login();
        QString passString=pass();
        loginString.replace(QLatin1Literal("%NUMBER%"),QString::number(client->number));
        passString.replace(QLatin1Literal("%NUMBER%"),QString::number(client->number));
        client->login=loginString;
        client->pass=passString;
        client->api->sendProtocol();
    }
}

void MultipleBotConnection::protocol_is_good(CatchChallengerClient *client)
{
    /*CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;*/

    client->api->tryLogin(client->login,client->pass);
}

//quint32,QString,quint16,quint16,quint8,quint16
void MultipleBotConnection::insert_player(CatchChallengerClient *client,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    if(player.simplifiedId==client->api->getId())
        client->direction=direction;
    client->have_informations=true;
}

void MultipleBotConnection::haveCharacter()
{
}

void MultipleBotConnection::logged(CatchChallengerClient *client,const QList<CatchChallenger::CharacterEntry> &characterEntryList)
{
    Q_UNUSED(characterEntryList);
    if(client->charactersList.count()<=0)
    {
        qDebug() << client->login << "have not character";
        if(autoCreateCharacter())
        {
            qDebug() << client->login << "create new character";
            quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
            QString pseudo="bot"+CatchChallenger::FacilityLib::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettings::commonSettings.max_pseudo_size-3);
            quint32 skinId;
            const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
            if(!profile.forcedskin.isEmpty())
                skinId=profile.forcedskin.at(rand()%profile.forcedskin.size());
            else
                skinId=rand()%skinsList.size();
            client->api->addCharacter(profileIndex,pseudo,skinId);
        }
        return;
    }
    if(multipleConnexion())
    {
        const quint32 &character_id=client->charactersList.at(rand()%client->charactersList.size()).character_id;
        if(!characterOnMap.contains(character_id))
        {
            characterOnMap << character_id;
            if(!client->api->selectCharacter(character_id))
                qDebug() << "Unable to do automatic character selection:" << character_id;
            else
                qDebug() << "Automatic select character:" << character_id;
        }
        return;
    }
}

void MultipleBotConnection::haveTheDatapack(CatchChallengerClient *client)
{
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
        if(autoCreateCharacter())
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
        if(multipleConnexion())
        {
            if(!apiToCatchChallengerClient[senderObject]->api->selectCharacter(character_id))
                qDebug() << "Unable to select character after datpack loading:" << character_id;
            else
                qDebug() << "Select character after datpack loading:" << character_id;
        }
    }
}

void MultipleBotConnection::ifMultipleConnexionStartCreation()
{
    if(multipleConnexion() && !connectTimer.isActive())
    {
        QObject::connect(&connectTimer,&QTimer::timeout,this,&MultipleBotConnection::connectTimerSlot);
        connectTimer.start(1000/ui->connectBySeconds->value());

        //for the other client
        ui->characterSelect->setEnabled(false);
        ui->characterList->setEnabled(false);
        ui->groupBox_MultipleConnexion->setEnabled(false);

        return;
    }
}

void MultipleBotConnection::connectTimerSlot()
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

void MultipleBotConnection::newCharacterId(const quint8 &returnCode, const quint32 &characterId)
{
    if(returnCode!=0x00)
    {
        qDebug() << "new character not created, server have returned a failed: " << returnCode;
        return;
    }
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
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

void MultipleBotConnection::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;
    apiToCatchChallengerClient[senderObject]->selectedCharacter=true;
    numberOfSelectedCharacter++;
    ui->numberOfSelectedCharacter->setText(tr("Selected character: %1").arg(numberOfSelectedCharacter));

    Q_UNUSED(informations);
//    DebugClass::debugConsole(QStringLiteral("MultipleBotConnection::have_current_player_info() pseudo: %1").arg(informations.public_informations.pseudo));
}

void MultipleBotConnection::newError(QString error,QString detailedError)
{
    haveEnError=true;

    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;

    apiToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
    ui->statusBar->showMessage(QStringLiteral("Error: %1, detailedError: %2").arg(error).arg(detailedError));
    CatchChallenger::DebugClass::debugConsole(QStringLiteral("MultipleBotConnection::newError() error: %1, detailedError: %2").arg(error).arg(detailedError));
    apiToCatchChallengerClient[senderObject]->socket->disconnectFromHost();
}

void MultipleBotConnection::newSocketError(QAbstractSocket::SocketError error)
{
    qDebug() << "newSocketError()" << error;
    haveEnError=true;

    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(QObject::sender());
    if(senderObject==NULL)
        return;

    connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
    if(error==0)
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MultipleBotConnection::newError() Connection refused"));
        ui->statusBar->showMessage(QStringLiteral("Connection refused"));
    }
    else if(error==13)
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MultipleBotConnection::newError() SslHandshakeFailedError"));
        ui->statusBar->showMessage(QStringLiteral("SslHandshakeFailedError"));
    }
    else
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MultipleBotConnection::newError() error: %1").arg(error));
        ui->statusBar->showMessage(QStringLiteral("Error: %1").arg(error));
    }

}

void MultipleBotConnection::new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type)
{
    if(!ui->chatRandomReply->isChecked() && chat_type!=CatchChallenger::Chat_type_pm)
        return;

    Q_UNUSED(text);
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
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

void MultipleBotConnection::on_connect_clicked()
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
    if(multipleConnexion())
        settings.setValue("multipleConnexion",ui->connexionCount->value());
    else
        settings.setValue("multipleConnexion",0);
    settings.setValue("connectBySeconds",ui->connectBySeconds->value());
    settings.setValue("maxDiffConnectedSelected",ui->maxDiffConnectedSelected->value());
    settings.setValue("autoCreateCharacter",autoCreateCharacter());

    if(!ui->connect->isEnabled())
        return;
    ui->connect->setEnabled(false);


    //do only the first client to download the datapack
    createClient();
}

void MultipleBotConnection::createClient()
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

    QObject::connect(sslSocket,&QSslSocket::readyRead,this,&MultipleBotConnection::readForFirstHeader,Qt::DirectConnection);
    QObject::connect(sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MultipleBotConnection::sslErrors,Qt::QueuedConnection);
    sslSocket->connectToHost(ui->host->text(),ui->port->value());
}

void MultipleBotConnection::connectTheExternalSocket(CatchChallengerClient * client)
{
    client->socket=new CatchChallenger::ConnectedSocket(client->sslSocket);
    client->api=new CatchChallenger::Api_client_real(client->socket,false);
    client->api->setDatapackPath(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/"));
    QObject::connect(client->api,&CatchChallenger::Api_client_real::insert_player,            this,&MultipleBotConnection::insert_player);
    QObject::connect(client->api,&CatchChallenger::Api_client_real::new_chat_text,            this,&MultipleBotConnection::new_chat_text,Qt::QueuedConnection);
    QObject::connect(client->api,&CatchChallenger::Api_client_real::haveCharacter,            this,&MultipleBotConnection::haveCharacter);
    QObject::connect(client->api,&CatchChallenger::Api_client_real::logged,                   this,&MultipleBotConnection::logged);
    QObject::connect(client->api,&CatchChallenger::Api_client_real::have_current_player_info, this,&MultipleBotConnection::have_current_player_info);
    QObject::connect(client->api,&CatchChallenger::Api_client_real::newError,                 this,&MultipleBotConnection::newError);
    QObject::connect(client->api,&CatchChallenger::Api_client_real::newCharacterId,           this,&MultipleBotConnection::newCharacterId);
    QObject::connect(client->api,&CatchChallenger::Api_client_real::lastReplyTime,            this,&MultipleBotConnection::lastReplyTime);
    QObject::connect(client->socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),                    this,&MultipleBotConnection::newSocketError);
    QObject::connect(client->socket,&CatchChallenger::ConnectedSocket::disconnected,          this,&MultipleBotConnection::disconnected);
    if(apiToCatchChallengerClient.isEmpty())
        QObject::connect(client->api,&CatchChallenger::Api_client_real::haveTheDatapack,      this,&MultipleBotConnection::haveTheDatapack);
    client->haveShowDisconnectionReason=false;
    client->have_informations=false;
    client->number=number;
    client->selectedCharacter=false;
    number++;
    apiToCatchChallengerClient[client->api]=client;
    connectedSocketToCatchChallengerClient[client->socket]=client;
    tryLink(client);
}

void MultipleBotConnection::sslErrors(const QList<QSslError> &errors)
{
    QSslSocket *senderObject = qobject_cast<QSslSocket *>(QObject::sender());
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

void MultipleBotConnection::on_characterSelect_clicked()
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

void MultipleBotConnection::sslHandcheckIsFinished()
{
    QSslSocket *socket=qobject_cast<QSslSocket *>(QObject::sender());
    if(socket==NULL)
        return;
    connectTheExternalSocket(sslSocketToCatchChallengerClient[socket]);
}

void MultipleBotConnection::readForFirstHeader()
{
    QSslSocket *socket=qobject_cast<QSslSocket *>(QObject::sender());
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
            QObject::connect(socket,&QSslSocket::encrypted,this,&MultipleBotConnection::sslHandcheckIsFinished);
        }
        else
            connectTheExternalSocket(client);
    }
}
