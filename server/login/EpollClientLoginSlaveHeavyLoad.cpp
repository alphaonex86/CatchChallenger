#include "EpollClientLoginSlave.h"
#include "EpollServerLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../base/PreparedDBQuery.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <iostream>

using namespace CatchChallenger;

void EpollClientLoginSlave::askLogin(const quint8 &query_id,const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQuery::db_query_login==NULL)
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

    const QString &queryText=QString(PreparedDBQuery::db_query_login).arg(QString(login.toHex()));
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
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.takeFirst());//but not delete because will reinster at the end
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
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("AskLoginParam"))
    {
        qDebug() << "is not AskLoginParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    {
        bool ok;
        if(!EpollServerLoginSlave::epollServerLoginSlave->databaseBaseLogin->next())
        {
            if(EpollClientLoginSlave::automatic_account_creation)
            {
                //network send
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(askLoginParam->query_id);
                #endif
                *(EpollClientLoginSlave::loginIsWrongBuffer+1)=(quint8)askLoginParam->query_id;
                *(EpollClientLoginSlave::loginIsWrongBuffer+3)=(quint8)0x07;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));
                delete askLoginParam;
                askLoginParam=NULL;
                is_logging_in_progess=false;
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
                delete askLoginParam;
                askLoginParam=NULL;
                return;
            }
        }
        else
        {
            QByteArray hashedToken;
            {
                bool found=false;
                quint32 index=0;
                while(index<BaseServerLogin::tokenForAuthSize)
                {
                    const BaseServerLogin::TokenLink &tokenLink=BaseServerLogin::tokenForAuth[index];
                    if(tokenLink.client==this)
                    {
                        const QString &secretToken(EpollServerLoginSlave::epollServerLoginSlave->databaseBaseLogin->value(1));
                        const QByteArray &secretTokenBinary=QByteArray::fromHex(secretToken.toLatin1());
                        QCryptographicHash hash(QCryptographicHash::Sha224);
                        hash.addData(secretTokenBinary);
                        hash.addData(tokenLink.value,CATCHCHALLENGER_TOKENSIZE);
                        hashedToken=hash.result();
                        BaseServerLogin::tokenForAuthSize--;
                        if(BaseServerLogin::tokenForAuthSize>0)
                        {
                            while(index<BaseServerLogin::tokenForAuthSize)
                            {
                                BaseServerLogin::tokenForAuth[index]=BaseServerLogin::tokenForAuth[index+1];
                                index++;
                            }
                            //don't work:memmove(BaseServerLogin::tokenForAuth+index*sizeof(TokenLink),BaseServerLogin::tokenForAuth+index*sizeof(TokenLink)+sizeof(TokenLink),sizeof(TokenLink)*(BaseServerLogin::tokenForAuthSize-index));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(BaseServerLogin::tokenForAuth[0].client==NULL)
                                abort();
                            #endif
                        }
                        found=true;
                        break;
                    }
                    index++;
                }
                if(!found)
                {
                    loginIsWrong(askLoginParam->query_id,0x02,QStringLiteral("No temp auth token found"));
                    return;
                }
            }
            if(hashedToken!=askLoginParam->pass)
            {
                loginIsWrong(askLoginParam->query_id,0x03,QStringLiteral("Password wrong: %1 for the login: %2").arg(QString(askLoginParam->pass.toHex())).arg(QString(askLoginParam->login.toHex())));
                delete askLoginParam;
                askLoginParam=NULL;
                return;
            }
            else
            {
                account_id=QString(EpollServerLoginSlave::epollServerLoginSlave->databaseBaseLogin->value(0)).toUInt(&ok);
                if(!ok)
                {
                    account_id=0;
                    loginIsWrong(askLoginParam->query_id,0x03,"Account id is not a number");
                    delete askLoginParam;
                    askLoginParam=NULL;
                    return;
                }
            }
        }
    }

    {
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
    if(serverListForReplyInSuspend==0 && characterTempListForReply.size()!=CharactersGroupForLogin::list.size())
    {
        qDebug() << "serverListForReplyInSuspend==0 && characterTempListForReply.size()!=CharactersGroupForLogin::list.size()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
}

void EpollClientLoginSlave::server_list_return(const quint8 &serverCount,char * const tempRawData,const int &tempRawDataSize)
{
    memcpy(serverListForReplyRawData+serverListForReplyRawDataSize,tempRawData,tempRawDataSize);
    serverListForReplyRawDataSize+=tempRawDataSize;
    serverPlayedTimeCount+=serverCount;
    serverListForReplyInSuspend--;
    if(serverListForReplyInSuspend==0)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(serverListForReplyInSuspend==0 && characterTempListForReply.size()!=CharactersGroupForLogin::list.size())
        {
            qDebug() << "serverListForReplyInSuspend==0 && characterTempListForReply.size()!=CharactersGroupForLogin::list.size()" << __FILE__ << __LINE__;
            abort();
        }
        #endif
        //characters group
        int tempSize=EpollClientLoginSlave::loginGoodSize;
        QMapIterator<quint8,CharacterListForReply> i(characterTempListForReply);
        while (i.hasNext()) {
            i.next();
            memcpy(EpollClientLoginSlave::loginGood+tempSize,i.value().rawData,i.value().rawDataSize);
            tempSize+=i.value().rawDataSize;
        }
        //Server list
        memcpy(EpollClientLoginSlave::loginGood+tempSize,serverListForReplyRawData,serverListForReplyRawDataSize);
        tempSize+=serverListForReplyRawDataSize;

        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(askLoginParam==NULL)
            abort();
        #endif
        postReply(askLoginParam->query_id,EpollClientLoginSlave::loginGood,tempSize);
        delete askLoginParam;
        askLoginParam=NULL;
    }
}

void EpollClientLoginSlave::createAccount(const quint8 &query_id, const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQuery::db_query_login==0 || PreparedDBQuery::db_query_login[0]=='\0')
    {
        errorParsingLayer(QStringLiteral("createAccount() Query login is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_insert_login==0 || PreparedDBQuery::db_query_insert_login[0]=='\0')
    {
        errorParsingLayer(QStringLiteral("createAccount() Query inset login is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_characters==0 || PreparedDBQuery::db_query_characters[0]=='\0')
    {
        errorParsingLayer(QStringLiteral("createAccount() Query characters is empty, bug"));
        return;
    }
    if(!automatic_account_creation)
    {
        errorParsingLayer(QStringLiteral("createAccount() Creation account not premited"));
        return;
    }
    #endif
    if(accountCharatersCount>=max_character)
    {
        loginIsWrong(query_id,0x03,QStringLiteral("Have already the max charaters: %1/%2").arg(accountCharatersCount).arg(max_character));
        return;
    }
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

    const QString &queryText=QString(PreparedDBQuery::db_query_login).arg(QString(login.toHex()));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseLogin.asyncRead(queryText.toLatin1(),this,&EpollClientLoginSlave::createAccount_static);
    if(callback==NULL)
    {
        is_logging_in_progess=false;
        loginIsWrong(askLoginParam->query_id,0x03,QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin.errorMessage()));
        delete askLoginParam;
        askLoginParam=NULL;
        return;
    }
    else
    {
        accountCharatersCount++;
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
            if(linkToMaster->queryNumberList.empty())
            {
                errorParsingLayer("Unable to get query id at createAccount_return");
                return;
            }
            const quint8 &queryNumber=linkToMaster->queryNumberList.back();
            linkToMaster->queryNumberList.pop_back();
            EpollClientLoginSlave::maxAccountIdRequest[0x03]=queryNumber;
            if(!internalSendRawSmallPacket(EpollClientLoginSlave::maxAccountIdRequest,sizeof(EpollClientLoginSlave::maxAccountIdRequest)))
            {
                errorParsingLayer("Unable to send at createAccount_return");
                return;
            }
            linkToMaster->newFullOutputQuery(0x11,0x01,queryNumber);
        }
        account_id=maxAccountIdList.takeFirst();
        dbQueryWriteLogin(QString(PreparedDBQuery::db_query_insert_login).arg(account_id).arg(QString(askLoginParam->login.toHex())).arg(QString(askLoginParam->pass.toHex())).arg(QDateTime::currentDateTime().toTime_t()).toUtf8().constData());
        //send the network reply
        QByteArray outputData;
        outputData[0x00]=0x01;
        postReply(askLoginParam->query_id,outputData);
        is_logging_in_progess=false;
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
    std::cout << "Do db write: " << queryText << std::endl;
    #endif
    databaseBaseLogin.asyncWrite(queryText);
}

void EpollClientLoginSlave::loginIsWrong(const quint8 &query_id, const quint8 &returnCode, const QString &debugMessage)
{
    //network send
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    EpollClientLoginSlave::loginIsWrongBuffer[1]=query_id;
    EpollClientLoginSlave::loginIsWrongBuffer[3]=returnCode;
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));

    //send to server to stop the connection
    errorParsingLayer(debugMessage);
}

void EpollClientLoginSlave::selectCharacter(const quint8 &query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId)
{
    if(charactersGroupIndex<=CharactersGroupForLogin::list.size())
    {
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() charactersGroupIndex is out of range");
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    if(!CharactersGroupForLogin::list.at(charactersGroupIndex)->containsServerUniqueKey(serverUniqueKey))
    {
        EpollClientLoginSlave::loginIsWrongBuffer[1]=query_id;
        EpollClientLoginSlave::loginIsWrongBuffer[3]=0x05;
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));
        return;
    }
    if(!EpollClientLoginSlave::linkToMaster->trySelectCharacter(this,query_id,serverUniqueKey,charactersGroupIndex,characterId))
    {
        EpollClientLoginSlave::loginIsWrongBuffer[1]=query_id;
        EpollClientLoginSlave::loginIsWrongBuffer[3]=0x04;
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() out of query for request the master server");
        return;
    }
}

void EpollClientLoginSlave::selectCharacter_ReturnToken(const quint8 &query_id,const char * const token)
{
    if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Direct)
        postReplyData(query_id,token,TOKEN_SIZE);
    else
    {
        //connect on the game server and pass in proxy mode
        errorParsingLayer("EpollClientLoginSlave::selectCharacter_ReturnToken() proxy mode not supported from now");
    }
}

void EpollClientLoginSlave::selectCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode)
{
    EpollClientLoginSlave::loginIsWrongBuffer[1]=query_id;
    EpollClientLoginSlave::loginIsWrongBuffer[3]=errorCode;
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));
    errorParsingLayer("EpollClientLoginSlave::selectCharacter_ReturnFailed() errorCode:"+QString::number(errorCode));
    return;
}

void EpollClientLoginSlave::addCharacter(const quint8 &query_id, const quint8 &characterGroupIndex, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId)
{
    if(characterGroupIndex<=CharactersGroupForLogin::list.size())
    {
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() charactersGroupIndex is out of range");
        return;
    }
    const qint8 &addCharacter=CharactersGroupForLogin::list.at(characterGroupIndex)->addCharacter(this,query_id,profileIndex,pseudo,skinId);
    if(addCharacter!=0)
    {
        if(addCharacter>0 && addCharacter<=3)
        {
            EpollClientLoginSlave::addCharacterIsWrongBuffer[1]=query_id;
            EpollClientLoginSlave::addCharacterIsWrongBuffer[3]=0x02;
            internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));
        }
        if(addCharacter<0)
            errorParsingLayer("EpollClientLoginSlave::selectCharacter() hack detected");
        return;
    }
}

void EpollClientLoginSlave::removeCharacter(const quint8 &query_id, const quint8 &characterGroupIndex, const quint32 &characterId)
{
    if(characterGroupIndex<=CharactersGroupForLogin::list.size())
    {
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() charactersGroupIndex is out of range");
        return;
    }
    if(!CharactersGroupForLogin::list.at(characterGroupIndex)->removeCharacter(this,query_id,characterId))
    {
        EpollClientLoginSlave::loginIsWrongBuffer[1]=query_id;
        EpollClientLoginSlave::loginIsWrongBuffer[3]=0x02;
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() out of query for request the master server");
        return;
    }
}

void EpollClientLoginSlave::addCharacter_ReturnOk(const quint8 &query_id,const quint32 &characterId)
{
    EpollClientLoginSlave::addCharacterReply[1]=query_id;
    EpollClientLoginSlave::addCharacterReply[3]=0x00;
    *reinterpret_cast<quint32 *>(EpollClientLoginSlave::addCharacterReply+0x04)=htole32(characterId);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::addCharacterReply),sizeof(EpollClientLoginSlave::addCharacterReply));
}

void EpollClientLoginSlave::addCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode)
{
    EpollClientLoginSlave::addCharacterReply[1]=query_id;
    EpollClientLoginSlave::addCharacterReply[3]=errorCode;
    *reinterpret_cast<quint32 *>(EpollClientLoginSlave::addCharacterReply+0x04)=(quint32)0;
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::addCharacterReply),sizeof(EpollClientLoginSlave::addCharacterReply));
    errorParsingLayer("EpollClientLoginSlave::addCharacter() out of query for request the master server");
}

void EpollClientLoginSlave::removeCharacter_ReturnOk(const quint8 &query_id)
{
    EpollClientLoginSlave::removeCharacterReply[1]=query_id;
    EpollClientLoginSlave::removeCharacterReply[3]=0x01;
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::removeCharacterReply),sizeof(EpollClientLoginSlave::removeCharacterReply));
}

void EpollClientLoginSlave::removeCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode)
{
    EpollClientLoginSlave::removeCharacterReply[1]=query_id;
    EpollClientLoginSlave::removeCharacterReply[3]=errorCode;
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::removeCharacterReply),sizeof(EpollClientLoginSlave::removeCharacterReply));
    errorParsingLayer("EpollClientLoginSlave::removeCharacter() out of query for request the master server");
}

void EpollClientLoginSlave::characterSelectionIsWrong(const quint8 &query_id,const quint8 &returnCode,const QString &debugMessage)
{
    EpollClientLoginSlave::loginIsWrongBuffer[1]=query_id;
    EpollClientLoginSlave::loginIsWrongBuffer[3]=returnCode;
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));

    //send to server to stop the connection
    errorParsingLayer(debugMessage);
}
