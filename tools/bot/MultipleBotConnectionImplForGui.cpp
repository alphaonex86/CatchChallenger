#include "MultipleBotConnectionImplForGui.h"

#include <iostream>
#ifdef QT_GUI_LIB
#include <QMessageBox>
#endif

bool MultipleBotConnectionImplForGui::displayingError=false;

MultipleBotConnectionImplForGui::MultipleBotConnectionImplForGui()
{
    firstCharacterSelected=false;
}

MultipleBotConnectionImplForGui::~MultipleBotConnectionImplForGui()
{
}

QString MultipleBotConnectionImplForGui::login()
{
    return mLogin;
}

QString MultipleBotConnectionImplForGui::pass()
{
    return mPass;
}

bool MultipleBotConnectionImplForGui::multipleConnexion()
{
    return mMultipleConnexion;
}

bool MultipleBotConnectionImplForGui::autoCreateCharacter()
{
    return mAutoCreateCharacter;
}

int MultipleBotConnectionImplForGui::connectBySeconds()
{
    return mConnectBySeconds;
}

int MultipleBotConnectionImplForGui::connexionCountTarget()
{
    return mConnexionCountTarget;
}

int MultipleBotConnectionImplForGui::maxDiffConnectedSelected()
{
    return mMaxDiffConnectedSelected;
}

QString MultipleBotConnectionImplForGui::proxy()
{
    return mProxy;
}

quint16 MultipleBotConnectionImplForGui::proxyport()
{
    return mProxyport;
}

QString MultipleBotConnectionImplForGui::host()
{
    return mHost;
}

quint16 MultipleBotConnectionImplForGui::port()
{
    return mPort;
}

/************************** function part ***********************************/
void MultipleBotConnectionImplForGui::detectSlowDown()
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
    emit emit_detectSlowDown(queryCount,worseTime);
}

void MultipleBotConnectionImplForGui::characterSelectForFirstCharacter(const quint32 &charId)
{
    if(!serverIsSelected)
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): !serverIsSelected (abort)";
        abort();
    }
    if(firstCharacterSelected)
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): firstCharacterSelected already selected";
        return;
    }
    else
        firstCharacterSelected=true;

    if(apiToCatchChallengerClient.size()!=1)
        qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): apiToCatchChallengerClient.size()!=1";

    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while(i.hasNext())
    {
        i.next();
        if(!characterOnMap.contains(charId))
        {
            if(i.value()->api!=NULL)
            {
                if(!i.value()->api->selectCharacter(charactersGroupIndex,serverUniqueKey,charId))
                    qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): Unable to manual select character:" << charId;
                else
                {
                    qDebug() << "add character on map: " << charId << " at " << __FILE__ << ":" << __LINE__;
                    characterOnMap << charId;
                    qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): Manual select character:" << charId;
                }
            }
            else
                qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): api null" << charId;
            return;
        }
        else
        {
            qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): BUG: characterOnMap.contains(charId): " << charId;
            return;
        }
    }

    qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): Have any first char to select:" << charId;
}

void MultipleBotConnectionImplForGui::serverSelect(const uint8_t &charactersGroupIndex,const quint32 &uniqueKey)
{
    #ifdef BOTTESTCONNECT
    qDebug() << "MultipleBotConnectionImplForGui::serverSelect(): " << charactersGroupIndex << ", uniqueKey: " << uniqueKey;
    #endif
    if(uniqueKey==0)
        qDebug() << "MultipleBotConnectionImplFoprGui::serverSelect(): uniqueKey==0, suspect bug";
    this->charactersGroupIndex=charactersGroupIndex;
    this->serverUniqueKey=uniqueKey;
    serverIsSelected=true;

    if(multipleConnexion())
    {
        QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *>::const_iterator i = apiToCatchChallengerClient.constBegin();
        if(i != apiToCatchChallengerClient.constEnd())
        {
            logged_with_client(i.value());
            return;
        }
        else
        {
            qDebug() << "MultipleBotConnectionImplFoprGui::serverSelect(): ui->characterList->count()==0 and no client found, abort()";
            abort();
        }
    }
    else
    {
        QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *>::const_iterator i = apiToCatchChallengerClient.constBegin();
        if(i != apiToCatchChallengerClient.constEnd())
        {
            logged_with_client(i.value());
            return;
        }
        else
        {
            qDebug() << "MultipleBotConnectionImplFoprGui::serverSelect(): ui->characterList->count()==0 and no client found, abort()";
            abort();
        }
    }
}

void MultipleBotConnectionImplForGui::insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;

    if(!apiToCatchChallengerClient.contains(senderObject))
    {
        std::cerr << "MultipleBotConnectionImplForGui::insert_player() !apiToCatchChallengerClient.contains(senderObject)" << std::endl;
        return;
    }
    CatchChallengerClient * catchChallengerClient=apiToCatchChallengerClient.value(senderObject);
    MultipleBotConnection::insert_player_with_client(catchChallengerClient,player,mapId,x,y,direction);

    if(catchChallengerClient->api==NULL)
        std::cerr << "MultipleBotConnectionImplForGui::insert_player() catchChallengerClient->api==NULL" << std::endl;
    else
    {
        if(botInterface!=NULL)
        {
            botInterface->insert_player_all(catchChallengerClient->api,player,mapId,x,y,direction);
            if(player.simplifiedId==catchChallengerClient->api->getId())
                botInterface->insert_player(catchChallengerClient->api,player,mapId,x,y,direction);
        }
    }
}

void MultipleBotConnectionImplForGui::remove_player(const uint16_t &id)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;
    botInterface->remove_player(senderObject,id);
}

void MultipleBotConnectionImplForGui::dropAllPlayerOnTheMap()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;
    botInterface->dropAllPlayerOnTheMap(senderObject);
}

void MultipleBotConnectionImplForGui::logged(const QList<CatchChallenger::ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::logged(): sender()==NULL";
        return;
    }

    apiToCatchChallengerClient.value(senderObject)->charactersList=characterEntryList;
    if(apiToCatchChallengerClient.size()==1)
    {
        //get the datapack
        apiToCatchChallengerClient.value(senderObject)->api->sendDatapackContentBase();
        emit loggedDone(senderObject,serverOrdenedList,characterEntryList,false);
        return;
    }
    else
    {
        logged_with_client(apiToCatchChallengerClient.value(senderObject));
        emit loggedDone(senderObject,serverOrdenedList,characterEntryList,true);
        return;
    }
}

void MultipleBotConnectionImplForGui::haveCharacter()
{
    qDebug() << "MultipleBotConnectionImplFoprGui::haveCharacter()";
    if(apiToCatchChallengerClient.size()==1)
    {
        CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
        if(senderObject==NULL)
        {
            qDebug() << "MultipleBotConnectionImplFoprGui::haveCharacter(): sender()==NULL";
            return;
        }
    }
    /*CatchChallenger::ClientFightEngine::fightEngine.public_and_private_informations.playerMonster=CatchChallenger::Api_client_real::client->player_informations.playerMonster;
    CatchChallenger::ClientFightEngine::fightEngine.setVariableContent(CatchChallenger::Api_client_real::client->get_player_informations());*/
    MultipleBotConnection::haveCharacter();
}

void MultipleBotConnectionImplForGui::connectTheExternalSocket(CatchChallengerClient * client)
{
    MultipleBotConnection::connectTheExternalSocket(client);
    if(!connect(client->api,&CatchChallenger::Api_client_real::new_chat_text,            this,&MultipleBotConnectionImplForGui::chat_text,Qt::QueuedConnection))
        abort();
}

void MultipleBotConnectionImplForGui::newCharacterId(const quint8 &returnCode, const quint32 &characterId)
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
    MultipleBotConnection::newCharacterId_with_client(apiToCatchChallengerClient[senderObject],returnCode,characterId);
}

void MultipleBotConnectionImplForGui::haveTheDatapack()
{
    qDebug() << "MultipleBotConnectionImplFoprGui::haveTheDatapack()";
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::haveTheDatapack(): senderObject==NULL";
        return;
    }

    if(!apiToCatchChallengerClient.contains(senderObject))
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::haveTheDatapack(): !apiToCatchChallengerClient.contains(senderObject)";
        return;
    }
    MultipleBotConnection::haveTheDatapack_with_client(apiToCatchChallengerClient.value(senderObject));
    #ifndef BOTTESTCONNECT
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.empty())
    {
        qDebug() << "Profile list is empty";
        return;
    }
    #endif
    emit datapackIsReady();
}

void MultipleBotConnectionImplForGui::haveTheDatapackMainSub()
{
    qDebug() << "MultipleBotConnectionImplFoprGui::haveTheDatapackMainSub()";
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::haveTheDatapackMainSub(): sender()==NULL";
        return;
    }

    MultipleBotConnection::haveTheDatapackMainSub_with_client(apiToCatchChallengerClient.value(senderObject));
    emit datapackMainSubIsReady();
}

void MultipleBotConnectionImplForGui::haveTheDatapackMainSubCode()
{
    qDebug() << "MultipleBotConnectionImplFoprGui::haveTheDatapackMainSubCode()";
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::haveTheDatapackMainSubCode(): sender()==NULL";
        return;
    }

    MultipleBotConnection::haveDatapackMainSubCode_with_client(apiToCatchChallengerClient.value(senderObject));
}

void MultipleBotConnectionImplForGui::sslErrors(const QList<QSslError> &errors)
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
    /*#ifdef QT_GUI_LIB
    QMessageBox::warning(this,tr("Ssl error"),sslErrors.join("\n"));
    realSocket->disconnectFromHost();*/
}

void MultipleBotConnectionImplForGui::protocol_is_good()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    MultipleBotConnection::protocol_is_good_with_client(apiToCatchChallengerClient.value(senderObject));
}

void MultipleBotConnectionImplForGui::newSocketError(QAbstractSocket::SocketError error)
{
    qDebug() << "newSocketError()" << error;
    haveEnError=true;

    QString errorString;
    switch(error)
    {
        case 0:
            errorString="The connection was refused by the peer (or timed out).";
        break;
        case 1:
            errorString="The remote host closed the connection. Note that the client socket (i.e., this socket) will be closed after the remote close notification has been sent.";
        break;
        case 2:
            errorString="The host address was not found.";
        break;
        case 3:
            errorString="The socket operation failed because the application lacked the required privileges.";
        break;
        case 4:
            errorString="The local system ran out of resources (e.g., too many sockets).";
        break;
        case 5:
            errorString="The socket operation timed out.";
        break;
        case 6:
            errorString="The datagram was larger than the operating system's limit (which can be as low as 8192 bytes).";
        break;
        case 7:
            errorString="An error occurred with the network (e.g., the network cable was accidentally plugged out).";
        break;
        case 8:
            errorString="The address specified to QAbstractSocket::bind() is already in use and was set to be exclusive.";
        break;
        case 9:
            errorString="The address specified to QAbstractSocket::bind() does not belong to the host.";
        break;
        case 10:
            errorString="The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support).";
        break;
        case 11:
            errorString="Used by QAbstractSocketEngine only, The last operation attempted has not finished yet (still in progress in the background).";
        break;
        case 12:
            errorString="The socket is using a proxy, and the proxy requires authentication.";
        break;
        case 13:
            errorString="The SSL/TLS handshake failed, so the connection was closed (only used in QSslSocket)";
        break;
        case 14:
            errorString="Could not contact the proxy server because the connection to that server was denied";
        break;
        case 15:
            errorString="The connection to the proxy server was closed unexpectedly (before the connection to the final peer was established)";
        break;
        case 16:
            errorString="The connection to the proxy server timed out or the proxy server stopped responding in the authentication phase.";
        break;
        case 17:
            errorString="The proxy address set with setProxy() (or the application proxy) was not found.";
        break;
        case 18:
            errorString="The connection negotiation with the proxy server failed, because the response from the proxy server could not be understood.";
        break;
        case 19:
            errorString="An operation was attempted while the socket was in a state that did not permit it.";
        break;
        case 20:
            errorString="The SSL library being used reported an internal error. This is probably the result of a bad installation or misconfiguration of the library.";
        break;
        case 21:
            errorString="Invalid data (certificate, key, cypher, etc.) was provided and its use resulted in an error in the SSL library.";
        break;
        case 22:
            errorString="A temporary error occurred (e.g., operation would block and socket is non-blocking).";
        break;
        default:
            errorString="An unidentified error occurred.";
        break;
    }

    qDebug() << QString("MultipleBotConnectionImplFoprGui::newError() error: %1 %2").arg(error).arg(errorString);
    statusError(QStringLiteral("Error: %1 %2").arg(error).arg(errorString));
    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(sender());
    if(senderObject!=NULL)
    {
        if(connectedSocketToCatchChallengerClient.contains(senderObject))
        {
            connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
            MultipleBotConnection::newSocketError_with_client(connectedSocketToCatchChallengerClient[senderObject],error);
        }
    }
    /*QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        i.value()->st;
    }no way here to stop the timer*/

    #ifdef QT_GUI_LIB
    if(!displayingError)
    {
        displayingError=true;
        QMessageBox::critical(NULL,tr("Error"),errorString);
        displayingError=false;
        QCoreApplication::exit(85);
    }
    #else
    std::cerr << errorString.toStdString() << std::endl;
    QCoreApplication::exit(85);
    #endif
}

void MultipleBotConnectionImplForGui::newError(QString error,QString detailedError)
{
    haveEnError=true;

    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
    {
        qDebug() << QString("MultipleBotConnectionImplFoprGui::newError() senderObject==NULL error: %1, detailedError: %2").arg(error).arg(detailedError);
        return;
    }

    apiToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
    statusError(QStringLiteral("Error: %1, detailedError: %2").arg(error).arg(detailedError));
    qDebug() << QString("MultipleBotConnectionImplFoprGui::newError() error: %1, detailedError: %2").arg(error).arg(detailedError);
    MultipleBotConnection::newError_with_client(apiToCatchChallengerClient[senderObject],error,detailedError);
}

void MultipleBotConnectionImplForGui::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
    {
        qDebug() << QString("MultipleBotConnectionImplFoprGui::have_current_player_info() senderObject==NULL");
        return;
    }
    MultipleBotConnection::have_current_player_info_with_client(apiToCatchChallengerClient[senderObject],informations);
}

void MultipleBotConnectionImplForGui::disconnected()
{
    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(sender());
    if(senderObject==NULL)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("MultipleBotConnectionImplForGui::disconnected() senderObject==NULL"));
        return;
    }

    if(!connectedSocketToCatchChallengerClient.contains(senderObject))
    {
        std::cerr << "MultipleBotConnectionImplForGui::disconnected() !connectedSocketToCatchChallengerClient.contains(senderObject)" << std::endl;
        abort();
    }
    CatchChallengerClient * catchChallengerClient=connectedSocketToCatchChallengerClient.value(senderObject);
    if(catchChallengerClient==NULL)
    {
        std::cerr << "MultipleBotConnectionImplForGui::disconnected() catchChallengerClient==NULL" << std::endl;
        abort();
    }
    CatchChallenger::Api_client_real *api=catchChallengerClient->api;
    if(api==NULL)
    {
        std::cerr << "MultipleBotConnectionImplForGui::disconnected() api==NULL" << std::endl;
        abort();
    }
    const CatchChallenger::Api_protocol::StageConnexion stage=api->stage();
    if(stage==CatchChallenger::Api_protocol::StageConnexion::Stage2)
    {
        std::cout << "MultipleBotConnectionImplForGui::disconnected() for reconnect, ignore" << std::endl;
        MultipleBotConnection::disconnected();
        return;
    }
    MultipleBotConnection::disconnected();

    catchChallengerClient->haveShowDisconnectionReason=false;
    if(catchChallengerClient->selectedCharacter==true)
    {
        catchChallengerClient->selectedCharacter=false;
        numberOfSelectedCharacter--;
        emit emit_numberOfSelectedCharacter(numberOfSelectedCharacter);
    }
    if(botInterface!=NULL)
        botInterface->removeClient(api);
}


void MultipleBotConnectionImplForGui::connectTimerSlot()
{
    MultipleBotConnection::connectTimerSlot();
}
