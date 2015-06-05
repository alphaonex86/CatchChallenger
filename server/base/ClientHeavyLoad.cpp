#include "Client.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonMap.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/ProtocolParsingCheck.h"
#include "SqlFunction.h"
#include "PreparedDBQuery.h"
#include "DictionaryLogin.h"
#include "DictionaryServer.h"
#include "BaseServerMasterSendDatapack.h"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerLogin.h"
#endif

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

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::askLogin(const quint8 &query_id,const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.isEmpty())
    {
        errorOutput(QStringLiteral("askLogin() Query login is empty, bug"));
        return;
    }
    if(PreparedDBQueryLogin::db_query_insert_login.isEmpty())
    {
        errorOutput(QStringLiteral("askLogin() Query inset login is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_characters.isEmpty())
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

    const QString &queryText=PreparedDBQueryLogin::db_query_login.arg(QString(login.toHex()));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_login->asyncRead(queryText.toLatin1(),this,&Client::askLogin_static);
    if(callback==NULL)
    {
        loginIsWrong(askLoginParam->query_id,0x04,QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_login->errorMessage()));
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
    GlobalServerData::serverPrivateVariables.db_login->clear();
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
        if(!GlobalServerData::serverPrivateVariables.db_login->next())
        {
            if(GlobalServerData::serverSettings.automatic_account_creation)
            {
                //network send
                *(Client::loginIsWrongBuffer+1)=(quint8)askLoginParam->query_id;
                *(Client::loginIsWrongBuffer+3)=(quint8)0x07;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(askLoginParam->query_id);
                #endif
                replyOutputSize.remove(askLoginParam->query_id);
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginIsWrongBuffer),sizeof(Client::loginIsWrongBuffer));
                delete askLoginParam;
                is_logging_in_progess=false;
                return;
/*                GlobalServerData::serverPrivateVariables.maxAccountId++;
                account_id=GlobalServerData::serverPrivateVariables.maxAccountId;
                dbQueryWrite(PreparedDBQuery::db_query_insert_login.arg(account_id).arg(QString(askLoginParam->login.toHex())).arg(QString(askLoginParam->pass.toHex())).arg(QDateTime::currentDateTime().toTime_t()));*/
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
                while(index<BaseServerLogin::tokenForAuthSize)
                {
                    const BaseServerLogin::TokenLink &tokenLink=BaseServerLogin::tokenForAuth[index];
                    if(tokenLink.client==this)
                    {
                        const QString &secretToken(GlobalServerData::serverPrivateVariables.db_login->value(1));
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
                return;
            }
            else
            {
                account_id=QString(GlobalServerData::serverPrivateVariables.db_login->value(0)).toUInt(&ok);
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
    const QString &queryText=PreparedDBQueryCommon::db_query_characters.arg(account_id).arg(CommonSettingsCommon::commonSettingsCommon.max_character*2);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::character_list_static);
    if(callback==NULL)
    {
        account_id=0;
        loginIsWrong(askLoginParam->query_id,0x04,QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage()));
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
    if(PreparedDBQueryLogin::db_query_login.isEmpty())
    {
        errorOutput(QStringLiteral("createAccount() Query login is empty, bug"));
        return;
    }
    if(PreparedDBQueryLogin::db_query_insert_login.isEmpty())
    {
        errorOutput(QStringLiteral("createAccount() Query inset login is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_characters.isEmpty())
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

    const QString &queryText=PreparedDBQueryLogin::db_query_login.arg(QString(login.toHex()));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_login->asyncRead(queryText.toLatin1(),this,&Client::createAccount_static);
    if(callback==NULL)
    {
        is_logging_in_progess=false;
        loginIsWrong(askLoginParam->query_id,0x03,QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_login->errorMessage()));
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
    GlobalServerData::serverPrivateVariables.db_login->clear();
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
    if(!GlobalServerData::serverPrivateVariables.db_login->next())
    {
        //network send
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //removeFromQueryReceived(askLoginParam->query_id);//->only if use fast path
        #endif
        GlobalServerData::serverPrivateVariables.maxAccountId++;
        account_id=GlobalServerData::serverPrivateVariables.maxAccountId;
        dbQueryWriteLogin(PreparedDBQueryLogin::db_query_insert_login.arg(account_id).arg(QString(askLoginParam->login.toHex())).arg(QString(askLoginParam->pass.toHex())).arg(QDateTime::currentDateTime().toTime_t()));
        //send the network reply
        QByteArray outputData;
        outputData[0x00]=0x01;
        postReply(askLoginParam->query_id,outputData.constData(),outputData.size());
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
    if(askLoginParam->tempOutputData.isEmpty())
    {
        delete askLoginParam;
        return;
    }
    //re use
    //delete askLoginParam;
    if(server_list())
        paramToPassToCallBack << askLoginParam;
}

QByteArray Client::character_list_return(const quint8 &query_id)
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

    out << (quint8)01;//all is good

    //login/common part
    out << (quint32)CommonSettingsCommon::commonSettingsCommon.character_delete_time;
    out << (quint8)CommonSettingsCommon::commonSettingsCommon.max_character;
    out << (quint8)CommonSettingsCommon::commonSettingsCommon.min_character;
    out << (quint8)CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
    out << (quint8)CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
    out << (quint16)CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters;
    out << (quint8)CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
    out << (quint16)CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems;

    outputData+=CommonSettingsCommon::commonSettingsCommon.datapackHashBase;
    out.device()->seek(out.device()->pos()+CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());

    {
        const QByteArray &httpDatapackMirrorRaw=FacilityLibGeneral::toUTF8WithHeader(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase);
        outputData+=httpDatapackMirrorRaw;
        out.device()->seek(out.device()->pos()+httpDatapackMirrorRaw.size());
    }

    /*if(GlobalServerData::serverSettings.sendPlayerNumber)
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
    out << (quint32)CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
    out << (quint8)CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange;
    out << (quint8)CommonSettingsServer::commonSettingsServer.forcedSpeed;
    out << (quint8)CommonSettingsServer::commonSettingsServer.useSP;
    out << (quint8)CommonSettingsServer::commonSettingsServer.tcpCork;
    out << (quint8)CommonSettingsServer::commonSettingsServer.autoLearn;
    out << (quint8)CommonSettingsServer::commonSettingsServer.dontSendPseudo;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_xp;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_gold;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_xp_pow;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_drop;
    out << (quint8)CommonSettingsServer::commonSettingsServer.chat_allow_all;
    out << (quint8)CommonSettingsServer::commonSettingsServer.chat_allow_local;
    out << (quint8)CommonSettingsServer::commonSettingsServer.chat_allow_private;
    out << (quint8)CommonSettingsServer::commonSettingsServer.chat_allow_clan;
    out << (quint8)CommonSettingsServer::commonSettingsServer.factoryPriceChange;
    out << (quint32)GlobalServerData::serverPrivateVariables.map_list.size();*/

    {
        const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
        QList<CharacterEntry> characterEntryList;
        bool ok;
        while(GlobalServerData::serverPrivateVariables.db_common->next() && characterEntryList.size()<CommonSettingsCommon::commonSettingsCommon.max_character)
        {
            CharacterEntry characterEntry;
            characterEntry.character_id=QString(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
            if(ok)
            {
                quint32 time_to_delete=QString(GlobalServerData::serverPrivateVariables.db_common->value(3)).toUInt(&ok);
                if(!ok)
                {
                    normalOutput(QStringLiteral("time_to_delete is not number: %1 for %2 fixed by 0").arg(QString(GlobalServerData::serverPrivateVariables.db_common->value(3))).arg(account_id));
                    time_to_delete=0;
                }
                characterEntry.played_time=QString(GlobalServerData::serverPrivateVariables.db_common->value(4)).toUInt(&ok);
                if(!ok)
                {
                    normalOutput(QStringLiteral("played_time is not number: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db_common->value(4)).arg(account_id));
                    characterEntry.played_time=0;
                }
                characterEntry.last_connect=QString(GlobalServerData::serverPrivateVariables.db_common->value(5)).toUInt(&ok);
                if(!ok)
                {
                    normalOutput(QStringLiteral("last_connect is not number: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db_common->value(5)).arg(account_id));
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
                    characterEntry.pseudo=GlobalServerData::serverPrivateVariables.db_common->value(1);
                    const quint32 &skinIdTemp=QString(GlobalServerData::serverPrivateVariables.db_common->value(2)).toUInt(&ok);
                    if(!ok)
                    {
                        normalOutput(QStringLiteral("character return skin is not number: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db_common->value(5)).arg(account_id));
                        characterEntry.skinId=0;
                        ok=true;
                    }
                    else
                    {
                        if(skinIdTemp>=(quint32)DictionaryLogin::dictionary_skin_database_to_internal.size())
                        {
                            normalOutput(QStringLiteral("character return skin out of range: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db_common->value(5)).arg(account_id));
                            characterEntry.skinId=0;
                            ok=true;
                        }
                        else
                            characterEntry.skinId=DictionaryLogin::dictionary_skin_database_to_internal.at(skinIdTemp);
                    }
                    if(ok)
                    {
                        /*characterEntry.mapId=QString(GlobalServerData::serverPrivateVariables.db->value(6)).toUInt(&ok);
                        if(!ok)
                            normalOutput(QStringLiteral("character return map is not number: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db_common->value(5)).arg(account_id));
                        else
                        {
                            if(characterEntry.mapId>=DictionaryServer::dictionary_map_database_to_internal.size())
                            {
                                normalOutput(QStringLiteral("character return map id out of range: %1 for %2 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db_common->value(5)).arg(account_id));
                                characterEntry.mapId=-1;
                                //ok=false;
                            }
                            else
                            {
                                if(DictionaryServer::dictionary_map_database_to_internal.at(characterEntry.mapId)==NULL)
                                {
                                    normalOutput(QStringLiteral("character return map id not resolved: %1 for %2 fixed by 0: %3").arg(characterEntry.character_id).arg(account_id).arg(characterEntry.mapId));
                                    characterEntry.mapId=-1;
                                    //ok=false;
                                }
                                else
                                {
                                    characterEntry.mapId=DictionaryServer::dictionary_map_database_to_internal.at(characterEntry.mapId)->id;
                                    validCharaterCount++;
                                }
                            }
                        }
                        if(ok)*/
                            characterEntryList << characterEntry;
                    }
                }
            }
            else
                normalOutput(QStringLiteral("Character id is not number: %1 for %2").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)).arg(account_id));
        }
        if(CommonSettingsCommon::commonSettingsCommon.max_character==0 && characterEntryList.isEmpty())
        {
            loginIsWrong(query_id,0x05,"Can't create character and don't have character");
            return QByteArray();
        }

        out << (quint8)0x01;//Number of characters group, characters group 0, all in one server

        number_of_character=characterEntryList.size();
        out << (quint8)characterEntryList.size();
        int index=0;
        while(index<characterEntryList.size())
        {
            const CharacterEntry &characterEntry=characterEntryList.at(index);
            out << (quint32)characterEntry.character_id;
            {
                const QByteArray &rawPseudo=FacilityLibGeneral::toUTF8WithHeader(characterEntry.pseudo);
                if(rawPseudo.isEmpty())
                {
                    loginIsWrong(query_id,0x05,"Can't create character and don't have character");
                    return QByteArray();
                }
                outputData+=rawPseudo;
                out.device()->seek(out.device()->pos()+rawPseudo.size());
            }
            out << (quint8)characterEntry.skinId;
            out << (quint32)characterEntry.delete_time_left;
            /// \todo optimise by simple suming here, not SQL query
            out << (quint32)characterEntry.played_time;
            out << (quint32)characterEntry.last_connect;
            index++;
        }
    }

    return outputData;
}

bool Client::server_list()
{
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(PreparedDBQueryCommon::db_query_select_server_time.arg(account_id).toLatin1(),this,&Client::server_list_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(PreparedDBQueryCommon::db_query_select_allow).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
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
    server_list_return(askLoginParam->query_id,askLoginParam->tempOutputData);
    delete askLoginParam;
}

void Client::server_list_return(const quint8 &query_id, const QByteArray &previousData)
{
    callbackRegistred.removeFirst();
    //send signals into the server

    //C20F
    {
        //no logical group
        QByteArray outputData;
        outputData[0]=0x01;
        outputData[1]=0x00;
        outputData[2]=0x00;outputData[3]=0x00;//16Bits
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
        {
            const int &newSize=FacilityLibGeneral::toUTF8With16BitsHeader(CommonSettingsServer::commonSettingsServer.exportedXml,outputData.data());
            if(newSize>65535 || newSize<=0)
            {
                errorOutput(QLatin1Literal("file path too big or not compatible with utf8"));
                return;
            }
            out.device()->seek(out.device()->pos()+newSize);
        }
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
    char * const tempRawData=new char[4*1024];
    //memset(tempRawData,0x00,sizeof(4*1024));
    int tempRawDataSize=0x01;

    const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
    bool ok;
    quint8 validServerCount=0;
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        //server index
        tempRawData[tempRawDataSize]=0;
        tempRawDataSize+=1;

        //played_time
        unsigned int played_time=QString(GlobalServerData::serverPrivateVariables.db_common->value(1)).toUInt(&ok);
        if(!ok)
        {
            qDebug() << (QStringLiteral("played_time is not number: %1 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db_common->value(4)));
            played_time=0;
        }
        *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(played_time);
        tempRawDataSize+=sizeof(quint32);

        //last_connect
        unsigned int last_connect=QString(GlobalServerData::serverPrivateVariables.db_common->value(2)).toUInt(&ok);
        if(!ok)
        {
            qDebug() << (QStringLiteral("last_connect is not number: %1 fixed by 0").arg(GlobalServerData::serverPrivateVariables.db_common->value(5)));
            last_connect=current_time;
        }
        *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(last_connect);
        tempRawDataSize+=sizeof(quint32);

        validServerCount++;
    }
    #ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
    if(validServerCount==0)
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_server_time.arg(0).arg(account_id).arg(QDateTime::currentDateTime().toTime_t()));
    else
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_server_time_last_connect.arg(QDateTime::currentDateTime().toTime_t()).arg(0).arg(account_id));
    #endif
    tempRawData[0]=validServerCount;

    const QByteArray newData(previousData+QByteArray(tempRawData,tempRawDataSize));
    postReply(query_id,newData.constData(),newData.size());
}

void Client::deleteCharacterNow(const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_monster_by_character_id.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_buff.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_buff is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_skill.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_skill is empty, bug"));
        return;
    }
    if(PreparedDBQueryServer::db_query_delete_bot_already_beaten.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_bot_already_beaten is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_character.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_character is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_all_item.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_item is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_by_character.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster is empty, bug"));
        return;
    }
    if(PreparedDBQueryServer::db_query_delete_plant.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_plant is empty, bug"));
        return;
    }
    if(PreparedDBQueryServer::db_query_delete_quest.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_quest is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_recipes.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_recipes is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_recipes.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_recipes is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_reputation.isEmpty())
    {
        errorOutput(QStringLiteral("deleteCharacterNow() Query db_query_delete_reputation is empty, bug"));
        return;
    }
    #endif
    DeleteCharacterNow *deleteCharacterNow=new DeleteCharacterNow;
    deleteCharacterNow->characterId=characterId;

    const QString &queryText=PreparedDBQueryCommon::db_query_monster_by_character_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::deleteCharacterNow_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
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
    GlobalServerData::serverPrivateVariables.db_common->clear();
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
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const quint32 &monsterId=QString(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(ok)
        {
            dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_monster_buff.arg(monsterId));
            dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_monster_skill.arg(monsterId));
        }
    }
    dbQueryWriteServer(PreparedDBQueryServer::db_query_delete_bot_already_beaten.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_character.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_all_item.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_all_item_warehouse.arg(characterId));
    dbQueryWriteServer(PreparedDBQueryServer::db_query_delete_all_item_market.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_monster_by_character.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_monster_warehouse_by_character.arg(characterId));
    dbQueryWriteServer(PreparedDBQueryServer::db_query_delete_plant.arg(characterId));
    dbQueryWriteServer(PreparedDBQueryServer::db_query_delete_quest.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_recipes.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_reputation.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_allow.arg(characterId));
}

void Client::addCharacter(const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_character_by_pseudo.isEmpty())
    {
        errorOutput(QStringLiteral("addCharacter() Query is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_insert_monster.isEmpty())
    {
        errorOutput(QStringLiteral("addCharacter() Query db_query_insert_monster is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_insert_monster_skill.isEmpty())
    {
        errorOutput(QStringLiteral("addCharacter() Query db_query_insert_monster_skill is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_insert_reputation.isEmpty())
    {
        errorOutput(QStringLiteral("addCharacter() Query db_query_insert_reputation is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_insert_item.isEmpty())
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
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    if(number_of_character>=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        errorOutput(QStringLiteral("You can't create more account, you have already %1 on %2 allowed").arg(number_of_character).arg(CommonSettingsCommon::commonSettingsCommon.max_character));
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonDatapack::commonDatapack.profileList.size()!=CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList.size())
    {
        errorOutput(QStringLiteral("profile common and server don't match"));
        return;
    }
    if(CommonDatapack::commonDatapack.profileList.size()!=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
    {
        errorOutput(QStringLiteral("profile common and server internal don't match"));
        return;
    }
    #endif
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        errorOutput(QStringLiteral("profile index: %1 out of range (profileList size: %2)").arg(profileIndex).arg(CommonDatapack::commonDatapack.profileList.size()));
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).valid)
    {
        errorOutput(QStringLiteral("profile index: %1 profil not valid").arg(profileIndex));
        return;
    }
    if(pseudo.size()>CommonSettingsCommon::commonSettingsCommon.max_pseudo_size)
    {
        errorOutput(QStringLiteral("pseudo size is too big: %1 because is greater than %2").arg(pseudo.size()).arg(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size));
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

    const QString &queryText=PreparedDBQueryCommon::db_query_select_character_by_pseudo.arg(SqlFunction::quoteSqlVariable(pseudo));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::addCharacter_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());

        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x02;
        out << (quint32)0x00000000;
        postReply(query_id,outputData.constData(),outputData.size());
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
    GlobalServerData::serverPrivateVariables.db_common->clear();
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
    GlobalServerData::serverPrivateVariables.db_common->clear();
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
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x01;
        out << (quint32)0x00000000;
        postReply(query_id,outputData.constData(),outputData.size());
        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    //const ServerProfile &serverProfile=CommonDatapack::commonDatapack.serverProfileList.at(profileIndex);
    const ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex);

    number_of_character++;
    GlobalServerData::serverPrivateVariables.maxCharacterId++;

    std::vector<unsigned int> monsterIdList;
    {
        bool ok;
        int index=0;
        while(index<profile.monsters.size())
        {
            monsterIdList.push_back(getMonsterId(&ok));
            if(!ok)
            {
                qDebug() << "getMonsterId(&ok) have failed, no more id to get?" << __FILE__ << __LINE__;
                QByteArray outputData;
                QDataStream out(&outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
                out << (quint8)0x03;
                out << (quint32)0x00000000;
                postReply(query_id,outputData.constData(),outputData.size());
                return;
            }
            index++;
        }
    }

    const quint32 &characterId=GlobalServerData::serverPrivateVariables.maxCharacterId;
    int index=0;
    int monster_position=1;
    dbQueryWriteCommon(serverProfileInternal.preparedQueryAdd.at(0)+
                 QString::number(characterId)+
                 serverProfileInternal.preparedQueryAdd.at(1)+
                 QString::number(account_id)+
                 serverProfileInternal.preparedQueryAdd.at(2)+
                 pseudo+
                 serverProfileInternal.preparedQueryAdd.at(3)+
                 QString::number(DictionaryLogin::dictionary_skin_internal_to_database.at(skinId))+
                 serverProfileInternal.preparedQueryAdd.at(4)+
                 QString::number(QDateTime::currentDateTime().toTime_t())+
                 serverProfileInternal.preparedQueryAdd.at(5)
                 );
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

            const quint32 &monster_id=monsterIdList.back();
            monsterIdList.pop_back();
            {
                dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster
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
            int sub_index=0;
            while(sub_index<skills.size())
            {
                quint8 endurance=0;
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(skills.value(sub_index).skill))
                    if(skills.value(sub_index).level<=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.size() && skills.value(sub_index).level>0)
                        endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.at(skills.value(sub_index).level-1).endurance;
                dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster_skill
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
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_reputation
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
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_item
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
    postReply(query_id,outputData.constData(),outputData.size());
}

void Client::removeCharacterLater(const quint8 &query_id, const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("removeCharacter() Query is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("removeCharacter() Query db_query_update_character_time_to_delete_by_id is empty, bug"));
        return;
    }
    #endif
    RemoveCharacterParam *removeCharacterParam=new RemoveCharacterParam;
    removeCharacterParam->query_id=query_id;
    removeCharacterParam->characterId=characterId;

    const QString &queryText=PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::removeCharacterLater_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x02;
        postReply(query_id,outputData.constData(),outputData.size());
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

void Client::removeCharacterLater_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->removeCharacterLater_object();
    GlobalServerData::serverPrivateVariables.db_common->clear();
}

void Client::removeCharacterLater_object()
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
    removeCharacterLater_return(removeCharacterParam->query_id,removeCharacterParam->characterId);
    delete removeCharacterParam;
}

void Client::removeCharacterLater_return(const quint8 &query_id,const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("RemoveCharacterParam"))
    {
        qDebug() << "is not RemoveCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    if(!GlobalServerData::serverPrivateVariables.db_common->next())
    {
        characterSelectionIsWrong(query_id,0x02,"Result return query to remove wrong");
        return;
    }
    bool ok;
    const quint32 &account_id=QString(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Account for character: %1 is not an id").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
        return;
    }
    if(this->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Character: %1 is not owned by the account: %2").arg(characterId).arg(account_id));
        return;
    }
    const quint32 &time_to_delete=QString(GlobalServerData::serverPrivateVariables.db_common->value(1)).toUInt(&ok);
    if(ok && time_to_delete>0)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Character: %1 is already in deleting for the account: %2").arg(characterId).arg(account_id));
        return;
    }
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id.arg(characterId)
                       .arg(
                           //date to delete, not time (no sens)
                           QDateTime::currentDateTime().toTime_t()+
                           CommonSettingsCommon::commonSettingsCommon.character_delete_time
                           )
                       );
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    postReply(query_id,outputData.constData(),outputData.size());
}
#endif

//load linked data (like item, quests, ...)
void Client::loadLinkedData()
{
    loadPlayerAllow();
}

bool Client::loadTheRawUTF8String()
{
    rawPseudo=FacilityLibGeneral::toUTF8WithHeader(public_and_private_informations.public_informations.pseudo);
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

QHash<QString,quint32> Client::datapack_file_list(const bool withHash)
{
    QHash<QString,quint32> filesList;

    const QStringList &returnList=FacilityLibGeneral::listFolder(GlobalServerData::serverSettings.datapack_basePath);
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
            if(!QFileInfo(fileName).suffix().isEmpty() && BaseServerMasterSendDatapack::extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(GlobalServerData::serverSettings.datapack_basePath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        #ifdef Q_OS_WIN32
                        fileName.replace(Client::text_antislash,Client::text_slash);//remplace if is under windows server
                        #endif
                        if(withHash)
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(file.readAll());
                            filesList[fileName]=*reinterpret_cast<const int *>(hashFile.result().constData());
                        }
                        else
                            filesList[fileName]=0;
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
    if(!CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.isEmpty())
    {
        errorOutput("Can't use because base mirror is already defined");
        return;
    }
    if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.isEmpty())
    {
        errorOutput("Can't use because server mirror is already defined");
        return;
    }
    #endif
    tempDatapackListReplyArray.clear();
    tempDatapackListReplyTestCount=0;
    BaseServerMasterSendDatapack::rawFilesBuffer.clear();
    BaseServerMasterSendDatapack::compressedFilesBuffer.clear();
    BaseServerMasterSendDatapack::rawFilesBufferCount=0;
    BaseServerMasterSendDatapack::compressedFilesBufferCount=0;
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
        sendFullPacket(0xC2,0x000C,outputData.constData(),outputData.size());
    }
    if(fileToSendList.isEmpty())
    {
        errorOutput("Ask datapack list where the checksum match");
        return;
    }
    qSort(fileToSendList);
    if(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.isEmpty())
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
        QByteArray outputData(FacilityLibGeneral::toUTF8WithHeader(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer));
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
                const QByteArray &rawFileName=FacilityLibGeneral::toUTF8WithHeader(fileToSendList.at(index).file);
                if(rawFileName.size()>255 || rawFileName.isEmpty())
                {
                    errorOutput(QLatin1Literal("file path too big or not compatible with utf8"));
                    return;
                }
                outputData+=rawFileName;
                out.device()->seek(out.device()->size());
                index++;
            }
            sendFullPacket(0xC2,0x000D,outputData.constData(),outputData.size());
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
    postReply(query_id,tempDatapackListReplyArray.constData(),tempDatapackListReplyArray.size());
    tempDatapackListReplyArray.clear();
}

void Client::sendFileContent()
{
    if(BaseServerMasterSendDatapack::rawFilesBuffer.size()>0 && BaseServerMasterSendDatapack::rawFilesBufferCount>0)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)BaseServerMasterSendDatapack::rawFilesBufferCount;
        const QByteArray newData(outputData+BaseServerMasterSendDatapack::rawFilesBuffer);
        sendFullPacket(0xC2,0x03,newData.constData(),newData.size());
        BaseServerMasterSendDatapack::rawFilesBuffer.clear();
        BaseServerMasterSendDatapack::rawFilesBufferCount=0;
    }
}

void Client::sendCompressedFileContent()
{
    if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>0 && BaseServerMasterSendDatapack::compressedFilesBufferCount>0)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)BaseServerMasterSendDatapack::compressedFilesBufferCount;
        const QByteArray newData(outputData+BaseServerMasterSendDatapack::compressedFilesBuffer);
        sendFullPacket(0xC2,0x04,newData.constData(),newData.size());
        BaseServerMasterSendDatapack::compressedFilesBuffer.clear();
        BaseServerMasterSendDatapack::compressedFilesBufferCount=0;
    }
}

bool Client::sendFile(const QString &fileName)
{
    if(fileName.size()>255 || fileName.isEmpty())
        return false;
    const QByteArray &fileNameRaw=FacilityLibGeneral::toUTF8WithHeader(fileName);
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
        if(BaseServerMasterSendDatapack::compressedExtension.contains(QFileInfo(file).suffix()) &&
                ProtocolParsing::compressionTypeServer!=ProtocolParsing::CompressionType::None &&
                content.size()<CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024)
        {
            BaseServerMasterSendDatapack::compressedFilesBuffer+=fileNameRaw+outputData+content;
            BaseServerMasterSendDatapack::compressedFilesBufferCount++;
            switch(ProtocolParsing::compressionTypeServer)
            {
                case ProtocolParsing::CompressionType::Xz:
                if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024 || BaseServerMasterSendDatapack::compressedFilesBufferCount>=255)
                    sendCompressedFileContent();
                break;
                default:
                case ProtocolParsing::CompressionType::Zlib:
                if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024 || BaseServerMasterSendDatapack::compressedFilesBufferCount>=255)
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
                const QByteArray newData(outputData2+fileNameRaw+outputData+content);
                sendFullPacket(0xC2,0x03,newData.constData(),newData.size());
            }
            else
            {
                BaseServerMasterSendDatapack::rawFilesBuffer+=fileNameRaw+outputData+content;
                BaseServerMasterSendDatapack::rawFilesBufferCount++;
                if(BaseServerMasterSendDatapack::rawFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024 || BaseServerMasterSendDatapack::rawFilesBufferCount>=255)
                    sendFileContent();
            }
        }
        file.close();
        return true;
    }
    else
        return false;
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::dbQueryWriteLogin(const QString &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.isEmpty())
    {
        errorOutput(QStringLiteral("dbQueryWriteLogin() Query is empty, bug"));
        return;
    }
    if(queryText.startsWith("SELECT"))
    {
            errorOutput(QStringLiteral("dbQueryWriteLogin() Query is SELECT but here is the write queue, bug"));
            return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput(QStringLiteral("Do db_login write: ")+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db_login->asyncWrite(queryText.toUtf8());
}
#endif

void Client::dbQueryWriteCommon(const QString &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.isEmpty())
    {
        errorOutput(QStringLiteral("dbQueryWriteCommon() Query is empty, bug"));
        return;
    }
    if(queryText.startsWith("SELECT"))
    {
            errorOutput(QStringLiteral("dbQueryWriteCommon() Query is SELECT but here is the write queue, bug"));
            return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput(QStringLiteral("Do db_common write: ")+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db_common->asyncWrite(queryText.toUtf8());
}

void Client::dbQueryWriteServer(const QString &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.isEmpty())
    {
        errorOutput(QStringLiteral("dbQueryWriteServer() Query is empty, bug"));
        return;
    }
    if(queryText.startsWith("SELECT"))
    {
            errorOutput(QStringLiteral("dbQueryWriteServer() Query is SELECT but here is the write queue, bug"));
            return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput(QStringLiteral("Do db_server write: ")+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText.toUtf8());
}

void Client::loadReputation()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_reputation_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadReputation() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=PreparedDBQueryCommon::db_query_select_reputation_by_id.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::loadReputation_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
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
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const int &type=QString(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QStringLiteral("reputation type is not a number, skip: %1").arg(type));
            continue;
        }
        qint32 point=QString(GlobalServerData::serverPrivateVariables.db_common->value(1)).toInt(&ok);
        if(!ok)
        {
            normalOutput(QStringLiteral("reputation point is not a number, skip: %1").arg(type));
            continue;
        }
        const qint32 &level=QString(GlobalServerData::serverPrivateVariables.db_common->value(2)).toInt(&ok);
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
        if(type>=DictionaryLogin::dictionary_reputation_database_to_internal.size())
        {
            normalOutput(QStringLiteral("The reputation: %1 don't exist").arg(type));
            continue;
        }
        if(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)==-1)
        {
            normalOutput(QStringLiteral("The reputation: %1 not resolved").arg(type));
            continue;
        }
        if(level>=0)
        {
            if(level>=CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_positive.size())
            {
                normalOutput(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 > max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_positive.size()));
                continue;
            }
        }
        else
        {
            if((-level)>CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_negative.size())
            {
                normalOutput(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 < max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_negative.size()));
                continue;
            }
        }
        if(point>0)
        {
            if(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_positive.size()==(level+1))//start at level 0 in positive
            {
                normalOutput(QStringLiteral("The reputation level is already at max, drop point"));
                point=0;
            }
            if(point>=CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_positive.at(level+1))//start at level 0 in positive
            {
                normalOutput(QStringLiteral("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_positive.at(level)));
                continue;
            }
        }
        else if(point<0)
        {
            if(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_negative.size()==-level)//start at level -1 in negative
            {
                normalOutput(QStringLiteral("The reputation level is already at min, drop point"));
                point=0;
            }
            if(point<CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_negative.at(-level))//start at level -1 in negative
            {
                normalOutput(QStringLiteral("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.value(type)).reputation_negative.at(level)));
                continue;
            }
        }
        PlayerReputation playerReputation;
        playerReputation.level=level;
        playerReputation.point=point;
        public_and_private_informations.reputation.insert(DictionaryLogin::dictionary_reputation_database_to_internal.value(type),playerReputation);
    }
    loadQuests();
}

void Client::loadQuests()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_quest_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadQuests() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=PreparedDBQueryServer::db_query_select_quest_by_id.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&Client::loadQuests_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_server->errorMessage());
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
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        PlayerQuest playerQuest;
        const quint32 &id=QString(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        playerQuest.finish_one_time=QVariant(GlobalServerData::serverPrivateVariables.db_server->value(1)).toBool();
        playerQuest.step=QString(GlobalServerData::serverPrivateVariables.db_server->value(2)).toUInt(&ok2);
        if(!ok || !ok2)
        {
            normalOutput(QStringLiteral("wrong value type for quest, skip: %1").arg(id));
            continue;
        }
        if(!CommonDatapackServerSpec::commonDatapackServerSpec.quests.contains(id))
        {
            normalOutput(QStringLiteral("quest is not into the quests list, skip: %1").arg(id));
            continue;
        }
        if((playerQuest.step<=0 && !playerQuest.finish_one_time) || playerQuest.step>CommonDatapackServerSpec::commonDatapackServerSpec.quests.value(id).steps.size())
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
