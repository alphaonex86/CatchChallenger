#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "MultipleBotConnection.h"

#include <QNetworkProxy>
#include <QPluginLoader>

MultipleBotConnection::MultipleBotConnection() :
    botInterface(NULL),
    haveEnError(false),
    charactersGroupIndex(0),
    serverUniqueKey(0),
    serverIsSelected(false)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<QList<quint32> >("QList<quint32>");
    qRegisterMetaType<QList<CatchChallenger::CharacterEntry> >("QList<CatchChallenger::CharacterEntry>");
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    numberToChangeLoginForMultipleConnexion=1;
    numberOfBotConnected=0;
    numberOfSelectedCharacter=0;
    haveEnError=false;
}

MultipleBotConnection::~MultipleBotConnection()
{
}

QString MultipleBotConnection::getResolvedPluginName(const QString &name)
{
    #if defined(__linux__)
        return QStringLiteral("lib")+name+QStringLiteral(".so");
    #elif defined(Q_OS_MAC)
        #if defined(QT_DEBUG)
            return QStringLiteral("lib")+name+QStringLiteral("_debug.dylib");
        #else
            return QStringLiteral("lib")+name+QStringLiteral(".dylib");
        #endif
    #elif defined(_WIN32)
        #if defined(QT_DEBUG)
            return name+QStringLiteral("d.dll");
        #else
            return name+QStringLiteral(".dll");
        #endif
    #else
        #error "Platform not supported"
    #endif
}

void MultipleBotConnection::disconnected()
{
    qDebug() << "disconnected()";
    haveEnError=true;
    numberOfBotConnected--;
    emit emit_numberOfBotConnected(numberOfBotConnected);

    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject!=NULL)
    {
        if(senderObject->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2 || senderObject->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage3)
        {
            senderObject->socketDisconnectedForReconnect();
            return;
        }
    }
    else
        qDebug() << "disconnected(): error, from unknown";
}

void MultipleBotConnection::lastReplyTime(const quint32 &time)
{
    emit emit_lastReplyTime(time);
}

void MultipleBotConnection::notLogged(const QString &reason)
{
    Q_UNUSED(reason);
    haveEnError=true;
    numberOfBotConnected--;
    emit emit_numberOfBotConnected(numberOfBotConnected);

    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        i.key()->disconnectClient();
    }
}

void MultipleBotConnection::tryLink(CatchChallengerClient * client)
{
    numberOfBotConnected++;
    emit emit_numberOfBotConnected(numberOfBotConnected);

    connect(client->api,&CatchChallenger::Api_client_real::protocol_is_good,this,&MultipleBotConnection::protocol_is_good);
    if(!multipleConnexion())
    {
        client->login=login();
        client->pass=pass();
    }
    else
    {
        QString loginString=login();
        QString passString=pass();
        loginString.replace(QLatin1Literal("%NUMBER%"),QString::number(client->number));
        passString.replace(QLatin1Literal("%NUMBER%"),QString::number(client->number));
        client->login=loginString;
        client->pass=passString;
    }
}

void MultipleBotConnection::protocol_is_good_with_client(CatchChallengerClient *client)
{
    /*CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;*/

    client->api->tryLogin(client->login,client->pass);
}

//quint32,QString,quint16,quint16,quint8,quint16
void MultipleBotConnection::insert_player_with_client(CatchChallengerClient *client,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);
    Q_UNUSED(player);

    client->have_informations=true;
}

void MultipleBotConnection::haveCharacter()
{
}

void MultipleBotConnection::logged_with_client(CatchChallengerClient *client)
{
    if(!serverIsSelected)
    {
        qDebug() << "!serverIsSelected";
        return;
    }
    if(client->charactersList.at(charactersGroupIndex).count()<=0)
    {
        qDebug() << client->login << "have not character";
        if((autoCreateCharacter() || multipleConnexion()) && serverIsSelected)
        {
            qDebug() << client->login << "create new character";
            quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
            QString pseudo="bot"+QString::fromStdString(CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettingsCommon::commonSettingsCommon.max_pseudo_size-3));
            quint32 skinId;
            const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
            if(!profile.forcedskin.empty())
                skinId=profile.forcedskin.at(rand()%profile.forcedskin.size());
            else
                skinId=rand()%skinsList.size();
            client->api->addCharacter(charactersGroupIndex,profileIndex,pseudo,skinId);
        }
        return;
    }
    if(multipleConnexion() && serverIsSelected)
    {
        const quint32 &character_id=client->charactersList.at(charactersGroupIndex).at(rand()%client->charactersList.at(charactersGroupIndex).size()).character_id;
        if(!characterOnMap.contains(character_id))
        {
            characterOnMap << character_id;
            if(!client->api->selectCharacter(charactersGroupIndex,serverUniqueKey,character_id))
                qDebug() << "Unable to do automatic character selection:" << character_id;
        }
        return;
    }
}

void MultipleBotConnection::haveTheDatapack_with_client(CatchChallengerClient *client)
{
    qDebug() << "Bot version:" << botInterface->name() << botInterface->version();
    //load the datapack
    {
        CatchChallenger::CommonDatapack::commonDatapack.parseDatapack((QCoreApplication::applicationDirPath()+"/datapack/").toStdString());
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
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.empty())
    {
        qDebug() << "Profile list is empty";
        return;
    }

    if(client->charactersList.count()<=0 || charactersGroupIndex>=client->charactersList.size() || client->charactersList.at(charactersGroupIndex).empty())
    {
        qDebug() << client->login << "have not character";
        if(autoCreateCharacter() || multipleConnexion())
        {
            qDebug() << client->login << "create new character";
            quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
            QString pseudo="bot"+QString::fromStdString(CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettingsCommon::commonSettingsCommon.max_pseudo_size-3));
            quint32 skinId;
            const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
            if(!profile.forcedskin.empty())
                skinId=profile.forcedskin.at(rand()%profile.forcedskin.size());
            else
                skinId=rand()%skinsList.size();
            client->api->addCharacter(charactersGroupIndex,profileIndex,pseudo,skinId);
        }
        return;
    }
    //ifMultipleConnexionStartCreation();
    //the actual client
    const quint32 &character_id=client->charactersList.at(charactersGroupIndex).at(rand()%client->charactersList.at(charactersGroupIndex).size()).character_id;
    if(!characterOnMap.contains(character_id))
    {
        characterOnMap << character_id;
        if(multipleConnexion() && serverIsSelected)
        {
            if(!serverIsSelected)
            {
                qDebug() << "if(!serverIsSelected) for multipleConnexion()";
                abort();
            }
            if(!client->api->selectCharacter(charactersGroupIndex,serverUniqueKey,character_id))
                qDebug() << "Unable to select character after datapack loading:" << character_id;
            else
                qDebug() << "Select character after datapack loading:" << character_id;
        }
    }
}

void MultipleBotConnection::haveTheDatapackMainSub_with_client(CatchChallengerClient *client)
{
    Q_UNUSED(client);
    qDebug() << "Bot version:" << botInterface->name() << botInterface->version();
    //load the datapack
    {
        CatchChallenger::CommonDatapack::commonDatapack.parseDatapack((QCoreApplication::applicationDirPath()+"/datapack/").toStdString());
    }
    ifMultipleConnexionStartCreation();
}

void MultipleBotConnection::ifMultipleConnexionStartCreation()
{
    if(multipleConnexion() && !connectTimer.isActive())
    {
        connect(&connectTimer,&QTimer::timeout,this,&MultipleBotConnection::connectTimerSlot);
        connectTimer.start(1000/connectBySeconds());
        return;
    }
}

void MultipleBotConnection::connectTimerSlot()
{
    if(apiToCatchChallengerClient.size()<connexionCount() && numberOfBotConnected<connexionCount())
    {
        const quint32 &diff=numberOfBotConnected-numberOfSelectedCharacter;
        if(diff<=(quint32)maxDiffConnectedSelected())
            createClient();
    }
    else
    {
        emit emit_all_player_connected();
        connectTimer.stop();
    }
}

void MultipleBotConnection::newCharacterId_with_client(MultipleBotConnection::CatchChallengerClient *client, const quint8 &returnCode, const quint32 &characterId)
{
    if(returnCode!=0x00)
    {
        qDebug() << "new character not created, server have returned a failed: " << returnCode;
        return;
    }

    if(!characterOnMap.contains(characterId))
    {
        characterOnMap << characterId;
        if(!client->api->selectCharacter(charactersGroupIndex,serverUniqueKey,characterId))
            qDebug() << "Unable to select character after creation:" << characterId;
        else
        {
            qDebug() << "Select new character created after creation:" << characterId;
            ifMultipleConnexionStartCreation();
        }
    }
    else
        qDebug() << client->login << "new character is already on map";
}

void MultipleBotConnection::have_current_player_info_with_client(CatchChallengerClient *client,const CatchChallenger::Player_private_and_public_informations &informations)
{
    client->selectedCharacter=true;
    numberOfSelectedCharacter++;
    emit emit_numberOfSelectedCharacter(numberOfSelectedCharacter);

    const quint32 &diff=numberOfBotConnected-numberOfSelectedCharacter;
    if(diff==0 && numberOfSelectedCharacter>=connexionCount())
        emit emit_all_player_on_map();

    Q_UNUSED(informations);
//    DebugClass::debugConsole(QStringLiteral("MultipleBotConnection::have_current_player_info() pseudo: %1").arg(informations.public_informations.pseudo));
}

void MultipleBotConnection::newError_with_client(CatchChallengerClient *client, QString error,QString detailedError)
{
    Q_UNUSED(error);
    Q_UNUSED(detailedError);
    haveEnError=true;
    client->socket->disconnectFromHost();
}

void MultipleBotConnection::newSocketError_with_client(CatchChallengerClient *client, QAbstractSocket::SocketError error)
{
    qDebug() << "newSocketError()" << error;
    haveEnError=true;
    Q_UNUSED(client);
}

void MultipleBotConnection::createClient()
{
    if(haveEnError)
        return;
    CatchChallengerClient * client=new CatchChallengerClient;

    QSslSocket *sslSocket=new QSslSocket();

    QNetworkProxy proxyObject;
    if(!proxy().isEmpty())
    {
        proxyObject.setType(QNetworkProxy::Socks5Proxy);
        proxyObject.setHostName(proxy());
        proxyObject.setPort(proxyport());
        sslSocket->setProxy(proxyObject);
    }

    client->sslSocket=sslSocket;
    sslSocketToCatchChallengerClient[client->sslSocket]=client;
    client->socket=new CatchChallenger::ConnectedSocket(client->sslSocket);
    client->api=new CatchChallenger::Api_client_real(client->socket,false);

    connect(sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),this,&MultipleBotConnection::sslErrors,Qt::QueuedConnection);
    connect(sslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error),this,&MultipleBotConnection::newSocketError);
    sslSocket->connectToHost(host(),port());
    connectTheExternalSocket(client);
}

void MultipleBotConnection::connectTheExternalSocket(CatchChallengerClient * client)
{
    client->api->setDatapackPath(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/"));
    connect(client->api,&CatchChallenger::Api_client_real::insert_player,            this,&MultipleBotConnection::insert_player);
    connect(client->api,&CatchChallenger::Api_client_real::haveCharacter,            this,&MultipleBotConnection::haveCharacter);
    connect(client->api,&CatchChallenger::Api_client_real::logged,                   this,&MultipleBotConnection::logged);
    connect(client->api,&CatchChallenger::Api_client_real::have_current_player_info, this,&MultipleBotConnection::have_current_player_info);
    connect(client->api,&CatchChallenger::Api_client_real::newError,                 this,&MultipleBotConnection::newError);
    connect(client->api,&CatchChallenger::Api_client_real::newCharacterId,           this,&MultipleBotConnection::newCharacterId);
    connect(client->api,&CatchChallenger::Api_client_real::lastReplyTime,            this,&MultipleBotConnection::lastReplyTime);
    connect(client->api,&CatchChallenger::Api_client_real::notLogged,                this,&MultipleBotConnection::notLogged);
    connect(client->socket,&CatchChallenger::ConnectedSocket::disconnected,          this,&MultipleBotConnection::disconnected);
    if(apiToCatchChallengerClient.isEmpty())
    {
        connect(client->api,&CatchChallenger::Api_client_real::haveTheDatapack,      this,&MultipleBotConnection::haveTheDatapack);
        connect(client->api,&CatchChallenger::Api_client_real::haveTheDatapackMainSub,      this,&MultipleBotConnection::haveTheDatapackMainSub);
    }
    client->haveShowDisconnectionReason=false;
    client->have_informations=false;
    client->number=numberToChangeLoginForMultipleConnexion;
    client->selectedCharacter=false;
    numberToChangeLoginForMultipleConnexion++;
    apiToCatchChallengerClient[client->api]=client;
    connectedSocketToCatchChallengerClient[client->socket]=client;
    tryLink(client);
}
