#include "../Client.hpp"
#include "../GlobalServerData.hpp"
#include "../DictionaryLogin.hpp"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../BaseServer/BaseServerLogin.hpp"
#endif
#ifndef CATCHCHALLENGER_DB_PREPAREDSTATEMENT
#include "../SqlFunction.hpp"
#endif
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/sha224/sha224.hpp"
#include <cstring>
#ifdef CATCHCHALLENGER_DB_FILE
#include <sys/stat.h>
#include <fstream>
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#endif

/// \todo solve disconnecting/destroy during the SQL loading

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
bool Client::askLogin(const uint8_t &query_id,const char *rawdata)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.empty())
    {
        errorOutput("askLogin() Query login is empty, bug");
        return false;
    }
    if(PreparedDBQueryLogin::db_query_insert_login.empty())
    {
        errorOutput("askLogin() Query inset login is empty, bug");
        return false;
    }
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    AskLoginParam *askLoginParam=new AskLoginParam;
    SHA224 ctx = SHA224();
    ctx.init();
    ctx.update(reinterpret_cast<const unsigned char *>(rawdata),CATCHCHALLENGER_SHA224HASH_SIZE);
    ctx.final(reinterpret_cast<unsigned char *>(askLoginParam->login));
    askLoginParam->query_id=query_id;
    memcpy(askLoginParam->pass,rawdata+CATCHCHALLENGER_SHA224HASH_SIZE,CATCHCHALLENGER_SHA224HASH_SIZE);

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    CatchChallenger::DatabaseBaseCallBack *callback=PreparedDBQueryLogin::db_query_login.asyncRead(this,&Client::askLogin_static,{binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)});
    if(callback==NULL)
    {
        loginIsWrong(askLoginParam->query_id,0x04,"Sql error for: "+PreparedDBQueryLogin::db_query_login.queryText()+", error: "+GlobalServerData::serverPrivateVariables.db_login->errorMessage());
        delete askLoginParam;
        return false;
    }
    else
    {
        paramToPassToCallBack.push(askLoginParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("AskLoginParam");
        #endif
        callbackRegistred.push(callback);
        return true;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    paramToPassToCallBack.push(askLoginParam);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    paramToPassToCallBackType.push("AskLoginParam");
    #endif
    askLogin_object();
    return true;
    #elif CATCHCHALLENGER_DB_FILE
    paramToPassToCallBack.push(askLoginParam);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    paramToPassToCallBackType.push("AskLoginParam");
    #endif
    askLogin_object();
    return true;
    #else
    #error Define what do here
    #endif
}

void Client::askLogin_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->askLogin_object();
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_login->clear();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    //delete askLoginParam; -> not here because need reuse later
}

void Client::askLogin_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    if(paramToPassToCallBack.size()!=1)
    {
        std::cerr << "paramToPassToCallBack.size()!=1" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
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
    if(paramToPassToCallBackType.front()!="AskLoginParam")
    {
        std::cerr << "is not AskLoginParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    callbackRegistred.pop();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
//    std::cerr << "askLogin_return for: database/accounts/" << binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE) << std::endl;
    #else
    #error Define what do here
    #endif
    {
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        if(!GlobalServerData::serverPrivateVariables.db_login->next())
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        struct stat sb;
        if(::stat(("database/accounts/"+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)).c_str(),&sb)!=0)
        #else
        #error Define what do here
        #endif
        {
            //return creation query to client
            if(GlobalServerData::serverSettings.automatic_account_creation)
            {
                //network send
                *(Client::loginIsWrongBuffer+1)=(uint8_t)askLoginParam->query_id;
                *(Client::loginIsWrongBuffer+1+1+4)=(uint8_t)0x07;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(askLoginParam->query_id);
                #endif
                inputQueryNumberToPacketCode[askLoginParam->query_id]=0;
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginIsWrongBuffer),sizeof(Client::loginIsWrongBuffer));
                delete askLoginParam;
                stat=ClientStat::ProtocolGood;
                return;
/*                GlobalServerData::serverPrivateVariables.maxAccountId++;
                account_id=GlobalServerData::serverPrivateVariables.maxAccountId;
                dbQueryWrite(PreparedDBQuery::db_query_insert_login.arg(account_id).arg(std::string(askLoginParam->login.toHex())).arg(sFrom1970()));*/
            }
            else
            {
                loginIsWrong(askLoginParam->query_id,0x02,"Bad login for: "+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)+", pass: "+binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE));
                delete askLoginParam;
                return;
            }
        }
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_DB_FILE)
        else
        {
            #ifdef CATCHCHALLENGER_DB_FILE
            std::ifstream in_file("database/accounts/"+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE), std::ifstream::binary);
            if(!in_file.good() || !in_file.is_open())
            {
                std::cerr << "Unable to open data base file " << "database/accounts/"+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE) << " (abort)" << std::endl;
                abort();
                return;
            }
            hps::StreamInputBuffer s(in_file);
            #endif

            #ifndef CATCHCHALLENGER_DB_FILE
            bool ok=false;
            #endif
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::vector<char> tempAddedToken;
            std::vector<char> secretTokenBinary;
            #endif
            {
                int32_t tokenForAuthIndex=0;
                while((uint32_t)tokenForAuthIndex<BaseServerLogin::tokenForAuthSize)
                {
                    const BaseServerLogin::TokenLink &tokenLink=BaseServerLogin::tokenForAuth[tokenForAuthIndex];
                    if(tokenLink.client==this)
                    {
                        #ifdef CATCHCHALLENGER_DB_FILE
                        std::vector<char> secretTokenBinary;
                        s >> secretTokenBinary;
                        if(secretTokenBinary.size()!=CATCHCHALLENGER_SHA224HASH_SIZE)
                        {
                            std::cerr << "convertion to binary for pass failed for: database/accounts/" << binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE) << " " << secretTokenBinary.size() << "!=" << CATCHCHALLENGER_SHA224HASH_SIZE << std::endl;
                            abort();
                        }
                        #else
                        const std::string &secretToken(GlobalServerData::serverPrivateVariables.db_login->value(1));
                        #ifndef CATCHCHALLENGER_EXTRA_CHECK
                        std::vector<char> secretTokenBinary;
                        #endif
                        secretTokenBinary=hexatoBinary(secretToken);
                        if(secretTokenBinary.size()!=CATCHCHALLENGER_SHA224HASH_SIZE)
                        {
                            std::cerr << "convertion to binary for pass failed for: " << GlobalServerData::serverPrivateVariables.db_login->value(1) << std::endl;
                            abort();
                        }
                        #endif
                        secretTokenBinary.resize(secretTokenBinary.size()+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        memcpy(secretTokenBinary.data()+CATCHCHALLENGER_SHA224HASH_SIZE,tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);

                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        //append the token
                        tempAddedToken.resize(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        memcpy(tempAddedToken.data(),tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        if(secretTokenBinary.size()!=(CATCHCHALLENGER_SHA224HASH_SIZE+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                        {
                            std::cerr << "secretTokenBinary.size()!=(CATCHCHALLENGER_SHA224HASH_SIZE+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)" << std::endl;
                            abort();
                        }
                        #endif
                        SHA224 ctx = SHA224();
                        ctx.init();
                        ctx.update(reinterpret_cast<const unsigned char *>(secretTokenBinary.data()),secretTokenBinary.size());
                        ctx.final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput));
                        BaseServerLogin::tokenForAuthSize--;
                        //see to do with SIMD
                        if(BaseServerLogin::tokenForAuthSize>0)
                        {
                            while((uint32_t)tokenForAuthIndex<BaseServerLogin::tokenForAuthSize)
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
                if(tokenForAuthIndex>=(int32_t)BaseServerLogin::tokenForAuthSize)
                {
                    loginIsWrong(askLoginParam->query_id,0x02,"No temp auth token found");
                    return;
                }
            }
            if(memcmp(ProtocolParsingBase::tempBigBufferForOutput,askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE)!=0)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                loginIsWrong(askLoginParam->query_id,0x03,"Password wrong: "+
                             binarytoHexa(secretTokenBinary)+
                             " + token "+
                             binarytoHexa(tempAddedToken)+
                             " = "+
                             " hashedToken: "+
                             binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,CATCHCHALLENGER_SHA224HASH_SIZE)+
                             " for the login: "+
                             binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)+
                             "sended pass + token: "+
                             binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE)
                             );
                #else
                loginIsWrong(askLoginParam->query_id,0x03,"Password wrong: "+
                             binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE)+
                             " for the login: "+
                             binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)
                             );
                #endif
                delete askLoginParam;
                return;
            }
            else
            {
                #ifdef CATCHCHALLENGER_DB_FILE
                s >> account_id_db;
                flags|=0x08;

                std::vector<CharacterEntry> characterEntryList;
                {
                    std::ifstream in_file("database/accounts/"+std::to_string(account_id_db), std::ifstream::binary);
                    if(!in_file.good() || !in_file.is_open())
                    {
                        std::cerr << "Unable to open data base file " << "database/accounts/"+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE) << " (abort)" << std::endl;
                        abort();
                        return;
                    }
                    hps::StreamInputBuffer s(in_file);
                    s >> characterEntryList;
                    if(CommonSettingsCommon::commonSettingsCommon.max_character==0 && characterEntryList.empty())
                    {
                        loginIsWrong(askLoginParam->query_id,0x05,"Can't create character and don't have character");
                        return;
                    }
                }


                uint32_t posOutput=0;
                char * data=ProtocolParsingBase::tempBigBufferForOutput;

                data[posOutput]=0x01;//Number of characters group, characters group 0, all in one server
                posOutput+=1;

                number_of_character=static_cast<uint8_t>(characterEntryList.size());
                data[posOutput]=static_cast<uint8_t>(characterEntryList.size());
                posOutput+=1;
                unsigned int index=0;
                while(index<characterEntryList.size())
                {
                    const CharacterEntry &characterEntry=characterEntryList.at(index);
                    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.character_id);
                    posOutput+=4;
                    {
                        {
                            const std::string &text=characterEntry.pseudo;
                            data[posOutput]=static_cast<uint8_t>(text.size());
                            posOutput+=1;
                            memcpy(data+posOutput,text.data(),text.size());
                            posOutput+=text.size();
                        }
                    }
                    data[posOutput]=characterEntry.skinId;
                    posOutput+=1;
                    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.delete_time_left);
                    posOutput+=4;
                    /// \todo optimise by simple suming here, not SQL query
                    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.played_time);
                    posOutput+=4;
                    *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.last_connect);
                    posOutput+=4;
                    index++;
                }

                stat=ClientStat::Logged;

                askLoginParam->characterOutputDataSize=send_characterEntryList(characterEntryList,ProtocolParsingBase::tempBigBufferForOutput,askLoginParam->query_id);
                if(askLoginParam->characterOutputDataSize==0)
                {
                    std::cerr << "send_characterEntryList(characterEntryList) wrong" << std::endl;
                    return;
                }

                askLoginParam->characterOutputData=(char *)malloc(askLoginParam->characterOutputDataSize);
                memcpy(askLoginParam->characterOutputData,ProtocolParsingBase::tempBigBufferForOutput,askLoginParam->characterOutputDataSize);
                //re use
                //delete askLoginParam;
                server_list_return(askLoginParam->query_id,askLoginParam->characterOutputData,askLoginParam->characterOutputDataSize);
                #else
                account_id=GlobalServerData::serverPrivateVariables.db_login->stringtouint32(GlobalServerData::serverPrivateVariables.db_login->value(0),&ok);
                if(!ok)
                {
                    account_id=0;
                    loginIsWrong(askLoginParam->query_id,0x03,"Account id is not a number");
                    delete askLoginParam;
                    return;
                }
                else
                {
                    flags|=0x08;
                }
                #endif
                stat=ClientStat::Logged;
            }
        }
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #else
        #error Define what do here
        #endif
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    CatchChallenger::DatabaseBaseCallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_characters.asyncRead(this,&Client::character_list_static,{std::to_string(account_id)});
    if(callback==NULL)
    {
        account_id=0;
        loginIsWrong(askLoginParam->query_id,0x04,"Sql error for: "+GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_characters.queryText()+", error: "+GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        delete askLoginParam;
        return;
    }
    else
    {
        paramToPassToCallBack.push(askLoginParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("AskLoginParam");
        #endif
        callbackRegistred.push(callback);
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

bool Client::createAccount(const uint8_t &query_id, const char *rawdata)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.empty())
    {
        errorOutput("createAccount() Query login is empty, bug");
        return false;
    }
    if(PreparedDBQueryLogin::db_query_insert_login.empty())
    {
        errorOutput("createAccount() Query inset login is empty, bug");
        return false;
    }
    if(!GlobalServerData::serverSettings.automatic_account_creation)
    {
        errorOutput("createAccount() Creation account not premited");
        return false;
    }
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    /// \note No SHA224 because already double hashed before
    AskLoginParam *askLoginParam=new AskLoginParam;
    askLoginParam->characterOutputData=nullptr;
    askLoginParam->characterOutputDataSize=0;
    memcpy(askLoginParam->login,rawdata,CATCHCHALLENGER_SHA224HASH_SIZE);
    memcpy(askLoginParam->pass,rawdata+CATCHCHALLENGER_SHA224HASH_SIZE,CATCHCHALLENGER_SHA224HASH_SIZE);
    askLoginParam->query_id=query_id;

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    CatchChallenger::DatabaseBaseCallBack *callback=PreparedDBQueryLogin::db_query_login.asyncRead(this,&Client::createAccount_static,{binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)});
    if(callback==NULL)
    {
        stat=ClientStat::ProtocolGood;
        loginIsWrong(askLoginParam->query_id,0x03,"Sql error for: "+PreparedDBQueryLogin::db_query_login.queryText()+", error: "+GlobalServerData::serverPrivateVariables.db_login->errorMessage());
        delete askLoginParam;
        return false;
    }
    else
    {
        number_of_character++;
        paramToPassToCallBack.push(askLoginParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("AskLoginParam");
        #endif
        callbackRegistred.push(callback);
        return true;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    number_of_character++;
    paramToPassToCallBack.push(askLoginParam);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    paramToPassToCallBackType.push("AskLoginParam");
    #endif
    createAccount_object();
    return true;
    #elif CATCHCHALLENGER_DB_FILE
    number_of_character++;
    paramToPassToCallBack.push(askLoginParam);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    paramToPassToCallBackType.push("AskLoginParam");
    #endif
    createAccount_object();
    return true;
    #else
    #error Define what do here
    #endif
}

void Client::createAccount_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->createAccount_object();
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_login->clear();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::createAccount_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    if(paramToPassToCallBack.size()!=1)
    {
        std::cerr << "paramToPassToCallBack.size()!=1" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
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
    if(paramToPassToCallBackType.front()!="AskLoginParam")
    {
        std::cerr << "is not AskLoginParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    callbackRegistred.pop();
    if(!GlobalServerData::serverPrivateVariables.db_login->next())
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    const std::string &hexa=binarytoHexa(public_and_private_informations.public_informations.pseudo.c_str(),
                                         public_and_private_informations.public_informations.pseudo.size());
    struct stat sb;
    if(::stat(("characters/"+hexa).c_str(), &sb)!=0)
    #else
    #error Define what do here
    #endif
    {
        //network send
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //removeFromQueryReceived(askLoginParam->query_id);//->only if use fast path
        #endif
        GlobalServerData::serverPrivateVariables.maxAccountId++;
        account_id_db=GlobalServerData::serverPrivateVariables.maxAccountId;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        PreparedDBQueryLogin::db_query_insert_login.asyncWrite({
                    std::to_string(account_id),
                    binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE),
                    binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE),
                    std::to_string(sFrom1970())
                    });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        {
            FILE *fp=fopen("database/server","r+");
            if(fp==NULL) {std::cerr << "error to open in write the file database/server " << __FILE__ << ":" << __LINE__ << std::endl;abort();}
            if(fseek(fp,sizeof(GlobalServerData::serverPrivateVariables.maxClanId),SEEK_SET)!=0)
            {std::cerr << "error to open in write seek the file database/server " << __FILE__ << ":" << __LINE__ << std::endl;abort();}
            fwrite(&GlobalServerData::serverPrivateVariables.maxAccountId,sizeof(GlobalServerData::serverPrivateVariables.maxAccountId),1,fp);
            fclose(fp);
        }
        {
            {
                std::ofstream out_file("database/accounts/"+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE), std::ofstream::binary);
                if(!out_file.good() || !out_file.is_open())
                {
                    std::cerr << "Unable to open data base file " << "database/accounts/"+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE) << " (abort)" << std::endl;
                    abort();
                    return;
                }

                std::vector<char> secretTokenBinary;
                secretTokenBinary.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
                memcpy(secretTokenBinary.data(),askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE);
                if(secretTokenBinary.size()!=CATCHCHALLENGER_SHA224HASH_SIZE)
                {
                    std::cerr << "convertion to binary for pass failed for: database/accounts/" << binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE) << " " << secretTokenBinary.size() << "!=" << CATCHCHALLENGER_SHA224HASH_SIZE << std::endl;
                    abort();
                }
                hps::to_stream(secretTokenBinary, out_file);
                hps::to_stream(account_id_db, out_file);
            }
            {
                std::ofstream out_file("database/accounts/"+std::to_string(account_id_db), std::ofstream::binary);
                if(!out_file.good() || !out_file.is_open())
                {
                    std::cerr << "Unable to open data base file " << "database/accounts/"+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE) << " (abort)" << std::endl;
                    abort();
                    return;
                }

                std::vector<CharacterEntry> characterEntryList;
                hps::to_stream(characterEntryList, out_file);

                //std::cerr << "createAccount_return) for: database/accounts/" << binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE) << std::endl;
            }
        }
        #else
        #error Define what do here
        #endif

        //send the network reply
        removeFromQueryReceived(askLoginParam->query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=askLoginParam->query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        stat=ClientStat::ProtocolGood;
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    else
        loginIsWrong(askLoginParam->query_id,0x02,"Login already used: "+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE));
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::character_list_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->character_list_object();
}

void Client::character_list_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    if(paramToPassToCallBack.size()!=1)
    {
        std::cerr << "paramToPassToCallBack.size()!=1" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    askLoginParam->characterOutputDataSize=character_list_return(ProtocolParsingBase::tempBigBufferForOutput,askLoginParam->query_id);
    if(askLoginParam->characterOutputDataSize==0)
        return;
    askLoginParam->characterOutputData=(char *)malloc(askLoginParam->characterOutputDataSize);
    memcpy(askLoginParam->characterOutputData,ProtocolParsingBase::tempBigBufferForOutput,askLoginParam->characterOutputDataSize);
    //re use
    //delete askLoginParam;
    if(server_list())
        paramToPassToCallBack.push(askLoginParam);
}

uint32_t Client::send_characterEntryList(const std::vector<CharacterEntry> &characterEntryList,char * data,const uint8_t &query_id)
{
    if(CommonSettingsCommon::commonSettingsCommon.max_character==0 && characterEntryList.empty())
    {
        loginIsWrong(query_id,0x05,"Can't create character and don't have character");
        return 0;
    }

    uint32_t posOutput=0;
    data[posOutput]=0x01;//Number of characters group, characters group 0, all in one server
    posOutput+=1;

    number_of_character=static_cast<uint8_t>(characterEntryList.size());
    data[posOutput]=static_cast<uint8_t>(characterEntryList.size());
    posOutput+=1;
    unsigned int index=0;
    while(index<characterEntryList.size())
    {
        const CharacterEntry &characterEntry=characterEntryList.at(index);
        *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.character_id);
        posOutput+=4;
        {
            {
                const std::string &text=characterEntry.pseudo;
                data[posOutput]=static_cast<uint8_t>(text.size());
                posOutput+=1;
                memcpy(data+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
        }
        data[posOutput]=characterEntry.skinId;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.delete_time_left);
        posOutput+=4;
        /// \todo optimise by simple suming here, not SQL query
        *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.played_time);
        posOutput+=4;
        *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.last_connect);
        posOutput+=4;
        index++;
    }
    return posOutput;
}

uint32_t Client::character_list_return(char * data,const uint8_t &query_id)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="AskLoginParam")
    {
        std::cerr << "is not AskLoginParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    callbackRegistred.pop();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    (void)data;
    (void)query_id;
    #elif CATCHCHALLENGER_DB_FILE
    (void)data;
    (void)query_id;
    #else
    #error Define what do here
    #endif
    //send signals into the server
    #ifndef SERVERBENCHMARK
    //normalOutput("Logged the account "+std::to_string(account_id));
    #endif
    //send the network reply

    {
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        std::vector<CharacterEntry> characterEntryList;
        const auto &current_time=sFrom1970();
        bool ok;
        while(GlobalServerData::serverPrivateVariables.db_common->next() && characterEntryList.size()<CommonSettingsCommon::commonSettingsCommon.max_character)
        {
            CharacterEntry characterEntry;
            characterEntry.character_id=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
            if(ok)
            {
                uint32_t time_to_delete=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(3),&ok);
                if(!ok)
                {
                    normalOutput("time_to_delete is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(3)+" for "+std::to_string(account_id)+" fixed by 0");
                    time_to_delete=0;
                }
                characterEntry.played_time=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(4),&ok);
                if(!ok)
                {
                    normalOutput("played_time is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(4)+" for "+std::to_string(account_id)+" fixed by 0");
                    characterEntry.played_time=0;
                }
                characterEntry.last_connect=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(5),&ok);
                if(!ok)
                {
                    normalOutput("last_connect is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(5)+" for "+std::to_string(account_id)+" fixed by 0");
                    characterEntry.last_connect=current_time;
                }
                if(current_time>=time_to_delete && time_to_delete!=0)
                    deleteCharacterNow(characterEntry.character_id);
                else
                {
                    if(time_to_delete==0)
                        characterEntry.delete_time_left=0;
                    else
                        characterEntry.delete_time_left=static_cast<uint32_t>(static_cast<uint64_t>(time_to_delete)-current_time);
                    characterEntry.pseudo=GlobalServerData::serverPrivateVariables.db_common->value(1);
                    if(characterEntry.pseudo.size()==0 || characterEntry.pseudo.size()>255)
                    {
                        normalOutput("character pseudo size out of range"+characterEntry.pseudo);
                        characterEntry.skinId=0;
                        ok=true;
                    }
                    const uint32_t &skinIdTemp=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
                    if(!ok)
                    {
                        normalOutput("character return skin is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(5)+" for "+std::to_string(account_id)+" fixed by 0");
                        characterEntry.skinId=0;
                        ok=true;
                    }
                    else
                    {
                        if(skinIdTemp>=(uint32_t)DictionaryLogin::dictionary_skin_database_to_internal.size())
                        {
                            normalOutput("character return skin out of range: "+
                                         GlobalServerData::serverPrivateVariables.db_common->value(5)+
                                         " for "+
                                         std::to_string(account_id)+
                                         "fixed by 0");
                            characterEntry.skinId=0;
                            ok=true;
                        }
                        else
                            characterEntry.skinId=DictionaryLogin::dictionary_skin_database_to_internal.at(skinIdTemp);
                    }
                    if(ok)
                        characterEntryList.push_back(characterEntry);
                }
            }
            else
                normalOutput("Character id is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" for "+std::to_string(account_id));
        }
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif

        #ifndef CATCHCHALLENGER_DB_FILE
        const uint32_t posOutput=send_characterEntryList(characterEntryList,data,query_id);
        if(posOutput==0)
        {
            std::cerr << "send_characterEntryList(characterEntryList) wrong" << std::endl;
            return 0;
        }
        return posOutput;
        #endif
    }

    return 0;
}
#endif
