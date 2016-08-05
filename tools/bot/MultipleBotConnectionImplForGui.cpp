#include "MultipleBotConnectionImplForGui.h"

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
    if(worseTime>3600*1000)
        emit emit_detectSlowDown(tr("Running query: %1 Query with worse time: %2h").arg(queryCount).arg(worseTime/(3600*1000)));
    else if(worseTime>2*60*1000)
        emit emit_detectSlowDown(tr("Running query: %1 Query with worse time: %2min").arg(queryCount).arg(worseTime/(60*1000)));
    else if(worseTime>5*1000)
        emit emit_detectSlowDown(tr("Running query: %1 Query with worse time: %2s").arg(queryCount).arg(worseTime/(1000)));
    else
        emit emit_detectSlowDown(tr("Running query: %1 Query with worse time: %2ms").arg(queryCount).arg(worseTime));
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
            if(!i.value()->api->selectCharacter(charactersGroupIndex,serverUniqueKey,charId))
                qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): Unable to manual select character:" << charId;
            else
            {
                qDebug() << "add character on map: " << charId << " at " << __FILE__ << ":" << __LINE__;
                characterOnMap << charId;
                qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): Manual select character:" << charId;
            }
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
}

void MultipleBotConnectionImplForGui::insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;

    MultipleBotConnection::insert_player_with_client(apiToCatchChallengerClient.value(senderObject),player,mapId,x,y,direction);

    if(player.simplifiedId==apiToCatchChallengerClient.value(senderObject)->api->getId())
        botInterface->insert_player(apiToCatchChallengerClient.value(senderObject)->api,player,mapId,x,y,direction);
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
        senderObject->sendDatapackContentMainSub();
    }
    MultipleBotConnection::haveCharacter();
}

void MultipleBotConnectionImplForGui::connectTheExternalSocket(CatchChallengerClient * client)
{
    MultipleBotConnection::connectTheExternalSocket(client);
    connect(client->api,&CatchChallenger::Api_client_real::new_chat_text,            this,&MultipleBotConnectionImplForGui::chat_text,Qt::QueuedConnection);
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
        return;

    MultipleBotConnection::haveTheDatapack_with_client(apiToCatchChallengerClient.value(senderObject));
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.empty())
    {
        qDebug() << "Profile list is empty";
        return;
    }
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
    /*QMessageBox::warning(this,tr("Ssl error"),sslErrors.join("\n"));
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

    if(error==0)
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::newError() Connection refused";
        statusError(QStringLiteral("Connection refused"));
    }
    else if(error==13)
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::newError() SslHandshakeFailedError";
        statusError(QStringLiteral("SslHandshakeFailedError"));
    }
    else
    {
        qDebug() << QString("MultipleBotConnectionImplFoprGui::newError() error: %1").arg(error);
        statusError(QStringLiteral("Error: %1").arg(error));
    }
    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(sender());
    if(senderObject!=NULL)
    {
        if(connectedSocketToCatchChallengerClient.contains(senderObject))
        {
            connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
            MultipleBotConnection::newSocketError_with_client(connectedSocketToCatchChallengerClient[senderObject],error);
        }
    }
}

void MultipleBotConnectionImplForGui::newError(QString error,QString detailedError)
{
    haveEnError=true;

    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    apiToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=true;
    statusError(QStringLiteral("Error: %1, detailedError: %2").arg(error).arg(detailedError));
    qDebug() << QString("MultipleBotConnectionImplFoprGui::newError() error: %1, detailedError: %2").arg(error).arg(detailedError);
    MultipleBotConnection::newError_with_client(apiToCatchChallengerClient[senderObject],error,detailedError);
}

void MultipleBotConnectionImplForGui::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;
    MultipleBotConnection::have_current_player_info_with_client(apiToCatchChallengerClient[senderObject],informations);
}

void MultipleBotConnectionImplForGui::disconnected()
{
    MultipleBotConnection::disconnected();

    CatchChallenger::ConnectedSocket *senderObject = qobject_cast<CatchChallenger::ConnectedSocket *>(sender());
    if(senderObject==NULL)
        return;

    connectedSocketToCatchChallengerClient[senderObject]->haveShowDisconnectionReason=false;
    if(connectedSocketToCatchChallengerClient.value(senderObject)->selectedCharacter==true)
    {
        connectedSocketToCatchChallengerClient[senderObject]->selectedCharacter=false;
        numberOfSelectedCharacter--;
        emit emit_numberOfSelectedCharacter(numberOfSelectedCharacter);
    }
    botInterface->removeClient(connectedSocketToCatchChallengerClient.value(senderObject)->api);
}


void MultipleBotConnectionImplForGui::connectTimerSlot()
{
    MultipleBotConnection::connectTimerSlot();
}
