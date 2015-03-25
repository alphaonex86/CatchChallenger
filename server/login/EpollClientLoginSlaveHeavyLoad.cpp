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
        if(!databaseBaseLogin.next())
        {
            if(automatic_account_creation)
            {
                //network send
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(askLoginParam->query_id);
                #endif
                *(EpollClientLoginSlave::loginIsWrongBuffer+1)=(quint8)askLoginParam->query_id;
                *(EpollClientLoginSlave::loginIsWrongBuffer+3)=(quint8)0x07;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBuffer),sizeof(EpollClientLoginSlave::loginIsWrongBuffer));
                delete askLoginParam;
                is_logging_in_progess=false;
                return;
/*                maxAccountId++;
                account_id=maxAccountId;
                dbQueryWrite(PreparedDBQuery::db_query_insert_login.arg(account_id).arg(QString(askLoginParam->login.toHex())).arg(QString(askLoginParam->pass.toHex())).arg(QDateTime::currentDateTime().toTime_t()).toUtf8().constData());*/
            }
            else
            {
                loginIsWrong(askLoginParam->query_id,0x02,QStringLiteral("Bad login for: %1, pass: %2")
                             .arg(QString(askLoginParam->login.toHex()))
                              .arg(QString(askLoginParam->pass.toHex()))
                              );
                delete askLoginParam;
                return;
            }
        }
        else
        {
            QByteArray hashedToken;
            {
                bool found=false;
                quint32 index=0;
                while(index<tokenForAuthSize)
                {
                    const TokenLink &tokenLink=tokenForAuth[index];
                    if(tokenLink.client==this)
                    {
                        const QString &secretToken(databaseBaseLogin.value(1));
                        const QByteArray &secretTokenBinary=QByteArray::fromHex(secretToken.toLatin1());
                        QCryptographicHash hash(QCryptographicHash::Sha224);
                        hash.addData(secretTokenBinary);
                        hash.addData(tokenLink.value,CATCHCHALLENGER_TOKENSIZE);
                        hashedToken=hash.result();
                        tokenForAuthSize--;
                        if(tokenForAuthSize>0)
                        {
                            while(index<tokenForAuthSize)
                            {
                                tokenForAuth[index]=tokenForAuth[index+1];
                                index++;
                            }
                            //don't work:memmove(tokenForAuth+index*sizeof(TokenLink),tokenForAuth+index*sizeof(TokenLink)+sizeof(TokenLink),sizeof(TokenLink)*(tokenForAuthSize-index));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(tokenForAuth[0].client==NULL)
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
                return;
            }
            else
            {
                account_id=QString(databaseBaseLogin.value(0)).toUInt(&ok);
                if(!ok)
                {
                    account_id=0;
                    loginIsWrong(askLoginParam->query_id,0x03,"Account id is not a number");
                    delete askLoginParam;
                    return;
                }
            }
        }
    }
    const QString &queryText=QString(PreparedDBQuery::db_query_characters).arg(account_id).arg(max_character*2);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseLogin.asyncRead(queryText.toLatin1(),this,&EpollClientLoginSlave::character_list_static);
    if(callback==NULL)
    {
        account_id=0;
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
            linkToMaster->newFullOutputQuery(0x11,0x0001,queryNumber);
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

void EpollClientLoginSlave::character_list_static(void *object)
{
    if(object!=NULL)
        static_cast<EpollClientLoginSlave *>(object)->character_list_object();
}

void EpollClientLoginSlave::character_list_object()
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
    character_list_return(askLoginParam->query_id);
    delete askLoginParam;
}

void EpollClientLoginSlave::character_list_return(const quint8 &query_id)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("AskLoginParam"))
    {
        qDebug() << "is not AskLoginParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    //send signals into the server
    #ifndef SERVERBENCHMARK
    std::cout << "Logged the account: " << account_id << std::endl;
    #endif

    if(!internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::serverLogicalGroupAndServerList),EpollClientLoginSlave::serverLogicalGroupAndServerListSize))
        return;

    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    out << (quint8)01;
    if(sendPlayerNumber)
        out << (quint16)max_players;
    else
    {
        if(max_players<=255)
            out << (quint16)255;
        else
            out << (quint16)65535;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(timer_city_capture==NULL)
        out << (quint32)0x00000000;
    else if(timer_city_capture->isActive())
    {
        const qint64 &time=time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
        out << (quint32)time/1000;
    }
    else
        out << (quint32)0x00000000;
    #else
    out << (quint32)0x00000000;
    #endif
    out << (quint8)city.capture.frenquency;

    //common settings
    out << (quint32)waitBeforeConnectAfterKick;
    out << (quint8)forceClientToSendAtMapChange;
    out << (quint8)forcedSpeed;
    out << (quint8)useSP;
    out << (quint8)tcpCork;
    out << (quint8)autoLearn;
    out << (quint8)dontSendPseudo;
    out << (quint8)max_character;
    out << (quint8)min_character;
    out << (quint8)max_pseudo_size;
    out << (quint32)character_delete_time;
    out << (float)rates_xp;
    out << (float)rates_gold;
    out << (float)rates_xp_pow;
    out << (float)rates_drop;
    out << (quint8)maxPlayerMonsters;
    out << (quint8)maxWarehousePlayerMonsters;
    out << (quint8)maxPlayerItems;
    out << (quint8)maxWarehousePlayerItems;
    out << (quint8)chat_allow_all;
    out << (quint8)chat_allow_local;
    out << (quint8)chat_allow_private;
    out << (quint8)chat_allow_clan;
    out << (quint8)factoryPriceChange;
    out << httpDatapackMirror;
    outputData+=datapackHash;
    out.device()->seek(out.device()->pos()+datapackHash.size());
    out << (quint32)map_list.size();

    {
        const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
        QList<CharacterEntry> characterEntryList;
        bool ok;
        int validCharaterCount=0;
        while(databaseBaseLogin.next() && validCharaterCount<max_character)
        {
            accountCharatersCount++;
            CharacterEntry characterEntry;
            characterEntry.character_id=QString(databaseBaseLogin.value(0)).toUInt(&ok);
            if(ok)
            {
                quint32 time_to_delete=QString(databaseBaseLogin.value(3)).toUInt(&ok);
                if(!ok)
                {
                    normalOutput(QStringLiteral("time_to_delete is not number: %1 for %2 fixed by 0").arg(QString(databaseBaseLogin.value(3))).arg(account_id));
                    time_to_delete=0;
                }
                characterEntry.played_time=QString(databaseBaseLogin.value(4)).toUInt(&ok);
                if(!ok)
                {
                    normalOutput(QStringLiteral("played_time is not number: %1 for %2 fixed by 0").arg(databaseBaseLogin.value(4)).arg(account_id));
                    characterEntry.played_time=0;
                }
                characterEntry.last_connect=QString(databaseBaseLogin.value(5)).toUInt(&ok);
                if(!ok)
                {
                    normalOutput(QStringLiteral("last_connect is not number: %1 for %2 fixed by 0").arg(databaseBaseLogin.value(5)).arg(account_id));
                    characterEntry.last_connect=current_time;
                }
                if(current_time>=time_to_delete && time_to_delete!=0)
                    deleteCharacterNow(characterEntry.character_id);
                else
                {
                    if(time_to_delete==0)
                        characterEntry.delete_time_left=0;
                    else
                        characterEntry.delete_time_left=time_to_delete-current_time;
                    characterEntry.pseudo=databaseBaseLogin.value(1);
                    const quint32 &skinIdTemp=QString(databaseBaseLogin.value(2)).toUInt(&ok);
                    if(!ok)
                    {
                        normalOutput(QStringLiteral("character return skin is not number: %1 for %2 fixed by 0").arg(databaseBaseLogin.value(5)).arg(account_id));
                        characterEntry.skinId=0;
                        ok=true;
                    }
                    else
                    {
                        if(skinIdTemp>=(quint32)dictionary_skin.size())
                        {
                            normalOutput(QStringLiteral("character return skin out of range: %1 for %2 fixed by 0").arg(databaseBaseLogin.value(5)).arg(account_id));
                            characterEntry.skinId=0;
                            ok=true;
                        }
                        else
                            characterEntry.skinId=dictionary_skin.at(skinIdTemp);
                    }
                    if(ok)
                    {
                        characterEntry.mapId=QString(databaseBaseLogin.value(6)).toUInt(&ok);
                        if(!ok)
                            normalOutput(QStringLiteral("character return map is not number: %1 for %2 fixed by 0").arg(databaseBaseLogin.value(5)).arg(account_id));
                        else
                        {
                            if(characterEntry.mapId>=dictionary_map.size())
                            {
                                normalOutput(QStringLiteral("character return map id out of range: %1 for %2 fixed by 0").arg(databaseBaseLogin.value(5)).arg(account_id));
                                characterEntry.mapId=-1;
                                //ok=false;
                            }
                            else
                            {
                                if(dictionary_map.at(characterEntry.mapId)==NULL)
                                {
                                    normalOutput(QStringLiteral("character return map id not resolved: %1 for %2 fixed by 0: %3").arg(characterEntry.character_id).arg(account_id).arg(characterEntry.mapId));
                                    characterEntry.mapId=-1;
                                    //ok=false;
                                }
                                else
                                {
                                    characterEntry.mapId=dictionary_map.at(characterEntry.mapId)->id;
                                    validCharaterCount++;
                                }
                            }
                        }
                        if(ok)
                            characterEntryList << characterEntry;
                    }
                }
            }
            else
                normalOutput(QStringLiteral("Character id is not number: %1 for %2").arg(databaseBaseLogin.value(0)).arg(account_id));
        }
        if(max_character==0)
        {
            if(characterEntryList.isEmpty())
            {
                loginIsWrong(query_id,0x05,"Can't create character and don't have character");
                return;
            }
        }

        number_of_character=characterEntryList.size();
        out << (quint8)characterEntryList.size();
        int index=0;
        while(index<characterEntryList.size())
        {
            const CharacterEntry &characterEntry=characterEntryList.at(index);
            out << (quint32)characterEntry.character_id;
            out << characterEntry.pseudo;
            out << (quint8)characterEntry.skinId;
            out << (quint32)characterEntry.delete_time_left;
            out << (quint32)characterEntry.played_time;
            out << (quint32)characterEntry.last_connect;
            out << (qint32)characterEntry.mapId;
            index++;
        }
    }

    postReply(query_id,outputData);
}

void EpollClientLoginSlave::deleteCharacterNow(const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQuery::db_query_monster_by_character_id==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_monster_buff==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_buff is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_monster_skill==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_skill is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_bot_already_beaten==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_bot_already_beaten is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_character==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_character is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_all_item==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_item is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_monster_by_character==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_plant==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_plant is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_quest==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_quest is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_recipes==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_recipes is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_reputation==0)
    {
        errorParsingLayer(QStringLiteral("deleteCharacterNow() Query db_query_delete_reputation is empty, bug"));
        return;
    }
    #endif
    DeleteCharacterNow *deleteCharacterNow=new DeleteCharacterNow;
    deleteCharacterNow->characterId=characterId;

    const QString &queryText=QString(PreparedDBQuery::db_query_monster_by_character_id).arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon.asyncRead(queryText.toLatin1(),this,&EpollClientLoginSlave::deleteCharacterNow_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon.errorMessage());
        delete deleteCharacterNow;
        return;
    }
    else
    {
        paramToPassToCallBack << deleteCharacterNow;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("DeleteCharacterNow");
        #endif
        callbackRegistred << callback;
    }
}

void EpollClientLoginSlave::deleteCharacterNow_static(void *object)
{
    if(object!=NULL)
        static_cast<EpollClientLoginSlave *>(object)->deleteCharacterNow_object();
    databaseBaseCommon.clear();
}

void EpollClientLoginSlave::deleteCharacterNow_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    DeleteCharacterNow *deleteCharacterNow=static_cast<DeleteCharacterNow *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(deleteCharacterNow==NULL)
        abort();
    #endif
    deleteCharacterNow_return(deleteCharacterNow->characterId);
    delete deleteCharacterNow;
}

void EpollClientLoginSlave::deleteCharacterNow_return(const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("DeleteCharacterNow"))
    {
        qDebug() << "is not DeleteCharacterNow" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    bool ok;
    while(databaseBaseCommon.next())
    {
        const quint32 &monsterId=QString(databaseBaseCommon.value(0)).toUInt(&ok);
        if(ok)
        {
            dbQueryWrite(QString(PreparedDBQuery::db_query_delete_monster_buff).arg(monsterId).toUtf8().constData());
            dbQueryWrite(QString(PreparedDBQuery::db_query_delete_monster_skill).arg(monsterId).toUtf8().constData());
        }
    }
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_bot_already_beaten).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_character).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_all_item).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_all_item_warehouse).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_all_item_market).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_monster_by_character).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_monster_warehouse_by_character).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_monster_market_by_character).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_plant).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_quest).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_recipes).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_reputation).arg(characterId).toUtf8().constData());
    dbQueryWrite(QString(PreparedDBQuery::db_query_delete_allow).arg(characterId).toUtf8().constData());
}

void EpollClientLoginSlave::addCharacter(const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQuery::db_query_select_character_by_pseudo==NULL)
    {
        errorParsingLayer(QStringLiteral("addCharacter() Query is empty, bug"));
        return;
    }
    #endif
    if(skinList.isEmpty())
    {
        qDebug() << QStringLiteral("Skin list is empty, unable to add charaters");
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)0x02;
        out << (quint32)0x00000000;
        postReply(query_id,outputData);
        return;
    }
    if(number_of_character>=max_character)
    {
        errorParsingLayer(QStringLiteral("You can't create more account, you have already %1 on %2 allowed").arg(number_of_character).arg(max_character));
        return;
    }
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        errorParsingLayer(QStringLiteral("profile index: %1 out of range (profileList size: %2)").arg(profileIndex).arg(CommonDatapack::commonDatapack.profileList.size()));
        return;
    }
    if(!serverProfileList.at(profileIndex).valid)
    {
        errorParsingLayer(QStringLiteral("profile index: %1 profil not valid").arg(profileIndex));
        return;
    }
    if(pseudo.size()>max_pseudo_size)
    {
        errorParsingLayer(QStringLiteral("pseudo size is too big: %1 because is greater than %2").arg(pseudo.size()).arg(max_pseudo_size));
        return;
    }
    if(skinId>=skinList.size())
    {
        errorParsingLayer(QStringLiteral("skin provided: %1 is not into skin listed").arg(skinId));
        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    if(!profile.forcedskin.isEmpty() && !profile.forcedskin.contains(skinId))
    {
        errorParsingLayer(QStringLiteral("skin provided: %1 is not into profile %2 forced skin list").arg(skinId).arg(profileIndex));
        return;
    }
    AddCharacterParam *addCharacterParam=new AddCharacterParam();
    addCharacterParam->query_id=query_id;
    addCharacterParam->profileIndex=profileIndex;
    addCharacterParam->pseudo=pseudo;
    addCharacterParam->skinId=skinId;

    const QString &queryText=QString(PreparedDBQuery::db_query_select_character_by_pseudo).arg(SqlFunction::quoteSqlVariable(pseudo));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon.asyncRead(queryText.toLatin1(),this,&EpollClientLoginSlave::addCharacter_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon.errorMessage());

        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)0x02;
        out << (quint32)0x00000000;
        postReply(query_id,outputData);
        delete addCharacterParam;
        return;
    }
    else
    {
        paramToPassToCallBack << addCharacterParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("AddCharacterParam");
        #endif
        callbackRegistred << callback;
    }
}

void EpollClientLoginSlave::addCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<EpollClientLoginSlave *>(object)->addCharacter_object();
    databaseBaseCommon.clear();
}

void EpollClientLoginSlave::addCharacter_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    AddCharacterParam *addCharacterParam=static_cast<AddCharacterParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(addCharacterParam==NULL)
        abort();
    #endif
    addCharacter_return(addCharacterParam->query_id,addCharacterParam->profileIndex,addCharacterParam->pseudo,addCharacterParam->skinId);
    delete addCharacterParam;
    databaseBaseCommon.clear();
}

void EpollClientLoginSlave::addCharacter_return(const quint8 &query_id,const quint8 &profileIndex,const QString &pseudo,const quint8 &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("AddCharacterParam"))
    {
        qDebug() << "is not AddCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    if(databaseBaseCommon.next())
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)0x01;
        out << (quint32)0x00000000;
        postReply(query_id,outputData);
        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    const ServerProfile &serverProfile=serverProfileList.at(profileIndex);

    number_of_character++;
    maxCharacterId++;

    const quint32 &characterId=maxCharacterId;
    int index=0;
    int monster_position=1;
    dbQueryWrite(serverProfile.preparedQuery.at(0)+QString::number(characterId)+serverProfile.preparedQuery.at(1)+QString::number(account_id)+serverProfile.preparedQuery.at(2)+pseudo+serverProfile.preparedQuery.at(3)+QString::number(dictionary_skin_reverse.at(skinId))+serverProfile.preparedQuery.at(4).toUtf8().constData());
    while(index<profile.monsters.size())
    {
        const quint32 &monsterId=profile.monsters.at(index).id;
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monsterId))
        {
            const Monster &monster=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monsterId);
            quint32 gender=Gender_Unknown;
            if(monster.ratio_gender!=-1)
            {
                if(rand()%101<monster.ratio_gender)
                    gender=Gender_Female;
                else
                    gender=Gender_Male;
            }
            CatchChallenger::Monster::Stat stat=CatchChallenger::CommonFightEngine::getStat(monster,profile.monsters.at(index).level);
            QList<CatchChallenger::PlayerMonster::PlayerSkill> skills;
            QList<CatchChallenger::Monster::AttackToLearn> attack=monster.learn;
            int sub_index=0;
            while(sub_index<attack.size())
            {
                if(attack.value(sub_index).learnAtLevel<=profile.monsters.at(index).level)
                {
                    CatchChallenger::PlayerMonster::PlayerSkill temp;
                    temp.level=attack.value(sub_index).learnSkillLevel;
                    temp.skill=attack.value(sub_index).learnSkill;
                    temp.endurance=0;
                    skills << temp;
                }
                sub_index++;
            }
            quint32 monster_id;
            {
                QMutexLocker(&monsterIdMutex);
                maxMonsterId++;
                monster_id=maxMonsterId;
            }
            while(skills.size()>4)
                skills.removeFirst();
            {
                dbQueryWrite(PreparedDBQuery::db_query_insert_monster
                   .arg(monster_id)
                   .arg(stat.hp)
                   .arg(characterId)
                   .arg(monsterId)
                   .arg(profile.monsters.at(index).level)
                   .arg(profile.monsters.at(index).captured_with)
                   .arg(gender)
                   .arg(monster_position)
                   .toUtf8().constData());
                monster_position++;
            }
            sub_index=0;
            while(sub_index<skills.size())
            {
                quint8 endurance=0;
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(skills.value(sub_index).skill))
                    if(skills.value(sub_index).level<=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.size() && skills.value(sub_index).level>0)
                        endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.at(skills.value(sub_index).level-1).endurance;
                dbQueryWrite(PreparedDBQuery::db_query_insert_monster_skill
                   .arg(monster_id)
                   .arg(skills.value(sub_index).skill)
                   .arg(skills.value(sub_index).level)
                   .arg(endurance)
                   .toUtf8().constData());
                sub_index++;
            }
            index++;
        }
        else
        {
            errorParsingLayer(QStringLiteral("monster not found to start: %1 is not into profile forced skin list: %2").arg(monsterId));
            return;
        }
    }
    index=0;
    while(index<profile.reputation.size())
    {
        dbQueryWrite(PreparedDBQuery::db_query_insert_reputation
           .arg(characterId)
           .arg(CommonDatapack::commonDatapack.reputation.at(profile.reputation.at(index).reputationId).reverse_database_id)
           .arg(profile.reputation.at(index).point)
           .arg(profile.reputation.at(index).level)
           .toUtf8().constData());
        index++;
    }
    index=0;
    while(index<profile.items.size())
    {
        dbQueryWrite(PreparedDBQuery::db_query_insert_item
           .arg(profile.items.at(index).id)
           .arg(characterId)
           .arg(profile.items.at(index).quantity)
           .toUtf8().constData());
        index++;
    }

    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x00;
    out << characterId;
    postReply(query_id,outputData);
}

void EpollClientLoginSlave::removeCharacter(const quint8 &query_id, const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQuery::db_query_account_time_to_delete_character_by_id==NULL)
    {
        errorParsingLayer(QStringLiteral("removeCharacter() Query is empty, bug"));
        return;
    }
    #endif
    RemoveCharacterParam *removeCharacterParam=new RemoveCharacterParam;
    removeCharacterParam->query_id=query_id;
    removeCharacterParam->characterId=characterId;

    const QString &queryText=PreparedDBQuery::db_query_account_time_to_delete_character_by_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon.asyncRead(queryText.toLatin1(),this,&EpollClientLoginSlave::removeCharacter_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon.errorMessage());
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)0x02;
        postReply(query_id,outputData);
        delete removeCharacterParam;
        return;
    }
    else
    {
        callbackRegistred << callback;
        paramToPassToCallBack << removeCharacterParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("RemoveCharacterParam");
        #endif
    }
}

void EpollClientLoginSlave::removeCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<EpollClientLoginSlave *>(object)->removeCharacter_object();
    databaseBaseCommon.clear();
}

void EpollClientLoginSlave::removeCharacter_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    RemoveCharacterParam *removeCharacterParam=static_cast<RemoveCharacterParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(removeCharacterParam==NULL)
        abort();
    #endif
    removeCharacter_return(removeCharacterParam->query_id,removeCharacterParam->characterId);
    delete removeCharacterParam;
}

void EpollClientLoginSlave::removeCharacter_return(const quint8 &query_id,const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("RemoveCharacterParam"))
    {
        qDebug() << "is not RemoveCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    if(!databaseBaseCommon.next())
    {
        characterSelectionIsWrong(query_id,0x02,"Result return query to remove wrong");
        return;
    }
    bool ok;
    const quint32 &account_id=QString(databaseBaseCommon.value(0)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Account for character: %1 is not an id").arg(databaseBaseCommon.value(0)));
        return;
    }
    if(this->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Character: %1 is not owned by the account: %2").arg(characterId).arg(account_id));
        return;
    }
    const quint32 &time_to_delete=QString(databaseBaseCommon.value(1)).toUInt(&ok);
    if(ok && time_to_delete>0)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Character: %1 is already in deleting for the account: %2").arg(characterId).arg(account_id));
        return;
    }
    dbQueryWrite(PreparedDBQuery::db_query_update_character_time_to_delete_by_id.arg(characterId).arg(character_delete_time).toUtf8().constData());
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    postReply(query_id,outputData);
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
