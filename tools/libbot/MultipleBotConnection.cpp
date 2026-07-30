#include "MultipleBotConnection.h"
#include "BotAbort.h"

#include <QCoreApplication>
#include <QNetworkProxy>
#include <iostream>

/* ------------------------------- the link ------------------------------- */
QtBotClientLink::QtBotClientLink() :
    sslSocket(NULL),
    socket(NULL),
    apiClient(NULL)
{
}

QtBotClientLink::~QtBotClientLink()
{
    //Only the protocol object is destroyed, exactly as the previous
    //MultipleBotConnection destructor did: ConnectedSocket and QSslSocket are
    //left alone on purpose (their ownership/parenting is Qt side and destroying
    //them here would risk a double free).
    delete apiClient;
    apiClient=NULL;
}

CatchChallenger::Api_protocol *QtBotClientLink::api() const
{
    return apiClient;
}

bool QtBotClientLink::connectToHost(const std::string &host,const uint16_t port,
                                    const std::string &proxyHost,const uint16_t proxyPort)
{
    if(sslSocket==NULL)
    {
        std::cerr << "QtBotClientLink::connectToHost(): sslSocket==NULL" << std::endl;
        return false;
    }
    QNetworkProxy proxyObject;
    if(!proxyHost.empty())
    {
        #ifdef CATCHCHALLENGER_BOT_TESTCONNECT
        qDebug() << "use proxy: " << QString::fromStdString(proxyHost) << ":" << proxyPort;
        #endif
        proxyObject.setType(QNetworkProxy::Socks5Proxy);
        proxyObject.setHostName(QString::fromStdString(proxyHost));
        proxyObject.setPort(proxyPort);
    }
    else
        proxyObject.setType(QNetworkProxy::NoProxy);
    sslSocket->setProxy(proxyObject);
    #ifdef CATCHCHALLENGER_BOT_TESTCONNECT
    qDebug() << "Try connect on: " << QString::fromStdString(host) << ":" << port;
    #endif
    sslSocket->connectToHost(QString::fromStdString(host),port);
    return true;
}

void QtBotClientLink::disconnectFromHost()
{
    if(socket!=NULL)
        socket->disconnectFromHost();
}

void QtBotClientLink::sendDatapackContentMainSub()
{
    if(apiClient!=NULL)
        apiClient->sendDatapackContentMainSub();
}

/* ---------------------------- the front-end ----------------------------- */
MultipleBotConnection::CatchChallengerClient::CatchChallengerClient() :
    api(NULL)
{
}

MultipleBotConnection::MultipleBotConnection()
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<QList<quint32> >("QList<quint32>");
    qRegisterMetaType<QList<CatchChallenger::CharacterEntry> >("QList<CatchChallenger::CharacterEntry>");
    qRegisterMetaType<std::vector<quint32> >("std::vector<quint32>");
    qRegisterMetaType<std::vector<CatchChallenger::CharacterEntry> >("std::vector<CatchChallenger::CharacterEntry>");
    qRegisterMetaType<std::vector<CatchChallenger::ServerFromPoolForDisplay*> >("std::vector<CatchChallenger::CharacterEntry>");

    //Connect the creation-timer tick ONCE here, with the return checked: this
    //is the first and only connect, so a false return is a genuine failure.
    //ifMultipleConnexionStartCreation() afterwards only (re)starts the timer --
    //it must NOT re-connect, because Qt::UniqueConnection returns false on the
    //already-made connection and a late re-entry on a slow server (the ESP32,
    //after the timer was stopped) would then look like a failure when it isn't.
    if(!connect(&connectTimer,&QTimer::timeout,this,&MultipleBotConnection::connectTimerSlot,Qt::UniqueConnection))
        BOT_ABORT();
}

MultipleBotConnection::~MultipleBotConnection()
{
    //The clients themselves belong to MultipleBotConnectionCore, whose
    //destructor runs right after this one: here only the Qt indexes go.
    apiToCatchChallengerClient.clear();
    connectedSocketToCatchChallengerClient.clear();
    sslSocketToCatchChallengerClient.clear();
}

QtBotClientLink *MultipleBotConnection::qtLink(MultipleBotConnectionCore::BotClient *client)
{
    return static_cast<QtBotClientLink *>(client->link);
}

MultipleBotConnection::CatchChallengerClient * MultipleBotConnection::createClient()
{
    return static_cast<CatchChallengerClient *>(createClientCore());
}

MultipleBotConnectionCore::BotClient * MultipleBotConnection::allocClient()
{
    CatchChallengerClient * const client=new CatchChallengerClient;
    QtBotClientLink * const link=new QtBotClientLink;
    link->sslSocket=new QSslSocket();
    link->socket=new CatchChallenger::ConnectedSocket(link->sslSocket);
    link->apiClient=new CatchChallenger::Api_client_real(link->socket);
    client->link=link;
    client->api=link->apiClient;
    sslSocketToCatchChallengerClient[link->sslSocket]=client;

    if(!connect(link->sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),this,&MultipleBotConnection::sslErrors,Qt::QueuedConnection))
        BOT_ABORT();
    #if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    if(!connect(link->sslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error),this,&MultipleBotConnection::newSocketError))
        BOT_ABORT();
    #else
    if(!connect(link->sslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::errorOccurred),this,&MultipleBotConnection::newSocketError))
        BOT_ABORT();
    #endif
    return client;
}

void MultipleBotConnection::startConnectTimer(const unsigned int intervalMs)
{
    connectTimer.start(intervalMs);
}

void MultipleBotConnection::stopConnectTimer()
{
    connectTimer.stop();
}

bool MultipleBotConnection::connectTimerIsActive() const
{
    return connectTimer.isActive();
}

std::string MultipleBotConnection::datapackPath() const
{
    return QCoreApplication::applicationDirPath().toStdString()+"/datapack/";
}

void MultipleBotConnection::connectTheExternalSocket(MultipleBotConnectionCore::BotClient *client)
{
    CatchChallengerClient * const qtClient=static_cast<CatchChallengerClient *>(client);
    QtBotClientLink * const link=qtLink(client);
    if(link->apiClient==NULL)
    {
        std::cerr << "connectTheExternalSocket link->apiClient==NULL" << std::endl;
        BOT_ABORT();
    }
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::Qtinsert_player,            this,&MultipleBotConnection::insert_player))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::Qtremove_player,            this,&MultipleBotConnection::remove_player))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::QtdropAllPlayerOnTheMap,    this,&MultipleBotConnection::dropAllPlayerOnTheMap))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::QthaveCharacter,            this,&MultipleBotConnection::haveCharacter))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::Qtlogged,                   this,&MultipleBotConnection::logged))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::Qthave_current_player_info, this,&MultipleBotConnection::have_current_player_info))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::QtnewError,                 this,&MultipleBotConnection::newError))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::QtnewCharacterId,           this,&MultipleBotConnection::newCharacterId))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::QtlastReplyTime,            this,&MultipleBotConnection::lastReplyTime))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::QtnotLogged,                this,&MultipleBotConnection::notLogged))
        BOT_ABORT();
    if(!connect(link->apiClient,&CatchChallenger::Api_client_real::Qtprotocol_is_good,         this,&MultipleBotConnection::protocol_is_good))
        BOT_ABORT();
    if(!connect(link->socket,&CatchChallenger::ConnectedSocket::disconnected,                  this,&MultipleBotConnection::disconnected))
        BOT_ABORT();
    if(apiToCatchChallengerClient.isEmpty())
    {
        if(!connect(link->apiClient,&CatchChallenger::Api_client_real::QthaveTheDatapack,        this,&MultipleBotConnection::haveTheDatapack,Qt::QueuedConnection/*Qt::QueuedConnection need the fix the order of event, need datapack vs already have datapack*/))
            BOT_ABORT();
        if(!connect(link->apiClient,&CatchChallenger::Api_client_real::QthaveTheDatapackMainSub, this,&MultipleBotConnection::haveTheDatapackMainSub,Qt::QueuedConnection))
            BOT_ABORT();
        if(!connect(link->apiClient,&CatchChallenger::Api_client_real::QthaveDatapackMainSubCode,this,&MultipleBotConnection::haveTheDatapackMainSubCode,Qt::QueuedConnection))
            BOT_ABORT();
    }
    apiToCatchChallengerClient[qtClient->api]=qtClient;
    connectedSocketToCatchChallengerClient[link->socket]=qtClient;
    MultipleBotConnectionCore::connectTheExternalSocket(client);
}

/* --- progress notifications -> Qt signals ------------------------------- */
void MultipleBotConnection::notifyNumberOfBotConnected(const uint16_t value)
{
    emit emit_numberOfBotConnected(value);
}

void MultipleBotConnection::notifyNumberOfSelectedCharacter(const uint16_t value)
{
    emit emit_numberOfSelectedCharacter(value);
}

void MultipleBotConnection::notifyNumberOfStartSelectingCharacter(const uint16_t value)
{
    emit emit_numberOfStartSelectingCharacter(value);
}

void MultipleBotConnection::notifyNumberOfHaveDatapackCharacter(const uint16_t value)
{
    emit emit_numberOfHaveDatapackCharacter(value);
}

void MultipleBotConnection::notifyNumberOfStartCreatingCharacter(const uint16_t value)
{
    emit emit_numberOfStartCreatingCharacter(value);
}

void MultipleBotConnection::notifyNumberOfStartCreatedCharacter(const uint16_t value)
{
    emit emit_numberOfStartCreatedCharacter(value);
}

void MultipleBotConnection::notifyUpdateClientListStatus()
{
    emit updateClientListStatus();
}

void MultipleBotConnection::notifyAllPlayerConnected()
{
    emit emit_all_player_connected();
}

void MultipleBotConnection::notifyAllPlayerOnMap()
{
    emit emit_all_player_on_map();
}

/* --- Qt slots: resolve the sender, then call the core -------------------- */
void MultipleBotConnection::disconnected()
{
    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(QObject::sender());
    if(senderObject==NULL)
    {
        mHaveAnError=true;
        std::cerr << "disconnected(): error, from unknown" << std::endl;
        return;
    }
    if(!connectedSocketToCatchChallengerClient.contains(senderObject))
    {
        mHaveAnError=true;
        std::cerr << "disconnected(): error, from unknown (not found)" << std::endl;
        return;
    }
    disconnected_with_client(connectedSocketToCatchChallengerClient.value(senderObject));
}

void MultipleBotConnection::lastReplyTime(const quint32 &time)
{
    emit emit_lastReplyTime(time);
}

void MultipleBotConnection::notLogged(const std::string &reason)
{
    notLoggedInternal(reason);
}

void MultipleBotConnection::connectTimerSlot()
{
    connectTimerTick();
}

void MultipleBotConnection::haveCharacter(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
    {
        std::cerr << "MultipleBotConnection::haveCharacter() but sender not resolved" << std::endl;
        return;
    }
    if(!apiToCatchChallengerClient.contains(senderObject))
    {
        std::cerr << "MultipleBotConnection::haveCharacter() but !apiToCatchChallengerClient.contains(senderObject)" << std::endl;
        return;
    }
    haveCharacter_with_client(apiToCatchChallengerClient.value(senderObject),mapId,x,y,direction);
}
