#include "EpollClientLoginSlave.h"
#include "EpollServerLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../base/PreparedDBQuery.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <iostream>
#include "../../general/base/CommonSettingsCommon.h"

using namespace CatchChallenger;

void EpollClientLoginSlave::askLogin(const quint8 &query_id,const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login==NULL)
    {
        errorParsingLayer(QStringLiteral("askLogin() Query login is empty, bug"));
        return;
    }
    #endif
    QByteArray login;
    {
        QCryptographicHash hash(QCryptographicHash::Sha224);
        hash.addData(rawdata,CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE);
        login=hash.result();
    }
    AskLoginParam *askLoginParam=new AskLoginParam;
    askLoginParam->query_id=query_id;
    askLoginParam->login=login;
    askLoginParam->pass=QByteArray(rawdata+CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE,CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE);

    const QString &queryText=QString(PreparedDBQueryLogin::db_query_login).arg(QString(login.toHex()));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseLogin.asyncRead(queryText.toLatin1(),this,&EpollClientLoginSlave::askLogin_static);
    if(callback==NULL)
    {
        loginIsWrong(askLoginParam->query_id,0x04,QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin.errorMessage()));
        delete askLoginParam;
        askLoginParam=NULL;
        return;
    }
    else
    {
        paramToPassToCallBack << askLoginParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("AskLoginParam");
        #endif
        callbackRegistred << callback;
    }
}

void EpollClientLoginSlave::askLogin_static(void *object)
{
    if(object!=NULL)
        static_cast<EpollClientLoginSlave *>(object)->askLogin_object();
    databaseBaseLogin.clear();
    //delete askLoginParam; -> not here because need reuse later
}

void EpollClientLoginSlave::askLogin_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.first());//but not delete because will reinster at the end, then not take!
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    askLogin_return(askLoginParam);
    //delete askLoginParam; -> not here because need reuse later
}

void EpollClientLoginSlave::askLogin_return(AskLoginParam *askLoginParam)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.first()!=QStringLiteral("AskLoginParam"))
    {
        qDebug() << "is not AskLoginParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    {
        bool ok;
        if(!databaseBaseLogin.next())
        {
            //create a new account
            if(CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
            {
                //network send
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                //removeFromQueryReceived(askLoginParam->query_id);//all list dropped at client destruction
                #endif
                *(EpollClientLoginSlave::loginIsWrongBufferReply+1)=(quint8)askLoginParam->query_id;
                *(EpollClientLoginSlave::loginIsWrongBufferReply+3)=(quint8)0x07;
                replyOutputSize.remove(askLoginParam->query_id);
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBufferReply),sizeof(EpollClientLoginSlave::loginIsWrongBufferReply));
                stat=EpollClientLoginStat::ProtocolGood;
                paramToPassToCallBack.clear();
                paramToPassToCallBackType.clear();
                delete askLoginParam;
                askLoginParam=NULL;
                return;
/*                PreparedDBQuery::maxAccountId++;
                account_id=PreparedDBQuery::maxAccountId;
                dbQueryWrite(PreparedDBQuery::db_query_insert_login.arg(account_id).arg(QString(askLoginParam->login.toHex())).arg(QString(askLoginParam->pass.toHex())).arg(QDateTime::currentDateTime().toTime_t()));*/
            }
            else
            {
                loginIsWrong(askLoginParam->query_id,0x02,QStringLiteral("Bad login for: %1, pass: %2")
                             .arg(QString(askLoginParam->login.toHex()))
                              .arg(QString(askLoginParam->pass.toHex()))
                              );
                paramToPassToCallBack.clear();
                paramToPassToCallBackType.clear();
                delete askLoginParam;
                askLoginParam=NULL;
                return;
            }
        }
        else
        {
            QByteArray hashedToken;
            qint32 tokenForAuthIndex=0;
            {
                while((quint32)tokenForAuthIndex<BaseServerLogin::tokenForAuthSize)
                {
                    const BaseServerLogin::TokenLink &tokenLink=BaseServerLogin::tokenForAuth[tokenForAuthIndex];
                    if(tokenLink.client==this)
                    {
                        const QString &secretToken(databaseBaseLogin.value(1));
                        const QByteArray &secretTokenBinary=QByteArray::fromHex(secretToken.toLatin1());
                        QCryptographicHash hash(QCryptographicHash::Sha224);
                        hash.addData(secretTokenBinary);
                        hash.addData(tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        hashedToken=hash.result();
                        BaseServerLogin::tokenForAuthSize--;
                        //see to do with SIMD
                        if(BaseServerLogin::tokenForAuthSize>0)
                        {
                            while((quint32)tokenForAuthIndex<BaseServerLogin::tokenForAuthSize)
                            {
                                BaseServerLogin::tokenForAuth[tokenForAuthIndex]=BaseServerLogin::tokenForAuth[tokenForAuthIndex+1];
                                tokenForAuthIndex++;
                            }
                            //don't work:memmove(BaseServerLogin::tokenForAuth+index*sizeof(TokenLink),BaseServerLogin::tokenForAuth+index*sizeof(TokenLink)+sizeof(TokenLink),sizeof(TokenLink)*(BaseServerLogin::tokenForAuthSize-index));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(BaseServerLogin::tokenForAuth[0].client==NULL)
                                abort();
                            #endif
                        }
                        tokenForAuthIndex--;
                        break;
                    }
                    tokenForAuthIndex++;
                }
                if(tokenForAuthIndex>=(qint32)BaseServerLogin::tokenForAuthSize)
                {
                    loginIsWrong(askLoginParam->query_id,0x02,QStringLiteral("No temp auth token found"));
                    return;
                }
            }
            if(hashedToken!=askLoginParam->pass)
            {
                loginIsWrong(askLoginParam->query_id,0x03,QStringLiteral("Password wrong: %1 with token %3 for the login: %2")
                             .arg(QString(askLoginParam->pass.toHex()))
                             .arg(QString(askLoginParam->login.toHex()))
                             .arg(QString(QByteArray(BaseServerLogin::tokenForAuth[tokenForAuthIndex].value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT).toHex()))
                             );
                paramToPassToCallBack.clear();
                paramToPassToCallBackType.clear();
                delete askLoginParam;
                askLoginParam=NULL;
                return;
            }
            else
            {
                account_id=QString(databaseBaseLogin.value(0)).toUInt(&ok);
                if(!ok)
                {
                    account_id=0;
                    loginIsWrong(askLoginParam->query_id,0x03,"Account id is not a number");
                    paramToPassToCallBack.clear();
                    paramToPassToCallBackType.clear();
                    delete askLoginParam;
                    askLoginParam=NULL;
                    return;
                }
            }
        }
    }

    {
        serverListForReplyInSuspend+=CharactersGroupForLogin::list.size();
        int index=0;
        while(index<CharactersGroupForLogin::list.size())
        {
            CharactersGroupForLogin::list.at(index)->character_list(this,account_id);
            index++;
        }
    }
}

void EpollClientLoginSlave::askLogin_cancel()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.takeFirst());
    loginIsWrong(askLoginParam->query_id,0x04,QStringLiteral("Canceled by the Charaters group"));
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    delete askLoginParam;
    askLoginParam=NULL;
}

void EpollClientLoginSlave::character_list_return(const quint8 &characterGroupIndex,char * const tempRawData,const int &tempRawDataSize)
{
    characterTempListForReply[characterGroupIndex].rawData=tempRawData;
    characterTempListForReply[characterGroupIndex].rawDataSize=tempRawDataSize;
    characterListForReplyInSuspend--;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(
            //if all the query is finished
            (characterListForReplyInSuspend==0 && serverListForReplyInSuspend==0)
            &&
            //and the group character list size to send don't match with real character list size
            (characterTempListForReply.size()!=CharactersGroupForLogin::list.size())
             )
    {
        qDebug() << "serverListForReplyInSuspend==0 && characterTempListForReply.size()!=CharactersGroupForLogin::list.size()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
}

void EpollClientLoginSlave::server_list_return(const quint8 &serverCount,char * const tempRawData,const int &tempRawDataSize)
{
    if(serverCount>0)
    {
        if(serverListForReplyRawData==NULL)
        {
            serverListForReplyRawDataSize=1;
            serverListForReplyRawData=static_cast<char *>(malloc(512*1024));
            serverListForReplyRawData[0x00]=0;
        }
        serverListForReplyRawData[0x00]+=serverCount;
        memcpy(serverListForReplyRawData+serverListForReplyRawDataSize,tempRawData,tempRawDataSize);
        serverListForReplyRawDataSize+=tempRawDataSize;
    }
    serverListForReplyInSuspend--;
    if(serverListForReplyInSuspend==0)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(
                //if all the query is finished
                (characterListForReplyInSuspend==0 && serverListForReplyInSuspend==0)
                &&
                //and the group character list size to send don't match with real character list size
                (characterTempListForReply.size()!=CharactersGroupForLogin::list.size())
                 )
        {
            qDebug() << "serverListForReplyInSuspend==0 && characterTempListForReply.size()!=CharactersGroupForLogin::list.size()" << __FILE__ << __LINE__;
            abort();
        }
        #endif

        unsigned int tempSize=EpollClientLoginSlave::loginGoodSize;

        //characters group
        EpollClientLoginSlave::loginGood[tempSize]=characterTempListForReply.size();
        tempSize+=sizeof(quint8);

        QMapIterator<quint8,CharacterListForReply> i(characterTempListForReply);
        while (i.hasNext()) {
            i.next();
            //copy buffer
            memcpy(EpollClientLoginSlave::loginGood+tempSize,i.value().rawData,i.value().rawDataSize);
            tempSize+=i.value().rawDataSize;
            //remove the old buffer
            delete i.value().rawData;
        }
        characterTempListForReply.clear();
        //Server list
        if(serverListForReplyRawData!=NULL)
        {
            memcpy(EpollClientLoginSlave::loginGood+tempSize,serverListForReplyRawData,serverListForReplyRawDataSize);
            tempSize+=serverListForReplyRawDataSize;
            //delete serverListForReplyRawData;//do into caller: CharactersGroupForLogin::character_list_object()
        }
        else
        {
            EpollClientLoginSlave::loginGood[tempSize]=0;
            tempSize+=sizeof(quint8);
        }

        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(paramToPassToCallBackType.first()!=QStringLiteral("AskLoginParam"))
        {
            qDebug() << "is not AskLoginParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
            abort();
        }
        #endif
        AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.takeFirst());
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(askLoginParam==NULL)
            abort();
        #endif
        //send C20F and C20E
        internalSendRawSmallPacket(EpollClientLoginSlave::serverLogicalGroupAndServerList,EpollClientLoginSlave::serverLogicalGroupAndServerListSize);
        //send the reply
        postReply(askLoginParam->query_id,EpollClientLoginSlave::loginGood,tempSize);
        paramToPassToCallBack.clear();
        paramToPassToCallBackType.clear();
        delete askLoginParam;
        askLoginParam=NULL;
        stat=EpollClientLoginStat::Logged;
    }
}

void EpollClientLoginSlave::createAccount(const quint8 &query_id, const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login==0 || PreparedDBQueryLogin::db_query_login[0]=='\0')
    {
        errorParsingLayer(QStringLiteral("createAccount() Query login is empty, bug"));
        return;
    }
    if(PreparedDBQueryLogin::db_query_insert_login==0 || PreparedDBQueryLogin::db_query_insert_login[0]=='\0')
    {
        errorParsingLayer(QStringLiteral("createAccount() Query inset login is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_characters==0 || PreparedDBQueryCommon::db_query_characters[0]=='\0')
    {
        errorParsingLayer(QStringLiteral("createAccount() Query characters is empty, bug"));
        return;
    }
    if(!CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
    {
        errorParsingLayer(QStringLiteral("createAccount() Creation account not premited"));
        return;
    }
    #endif
    if(maxAccountIdList.isEmpty())
    {
        loginIsWrong(query_id,0x04,QStringLiteral("maxAccountIdList is empty"));
        return;
    }
    QByteArray login;
    {
        QCryptographicHash hash(QCryptographicHash::Sha224);
        hash.addData(rawdata,CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE);
        login=hash.result();
    }
    AskLoginParam *askLoginParam=new AskLoginParam;
    askLoginParam->query_id=query_id;
    askLoginParam->login=login;
    askLoginParam->pass=QByteArray(rawdata+CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE,CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE);

    const QString &queryText=QString(PreparedDBQueryLogin::db_query_login).arg(QString(login.toHex()));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseLogin.asyncRead(queryText.toLatin1(),this,&EpollClientLoginSlave::createAccount_static);
    if(callback==NULL)
    {
        stat=EpollClientLoginStat::ProtocolGood;
        loginIsWrong(askLoginParam->query_id,0x03,QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin.errorMessage()));
        delete askLoginParam;
        askLoginParam=NULL;
        return;
    }
    else
    {
        paramToPassToCallBack << askLoginParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("AskLoginParam");
        #endif
        callbackRegistred << callback;
    }
}

void EpollClientLoginSlave::createAccount_static(void *object)
{
    if(object!=NULL)
        static_cast<EpollClientLoginSlave *>(object)->createAccount_object();
    databaseBaseLogin.clear();
}

void EpollClientLoginSlave::createAccount_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    createAccount_return(askLoginParam);
    delete askLoginParam;
    askLoginParam=NULL;
}

void EpollClientLoginSlave::createAccount_return(AskLoginParam *askLoginParam)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("AskLoginParam"))
    {
        qDebug() << "is not AskLoginParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    if(!databaseBaseLogin.next())
    {
        //network send
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //removeFromQueryReceived(askLoginParam->query_id);//->only if use fast path
        #endif
        if(maxAccountIdList.isEmpty())
        {
            loginIsWrong(askLoginParam->query_id,0x04,QStringLiteral("maxAccountIdList is empty"));
            return;
        }
        if(maxAccountIdList.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK && !EpollClientLoginSlave::maxAccountIdRequested)
        {
            EpollClientLoginSlave::maxAccountIdRequested=true;
            if(LinkToMaster::linkToMaster->queryNumberList.empty())
            {
                errorParsingLayer("Unable to get query id at createAccount_return");
                return;
            }
            const quint8 &queryNumber=LinkToMaster::linkToMaster->queryNumberList.back();
            LinkToMaster::linkToMaster->queryNumberList.pop_back();
            EpollClientLoginSlave::maxAccountIdRequest[0x03]=queryNumber;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            queryReceived.remove(queryNumber);
            #endif
            replyOutputSize.remove(queryNumber);
            if(!internalSendRawSmallPacket(EpollClientLoginSlave::maxAccountIdRequest,sizeof(EpollClientLoginSlave::maxAccountIdRequest)))
            {
                errorParsingLayer("Unable to send at createAccount_return");
                return;
            }
            LinkToMaster::linkToMaster->newFullOutputQuery(0x11,0x01,queryNumber);
        }
        account_id=maxAccountIdList.takeFirst();
        dbQueryWriteLogin(QString(PreparedDBQueryLogin::db_query_insert_login).arg(account_id).arg(QString(askLoginParam->login.toHex())).arg(QString(askLoginParam->pass.toHex())).arg(QDateTime::currentDateTime().toTime_t()).toUtf8().constData());
        //send the network reply
        QByteArray outputData;
        outputData[0x00]=0x01;
        postReply(askLoginParam->query_id,outputData.constData(),outputData.size());
        stat=EpollClientLoginStat::ProtocolGood;
    }
    else
        loginIsWrong(askLoginParam->query_id,0x02,QStringLiteral("Login already used: %1").arg(QString(askLoginParam->login.toHex())));
}

void EpollClientLoginSlave::dbQueryWriteLogin(const char * const queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText==NULL || queryText[0]=='\0')
    {
        errorParsingLayer(QStringLiteral("dbQuery() Query is empty, bug"));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << "Do login db write: " << queryText << std::endl;
    #endif
    databaseBaseLogin.asyncWrite(queryText);
}

void EpollClientLoginSlave::loginIsWrong(const quint8 &query_id, const quint8 &returnCode, const QString &debugMessage)
{
    //network send
    EpollClientLoginSlave::loginIsWrongBufferReply[1]=query_id;
    EpollClientLoginSlave::loginIsWrongBufferReply[3]=returnCode;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    replyOutputSize.remove(query_id);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBufferReply),sizeof(EpollClientLoginSlave::loginIsWrongBufferReply));

    //send to server to stop the connection
    errorParsingLayer(debugMessage);
}

void EpollClientLoginSlave::selectCharacter(const quint8 &query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId)
{
    if(charactersGroupIndex>=CharactersGroupForLogin::list.size())
    {
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() charactersGroupIndex is out of range");
        return;
    }
    //account id verified on game server when loading the character
    if(!CharactersGroupForLogin::list.at(charactersGroupIndex)->containsServerUniqueKey(serverUniqueKey))
    {
        EpollClientLoginSlave::characterSelectionIsWrongBufferServerNotFound[1]=query_id;

        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        removeFromQueryReceived(query_id);
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        replyOutputCompression.remove(query_id);
        #endif
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::characterSelectionIsWrongBufferServerNotFound),EpollClientLoginSlave::characterSelectionIsWrongBufferSize);
        return;
    }
    if(!LinkToMaster::linkToMaster->trySelectCharacter(this,query_id,serverUniqueKey,charactersGroupIndex,characterId))
    {
        EpollClientLoginSlave::characterSelectionIsWrongBufferServerInternalProblem[1]=query_id;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        removeFromQueryReceived(query_id);
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        replyOutputCompression.remove(query_id);
        #endif
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::characterSelectionIsWrongBufferServerInternalProblem),EpollClientLoginSlave::characterSelectionIsWrongBufferSize);
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() out of query for request the master server");
        return;
    }
    stat=CharacterSelecting;
    this->charactersGroupIndex=charactersGroupIndex;
    this->serverUniqueKey=serverUniqueKey;
}

void EpollClientLoginSlave::selectCharacter_ReturnToken(const quint8 &query_id,const char * const token)
{
    if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Reconnect)
        postReplyData(query_id,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
    else
    {
        //connect on the game server and pass in proxy mode
        errorParsingLayer("EpollClientLoginSlave::selectCharacter_ReturnToken() proxy mode not supported from now");
    }
}

void EpollClientLoginSlave::selectCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    replyOutputCompression.remove(query_id);
    switch(errorCode)
    {
        case 0x02:
        EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterNotFound[1]=query_id;
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterNotFound),EpollClientLoginSlave::characterSelectionIsWrongBufferSize);
        errorParsingLayer("EpollClientLoginSlave::selectCharacter_ReturnFailed() errorCode:"+QString::number(errorCode));
        break;
        default:
        case 0x03:
        EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline[1]=query_id;
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline),EpollClientLoginSlave::characterSelectionIsWrongBufferSize);
        errorParsingLayer("EpollClientLoginSlave::selectCharacter_ReturnFailed() errorCode:"+QString::number(errorCode));
        break;
        case 0x04:
        EpollClientLoginSlave::characterSelectionIsWrongBufferServerInternalProblem[1]=query_id;
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::characterSelectionIsWrongBufferServerInternalProblem),EpollClientLoginSlave::characterSelectionIsWrongBufferSize);
        errorParsingLayer("EpollClientLoginSlave::selectCharacter_ReturnFailed() errorCode:"+QString::number(errorCode));
        break;
        case 0x05:
        EpollClientLoginSlave::characterSelectionIsWrongBufferServerNotFound[1]=query_id;
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::characterSelectionIsWrongBufferServerNotFound),EpollClientLoginSlave::characterSelectionIsWrongBufferSize);
        break;
    }
}

void EpollClientLoginSlave::addCharacter(const quint8 &query_id, const quint8 &characterGroupIndex, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId)
{
    if(characterGroupIndex>=CharactersGroupForLogin::list.size())
    {
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() charactersGroupIndex is out of range");
        return;
    }
    const qint8 &addCharacter=CharactersGroupForLogin::list.at(characterGroupIndex)->addCharacter(this,query_id,profileIndex,pseudo,skinId);
    //error case
    if(addCharacter!=0)
    {
        if(addCharacter>0 && addCharacter<=3)
        {
            EpollClientLoginSlave::addCharacterIsWrongBuffer[1]=query_id;
            EpollClientLoginSlave::addCharacterIsWrongBuffer[3]=0x03;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            removeFromQueryReceived(query_id);
            #endif
            replyOutputSize.remove(query_id);
            internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::addCharacterIsWrongBuffer),sizeof(EpollClientLoginSlave::addCharacterIsWrongBuffer));
        }
        if(addCharacter<0)
            errorParsingLayer("EpollClientLoginSlave::selectCharacter() hack detected");
        return;
    }
}

void EpollClientLoginSlave::removeCharacter(const quint8 &query_id, const quint8 &characterGroupIndex, const quint32 &characterId)
{
    if(characterGroupIndex>=CharactersGroupForLogin::list.size())
    {
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() charactersGroupIndex is out of range");
        return;
    }
    if(!CharactersGroupForLogin::list.at(characterGroupIndex)->removeCharacter(this,query_id,characterId))
    {
        EpollClientLoginSlave::loginIsWrongBufferReply[1]=query_id;
        EpollClientLoginSlave::loginIsWrongBufferReply[3]=0x02;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        removeFromQueryReceived(query_id);
        #endif
        replyOutputSize.remove(query_id);
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBufferReply),sizeof(EpollClientLoginSlave::loginIsWrongBufferReply));
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() out of query for request the master server");
        return;
    }
}

void EpollClientLoginSlave::addCharacter_ReturnOk(const quint8 &query_id,const quint32 &characterId)
{
    EpollClientLoginSlave::addCharacterReply[1]=query_id;
    EpollClientLoginSlave::addCharacterReply[3]=0x00;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    replyOutputSize.remove(query_id);
    *reinterpret_cast<quint32 *>(EpollClientLoginSlave::addCharacterReply+0x04)=htole32(characterId);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::addCharacterReply),sizeof(EpollClientLoginSlave::addCharacterReply));
}

void EpollClientLoginSlave::addCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode)
{
    EpollClientLoginSlave::addCharacterReply[1]=query_id;
    EpollClientLoginSlave::addCharacterReply[3]=errorCode;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    replyOutputSize.remove(query_id);
    *reinterpret_cast<quint32 *>(EpollClientLoginSlave::addCharacterReply+0x04)=(quint32)0;
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::addCharacterReply),sizeof(EpollClientLoginSlave::addCharacterReply));
    if(errorCode!=0x01)
        errorParsingLayer(QStringLiteral("EpollClientLoginSlave::addCharacter() out of query for request the master server: %1").arg(errorCode));
}

void EpollClientLoginSlave::removeCharacter_ReturnOk(const quint8 &query_id)
{
    EpollClientLoginSlave::removeCharacterReply[1]=query_id;
    EpollClientLoginSlave::removeCharacterReply[3]=0x01;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    replyOutputSize.remove(query_id);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::removeCharacterReply),sizeof(EpollClientLoginSlave::removeCharacterReply));
}

void EpollClientLoginSlave::removeCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode,const QString &errorString)
{
    EpollClientLoginSlave::removeCharacterReply[1]=query_id;
    EpollClientLoginSlave::removeCharacterReply[3]=errorCode;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    replyOutputSize.remove(query_id);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::removeCharacterReply),sizeof(EpollClientLoginSlave::removeCharacterReply));
    if(errorString.isEmpty())
        errorParsingLayer(QStringLiteral("EpollClientLoginSlave::removeCharacter() out of query for request the master server: %1").arg(errorCode));
    else
        errorParsingLayer(QStringLiteral("%1: %2").arg(errorString).arg(errorCode));
}
