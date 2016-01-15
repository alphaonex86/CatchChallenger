#include "MultipleBotConnectionImplFoprGui.h"

MultipleBotConnectionImplFoprGui::MultipleBotConnectionImplFoprGui()
{
}

MultipleBotConnectionImplFoprGui::~MultipleBotConnectionImplFoprGui()
{
}

QString MultipleBotConnectionImplFoprGui::login()
{
    return mLogin;
}

QString MultipleBotConnectionImplFoprGui::pass()
{
    return mPass;
}

bool MultipleBotConnectionImplFoprGui::multipleConnexion()
{
    return mMultipleConnexion;
}

bool MultipleBotConnectionImplFoprGui::autoCreateCharacter()
{
    return mAutoCreateCharacter;
}

int MultipleBotConnectionImplFoprGui::connectBySeconds()
{
    return mConnectBySeconds;
}

int MultipleBotConnectionImplFoprGui::connexionCount()
{
    return mConnexionCount;
}

int MultipleBotConnectionImplFoprGui::maxDiffConnectedSelected()
{
    return mMaxDiffConnectedSelected;
}

QString MultipleBotConnectionImplFoprGui::proxy()
{
    return mProxy;
}

quint16 MultipleBotConnectionImplFoprGui::proxyport()
{
    return mProxyport;
}

QString MultipleBotConnectionImplFoprGui::host()
{
    return mHost;
}

quint16 MultipleBotConnectionImplFoprGui::port()
{
    return mPort;
}

/************************** function part ***********************************/
void MultipleBotConnectionImplFoprGui::detectSlowDown()
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

void MultipleBotConnectionImplFoprGui::characterSelect(const quint32 &charId)
{
    if(!serverIsSelected)
    {
        qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): !serverIsSelected (abort)";
        abort();
    }
    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        if(!i.value()->api->selectCharacter(charactersGroupIndex,serverUniqueKey,charId))
            qDebug() << "Unable to manual select character:" << charId;
        else
        {
            characterOnMap << charId;
            qDebug() << "Manual select character:" << charId;
        }
    }
}

void MultipleBotConnectionImplFoprGui::serverSelect(const uint8_t &charactersGroupIndex,const quint32 &uniqueKey)
{
    this->charactersGroupIndex=charactersGroupIndex;
    this->serverUniqueKey=uniqueKey;
    serverIsSelected=true;
}

void MultipleBotConnectionImplFoprGui::insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(senderObject==NULL)
        return;

    MultipleBotConnection::insert_player_with_client(apiToCatchChallengerClient.value(senderObject),player,mapId,x,y,direction);

    if(player.simplifiedId==apiToCatchChallengerClient.value(senderObject)->api->getId())
        botInterface->insert_player(apiToCatchChallengerClient.value(senderObject)->api,player,mapId,x,y,direction);
}

void MultipleBotConnectionImplFoprGui::logged(const QList<CatchChallenger::ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    apiToCatchChallengerClient.value(senderObject)->charactersList=characterEntryList;
    if(apiToCatchChallengerClient.size()==1)
    {
        //get the datapack
        apiToCatchChallengerClient.value(senderObject)->api->sendDatapackContentBase();
        emit loggedDone(serverOrdenedList,characterEntryList,false);
        return;
    }
    else
    {
        logged_with_client(apiToCatchChallengerClient.value(senderObject));
        emit loggedDone(serverOrdenedList,characterEntryList,true);
        return;
    }
}

void MultipleBotConnectionImplFoprGui::haveCharacter()
{
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

void MultipleBotConnectionImplFoprGui::connectTheExternalSocket(CatchChallengerClient * client)
{
    MultipleBotConnection::connectTheExternalSocket(client);
    connect(client->api,&CatchChallenger::Api_client_real::new_chat_text,            this,&MultipleBotConnectionImplFoprGui::chat_text,Qt::QueuedConnection);
}

void MultipleBotConnectionImplFoprGui::newCharacterId(const quint8 &returnCode, const quint32 &characterId)
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

void MultipleBotConnectionImplFoprGui::haveTheDatapack()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    MultipleBotConnection::haveTheDatapack_with_client(apiToCatchChallengerClient.value(senderObject));
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.empty())
    {
        qDebug() << "Profile list is empty";
        return;
    }
}

void MultipleBotConnectionImplFoprGui::haveTheDatapackMainSub()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    MultipleBotConnection::haveTheDatapackMainSub_with_client(apiToCatchChallengerClient.value(senderObject));
}

void MultipleBotConnectionImplFoprGui::sslErrors(const QList<QSslError> &errors)
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

void MultipleBotConnectionImplFoprGui::protocol_is_good()
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    MultipleBotConnection::protocol_is_good_with_client(apiToCatchChallengerClient.value(senderObject));
}

void MultipleBotConnectionImplFoprGui::newSocketError(QAbstractSocket::SocketError error)
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

void MultipleBotConnectionImplFoprGui::newError(QString error,QString detailedError)
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

void MultipleBotConnectionImplFoprGui::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;
    MultipleBotConnection::have_current_player_info_with_client(apiToCatchChallengerClient[senderObject],informations);
}

void MultipleBotConnectionImplFoprGui::disconnected()
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


void MultipleBotConnectionImplFoprGui::connectTimerSlot()
{
    if(apiToCatchChallengerClient.size()<connexionCount())
    {
        const quint32 &diff=numberOfBotConnected-numberOfSelectedCharacter;
        if(diff<=(quint32)maxDiffConnectedSelected())
            createClient();
    }
    else
        connectTimer.stop();
}
