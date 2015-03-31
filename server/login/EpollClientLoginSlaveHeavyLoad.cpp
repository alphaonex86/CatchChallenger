#include "EpollClientLoginSlave.h"
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
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    askLogin_return(askLoginParam);
    //delete askLoginParam; -> not here because need reuse later
}

void EpollClientLoginSlave::character_list_return(const quint8 &characterGroupIndex,char * const tempRawData,const int &tempRawDataSize)
{
    characterTempListForReply[characterGroupIndex].rawData=tempRawData;
    characterTempListForReply[characterGroupIndex].rawDataSize=tempRawDataSize;
    characterListForReplyInSuspend--;
}

void EpollClientLoginSlave::server_list_return(const quint8 &serverCount,char * const tempRawData,const int &tempRawDataSize)
{
    memcpy(serverListForReplyRawData+serverListForReplyRawDataSize,tempRawData,tempRawDataSize);
    serverListForReplyRawDataSize+=tempRawDataSize;
    serverPlayedTimeCount+=serverCount;
    serverListForReplyInSuspend--;
    if(serverListForReplyInSuspend==0)
    {
EpollClientLoginSlave::loginGood
        and the previous reply
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
    normalOutput(QStringLiteral("Do db write: ")+queryText);
    #endif
    databaseBaseLogin.asyncWrite(queryText);
}

void EpollClientLoginSlave::dbQueryWriteCommon(const char * const queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText==NULL || queryText[0]=='\0')
    {
        errorParsingLayer(QStringLiteral("dbQuery() Query is empty, bug"));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput(QStringLiteral("Do db write: ")+queryText);
    #endif
    databaseBaseCommon.asyncWrite(queryText);
}

void EpollClientLoginSlave::loginIsWrong(const quint8 &query_id, const quint8 &returnCode, const QString &debugMessage)
{
    //network send
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    *(Client::loginIsWrongBuffer+1)=query_id;
    *(Client::loginIsWrongBuffer+3)=returnCode;
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));

    //send to server to stop the connection
    errorOutput(debugMessage);
}
