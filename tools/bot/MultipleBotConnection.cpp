#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../client/qt/FacilityLibClient.hpp"
#include "MultipleBotConnection.h"

#include <QNetworkProxy>
#include <QPluginLoader>
#include <iostream>

MultipleBotConnection::MultipleBotConnection() :
    botInterface(NULL),
    mHaveAnError(false),
    charactersGroupIndex(0),
    serverUniqueKey(-1),
    serverIsSelected(false)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<QList<quint32> >("QList<quint32>");
    qRegisterMetaType<QList<CatchChallenger::CharacterEntry> >("QList<CatchChallenger::CharacterEntry>");
    qRegisterMetaType<std::vector<quint32> >("std::vector<quint32>");
    qRegisterMetaType<std::vector<CatchChallenger::CharacterEntry> >("std::vector<CatchChallenger::CharacterEntry>");
    qRegisterMetaType<std::vector<CatchChallenger::ServerFromPoolForDisplay*> >("std::vector<CatchChallenger::CharacterEntry>");

    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    numberToChangeLoginForMultipleConnexion=1;
    numberOfBotConnected=0;
    numberOfSelectedCharacter=0;
    mHaveAnError=false;
}

MultipleBotConnection::~MultipleBotConnection()
{
    QHashIterator<QSslSocket *,CatchChallengerClient *> i(sslSocketToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        delete i.value()->api;
        delete i.value();
    }
    apiToCatchChallengerClient.clear();
    connectedSocketToCatchChallengerClient.clear();
    sslSocketToCatchChallengerClient.clear();
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

    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(QObject::sender());
    if(senderObject!=NULL)
    {
        if(connectedSocketToCatchChallengerClient.contains(senderObject))
        {
            CatchChallengerClient * catchChallengerClient=connectedSocketToCatchChallengerClient.value(senderObject);
            CatchChallenger::Api_client_real *api=catchChallengerClient->api;
            if(api!=NULL)
            {
                if(api->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2 ||
                        api->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage3)
                {
                    qDebug() << "disconnected(): For reason: api->socketDisconnectedForReconnect()";
                    api->socketDisconnectedForReconnect();
                    return;
                }
                else
                {
                    qDebug() << "disconnected(): For reason: " << connectedSocketToCatchChallengerClient[senderObject];
                    mHaveAnError=true;
                    if(catchChallengerClient->haveBeenDiscounted==false)
                    {
                        connectedSocketToCatchChallengerClient[senderObject]->haveBeenDiscounted=true;
                        numberOfBotConnected--;
                        emit emit_numberOfBotConnected(numberOfBotConnected);
                        std::cout << "disconnected(): numberOfBotConnected--: " << connectedSocketToCatchChallengerClient[senderObject]->api->player_informations.public_informations.pseudo << std::endl;
                    }
                }
            }
            else
            {
                mHaveAnError=true;
                qDebug() << "disconnected(): error, api null";
            }
        }
        else
        {
            mHaveAnError=true;
            qDebug() << "disconnected(): error, from unknown (not found)";
        }
    }
    else
    {
        mHaveAnError=true;
        qDebug() << "disconnected(): error, from unknown";
    }
}

void MultipleBotConnection::lastReplyTime(const quint32 &time)
{
    emit emit_lastReplyTime(time);
}

void MultipleBotConnection::notLogged(const std::string &reason)
{
    Q_UNUSED(reason);
    mHaveAnError=true;

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
    //std::cout << "MultipleBotConnection::tryLink(): numberOfBotConnected++: multipleConnexion():" << std::to_string(multipleConnexion()) << std::endl;

    if(client->api==NULL)
    {
        std::cerr << "tryLink client->api==NULL" << std::endl;
        abort();
    }
    if(!connect(client->api,&CatchChallenger::Api_client_real::Qtprotocol_is_good,this,&MultipleBotConnection::protocol_is_good))
        abort();
    if(!multipleConnexion())
    {
        client->login=login();
        client->pass=pass();
    }
    else
    {
        QString loginString=login();
        QString passString=pass();
        loginString.replace(QStringLiteral("%NUMBER%"),QString::number(client->number));
        passString.replace(QStringLiteral("%NUMBER%"),QString::number(client->number));
        client->login=loginString;
        client->pass=passString;
    }
}

void MultipleBotConnection::protocol_is_good_with_client(CatchChallengerClient *client)
{
    /*CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;*/

    if(client->api==NULL)
    {
        std::cerr << "protocol_is_good_with_client client->api==NULL" << std::endl;
        abort();
    }
    client->api->tryLogin(client->login.toStdString(),client->pass.toStdString());
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

std::string MultipleBotConnection::getNewPseudo()
{
    return std::string("bot")+CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettingsCommon::commonSettingsCommon.max_pseudo_size-3);
}

void MultipleBotConnection::logged_with_client(CatchChallengerClient *client)
{
    if(!serverIsSelected)
    {
        qDebug() << "!serverIsSelected";
        return;
    }
    if(client->api==NULL)
    {
        std::cerr << "logged_with_client client->api==NULL" << std::endl;
        abort();
    }
    if(client->charactersList.empty() || client->charactersList.at(charactersGroupIndex).size()<=0)
    {
        qDebug() << client->login << "have not character";
        if((autoCreateCharacter() || multipleConnexion()) && serverIsSelected)
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.profileList.empty())
                qDebug() << "Profile list is empty for initial creation";
            else
            {
                qDebug() << client->login << "create new character";
                quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
                QString pseudo=QString::fromStdString(getNewPseudo());
                uint8_t skinId;
                const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
                if(!profile.forcedskin.empty())
                    skinId=profile.forcedskin.at(rand()%profile.forcedskin.size());
                else
                    skinId=rand()%skinsList.size();
                uint8_t monstergroupId=rand()%profile.monstergroup.size();
                client->api->addCharacter(charactersGroupIndex,profileIndex,pseudo.toStdString(),monstergroupId,skinId);
            }
        }
        return;
    }
    if(multipleConnexion() && serverIsSelected)
    {
        if(!client->charactersList.empty() && charactersGroupIndex<client->charactersList.size())
        {
            const quint32 &character_id=client->charactersList.at(charactersGroupIndex).at(rand()%client->charactersList.at(charactersGroupIndex).size()).character_id;
            if(!characterOnMap.contains(character_id))
            {
                if(!client->api->selectCharacter(charactersGroupIndex,serverUniqueKey,character_id))
                    qDebug() << "Unable to do automatic character selection:" << character_id;
                else
                {
                    //if not the first bot
                    if(!tempMapList.empty())
                    {
                        //then do fake init normaly done into haveDatapackMainSubCode_with_client(
                        if(!client->api->setMapNumber(tempMapList.size()))
                            abort();
                        client->api->have_main_and_sub_datapack_loaded();
                    }
                    //qDebug() << "MultipleBotConnection::logged_with_client(): Add char on map: Manual select character:" << character_id;
                    characterOnMap << character_id;
                }
            }
        }
        return;
    }
}

void MultipleBotConnection::haveTheDatapack_with_client(CatchChallengerClient *client)
{
    if(client->api==NULL)
    {
        std::cerr << "haveTheDatapack_with_client client->api==NULL" << std::endl;
        abort();
    }
    if(botInterface!=NULL)
        qDebug() << "MultipleBotConnection::haveTheDatapack_with_client(): Bot version:" << botInterface->name() << botInterface->version();
    //load the profil list
    {
        CatchChallenger::CommonDatapack::commonDatapack.parseDatapack((QCoreApplication::applicationDirPath()+"/datapack/").toStdString());//load always after the rates
        //load the skins list
        QDir dir(QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/skin/fighter/"));
        QFileInfoList entryList=dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
        int index=0;
        while(index<entryList.size())
        {
            skinsList << entryList.at(index);
            index++;
        }
    }

    if(client->charactersList.size()<=0 || charactersGroupIndex>=client->charactersList.size() || client->charactersList.at(charactersGroupIndex).empty())
    {
        if(serverIsSelected)
        {
            qDebug() << client->login << "have not character";
            if(autoCreateCharacter() || multipleConnexion())
            {
                if(CatchChallenger::CommonDatapack::commonDatapack.profileList.empty())
                {
                    qDebug() << "Profile list is empty";
                    return;
                }
                qDebug() << client->login << "create new character";
                quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
                QString pseudo=QString::fromStdString(getNewPseudo());
                uint8_t skinId;
                const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
                if(!profile.forcedskin.empty())
                    skinId=profile.forcedskin.at(rand()%profile.forcedskin.size());
                else
                    skinId=rand()%skinsList.size();
                uint8_t monstergroupId=rand()%profile.monstergroup.size();
                client->api->addCharacter(charactersGroupIndex,profileIndex,pseudo.toStdString(),monstergroupId,skinId);
            }
        }
        else
        {
            qDebug() << client->login << "have no character and no server selected: " << client->charactersList.size() << ", charactersGroupIndex: " << charactersGroupIndex;
            if(client->charactersList.size()>0 && charactersGroupIndex<client->charactersList.size())
                qDebug() << client->login << "client->charactersList.at(charactersGroupIndex).empty()";
        }
        return;
    }
    //ifMultipleConnexionStartCreation();
    //the actual client
    const quint32 &character_id=client->charactersList.at(charactersGroupIndex).at(rand()%client->charactersList.at(charactersGroupIndex).size()).character_id;
    if(!characterOnMap.contains(character_id))
    {
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
            {
                qDebug() << "haveTheDatapack_with_client: Manual select character:" << character_id;
                characterOnMap << character_id;
                qDebug() << "Select character after datapack loading:" << character_id;
            }
        }
    }
}

void MultipleBotConnection::haveDatapackMainSubCode_with_client(CatchChallengerClient *client)
{
    if(botInterface!=NULL)
        qDebug() << "MultipleBotConnection::haveDatapackMainSubCode_with_client(): Bot version:" << botInterface->name() << botInterface->version();
    if(client->api==NULL)
    {
        std::cerr << "haveDatapackMainSubCode_with_client client->api==NULL" << std::endl;
        abort();
    }
    client->api->sendDatapackContentMainSub();
}

void MultipleBotConnection::haveTheDatapackMainSub_with_client(CatchChallengerClient *client)
{
    if(client->api==NULL)
    {
        std::cerr << "haveTheDatapackMainSub_with_client client->api==NULL" << std::endl;
        abort();
    }
    Q_UNUSED(client);
    if(botInterface!=NULL)
        qDebug() << "MultipleBotConnection::haveTheDatapackMainSub_with_client(): Bot version:" << botInterface->name() << botInterface->version();
    {
        if(CommonSettingsServer::commonSettingsServer.mainDatapackCode=="[main]")
        {
            qDebug() << "CommonSettingsServer::commonSettingsServer.mainDatapackCode==[main]";
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            abort();
            #else
            return;
            #endif
        }
        if(CommonSettingsServer::commonSettingsServer.subDatapackCode=="[sub]")
        {
            qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode==[sub]";
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            abort();
            #else
            return;
            #endif
        }
    }
    //load the datapack
    if(!tempMapList.isEmpty())
    {
        qDebug() << "MultipleBotConnection::haveTheDatapackMainSub_with_client() !tempMapList.isEmpty() internal bug";
        abort();
    }
    {
        const QString &datapackPath=QCoreApplication::applicationDirPath()+"/datapack/";
        CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapackPath.toStdString());
        CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapack(datapackPath.toStdString(),CommonSettingsServer::commonSettingsServer.mainDatapackCode,CommonSettingsServer::commonSettingsServer.subDatapackCode);

        const std::vector<std::string> &returnList=
                    CatchChallenger::FacilityLibGeneral::listFolder(
                        (datapackPath.toStdString()+"map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/")
                        );

        //load the map
        const int &size=returnList.size();
        int index=0;
        QRegularExpression mapFilter(QStringLiteral("\\.tmx$"));
        QRegularExpression mapExclude(QStringLiteral("[\"']"));
        QHash<QString,QString> sortToFull;
        while(index<size)
        {
            const QString &fileName=QString::fromStdString(returnList.at(index));
            QString sortFileName=fileName;
            if(fileName.contains(mapFilter) && !fileName.contains(mapExclude))
            {
                sortFileName.remove(mapFilter);
                sortToFull[sortFileName]=fileName;
                tempMapList << sortFileName;
            }
            index++;
        }
    }
    if(!client->api->setMapNumber(tempMapList.size()))
        abort();
    client->api->have_main_and_sub_datapack_loaded();
    ifMultipleConnexionStartCreation();
}

void MultipleBotConnection::ifMultipleConnexionStartCreation()
{
    if(multipleConnexion())
    {
        if(!connectTimer.isActive())
        {
            qDebug() << "MultipleBotConnection::ifMultipleConnexionStartCreation(): start the multiple timer co";
            if(!connect(&connectTimer,&QTimer::timeout,this,&MultipleBotConnection::connectTimerSlot))
                abort();
            connectTimer.start(1000/connectBySeconds());
            return;
        }
        else
            qDebug() << "MultipleBotConnection::ifMultipleConnexionStartCreation(): connectTimer.isActive()";
    }
    else
    {
        qDebug() << "MultipleBotConnection::ifMultipleConnexionStartCreation(): !multipleConnexion()";
        emit emit_all_player_connected();
    }
}

void MultipleBotConnection::connectTimerSlot()
{
    const auto connexionCountVar=connexionCountTarget();
    if(apiToCatchChallengerClient.size()<connexionCountVar && numberOfBotConnected<connexionCountVar)
    {
        if(numberOfBotConnected<numberOfSelectedCharacter)
        {
            qDebug() << "MultipleBotConnection::connectTimerSlot(): numberOfBotConnected(" << numberOfBotConnected << ")<numberOfSelectedCharacter(" << numberOfSelectedCharacter << ")";
            mHaveAnError=true;
            connectTimer.stop();
        }
        else
        {
            //qDebug() << "MultipleBotConnection::connectTimerSlot(): ping";
            const quint32 &diff=numberOfBotConnected-numberOfSelectedCharacter;
            if(diff<=(quint32)maxDiffConnectedSelected())
            {
                createClient();
                //qDebug() << "MultipleBotConnection::connectTimerSlot(): createClient()";
            }
        }
    }
    else
    {
        qDebug() << "MultipleBotConnection::connectTimerSlot(): finish, stop it";
        emit emit_all_player_connected();
        connectTimer.stop();
    }
}

void MultipleBotConnection::newCharacterId_with_client(MultipleBotConnection::CatchChallengerClient *client, const quint8 &returnCode, const quint32 &characterId)
{
    if(client->api==NULL)
    {
        std::cerr << "newCharacterId_with_client client->api==NULL" << std::endl;
        abort();
    }
    if(returnCode!=0x00)
    {
        qDebug() << "new character not created, server have returned a failed: " << returnCode;
        return;
    }
    if(serverUniqueKey==-1)
    {
        qDebug() << "Unable to select the freshly created char because don't have select the server: " << returnCode;
        return;
    }

    if(!characterOnMap.contains(characterId))
    {
        if(!client->api->selectCharacter(charactersGroupIndex,serverUniqueKey,characterId))
            qDebug() << "Unable to select character after creation:" << characterId;
        else
        {
            qDebug() << "add character on map: " << characterId << " at " << __FILE__ << ":" << __LINE__;
            characterOnMap << characterId;
            qDebug() << "Select new character created after creation:" << characterId;
            ifMultipleConnexionStartCreation();
        }
    }
    else
        qDebug() << client->login << "new character is already on map";
}

void MultipleBotConnection::have_current_player_info_with_client(CatchChallengerClient *client,const CatchChallenger::Player_private_and_public_informations &informations)
{
    if(client->selectedCharacter==true)
        return;
    client->selectedCharacter=true;
    numberOfSelectedCharacter++;
    emit emit_numberOfSelectedCharacter(numberOfSelectedCharacter);

    if(multipleConnexion())
    {
        if(numberOfBotConnected>=numberOfSelectedCharacter)
        {
            const quint32 &diff=numberOfBotConnected-numberOfSelectedCharacter;
            if(diff==0 && numberOfSelectedCharacter>=connexionCountTarget())
                emit emit_all_player_on_map();
        }
    }
    else
        emit emit_all_player_on_map();

    Q_UNUSED(informations);
    //std::cout << "MultipleBotConnection::have_current_player_info() pseudo: " << informations.public_informations.pseudo << std::endl;
}

void MultipleBotConnection::newError_with_client(CatchChallengerClient *client, QString error,QString detailedError)
{
    Q_UNUSED(error);
    Q_UNUSED(detailedError);
    mHaveAnError=true;
    client->socket->disconnectFromHost();
}

void MultipleBotConnection::newSocketError_with_client(CatchChallengerClient *client, QAbstractSocket::SocketError error)
{
    qDebug() << "newSocketError()" << error;
    mHaveAnError=true;
    Q_UNUSED(client);
}

bool MultipleBotConnection::haveAnError()
{
    return mHaveAnError;
}

MultipleBotConnection::CatchChallengerClient * MultipleBotConnection::createClient()
{
    if(mHaveAnError)
    {
        connectTimer.stop();
        return NULL;
    }
    CatchChallengerClient * client=new CatchChallengerClient;

    QSslSocket *sslSocket=new QSslSocket();

    QNetworkProxy proxyObject;
    if(!proxy().isEmpty())
    {
        #ifdef BOTTESTCONNECT
        qDebug() << "use proxy: " << proxy() << ":" << proxyport();
        #endif
        proxyObject.setType(QNetworkProxy::Socks5Proxy);
        proxyObject.setHostName(proxy());
        proxyObject.setPort(proxyport());
        sslSocket->setProxy(proxyObject);
    }
    else
    {
        proxyObject.setType(QNetworkProxy::NoProxy);
        sslSocket->setProxy(proxyObject);
    }

    client->sslSocket=sslSocket;
    sslSocketToCatchChallengerClient[client->sslSocket]=client;
    client->socket=new CatchChallenger::ConnectedSocket(client->sslSocket);
    client->api=new CatchChallenger::Api_client_real(client->socket);
    client->preferences.plant=100;
    client->preferences.item=100;
    client->preferences.fight=100;
    client->preferences.shop=100;
    client->preferences.wild=100;

    if(!connect(sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),this,&MultipleBotConnection::sslErrors,Qt::QueuedConnection))
        abort();
    if(!connect(sslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::errorOccurred),this,&MultipleBotConnection::newSocketError))
        abort();
    #ifdef BOTTESTCONNECT
    qDebug() << "Try connect on: " << host() << ":" << port();
    #endif
    sslSocket->connectToHost(host(),port());
    connectTheExternalSocket(client);
    return client;
}

void MultipleBotConnection::connectTheExternalSocket(CatchChallengerClient * client)
{
    if(client->api==NULL)
    {
        std::cerr << "connectTheExternalSocket client->api==NULL" << std::endl;
        abort();
    }
    client->api->setDatapackPath(QCoreApplication::applicationDirPath().toStdString()+"/datapack/");
    if(!connect(client->api,&CatchChallenger::Api_client_real::Qtinsert_player,            this,&MultipleBotConnection::insert_player))
        abort();
    if(!connect(client->api,&CatchChallenger::Api_client_real::Qtremove_player,            this,&MultipleBotConnection::remove_player))
        abort();
    if(!connect(client->api,&CatchChallenger::Api_client_real::QtdropAllPlayerOnTheMap,    this,&MultipleBotConnection::dropAllPlayerOnTheMap))
        abort();
    if(!connect(client->api,&CatchChallenger::Api_client_real::QthaveCharacter,            this,&MultipleBotConnection::haveCharacter))
        abort();
    if(!connect(client->api,&CatchChallenger::Api_client_real::Qtlogged,                   this,&MultipleBotConnection::logged))
        abort();
    if(!connect(client->api,&CatchChallenger::Api_client_real::Qthave_current_player_info, this,&MultipleBotConnection::have_current_player_info))
        abort();
    if(!connect(client->api,&CatchChallenger::Api_client_real::QtnewError,                 this,&MultipleBotConnection::newError))
        abort();
    if(!connect(client->api,&CatchChallenger::Api_client_real::QtnewCharacterId,           this,&MultipleBotConnection::newCharacterId))
        abort();
    if(!connect(client->api,&CatchChallenger::Api_client_real::QtlastReplyTime,            this,&MultipleBotConnection::lastReplyTime))
        abort();
    if(!connect(client->api,&CatchChallenger::Api_client_real::QtnotLogged,                this,&MultipleBotConnection::notLogged))
        abort();
    if(!connect(client->socket,&CatchChallenger::ConnectedSocket::disconnected,          this,&MultipleBotConnection::disconnected))
        abort();
    if(apiToCatchChallengerClient.isEmpty())
    {
        if(!connect(client->api,&CatchChallenger::Api_client_real::QthaveTheDatapack,         this,&MultipleBotConnection::haveTheDatapack,Qt::QueuedConnection/*Qt::QueuedConnection need the fix the order of event, need datapack vs already have datapack*/))
            abort();
        if(!connect(client->api,&CatchChallenger::Api_client_real::QthaveTheDatapackMainSub,  this,&MultipleBotConnection::haveTheDatapackMainSub,Qt::QueuedConnection))
            abort();
        if(!connect(client->api,&CatchChallenger::Api_client_real::QthaveDatapackMainSubCode,  this,&MultipleBotConnection::haveTheDatapackMainSubCode,Qt::QueuedConnection))
            abort();
    }
    client->haveShowDisconnectionReason=false;
    client->haveBeenDiscounted=false;
    client->have_informations=false;
    client->number=numberToChangeLoginForMultipleConnexion;
    client->selectedCharacter=false;
    numberToChangeLoginForMultipleConnexion++;
    apiToCatchChallengerClient[client->api]=client;
    connectedSocketToCatchChallengerClient[client->socket]=client;
    tryLink(client);
}
