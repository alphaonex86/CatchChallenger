#include "Client.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonMap.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/ProtocolParsingCheck.h"
#include "SqlFunction.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <QCryptographicHash>

#ifdef EPOLLCATCHCHALLENGERSERVER
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB
    #endif
#endif

/// \todo solve disconnecting/destroy during the SQL loading

using namespace CatchChallenger;

void Client::askLogin(const quint8 &query_id,const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_login.isEmpty())
    {
        errorOutput(QStringLiteral("askLogin() Query login is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_login.isEmpty())
    {
        errorOutput(QStringLiteral("askLogin() Query inset login is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_characters.isEmpty())
    {
        errorOutput(QStringLiteral("askLogin() Query characters is empty, bug"));
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

    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_login.arg(QString(login.toHex()));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::askLogin_static);
    if(callback==NULL)
    {
        loginIsWrong(askLoginParam->query_id,0x04,QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage()));
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

void Client::askLogin_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->askLogin_object();
    GlobalServerData::serverPrivateVariables.db.clear();
    //delete askLoginParam; -> not here because need reuse later
}

void Client::askLogin_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    if(paramToPassToCallBack.size()!=1)
    {
        qDebug() << "paramToPassToCallBack.size()!=1" << __FILE__ << __LINE__;
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

void Client::askLogin_return(AskLoginParam *askLoginParam)
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
        if(!GlobalServerData::serverPrivateVariables.db.next())
        {
            if(GlobalServerData::serverSettings.automatic_account_creation)
            {
                //network send
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(askLoginParam->query_id);
                #endif
                *(Client::loginIsWrongBuffer+1)=(quint8)askLoginParam->query_id;
                *(Client::loginIsWrongBuffer+3)=(quint8)0x07;
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginIsWrongBuffer),sizeof(Client::loginIsWrongBuffer));
                delete askLoginParam;
                is_logging_in_progess=false;
                return;
/*                GlobalServerData::serverPrivateVariables.maxAccountId++;
                account_id=GlobalServerData::serverPrivateVariables.maxAccountId;
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_login.arg(account_id).arg(QString(askLoginParam->login.toHex())).arg(QString(askLoginParam->pass.toHex())).arg(QDateTime::currentDateTime().toTime_t()));*/
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
                while(index<GlobalServerData::serverPrivateVariables.tokenForAuthSize)
                {
                    const TokenLink &tokenLink=GlobalServerData::serverPrivateVariables.tokenForAuth[index];
                    if(tokenLink.client==this)
                    {
                        const QString &secretToken(GlobalServerData::serverPrivateVariables.db.value(1));
                        const QByteArray &secretTokenBinary=QByteArray::fromHex(secretToken.toLatin1());
                        QCryptographicHash hash(QCryptographicHash::Sha224);
                        hash.addData(secretTokenBinary);
                        hash.addData(tokenLink.value,CATCHCHALLENGER_TOKENSIZE);
                        hashedToken=hash.result();
                        GlobalServerData::serverPrivateVariables.tokenForAuthSize--;
                        if(GlobalServerData::serverPrivateVariables.tokenForAuthSize>0)
                        {
                            while(index<GlobalServerData::serverPrivateVariables.tokenForAuthSize)
                            {
                                GlobalServerData::serverPrivateVariables.tokenForAuth[index]=GlobalServerData::serverPrivateVariables.tokenForAuth[index+1];
                                index++;
                            }
                            //don't work:memmove(GlobalServerData::serverPrivateVariables.tokenForAuth+index*sizeof(TokenLink),GlobalServerData::serverPrivateVariables.tokenForAuth+index*sizeof(TokenLink)+sizeof(TokenLink),sizeof(TokenLink)*(GlobalServerData::serverPrivateVariables.tokenForAuthSize-index));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(GlobalServerData::serverPrivateVariables.tokenForAuth[0].client==NULL)
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
                account_id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
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
    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_characters.arg(account_id).arg(CommonSettings::commonSettings.max_character*2);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::character_list_static);
    if(callback==NULL)
    {
        account_id=0;
        loginIsWrong(askLoginParam->query_id,0x04,QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage()));
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

void Client::createAccount(const quint8 &query_id, const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_login.isEmpty())
    {
        errorOutput(QStringLiteral("createAccount() Query login is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_login.isEmpty())
    {
        errorOutput(QStringLiteral("createAccount() Query inset login is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_characters.isEmpty())
    {
        errorOutput(QStringLiteral("createAccount() Query characters is empty, bug"));
        return;
    }
    if(!GlobalServerData::serverSettings.automatic_account_creation)
    {
        errorOutput(QStringLiteral("createAccount() Creation account not premited"));
        return;
    }
    #endif
    if(number_of_character>=CommonSettings::commonSettings.max_character)
    {
        loginIsWrong(query_id,0x03,QStringLiteral("Have already the max charaters: %1/%2").arg(number_of_character).arg(CommonSettings::commonSettings.max_character));
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

    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_login.arg(QString(login.toHex()));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::createAccount_static);
    if(callback==NULL)
    {
        is_logging_in_progess=false;
        loginIsWrong(askLoginParam->query_id,0x03,QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage()));
        delete askLoginParam;
        return;
    }
    else
    {
        number_of_character++;
        paramToPassToCallBack << askLoginParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("AskLoginParam");
        #endif
        callbackRegistred << callback;
    }
}

void Client::createAccount_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->createAccount_object();
    GlobalServerData::serverPrivateVariables.db.clear();
}

void Client::createAccount_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    if(paramToPassToCallBack.size()!=1)
    {
        qDebug() << "paramToPassToCallBack.size()!=1" << __FILE__ << __LINE__;
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

void Client::createAccount_return(AskLoginParam *askLoginParam)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("AskLoginParam"))
    {
        qDebug() << "is not AskLoginParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    if(!GlobalServerData::serverPrivateVariables.db.next())
    {
        //network send
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //removeFromQueryReceived(askLoginParam->query_id);//->only if use fast path
        #endif
        GlobalServerData::serverPrivateVariables.maxAccountId++;
        account_id=GlobalServerData::serverPrivateVariables.maxAccountId;
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_login.arg(account_id).arg(QString(askLoginParam->login.toHex())).arg(QString(askLoginParam->pass.toHex())).arg(QDateTime::currentDateTime().toTime_t()));
        //send the network reply
        QByteArray outputData;
        outputData[0x00]=0x01;
        postReply(askLoginParam->query_id,outputData);
        is_logging_in_progess=false;
    }
    else
        loginIsWrong(askLoginParam->query_id,0x02,QStringLiteral("Login already used: %1").arg(QString(askLoginParam->login.toHex())));
}

void Client::character_list_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->character_list_object();
}

void Client::character_list_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    if(paramToPassToCallBack.size()!=1)
    {
        qDebug() << "paramToPassToCallBack.size()!=1" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    askLoginParam->tempOutputData=character_list_return(askLoginParam->query_id);
    //re use
    //delete askLoginParam;
    if(server_list())
        paramToPassToCallBack<< askLoginParam;
}

void Client::character_list_return(const quint8 &query_id)
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
    normalOutput(QStringLiteral("Logged the account %1").arg(account_id));
    #endif
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    out << (quint8)01;
    if(GlobalServerData::serverSettings.sendPlayerNumber)
        out << (quint16)GlobalServerData::serverSettings.max_players;
    else
    {
        if(GlobalServerData::serverSettings.max_players<=255)
            out << (quint16)255;
        else
            out << (quint16)65535;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(GlobalServerData::serverPrivateVariables.timer_city_capture==NULL)
        out << (quint32)0x00000000;
    else if(GlobalServerData::serverPrivateVariables.timer_city_capture->isActive())
    {
        const qint64 &time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
        out << (quint32)time/1000;
    }
    else
        out << (quint32)0x00000000;
    #else
    out << (quint32)0x00000000;
    #endif
    out << (quint8)GlobalServerData::serverSettings.city.capture.frenquency;

    //common settings
    out << (quint32)CommonSettings::commonSettings.waitBeforeConnectAfterKick;
    out << (quint8)CommonSettings::commonSettings.forceClientToSendAtMapChange;
    out << (quint8)CommonSettings::commonSettings.forcedSpeed;
    out << (quint8)CommonSettings::commonSettings.useSP;
    out << (quint8)CommonSettings::commonSettings.tcpCork;
    out << (quint8)CommonSettings::commonSettings.autoLearn;
    out << (quint8)CommonSettings::commonSettings.dontSendPseudo;
    out << (quint8)CommonSettings::commonSettings.max_character;
    out << (quint8)CommonSettings::commonSettings.min_character;
    out << (quint8)CommonSettings::commonSettings.max_pseudo_size;
    out << (quint32)CommonSettings::commonSettings.character_delete_time;
    out << (float)CommonSettings::commonSettings.rates_xp;
    out << (float)CommonSettings::commonSettings.rates_gold;
    out << (float)CommonSettings::commonSettings.rates_xp_pow;
    out << (float)CommonSettings::commonSettings.rates_drop;
    out << (quint8)CommonSettings::commonSettings.maxPlayerMonsters;
    out << (quint8)CommonSettings::commonSettings.maxWarehousePlayerMonsters;
    out << (quint8)CommonSettings::commonSettings.maxPlayerItems;
    out << (quint8)CommonSettings::commonSettings.maxWarehousePlayerItems;
    out << (quint8)CommonSettings::commonSettings.chat_allow_all;
    out << (quint8)CommonSettings::commonSettings.chat_allow_local;
    out << (quint8)CommonSettings::commonSettings.chat_allow_private;
    out << (quint8)CommonSettings::commonSettings.chat_allow_clan;
    out << (quint8)CommonSettings::commonSettings.factoryPriceChange;
    out << CommonSettings::commonSettings.httpDatapackMirror;
    outputData+=CommonSettings::commonSettings.datapackHash;
    out.device()->seek(out.device()->pos()+CommonSettings::commonSettings.datapackHash.size());
    out << (quint32)GlobalServerData::serverPrivateVariables.map_list.size();

    {
        const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
        QList<CharacterEntry> characterEntryList;
        bool ok;
        int validCharaterCount=0;
        while(GlobalServerData::serverPrivateVariables.db.next() && validCharaterCount<CommonSettings::commonSettings.max_character)
        {
            CharacterEntry characterEntry;
            characterEntry.character_id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
            if(ok)
            {
                quint32 time_to_delete=QString(GlobalServerData::serverPrivateVariables.db.value(3)).toUInt(&ok);
                if(!ok)
                {
                    normalOutput(QStringLiteral("time_to_delete is not number: %1 for %2 fixed by 0").arg(QString(GlobalServerData::serverPrivateVariables.db.value(3))).arg(account_id));
                    time_to_delete=0;
                }
                characterEntry.played_time=QString(GlobalServerData::serverPrivateVariables.db.value(4)).toUInt(&ok);
                if(!ok)
                {
                    normalOutput(QStringLiteral("played_time is not number: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db.value(4)).arg(account_id));
                    characterEntry.played_time=0;
                }
                characterEntry.last_connect=QString(GlobalServerData::serverPrivateVariables.db.value(5)).toUInt(&ok);
                if(!ok)
                {
                    normalOutput(QStringLiteral("last_connect is not number: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db.value(5)).arg(account_id));
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
                    characterEntry.pseudo=GlobalServerData::serverPrivateVariables.db.value(1);
                    const quint32 &skinIdTemp=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok);
                    if(!ok)
                    {
                        normalOutput(QStringLiteral("character return skin is not number: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db.value(5)).arg(account_id));
                        characterEntry.skinId=0;
                        ok=true;
                    }
                    else
                    {
                        if(skinIdTemp>=(quint32)GlobalServerData::serverPrivateVariables.dictionary_skin.size())
                        {
                            normalOutput(QStringLiteral("character return skin out of range: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db.value(5)).arg(account_id));
                            characterEntry.skinId=0;
                            ok=true;
                        }
                        else
                            characterEntry.skinId=GlobalServerData::serverPrivateVariables.dictionary_skin.at(skinIdTemp);
                    }
                    if(ok)
                    {
                        characterEntry.mapId=QString(GlobalServerData::serverPrivateVariables.db.value(6)).toUInt(&ok);
                        if(!ok)
                            normalOutput(QStringLiteral("character return map is not number: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db.value(5)).arg(account_id));
                        else
                        {
                            if(characterEntry.mapId>=GlobalServerData::serverPrivateVariables.dictionary_map.size())
                            {
                                normalOutput(QStringLiteral("character return map id out of range: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db.value(5)).arg(account_id));
                                characterEntry.mapId=-1;
                                //ok=false;
                            }
                            else
                            {
                                if(GlobalServerData::serverPrivateVariables.dictionary_map.at(characterEntry.mapId)==NULL)
                                {
                                    normalOutput(QStringLiteral("character return map id not resolved: %1 for %2 fixed by 0: %3").arg(characterEntry.character_id).arg(account_id).arg(characterEntry.mapId));
                                    characterEntry.mapId=-1;
                                    //ok=false;
                                }
                                else
                                {
                                    characterEntry.mapId=GlobalServerData::serverPrivateVariables.dictionary_map.at(characterEntry.mapId)->id;
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
                normalOutput(QStringLiteral("Character id is not number: %1 for %2").arg(GlobalServerData::serverPrivateVariables.db.value(0)).arg(account_id));
        }
        if(CommonSettings::commonSettings.max_character==0)
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
            put into utf8
            out << characterEntry.pseudo;
            out << (quint8)characterEntry.skinId;
            out << (quint32)characterEntry.delete_time_left;
            out << (quint32)characterEntry.played_time;
            out << (quint32)characterEntry.last_connect;
            index++;
        }
    }

    return outputData;
}

bool Client::server_list()
{
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(GlobalServerData::serverPrivateVariables.db_query_select_server.arg(character_id).toLatin1(),this,&Client::server_list_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(GlobalServerData::serverPrivateVariables.db_query_select_allow).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        errorOutput("Unable to get the server list");
        return false;
    }
    else
    {
        callbackRegistred << callback;
        return true;
    }
}

void Client::server_list_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->server_list_object();
}

void Client::server_list_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    if(paramToPassToCallBack.size()!=1)
    {
        qDebug() << "paramToPassToCallBack.size()!=1" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    askLoginParam->tempOutputData=server_list_return(askLoginParam->query_id,askLoginParam->tempOutputData);
    delete askLoginParam;
}

void Client::server_list_return(const quint8 &query_id, const QByteArray &previousData)
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
    normalOutput(QStringLiteral("Logged the account %1").arg(account_id));
    #endif

    //C20F
    {
        //no logical group
        QByteArray outputData;
        outputData[0]=0x01;
        outputData[1]=0x00;
        outputData[2]=0x00;
        sendFullPacket(0xC2,0x0F,outputData.constData(),outputData.size());
    }
    //C20E
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x02;//Server mode, unique then proxy
        out << (quint8)0x01;//server list size, only this alone server
        out << (quint8)0x00;//charactersgroup empty
        out << (quint32)0x00000000;//unique key, useless here
        out << (quint16)0x0000;//description is empty
        out << (quint8)0x00;//logical group empty
        if(GlobalServerData::serverSettings.sendPlayerNumber)
        {
            out << (quint16)GlobalServerData::serverSettings.max_players;
            out << (quint16)Client::clientBroadCastList.size();//charactersgroup empty
        }
        else
        {
            if(GlobalServerData::serverSettings.max_players<=255)
                out << (quint16)255;
            else
                out << (quint16)65535;
            out << (quint16)0x0000;//current player
        }
        sendFullPacket(0xC2,0x0E,outputData.constData(),outputData.size());
    }
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    out << (quint8)0x01;//all is good
    out << (quint32)CommonSettings::commonSettings.character_delete_time;
    out << (quint8)CommonSettings::commonSettings.max_character;
    out << (quint8)CommonSettings::commonSettings.min_character;
    out << (quint8)CommonSettings::commonSettings.max_pseudo_size;

    out << (quint8)CommonSettings::commonSettings.maxPlayerMonsters;
    out << (quint16)CommonSettings::commonSettings.maxWarehousePlayerMonsters;
    out << (quint8)CommonSettings::commonSettings.maxPlayerItems;
    out << (quint16)CommonSettings::commonSettings.maxWarehousePlayerItems;

    outputData+=CommonSettings::commonSettings.datapackHash;
    out.device()->seek(out.device()->pos()+CommonSettings::commonSettings.datapackHash.size());
    {
        QByteArray httpDatapackMirrorData;
        httpDatapackMirrorData.resize(300);
        httpDatapackMirrorData.resize(FacilityLibGeneral::toUTF8WithHeader(CommonSettings::commonSettings.httpDatapackMirror,httpDatapackMirrorData.data()));
        outputData+=httpDatapackMirrorData;
        out.device()->seek(out.device()->pos()+httpDatapackMirrorData.size());
    }

    char * const tempRawData=new char[4*1024];
    //memset(tempRawData,0x00,sizeof(4*1024));
    int tempRawDataSize=0x01;

    const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
    bool ok;
    quint8 validServerCount=0;
    while(databaseBaseCommon->next() && validServerCount<EpollClientLoginSlave::max_character)
    {
        unsigned int server_id=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(ok)
        {
            qint16 serverIndex=-1;
            if(server_id<(unsigned int)CharactersGroupForLogin::dictionary_server_database_to_index.size())
                if(CharactersGroupForLogin::dictionary_server_database_to_index.at(server_id)!=-1)
                    serverIndex=CharactersGroupForLogin::dictionary_server_database_to_index.at(server_id);
            if(serverIndex!=-1)
            {
                //server index
                tempRawData[tempRawDataSize]=serverIndex;
                tempRawDataSize+=1;

                //played_time
                unsigned int played_time=QString(databaseBaseCommon->value(1)).toUInt(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("played_time is not number: %1 fixed by 0").arg(databaseBaseCommon->value(4)));
                    played_time=0;
                }
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(played_time);
                tempRawDataSize+=sizeof(quint32);

                //last_connect
                unsigned int last_connect=QString(databaseBaseCommon->value(2)).toUInt(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("last_connect is not number: %1 fixed by 0").arg(databaseBaseCommon->value(5)));
                    last_connect=current_time;
                }
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(last_connect);
                tempRawDataSize+=sizeof(quint32);

                validServerCount++;
            }
        }
        else
            qDebug() << (QStringLiteral("Character id is not number: %1").arg(databaseBaseCommon->value(0)));
    }
    tempRawData[0]=validServerCount;

    postReply(query_id,previousData+outputData+QByteArray(tempRawData,tempRawDataSize));
}

void Client::deleteCharacterNow(const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_monster_by_character_id.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_monster_buff.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_buff is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_monster_skill.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_skill is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_bot_already_beaten.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_bot_already_beaten is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_character.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_character is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_all_item.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_item is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_character.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_plant.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_plant is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_quest.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_quest is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_recipes.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_recipes is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_reputation.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_reputation is empty, bug"));
        return;
    }
    #endif
    DeleteCharacterNow *deleteCharacterNow=new DeleteCharacterNow;
    deleteCharacterNow->characterId=characterId;

    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_monster_by_character_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::deleteCharacterNow_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
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

void Client::deleteCharacterNow_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->deleteCharacterNow_object();
    GlobalServerData::serverPrivateVariables.db.clear();
}

void Client::deleteCharacterNow_object()
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

void Client::deleteCharacterNow_return(const quint32 &characterId)
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
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        const quint32 &monsterId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(ok)
        {
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_buff.arg(monsterId));
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_skill.arg(monsterId));
        }
    }
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_bot_already_beaten.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_character.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_all_item.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_all_item_warehouse.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_all_item_market.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_character.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_warehouse_by_character.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_market_by_character.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_plant.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_quest.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_recipes.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_reputation.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_allow.arg(characterId));
}

void Client::addCharacter(const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_character_by_pseudo.isEmpty())
    {
        errorOutput(QStringLiteral("addCharacter() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_monster.isEmpty())
    {
        errorOutput(QStringLiteral("addCharacter() Query db_query_insert_monster is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_monster_skill.isEmpty())
    {
        errorOutput(QStringLiteral("addCharacter() Query db_query_insert_monster_skill is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_reputation.isEmpty())
    {
        errorOutput(QStringLiteral("addCharacter() Query db_query_insert_reputation is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_item.isEmpty())
    {
        errorOutput(QStringLiteral("addCharacter() Query db_query_insert_item is empty, bug"));
        return;
    }
    #endif
    if(GlobalServerData::serverPrivateVariables.skinList.isEmpty())
    {
        qDebug() << QStringLiteral("Skin list is empty, unable to add charaters");
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x02;
        out << (quint32)0x00000000;
        postReply(query_id,outputData);
        return;
    }
    if(number_of_character>=CommonSettings::commonSettings.max_character)
    {
        errorOutput(QStringLiteral("You can't create more account, you have already %1 on %2 allowed").arg(number_of_character).arg(CommonSettings::commonSettings.max_character));
        return;
    }
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        errorOutput(QStringLiteral("profile index: %1 out of range (profileList size: %2)").arg(profileIndex).arg(CommonDatapack::commonDatapack.profileList.size()));
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.serverProfileList.at(profileIndex).valid)
    {
        errorOutput(QStringLiteral("profile index: %1 profil not valid").arg(profileIndex));
        return;
    }
    if(pseudo.size()>CommonSettings::commonSettings.max_pseudo_size)
    {
        errorOutput(QStringLiteral("pseudo size is too big: %1 because is greater than %2").arg(pseudo.size()).arg(CommonSettings::commonSettings.max_pseudo_size));
        return;
    }
    if(skinId>=GlobalServerData::serverPrivateVariables.skinList.size())
    {
        errorOutput(QStringLiteral("skin provided: %1 is not into skin listed").arg(skinId));
        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    if(!profile.forcedskin.isEmpty() && !profile.forcedskin.contains(skinId))
    {
        errorOutput(QStringLiteral("skin provided: %1 is not into profile %2 forced skin list").arg(skinId).arg(profileIndex));
        return;
    }
    AddCharacterParam *addCharacterParam=new AddCharacterParam();
    addCharacterParam->query_id=query_id;
    addCharacterParam->profileIndex=profileIndex;
    addCharacterParam->pseudo=pseudo;
    addCharacterParam->skinId=skinId;

    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_select_character_by_pseudo.arg(SqlFunction::quoteSqlVariable(pseudo));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::addCharacter_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());

        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
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

void Client::addCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->addCharacter_object();
    GlobalServerData::serverPrivateVariables.db.clear();
}

void Client::addCharacter_object()
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
    GlobalServerData::serverPrivateVariables.db.clear();
}

void Client::addCharacter_return(const quint8 &query_id,const quint8 &profileIndex,const QString &pseudo,const quint8 &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("AddCharacterParam"))
    {
        qDebug() << "is not AddCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    if(GlobalServerData::serverPrivateVariables.db.next())
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x01;
        out << (quint32)0x00000000;
        postReply(query_id,outputData);
        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    const ServerProfile &serverProfile=GlobalServerData::serverPrivateVariables.serverProfileList.at(profileIndex);

    number_of_character++;
    GlobalServerData::serverPrivateVariables.maxCharacterId++;

    const quint32 &characterId=GlobalServerData::serverPrivateVariables.maxCharacterId;
    int index=0;
    int monster_position=1;
    dbQueryWrite(serverProfile.preparedQuery.at(0)+QString::number(characterId)+serverProfile.preparedQuery.at(1)+QString::number(account_id)+serverProfile.preparedQuery.at(2)+pseudo+serverProfile.preparedQuery.at(3)+QString::number(GlobalServerData::serverPrivateVariables.dictionary_skin_reverse.at(skinId))+serverProfile.preparedQuery.at(4));
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
            const QList<CatchChallenger::PlayerMonster::PlayerSkill> &skills=CommonFightEngine::generateWildSkill(monster,profile.monsters.at(index).level);

            quint32 monster_id;
            {
                QMutexLocker(&GlobalServerData::serverPrivateVariables.monsterIdMutex);
                GlobalServerData::serverPrivateVariables.maxMonsterId++;
                monster_id=GlobalServerData::serverPrivateVariables.maxMonsterId;
            }
            {
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster
                   .arg(monster_id)
                   .arg(stat.hp)
                   .arg(characterId)
                   .arg(monsterId)
                   .arg(profile.monsters.at(index).level)
                   .arg(profile.monsters.at(index).captured_with)
                   .arg(gender)
                   .arg(monster_position)
                    );
                monster_position++;
            }
            sub_index=0;
            while(sub_index<skills.size())
            {
                quint8 endurance=0;
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(skills.value(sub_index).skill))
                    if(skills.value(sub_index).level<=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.size() && skills.value(sub_index).level>0)
                        endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.at(skills.value(sub_index).level-1).endurance;
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster_skill
                   .arg(monster_id)
                   .arg(skills.value(sub_index).skill)
                   .arg(skills.value(sub_index).level)
                   .arg(endurance)
                        );
                sub_index++;
            }
            index++;
        }
        else
        {
            errorOutput(QStringLiteral("monster not found to start: %1 is not into profile forced skin list: %2").arg(monsterId));
            return;
        }
    }
    index=0;
    while(index<profile.reputation.size())
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_reputation
           .arg(characterId)
           .arg(CommonDatapack::commonDatapack.reputation.at(profile.reputation.at(index).reputationId).reverse_database_id)
           .arg(profile.reputation.at(index).point)
           .arg(profile.reputation.at(index).level)
                );
        index++;
    }
    index=0;
    while(index<profile.items.size())
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_item
           .arg(profile.items.at(index).id)
           .arg(characterId)
           .arg(profile.items.at(index).quantity)
                );
        index++;
    }

    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x00;
    out << characterId;
    postReply(query_id,outputData);
}

void Client::removeCharacter(const quint8 &query_id, const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_account_time_to_delete_character_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("removeCharacter() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("removeCharacter() Query db_query_update_character_time_to_delete_by_id is empty, bug"));
        return;
    }
    #endif
    RemoveCharacterParam *removeCharacterParam=new RemoveCharacterParam;
    removeCharacterParam->query_id=query_id;
    removeCharacterParam->characterId=characterId;

    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_account_time_to_delete_character_by_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::removeCharacter_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
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

void Client::removeCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->removeCharacter_object();
    GlobalServerData::serverPrivateVariables.db.clear();
}

void Client::removeCharacter_object()
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

void Client::removeCharacter_return(const quint8 &query_id,const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("RemoveCharacterParam"))
    {
        qDebug() << "is not RemoveCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    if(!GlobalServerData::serverPrivateVariables.db.next())
    {
        characterSelectionIsWrong(query_id,0x02,"Result return query to remove wrong");
        return;
    }
    bool ok;
    const quint32 &account_id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Account for character: %1 is not an id").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
        return;
    }
    if(this->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Character: %1 is not owned by the account: %2").arg(characterId).arg(account_id));
        return;
    }
    const quint32 &time_to_delete=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
    if(ok && time_to_delete>0)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Character: %1 is already in deleting for the account: %2").arg(characterId).arg(account_id));
        return;
    }
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete_by_id.arg(characterId).arg(CommonSettings::commonSettings.character_delete_time));
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    postReply(query_id,outputData);
}

//load linked data (like item, quests, ...)
void Client::loadLinkedData()
{
    loadPlayerAllow();
}

bool Client::loadTheRawUTF8String()
{
    rawPseudo=FacilityLib::toUTF8WithHeader(public_and_private_informations.public_informations.pseudo);
    if(rawPseudo.isEmpty())
    {
        normalOutput(QStringLiteral("Unable to convert the pseudo to utf8: %1").arg(public_and_private_informations.public_informations.pseudo));
        return false;
    }
    return true;
}

QHash<QString, quint32> Client::datapack_file_list_cached()
{
    if(GlobalServerData::serverSettings.datapackCache==-1)
        return datapack_file_list();
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp==0)
        {
            Client::datapack_list_cache_timestamp=QDateTime::currentDateTime().toTime_t();
            Client::datapack_file_list_cache=datapack_file_list();
        }
        return Client::datapack_file_list_cache;
    }
    else
    {
        const quint64 &currentTime=QDateTime::currentDateTime().toTime_t();
        if(Client::datapack_list_cache_timestamp<(currentTime-GlobalServerData::serverSettings.datapackCache))
        {
            Client::datapack_list_cache_timestamp=currentTime;
            Client::datapack_file_list_cache=datapack_file_list();
        }
        return Client::datapack_file_list_cache;
    }
}

QHash<QString,quint32> Client::datapack_file_list()
{
    QHash<QString,quint32> filesList;

    const QStringList &returnList=FacilityLib::listFolder(GlobalServerData::serverSettings.datapack_basePath);
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        #ifdef Q_OS_WIN32
        QString fileName=returnList.at(index);
        #else
        const QString &fileName=returnList.at(index);
        #endif
        if(fileName.contains(GlobalServerData::serverPrivateVariables.datapack_rightFileName))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(GlobalServerData::serverSettings.datapack_basePath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        #ifdef Q_OS_WIN32
                        fileName.replace(Client::text_antislash,Client::text_slash);//remplace if is under windows server
                        #endif
                        QCryptographicHash hashFile(QCryptographicHash::Sha224);
                        hashFile.addData(file.readAll());
                        filesList[fileName]=*reinterpret_cast<const int *>(hashFile.result().constData());
                        file.close();
                    }
                }
            }
        }
        index++;
    }
    return filesList;
}

//check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
void Client::datapackList(const quint8 &query_id,const QStringList &files,const QList<quint32> &partialHashList)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    //do in network read to prevent DDOS
    if(!CommonSettings::commonSettings.httpDatapackMirror.isEmpty())
    {
        errorOutput("Can't use because mirror is defined");
        return;
    }
    #endif
    tempDatapackListReplyArray.clear();
    tempDatapackListReplyTestCount=0;
    rawFiles.clear();
    compressedFiles.clear();
    rawFilesCount=0;
    compressedFilesCount=0;
    tempDatapackListReply=0;
    tempDatapackListReplySize=0;
    const QHash<QString,quint32> &filesList=datapack_file_list_cached();
    QList<FileToSend> fileToSendList;

    const int &loop_size=files.size();
    //send the size to download on the client
    {
        QHash<QString,quint32> filesListForSize(filesList);
        int index=0;
        quint32 datapckFileNumber=0;
        quint32 datapckFileSize=0;
        while(index<loop_size)
        {
            const QString &fileName=files.at(index);
            const quint32 &remote_partialHash=partialHashList.at(index);
            if(fileName.contains(Client::text_dotslash) || fileName.contains(Client::text_antislash) || fileName.contains(Client::text_double_slash))
            {
                errorOutput(QStringLiteral("file name contains illegale char: %1").arg(fileName));
                return;
            }
            if(fileName.contains(fileNameStartStringRegex) || fileName.startsWith(Client::text_slash))
            {
                errorOutput(QStringLiteral("start with wrong string: %1").arg(fileName));
                return;
            }
            if(filesListForSize.contains(fileName))
            {
                quint32 partialHash;
                quint32 server_file_mtime;
                server_file_mtime=filesListForSize.value(fileName);
                if(!Client::datapack_file_hash_cache.contains(fileName))
                {
                    QFile file(GlobalServerData::serverSettings.datapack_basePath+fileName);
                    if(file.open(QIODevice::ReadOnly))
                    {
                        QCryptographicHash hashFile(QCryptographicHash::Sha224);
                        hashFile.addData(file.readAll());
                        Client::DatapackCacheFile newCacheFile;
                        newCacheFile.mtime=QFileInfo(file).lastModified().toTime_t();
                        newCacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                        partialHash=newCacheFile.partialHash;
                        Client::datapack_file_hash_cache[fileName]=newCacheFile;
                        file.close();
                    }
                    else
                    {
                        errorOutput(QStringLiteral("unable to read: %1").arg(fileName));
                        return;
                    }
                }
                else
                {
                    const Client::DatapackCacheFile &cacheFile=Client::datapack_file_hash_cache.value(fileName);
                    if(cacheFile.mtime==server_file_mtime)
                        partialHash=cacheFile.partialHash;
                    else
                    {
                        QFile file(GlobalServerData::serverSettings.datapack_basePath+fileName);
                        if(file.open(QIODevice::ReadOnly))
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(file.readAll());
                            Client::DatapackCacheFile newCacheFile;
                            newCacheFile.mtime=QFileInfo(file).lastModified().toTime_t();
                            newCacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                            partialHash=newCacheFile.partialHash;
                            Client::datapack_file_hash_cache[fileName]=newCacheFile;
                            file.close();
                        }
                        else
                        {
                            errorOutput(QStringLiteral("unable to read: %1").arg(fileName));
                            return;
                        }
                    }
                }
                if(partialHash==remote_partialHash)
                    addDatapackListReply(false);//found
                else
                {
                    addDatapackListReply(false);//found but updated
                    datapckFileNumber++;
                    datapckFileSize+=QFile(GlobalServerData::serverSettings.datapack_basePath+fileName).size();
                    FileToSend fileToSend;
                    fileToSend.file=fileName;
                    fileToSendList << fileToSend;
                }
                filesListForSize.remove(fileName);
            }
            else
                addDatapackListReply(true);//to delete
            index++;
        }
        QHashIterator<QString,quint32> i(filesListForSize);
        while (i.hasNext()) {
            i.next();
            datapckFileNumber++;
            datapckFileSize+=QFile(GlobalServerData::serverSettings.datapack_basePath+i.key()).size();
            FileToSend fileToSend;
            fileToSend.file=i.key();
            fileToSendList << fileToSend;
        }
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint32)datapckFileNumber;
        out << (quint32)datapckFileSize;
        sendFullPacket(0xC2,0x000C,outputData);
    }
    if(fileToSendList.isEmpty())
    {
        errorOutput("Ask datapack list where the checksum match");
        return;
    }
    qSort(fileToSendList);
    if(CommonSettings::commonSettings.httpDatapackMirror.isEmpty())
    {
        //validate, remove or update the file actualy on the client
        if(tempDatapackListReplyTestCount!=files.size())
        {
            errorOutput("Bit count return not match");
            return;
        }
        //send not in the list
        {
            int index=0;
            while(index<fileToSendList.size())
            {
                sendFile(fileToSendList.at(index).file);
                index++;
            }
        }
        sendFileContent();
        sendCompressedFileContent();
        purgeDatapackListReply(query_id);
    }
    else
    {
        QByteArray outputData(FacilityLib::toUTF8WithHeader(CommonSettings::commonSettings.httpDatapackMirror));
        if(outputData.size()>255 || outputData.isEmpty())
        {
            errorOutput(QLatin1Literal("httpDatapackMirror too big or not compatible with utf8"));
            return;
        }
        //validate, remove or update the file actualy on the client
        if(tempDatapackListReplyTestCount!=files.size())
        {
            errorOutput("Bit count return not match");
            return;
        }
        if(!fileToSendList.isEmpty())
        {
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            out.device()->seek(out.device()->size());
            out << (quint32)fileToSendList.size();
            quint32 index=0;
            const quint32 &fileHttpListNameSize=fileToSendList.size();
            while(index<fileHttpListNameSize)
            {
                const QByteArray &rawFileName=FacilityLib::toUTF8WithHeader(fileToSendList.at(index).file);
                if(rawFileName.size()>255 || rawFileName.isEmpty())
                {
                    errorOutput(QLatin1Literal("file path too big or not compatible with utf8"));
                    return;
                }
                outputData+=rawFileName;
                out.device()->seek(out.device()->size());
                index++;
            }
            sendFullPacket(0xC2,0x000D,outputData);
        }
        purgeDatapackListReply(query_id);
    }
}

bool CatchChallenger::operator<(const CatchChallenger::FileToSend &fileToSend1,const CatchChallenger::FileToSend &fileToSend2)
{
    if(fileToSend1.file<fileToSend2.file)
        return false;
    return true;
}

bool CatchChallenger::operator!=(const CatchChallenger::PlayerMonster &monster1,const CatchChallenger::PlayerMonster &monster2)
{
    if(monster1.remaining_xp!=monster2.remaining_xp)
        return true;
    if(monster1.sp!=monster2.sp)
        return true;
    if(monster1.egg_step!=monster2.egg_step)
        return true;
    if(monster1.id!=monster2.id)
        return true;
/* transfer in 0.6 and check it
    if(monster1.character_origin!=monster2.character_origin)
        return true;*/
    if(monster1.skills.size()!=monster2.skills.size())
        return true;
    int index=0;
    while(index<monster1.skills.size())
    {
        const PlayerMonster::PlayerSkill &skill1=monster1.skills.at(index);
        const PlayerMonster::PlayerSkill &skill2=monster2.skills.at(index);
        if(skill1.endurance!=skill2.endurance)
            return true;
        if(skill1.level!=skill2.level)
            return true;
        if(skill1.skill!=skill2.skill)
            return true;
        index++;
    }
    return false;
}

void Client::addDatapackListReply(const bool &fileRemove)
{
    tempDatapackListReplyTestCount++;
    switch(tempDatapackListReplySize)
    {
        case 0:
            if(fileRemove)
                tempDatapackListReply|=0x01;
            else
                tempDatapackListReply&=~0x01;
        break;
        case 1:
            if(fileRemove)
                tempDatapackListReply|=0x02;
            else
                tempDatapackListReply&=~0x02;
        break;
        case 2:
            if(fileRemove)
                tempDatapackListReply|=0x04;
            else
                tempDatapackListReply&=~0x04;
        break;
        case 3:
            if(fileRemove)
                tempDatapackListReply|=0x08;
            else
                tempDatapackListReply&=~0x08;
        break;
        case 4:
            if(fileRemove)
                tempDatapackListReply|=0x10;
            else
                tempDatapackListReply&=~0x10;
        break;
        case 5:
            if(fileRemove)
                tempDatapackListReply|=0x20;
            else
                tempDatapackListReply&=~0x20;
        break;
        case 6:
            if(fileRemove)
                tempDatapackListReply|=0x40;
            else
                tempDatapackListReply&=~0x40;
        break;
        case 7:
            if(fileRemove)
                tempDatapackListReply|=0x80;
            else
                tempDatapackListReply&=~0x80;
        break;
        default:
        break;
    }
    tempDatapackListReplySize++;
    if(tempDatapackListReplySize>=8)
    {
        tempDatapackListReplyArray[tempDatapackListReplyArray.size()]=tempDatapackListReply;
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
}

void Client::purgeDatapackListReply(const quint8 &query_id)
{
    if(tempDatapackListReplySize>0)
    {
        tempDatapackListReplyArray[tempDatapackListReplyArray.size()]=tempDatapackListReply;
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
    if(tempDatapackListReplyArray.isEmpty())
        tempDatapackListReplyArray[0x00]=0x00;
    postReply(query_id,tempDatapackListReplyArray);
    tempDatapackListReplyArray.clear();
}

void Client::sendFileContent()
{
    if(rawFiles.size()>0 && rawFilesCount>0)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)rawFilesCount;
        sendFullPacket(0xC2,0x03,outputData+rawFiles);
        rawFiles.clear();
        rawFilesCount=0;
    }
}

void Client::sendCompressedFileContent()
{
    if(compressedFiles.size()>0 && compressedFilesCount>0)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)compressedFilesCount;
        sendFullPacket(0xC2,0x04,outputData+compressedFiles);
        compressedFiles.clear();
        compressedFilesCount=0;
    }
}

bool Client::sendFile(const QString &fileName)
{
    if(fileName.size()>255 || fileName.isEmpty())
        return false;
    const QByteArray &fileNameRaw=FacilityLib::toUTF8WithHeader(fileName);
    if(fileNameRaw.size()>255 || fileNameRaw.isEmpty())
        return false;
    QFile file(GlobalServerData::serverSettings.datapack_basePath+fileName);
    if(file.open(QIODevice::ReadOnly))
    {
        const QByteArray &content=file.readAll();
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint32)content.size();
        if(compressedExtension.contains(QFileInfo(file).suffix()) && ProtocolParsing::compressionType!=ProtocolParsing::CompressionType_None && content.size()<CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024)
        {
            compressedFiles+=fileNameRaw+outputData+content;
            compressedFilesCount++;
            switch(ProtocolParsing::compressionType)
            {
                case ProtocolParsing::CompressionType_Xz:
                if(compressedFiles.size()>CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024 || compressedFilesCount>=255)
                    sendCompressedFileContent();
                break;
                default:
                case ProtocolParsing::CompressionType_Zlib:
                if(compressedFiles.size()>CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024 || compressedFilesCount>=255)
                    sendCompressedFileContent();
                break;
            }
        }
        else
        {
            if(content.size()>CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024)
            {
                QByteArray outputData2;
                outputData2[0x00]=0x01;
                sendFullPacket(0xC2,0x03,outputData2+fileNameRaw+outputData+content);
            }
            else
            {
                rawFiles+=fileNameRaw+outputData+content;
                rawFilesCount++;
                if(rawFiles.size()>CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024 || rawFilesCount>=255)
                    sendFileContent();
            }
        }
        file.close();
        return true;
    }
    else
        return false;
}

void Client::dbQueryWrite(const QString &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.isEmpty())
    {
        errorOutput(QStringLiteral("dbQuery() Query is empty, bug"));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput(QStringLiteral("Do db write: ")+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db.asyncWrite(queryText.toUtf8());
}

void Client::loadReputation()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_reputation_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadReputation() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_select_reputation_by_id.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::loadReputation_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        loadQuests();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadReputation_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadReputation_return();
}

void Client::loadReputation_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        const int &type=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QStringLiteral("reputation type is not a number, skip: %1").arg(type));
            continue;
        }
        qint32 point=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toInt(&ok);
        if(!ok)
        {
            normalOutput(QStringLiteral("reputation point is not a number, skip: %1").arg(type));
            continue;
        }
        const qint32 &level=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toInt(&ok);
        if(!ok)
        {
            normalOutput(QStringLiteral("reputation level is not a number, skip: %1").arg(type));
            continue;
        }
        if(level<-100 || level>100)
        {
            normalOutput(QStringLiteral("reputation level is <100 or >100, skip: %1").arg(type));
            continue;
        }
        if(type>=GlobalServerData::serverPrivateVariables.dictionary_reputation.size())
        {
            normalOutput(QStringLiteral("The reputation: %1 don't exist").arg(type));
            continue;
        }
        if(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)==-1)
        {
            normalOutput(QStringLiteral("The reputation: %1 not resolved").arg(type));
            continue;
        }
        if(level>=0)
        {
            if(level>=CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_positive.size())
            {
                normalOutput(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 > max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_positive.size()));
                continue;
            }
        }
        else
        {
            if((-level)>CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_negative.size())
            {
                normalOutput(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 < max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_negative.size()));
                continue;
            }
        }
        if(point>0)
        {
            if(CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_positive.size()==(level+1))//start at level 0 in positive
            {
                normalOutput(QStringLiteral("The reputation level is already at max, drop point"));
                point=0;
            }
            if(point>=CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_positive.at(level+1))//start at level 0 in positive
            {
                normalOutput(QStringLiteral("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_positive.at(level)));
                continue;
            }
        }
        else if(point<0)
        {
            if(CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_negative.size()==-level)//start at level -1 in negative
            {
                normalOutput(QStringLiteral("The reputation level is already at min, drop point"));
                point=0;
            }
            if(point<CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_negative.at(-level))//start at level -1 in negative
            {
                normalOutput(QStringLiteral("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation.at(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type)).reputation_negative.at(level)));
                continue;
            }
        }
        PlayerReputation playerReputation;
        playerReputation.level=level;
        playerReputation.point=point;
        public_and_private_informations.reputation.insert(GlobalServerData::serverPrivateVariables.dictionary_reputation.value(type),playerReputation);
    }
    loadQuests();
}

void Client::loadQuests()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_quest_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadQuests() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_select_quest_by_id.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::loadQuests_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        loadBotAlreadyBeaten();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadQuests_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadQuests_return();
}

void Client::loadQuests_return()
{
    callbackRegistred.removeFirst();
    //do the query
    bool ok,ok2;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        PlayerQuest playerQuest;
        const quint32 &id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        playerQuest.finish_one_time=QVariant(GlobalServerData::serverPrivateVariables.db.value(1)).toBool();
        playerQuest.step=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok2);
        if(!ok || !ok2)
        {
            normalOutput(QStringLiteral("wrong value type for quest, skip: %1").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.quests.contains(id))
        {
            normalOutput(QStringLiteral("quest is not into the quests list, skip: %1").arg(id));
            continue;
        }
        if((playerQuest.step<=0 && !playerQuest.finish_one_time) || playerQuest.step>CommonDatapack::commonDatapack.quests.value(id).steps.size())
        {
            normalOutput(QStringLiteral("step out of quest range, skip: %1").arg(id));
            continue;
        }
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            normalOutput(QStringLiteral("can't be to step 0 if have never finish the quest, skip: %1").arg(id));
            continue;
        }
        public_and_private_informations.quests[id]=playerQuest;
        if(playerQuest.step>0)
            addQuestStepDrop(id,playerQuest.step);
    }
    loadBotAlreadyBeaten();
}
