#include "EventLoopServerLoginMaster.hpp"
#include <cstring>
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../base/VariableServer.hpp"
#include "../../general/fight/CommonFightEngineBase.hpp"
// Concrete DB-backend header (full type for `new EventLoopDb()` and
// `static_cast<EventLoopDb *>`).
#if defined(CATCHCHALLENGER_DB_POSTGRESQL)
#include "../cli/db/EventLoopPostgresql.hpp"
#elif defined(CATCHCHALLENGER_DB_MYSQL)
#include "../cli/db/EventLoopMySQL.hpp"
#endif

using namespace CatchChallenger;

#include <vector>
#include <regex>

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <iostream>

// 1) max account id
// 2) common datapack
// 3) sql dictionary
// 4) datapack sum and cache
// 5) compose reply cache

/// \todo the back id get

#include "EventLoopClientLoginMaster.hpp"

EventLoopServerLoginMaster *EventLoopServerLoginMaster::unixServerLoginMaster=NULL;
std::vector<char> EventLoopServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver;
int EventLoopServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize=0;

EventLoopServerLoginMaster::EventLoopServerLoginMaster() :
    purgeTheLockedAccount(NULL),
    checkTimeoutGameServer(NULL),
    automaticPingSend(NULL),
    server_ip(NULL),
    server_port(NULL),
    rawServerListForC211Size(0),
    databaseBaseLogin(NULL),
    databaseBaseBase(NULL),
    purgeLockPeriod(3*60),
    maxLockAge(10*60)
{
    CommonSettingsCommon::commonSettingsCommon.automatic_account_creation   = false;
    CommonSettingsCommon::commonSettingsCommon.character_delete_time        = 3600;
    CommonSettingsCommon::commonSettingsCommon.min_character                = 0;
    CommonSettingsCommon::commonSettingsCommon.max_character                = 3;
    CommonSettingsCommon::commonSettingsCommon.max_pseudo_size              = 20;
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters            = 8;
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters   = 30;
    {
        //empty buffer
        memset(EventLoopClientLoginMaster::serverServerList,0x00,sizeof(EventLoopClientLoginMaster::serverServerList));
        memset(EventLoopClientLoginMaster::serverLogicalGroupList,0x00,sizeof(EventLoopClientLoginMaster::serverLogicalGroupList));
        //size the C211 scratch buffer once (resize zero-fills); it is filled by
        //charactersGroupListReply()+loadTheProfile() via .data()+offset writes
        //bounded by rawServerListForC211Size, then released in loadTheProfile().
        rawServerListForC211.resize(sizeof(EventLoopClientLoginMaster::loginSettingsAndCharactersGroup),0x00);
    }

    TinyXMLSettings settings(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/master.xml");

    srand(time(NULL));

    loadLoginSettings(settings);
    loadDBLoginSettings(settings);
    std::vector<std::string> charactersGroupList=loadCharactersGroup(settings);
    charactersGroupListReply(charactersGroupList);
    loadTheDatapack();
    doTheLogicalGroup(settings);
    doTheServerList();

    EventLoopClientLoginMaster::fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(EventLoopClientLoginMaster::fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
}

EventLoopServerLoginMaster::~EventLoopServerLoginMaster()
{
    fclose(EventLoopClientLoginMaster::fpRandomFile);

    memset(EventLoopClientLoginMaster::private_token,0x00,sizeof(EventLoopClientLoginMaster::private_token));
    if(server_ip!=NULL)
    {
        delete[] server_ip;   //new char[] -> must be delete[], not scalar delete
        server_ip=NULL;
    }
    if(server_port!=NULL)
    {
        delete[] server_port;   //new char[] -> must be delete[], not scalar delete
        server_port=NULL;
    }
    if(databaseBaseLogin!=NULL)
    {
        EventLoopDb *databaseBasePg=static_cast<EventLoopDb *>(databaseBaseLogin);
        delete databaseBasePg;
        databaseBaseLogin=NULL;
    }
    if(databaseBaseBase!=NULL)
    {
        EventLoopDb *databaseBasePg=static_cast<EventLoopDb *>(databaseBaseBase);
        delete databaseBasePg;
        databaseBaseBase=NULL;
    }
    //rawServerListForC211 / fixedValuesRawDictionaryCacheForGameserver are now
    //std::vector<char> (RAII) — they free themselves; no manual delete (the old
    //`delete` on these malloc'd buffers was a free()/delete mismatch, UB).
    rawServerListForC211Size=0;
    EventLoopServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize=0;
    /*
    auto i = CharactersGroup::hash.begin();
    while (i != CharactersGroup::hash.cend()) {
        delete i->second;
        ++i;
    }*/
    {
        unsigned int index=0;
        while(index<CharactersGroup::list.size())
        {
            delete CharactersGroup::list.at(index);
            index++;
        }
    }
    CharactersGroup::hash.clear();
    CharactersGroup::list.clear();
    if(purgeTheLockedAccount!=NULL)
    {
        delete purgeTheLockedAccount;
        purgeTheLockedAccount=NULL;
    }
    if(checkTimeoutGameServer!=NULL)
    {
        delete checkTimeoutGameServer;
        checkTimeoutGameServer=NULL;
    }
    if(automaticPingSend!=NULL)
    {
        delete automaticPingSend;
        automaticPingSend=NULL;
    }
}

void EventLoopServerLoginMaster::loadLoginSettings(TinyXMLSettings &settings)
{
    if(!settings.contains("ip"))
        settings.setValue("ip","");
    const std::string &server_ip_string=settings.value("ip");
    if(!server_ip_string.empty())
    {
        server_ip=new char[server_ip_string.size()+1];
        strcpy(server_ip,server_ip_string.data());
    }
    if(!settings.contains("port"))
        settings.setValue("port",rand()%40000+10000);
    const std::string &server_port_data=settings.value("port");
    server_port=new char[server_port_data.size()+1];
    strcpy(server_port,server_port_data.data());
    if(!settings.contains("token"))
        generateToken(settings);
    std::string token=settings.value("token");
    if(token.size()!=(TOKEN_SIZE_FOR_MASTERAUTH*2))
        generateToken(settings);
    token=settings.value("token");
    const std::vector<char> &tokenbinary=hexatoBinary(token);
    if(tokenbinary.empty())
    {
        std::cerr << "convertion to binary for pass failed for: " << token << std::endl;
        abort();
    }
    memcpy(EventLoopClientLoginMaster::private_token,tokenbinary.data(),TOKEN_SIZE_FOR_MASTERAUTH);

    //connection
    #ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
    if(!settings.contains("compression"))
        settings.setValue("compression","zstd");
    if(settings.value("compression").toString()=="none")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::None;
    else if(settings.value("compression").toString()=="zstd")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Zstd;
    else
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Zstd;
    ProtocolParsing::compressionLevel          = stringtouint8(settings.value("compressionLevel"));
    #endif
    if(!settings.contains("automatic_account_creation"))
        settings.setValue("automatic_account_creation",false);
    if(!settings.contains("character_delete_time"))
        settings.setValue("character_delete_time",604800);
    if(!settings.contains("max_pseudo_size"))
        settings.setValue("max_pseudo_size",20);
    if(!settings.contains("max_character"))
        settings.setValue("max_character",3);
    if(!settings.contains("min_character"))
        settings.setValue("min_character",1);
    if(!settings.contains("maxLockAge"))
        settings.setValue("maxLockAge",5);
    if(!settings.contains("purgeLockPeriod"))
        settings.setValue("purgeLockPeriod",3*60);
    settings.sync();
    CommonSettingsCommon::commonSettingsCommon.automatic_account_creation=stringtobool(settings.value("automatic_account_creation"));
    bool ok;
    CommonSettingsCommon::commonSettingsCommon.character_delete_time=stringtouint32(settings.value("character_delete_time"),&ok);
    if(CommonSettingsCommon::commonSettingsCommon.character_delete_time==0 || !ok)
    {
        std::cerr << "character_delete_time==0 (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.min_character=stringtouint8(settings.value("min_character"),&ok);
    if(!ok)
    {
        std::cerr << "CommonSettingsCommon::commonSettingsCommon.min_character not number (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.max_character=stringtouint8(settings.value("max_character"),&ok);
    if(CommonSettingsCommon::commonSettingsCommon.max_character<=0 || !ok)
    {
        std::cerr << "max_character<=0 (abort)" << std::endl;
        abort();
    }
    if(CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character)
    {
        std::cerr << "max_character<min_character (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.max_pseudo_size=stringtouint8(settings.value("max_pseudo_size"),&ok);
    if(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size==0 || !ok)
    {
        std::cerr << "max_pseudo_size==0 (abort)" << std::endl;
        abort();
    }
    maxLockAge=stringtouint16(settings.value("maxLockAge"),&ok);
    if(maxLockAge<1 || maxLockAge>3600 || !ok)
    {
        std::cerr << "maxLockAge<1 || maxLockAge>3600 || not number (abort)" << std::endl;
        abort();
    }
    purgeLockPeriod=stringtouint16(settings.value("purgeLockPeriod"),&ok);
    if(purgeLockPeriod<1 || purgeLockPeriod>3600 || !ok)
    {
        std::cerr << "purgeLockPeriod<1 || purgeLockPeriod>3600 || not number (abort)" << std::endl;
        abort();
    }
    if(purgeLockPeriod>maxLockAge)
        std::cerr << "Warning: purgeLockPeriod>maxLockAge" << std::endl;
    if(purgeTheLockedAccount!=NULL)
        delete purgeTheLockedAccount;
    purgeTheLockedAccount=new PurgeTheLockedAccount(purgeLockPeriod);
    if(!settings.contains("maxPlayerMonsters"))
        settings.setValue("maxPlayerMonsters",8);
    if(!settings.contains("maxWarehousePlayerMonsters"))
        settings.setValue("maxWarehousePlayerMonsters",30);
    settings.sync();
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters=stringtouint8(settings.value("maxPlayerMonsters"),&ok);
    if(CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters==0 || !ok)
    {
        std::cerr << "maxPlayerMonsters==0 (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters=stringtouint16(settings.value("maxWarehousePlayerMonsters"),&ok);
    if(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters==0 || !ok)
    {
        std::cerr << "maxWarehousePlayerMonsters==0 (abort)" << std::endl;
        abort();
    }

    {
        CharactersGroup::lastPingStarted=msFrom1970();
        settings.beginGroup("gameserver");
        if(!settings.contains("checkTimeoutGameServerSecond"))
            settings.setValue("checkTimeoutGameServerSecond",1);
        CharactersGroup::checkTimeoutGameServerMSecond=stringtouint32(settings.value("checkTimeoutGameServerSecond"),&ok)*1000;
        if(CharactersGroup::checkTimeoutGameServerMSecond==0 || !ok)
        {
            std::cerr << "gameserver pingSecond (abort)" << std::endl;
            abort();
        }
        checkTimeoutGameServer=new CheckTimeoutGameServer(CharactersGroup::checkTimeoutGameServerMSecond);

        if(!settings.contains("pingSecond"))
            settings.setValue("pingSecond",60);
        CharactersGroup::pingMSecond=stringtouint32(settings.value("pingSecond"),&ok)*1000;
        if(CharactersGroup::pingMSecond==0 || !ok)
        {
            std::cerr << "gameserver pingSecond (abort)" << std::endl;
            abort();
        }
        automaticPingSend=new AutomaticPingSend(CharactersGroup::pingMSecond);

        if(!settings.contains("gameserverTimeoutms"))
            settings.setValue("gameserverTimeoutms",1000);
        CharactersGroup::gameserverTimeoutms=stringtouint16(settings.value("gameserverTimeoutms"),&ok);
        if(CharactersGroup::gameserverTimeoutms==0 || !ok)
        {
            std::cerr << "gameserver gameserverTimeoutms (abort)" << std::endl;
            abort();
        }
        settings.sync();
        settings.endGroup();
    }
}

void EventLoopServerLoginMaster::loadDBLoginSettings(TinyXMLSettings &settings)
{
    if(CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
    {
        std::string db;
        std::string host;
        std::string login;
        std::string pass;
        std::string type;

        settings.beginGroup("db-login");
        if(!settings.contains("considerDownAfterNumberOfTry"))
            settings.setValue("considerDownAfterNumberOfTry",30);
        if(!settings.contains("tryInterval"))
            settings.setValue("tryInterval",1);
        if(!settings.contains("db"))
            settings.setValue("db","catchchallenger_login");
        if(!settings.contains("host"))
            settings.setValue("host","localhost");
        if(!settings.contains("login"))
            settings.setValue("login","root");
        if(!settings.contains("pass"))
            settings.setValue("pass","root");
        if(!settings.contains("type"))
        {
            #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                settings.setValue("type","postgresql");
            #else
                #ifdef CATCHCHALLENGER_DB_MYSQL
                    settings.setValue("type","mysql");
                #else
                    settings.setValue("type","mysql");//CATCHCHALLENGER_CLASS_QT
                #endif
            #endif
        }
        if(!settings.contains("comment"))
            settings.setValue("comment","to do maxAccountId");
        settings.sync();

        bool ok;
        //to load the dictionary
        {
            databaseBaseLogin=new EventLoopDb();
            //here to have by login server an auth

            databaseBaseLogin->considerDownAfterNumberOfTry=stringtouint32(settings.value("considerDownAfterNumberOfTry"),&ok);
            if(databaseBaseLogin->considerDownAfterNumberOfTry==0 || !ok)
            {
                std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
                abort();
            }
            db=settings.value("db");
            host=settings.value("host");
            login=settings.value("login");
            pass=settings.value("pass");
            databaseBaseLogin->tryInterval=stringtouint32(settings.value("tryInterval"),&ok);
            if(databaseBaseLogin->tryInterval==0 || !ok)
            {
                std::cerr << "tryInterval==0 (abort)" << std::endl;
                abort();
            }
            type=settings.value("type");
            #if defined(CATCHCHALLENGER_DB_POSTGRESQL)
            if(type!="postgresql")
#elif defined(CATCHCHALLENGER_DB_MYSQL)
            if(type!="mysql")
#else
#error No DB backend
#endif
            {
                std::cerr << "db type " << type << " does not match compile-time backend (abort)" << std::endl;
                abort();
            }
            if(!databaseBaseLogin->syncConnect(host.c_str(),db.c_str(),login.c_str(),pass.c_str()))
            {
                std::cerr << "Connect to login database failed:" << databaseBaseLogin->errorMessage() << ", host: " << host << ", db: " << db << ", login: " << login << std::endl;
                abort();
            }
        }

        CharactersGroup::serverWaitedToBeReady++;
        load_account_max_id();
        settings.endGroup();
    }
    else
    {
        EventLoopClientLoginMaster::maxAccountId=1;
        databaseBaseLogin=NULL;
    }
    {
        std::string db;
        std::string host;
        std::string login;
        std::string pass;
        std::string type;

        settings.beginGroup("db-base");
        if(!settings.contains("considerDownAfterNumberOfTry"))
            settings.setValue("considerDownAfterNumberOfTry",30);
        if(!settings.contains("tryInterval"))
            settings.setValue("tryInterval",1);
        if(!settings.contains("db"))
            settings.setValue("db","catchchallenger_base");
        if(!settings.contains("host"))
            settings.setValue("host","localhost");
        if(!settings.contains("login"))
            settings.setValue("login","root");
        if(!settings.contains("pass"))
            settings.setValue("pass","root");
        if(!settings.contains("type"))
        {
            #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                settings.setValue("type","postgresql");
            #else
                #ifdef CATCHCHALLENGER_DB_MYSQL
                    settings.setValue("type","mysql");
                #else
                    settings.setValue("type","mysql");//CATCHCHALLENGER_CLASS_QT
                #endif
            #endif
        }
        if(!settings.contains("comment"))
            settings.setValue("comment","to do maxAccountId");
        settings.sync();

        bool ok;
        //to load the dictionary
        {
            databaseBaseBase=new EventLoopDb();
            //here to have by login server an auth

            databaseBaseBase->considerDownAfterNumberOfTry=stringtouint32(settings.value("considerDownAfterNumberOfTry"),&ok);
            if(databaseBaseBase->considerDownAfterNumberOfTry==0 || !ok)
            {
                std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
                abort();
            }
            db=settings.value("db");
            host=settings.value("host");
            login=settings.value("login");
            pass=settings.value("pass");
            databaseBaseBase->tryInterval=stringtouint32(settings.value("tryInterval"),&ok);
            if(databaseBaseBase->tryInterval==0 || !ok)
            {
                std::cerr << "tryInterval==0 (abort)" << std::endl;
                abort();
            }
            type=settings.value("type");
            #if defined(CATCHCHALLENGER_DB_POSTGRESQL)
            if(type!="postgresql")
#elif defined(CATCHCHALLENGER_DB_MYSQL)
            if(type!="mysql")
#else
#error No DB backend
#endif
            {
                std::cerr << "db type " << type << " does not match compile-time backend (abort)" << std::endl;
                abort();
            }
            if(!databaseBaseBase->syncConnect(host.c_str(),db.c_str(),login.c_str(),pass.c_str()))
            {
                std::cerr << "Connect to base database failed:" << databaseBaseBase->errorMessage() << ", host: " << host << ", db: " << db << ", login: " << login << std::endl;
                abort();
            }
        }

        CharactersGroup::serverWaitedToBeReady++;
        settings.endGroup();
    }
}

std::vector<std::string> EventLoopServerLoginMaster::loadCharactersGroup(TinyXMLSettings &settings)
{
    std::string db;
    std::string host;
    std::string login;
    std::string pass;
    std::string type;
    bool ok;

    std::vector<std::string> charactersGroupList;
    bool continueCharactersGroupSettings=true;
    int charactersGroupId=0;
    while(continueCharactersGroupSettings)
    {
        settings.beginGroup("db-common-"+std::to_string(charactersGroupId));
        if(charactersGroupId==0)
        {
            if(!settings.contains("considerDownAfterNumberOfTry"))
                settings.setValue("considerDownAfterNumberOfTry",3);
            if(!settings.contains("tryInterval"))
                settings.setValue("tryInterval",5);
            if(!settings.contains("db"))
                settings.setValue("db","catchchallenger_common");
            if(!settings.contains("host"))
                settings.setValue("host","localhost");
            if(!settings.contains("login"))
                settings.setValue("login","root");
            if(!settings.contains("pass"))
                settings.setValue("pass","root");
            if(!settings.contains("type"))
            {
                #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                    settings.setValue("type","postgresql");
                #else
                    #ifdef CATCHCHALLENGER_DB_MYSQL
                        settings.setValue("type","mysql");
                    #else
                        settings.setValue("type","mysql");//CATCHCHALLENGER_CLASS_QT
                    #endif
                #endif
            }
            if(!settings.contains("charactersGroup"))
                settings.setValue("charactersGroup","");
            if(!settings.contains("comment"))
                settings.setValue("comment","to do maxClanId, maxCharacterId, maxMonsterId");
        }
        settings.sync();
        if(settings.contains("login"))
        {
            const std::string &charactersGroup=settings.value("charactersGroup");
            if(CharactersGroup::hash.find(charactersGroup)==CharactersGroup::hash.cend())
            {
                CharactersGroup::serverWaitedToBeReady++;
                const uint8_t &considerDownAfterNumberOfTry=stringtouint8(settings.value("considerDownAfterNumberOfTry"),&ok);
                if(considerDownAfterNumberOfTry==0 || !ok)
                {
                    std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
                    abort();
                }
                db=settings.value("db");
                host=settings.value("host");
                login=settings.value("login");
                pass=settings.value("pass");
                const uint8_t &tryInterval=stringtouint8(settings.value("tryInterval"),&ok);
                if(tryInterval==0 || !ok)
                {
                    std::cerr << "tryInterval==0 (abort)" << std::endl;
                    abort();
                }
                type=settings.value("type");
                #if defined(CATCHCHALLENGER_DB_POSTGRESQL)
            if(type!="postgresql")
#elif defined(CATCHCHALLENGER_DB_MYSQL)
            if(type!="mysql")
#else
#error No DB backend
#endif
            {
                std::cerr << "db type " << type << " does not match compile-time backend (abort)" << std::endl;
                    abort();
                }
                CharactersGroup::hash[charactersGroup]=new CharactersGroup(db.c_str(),host.c_str(),login.c_str(),pass.c_str(),considerDownAfterNumberOfTry,tryInterval,charactersGroup);
                CharactersGroup::hash[charactersGroup]->setMaxLockAge(maxLockAge);
                charactersGroupList.push_back(charactersGroup);
            }
            else
            {
                std::cerr << "charactersGroup already found for group " << charactersGroupId << std::endl;
                abort();
            }
            charactersGroupId++;
        }
        else
            continueCharactersGroupSettings=false;
        settings.endGroup();
    }
    std::sort(charactersGroupList.begin(),charactersGroupList.end());
    CharactersGroup::lastPingStarted=msFrom1970();
    return charactersGroupList;
}

void EventLoopServerLoginMaster::charactersGroupListReply(std::vector<std::string> &charactersGroupList)
{
    rawServerListForC211[0x00]=CommonSettingsCommon::commonSettingsCommon.automatic_account_creation;
    {const uint32_t _tmp_le=(htole32(CommonSettingsCommon::commonSettingsCommon.character_delete_time));memcpy(rawServerListForC211.data()+0x01,&_tmp_le,sizeof(_tmp_le));}

    rawServerListForC211[0x05]=CommonSettingsCommon::commonSettingsCommon.min_character;
    rawServerListForC211[0x06]=CommonSettingsCommon::commonSettingsCommon.max_character;
    rawServerListForC211[0x07]=CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
    rawServerListForC211[0x08]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
    {const uint16_t _tmp_le=(htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters));memcpy(rawServerListForC211.data()+0x09,&_tmp_le,sizeof(_tmp_le));}

    //Login expects 3 reserved bytes between maxWarehousePlayerMonsters
    //and the charactersGroup count — see
    //server/login/LinkToMasterProtocolParsingMessage.cpp where the
    //compare cursor jumps from 0x0B to 0x0E with nothing read in
    //between. Zero them so the memcmp succeeds.
    rawServerListForC211[0x0B]=0;
    rawServerListForC211[0x0C]=0;
    rawServerListForC211[0x0D]=0;
    rawServerListForC211Size=0x0E;
    //do the Characters group
    rawServerListForC211[rawServerListForC211Size]=(unsigned char)charactersGroupList.size();
    rawServerListForC211Size+=sizeof(unsigned char);
    unsigned int index=0;
    while(index<charactersGroupList.size())
    {
        const std::string &charactersGroupName=charactersGroupList.at(index);
        int newSize=0;
        if(!charactersGroupName.empty())
            newSize=FacilityLibGeneral::toUTF8WithHeader(charactersGroupName,rawServerListForC211.data()+rawServerListForC211Size);
        else
        {
            rawServerListForC211[rawServerListForC211Size]=0x00;
            newSize=1;
        }
        if(newSize==0 || charactersGroupName.size()>20)
        {
            std::cerr << "charactersGroupName to hurge, null or unable to translate in utf8 (abort)" << std::endl;
            abort();
        }
        rawServerListForC211Size+=newSize;
        CharactersGroup::list.push_back(CharactersGroup::hash.at(charactersGroupName));
        CharactersGroup::list.back()->index=index;

        index++;
    }
}

void EventLoopServerLoginMaster::doTheLogicalGroup(TinyXMLSettings &settings)
{
    //do the LogicalGroup
    memset(EventLoopClientLoginMaster::serverLogicalGroupList,0x00,sizeof(EventLoopClientLoginMaster::serverLogicalGroupList));
    EventLoopClientLoginMaster::serverLogicalGroupList[0x00]=0x44;
    EventLoopClientLoginMaster::serverLogicalGroupListSize=1+4+0x01/*logicalGroupSize*/;

    std::string textToConvert;
    uint8_t logicalGroup=0;
    bool logicalGroupContinue=true;
    while(logicalGroupContinue)
    {
        settings.beginGroup("logical-group-"+std::to_string(logicalGroup));
        logicalGroupContinue=settings.contains("path") && settings.contains("xml") && logicalGroup<255;
        if(!logicalGroupContinue && logicalGroup==0)
        {
            if(!settings.contains("path"))
                settings.setValue("path","");
            if(!settings.contains("xml"))
                settings.setValue("xml","<name>root</name>");
            logicalGroupContinue=true;
        }
        if(logicalGroupContinue)
        {
            //path
            {
                textToConvert=settings.value("path");
                if(textToConvert.size()>20)
                {
                    std::cerr << "path too hurge (abort)" << std::endl;
                    abort();
                }

                EventLoopClientLoginMaster::logicalGroupHash[textToConvert]=logicalGroup;

                if(textToConvert.size()>255)
                {
                    std::cerr << "logical group converted too big (abort)" << std::endl;
                    abort();
                }
                EventLoopClientLoginMaster::serverLogicalGroupList[EventLoopClientLoginMaster::serverLogicalGroupListSize]=textToConvert.size();
                EventLoopClientLoginMaster::serverLogicalGroupListSize+=1;
                memcpy(EventLoopClientLoginMaster::serverLogicalGroupList+EventLoopClientLoginMaster::serverLogicalGroupListSize,textToConvert.data(),textToConvert.size());
                EventLoopClientLoginMaster::serverLogicalGroupListSize+=textToConvert.size();
            }
            //translation
            {
                textToConvert=settings.value("xml");
                if(textToConvert.size()>0)
                {
                    if(textToConvert.size()>4*1024)
                    {
                        std::cerr << "translation too hurge (abort)" << std::endl;
                        abort();
                    }
                    int newSize=FacilityLibGeneral::toUTF8With16BitsHeader(textToConvert,EventLoopClientLoginMaster::serverLogicalGroupList+EventLoopClientLoginMaster::serverLogicalGroupListSize);
                    if(newSize==0)
                    {
                        std::cerr << "translation null or unable to translate in utf8 (abort)" << std::endl;
                        abort();
                    }
                    EventLoopClientLoginMaster::serverLogicalGroupListSize+=newSize;
                }
                else
                {
                    {const uint16_t _tmp_le=(0);memcpy(EventLoopClientLoginMaster::serverLogicalGroupList+EventLoopClientLoginMaster::serverLogicalGroupListSize,&_tmp_le,sizeof(_tmp_le));}//not convert to le16... 0 remain 0
                    EventLoopClientLoginMaster::serverLogicalGroupListSize+=2;
                }
            }

            logicalGroup++;
        }
        settings.endGroup();
    }
    {const uint32_t _tmp_le=(htole32(EventLoopClientLoginMaster::serverLogicalGroupListSize-1-4));memcpy(EventLoopClientLoginMaster::serverLogicalGroupList+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size
    EventLoopClientLoginMaster::serverLogicalGroupList[1+4]=logicalGroup;

    if(EventLoopClientLoginMaster::serverLogicalGroupListSize==0)
    {
        std::cerr << "EventLoopClientLoginMaster::serverLogicalGroupListSize==0 (abort)" << std::endl;
        abort();
    }
}

/// \todo can be optimized by just memory manipulation, and add/remove block and cache the serialisation
void EventLoopServerLoginMaster::doTheServerList()
{
    //do the server list
    EventLoopClientLoginMaster::serverPartialServerListSize=0;
    //memset(EventLoopClientLoginMaster::serverPartialServerList,0x00,sizeof(EventLoopClientLoginMaster::serverPartialServerList));//improve the performance
    unsigned int pos=0x00;

    #ifdef CATCHCHALLENGER_HARDENED
    std::unordered_map<uint8_t/*charactersgroup index*/,std::unordered_set<uint32_t/*unique key*/> > duplicateDetect;
    #endif
    EventLoopClientLoginMaster::serverPartialServerList[pos]=EventLoopClientLoginMaster::gameServers.size();
    pos+=1;
    unsigned int serverListIndex=0;
    while(serverListIndex<EventLoopClientLoginMaster::gameServers.size())
    {
        #ifdef CATCHCHALLENGER_HARDENED
        if(serverListIndex>=EventLoopClientLoginMaster::gameServers.size())
        {
            std::cerr << "serverListIndex>=EventLoopClientLoginMaster::gameServers.size() (abort)" << std::endl;
            abort();
        }
        #endif
        const EventLoopClientLoginMaster * const gameServerOnEpollClientLoginMaster=EventLoopClientLoginMaster::gameServers.at(serverListIndex);
        #ifdef CATCHCHALLENGER_HARDENED
        if(gameServerOnEpollClientLoginMaster==NULL)
        {
            std::cerr << "charactersGroup==NULL (abort)" << std::endl;
            abort();
        }
        #endif
        const CharactersGroup::InternalGameServer * const gameServerOnCharactersGroup=gameServerOnEpollClientLoginMaster->charactersGroupForGameServerInformation;
        #ifdef CATCHCHALLENGER_HARDENED
        if(gameServerOnCharactersGroup==NULL)
        {
            std::cerr << "charactersGroup==NULL (abort)" << std::endl;
            abort();
        }
        #endif

        //charactersGroup
        {
            EventLoopClientLoginMaster::serverPartialServerList[pos]=gameServerOnEpollClientLoginMaster->charactersGroupForGameServer->index;
            pos+=1;
        }
        //key
        {
            {const uint32_t _tmp_le=(htole32(gameServerOnEpollClientLoginMaster->uniqueKey));memcpy(EventLoopClientLoginMaster::serverPartialServerList+pos,&_tmp_le,sizeof(_tmp_le));}

            pos+=sizeof(gameServerOnEpollClientLoginMaster->uniqueKey);
        }
        #ifdef CATCHCHALLENGER_HARDENED
        //add more control
        {
            const uint8_t &charactersGroupIndex=gameServerOnEpollClientLoginMaster->charactersGroupForGameServer->index;
            const uint32_t &serverUniqueKey=gameServerOnEpollClientLoginMaster->uniqueKey;
            if(duplicateDetect.find(charactersGroupIndex)==duplicateDetect.cend())
                duplicateDetect[charactersGroupIndex]=std::unordered_set<uint32_t/*unique key*/>();
            std::unordered_set<uint32_t/*unique key*/> &duplicateDetectEntry=duplicateDetect[charactersGroupIndex];
            if(duplicateDetectEntry.find(serverUniqueKey)!=duplicateDetectEntry.cend())//exists, bug
            {
                std::cerr << "Duplicate unique key for packet 45 found: " << std::to_string(serverUniqueKey) << std::endl;
                abort();
            }
            else
                duplicateDetectEntry.insert(serverUniqueKey);
        }
        #endif
        //host
        {
            int newSize=FacilityLibGeneral::toUTF8WithHeader(gameServerOnCharactersGroup->host,EventLoopClientLoginMaster::serverPartialServerList+pos);
            if(newSize==0)
            {
                std::cerr << "host null or unable to translate in utf8 (abort): " << gameServerOnCharactersGroup->host << std::endl;
                abort();
            }
            pos+=newSize;
        }
        //port
        {
            *reinterpret_cast<unsigned short int *>(EventLoopClientLoginMaster::serverPartialServerList+pos)=(unsigned short int)htole16((unsigned short int)gameServerOnCharactersGroup->port);
            pos+=sizeof(unsigned short int);
        }
        //metaData
        {
            if(gameServerOnCharactersGroup->metaData.size()>4*1024)
            {
                std::cerr << "metaData too hurge (abort)" << std::endl;
                abort();
            }
            {
                if(gameServerOnCharactersGroup->metaData.size()>65535)
                {
                    {const uint16_t _tmp_le=(0);memcpy(EventLoopClientLoginMaster::serverPartialServerList+pos,&_tmp_le,sizeof(_tmp_le));}

                    pos+=2;
                }
                else
                {
                    {const uint16_t _tmp_le=(htole16(gameServerOnCharactersGroup->metaData.size()));memcpy(EventLoopClientLoginMaster::serverPartialServerList+pos,&_tmp_le,sizeof(_tmp_le));}

                    pos+=2;
                    memcpy(EventLoopClientLoginMaster::serverPartialServerList+pos,gameServerOnCharactersGroup->metaData.data(),gameServerOnCharactersGroup->metaData.size());
                    pos+=gameServerOnCharactersGroup->metaData.size();
                }
            }
        }
        //logicalGroup
        {
            EventLoopClientLoginMaster::serverPartialServerList[pos]=gameServerOnCharactersGroup->logicalGroupIndex;
            pos+=1;
        }
        //max player
        *reinterpret_cast<unsigned short int *>(EventLoopClientLoginMaster::serverPartialServerList+pos)=(unsigned short int)htole16(gameServerOnCharactersGroup->maxPlayer);
        pos+=sizeof(unsigned short int);

        serverListIndex++;
    }
    EventLoopClientLoginMaster::serverPartialServerListSize=pos;

    //Second list part with same size
    serverListIndex=0;
    while(serverListIndex<EventLoopClientLoginMaster::gameServers.size())
    {
        #ifdef CATCHCHALLENGER_HARDENED
        if(serverListIndex>=EventLoopClientLoginMaster::gameServers.size())
        {
            std::cerr << "serverListIndex>=EventLoopClientLoginMaster::gameServers.size() (abort)" << std::endl;
            abort();
        }
        #endif
        const EventLoopClientLoginMaster * const gameServerOnEpollClientLoginMaster=EventLoopClientLoginMaster::gameServers.at(serverListIndex);
        const CharactersGroup::InternalGameServer * const gameServerOnCharactersGroup=gameServerOnEpollClientLoginMaster->charactersGroupForGameServerInformation;
        #ifdef CATCHCHALLENGER_HARDENED
        if(gameServerOnCharactersGroup==NULL)
        {
            std::cerr << "charactersGroup==NULL (abort)" << std::endl;
            abort();
        }
        if(gameServerOnEpollClientLoginMaster==NULL)
        {
            std::cerr << "charactersGroup==NULL (abort)" << std::endl;
            abort();
        }
        #endif

        //connected player
        *reinterpret_cast<unsigned short int *>(EventLoopClientLoginMaster::serverPartialServerList+pos)=(unsigned short int)htole16(gameServerOnCharactersGroup->currentPlayer);
        pos+=sizeof(unsigned short int);

        serverListIndex++;
    }

    //send the network message
    EventLoopClientLoginMaster::serverServerList[0x00]=0x45;
    {const uint32_t _tmp_le=(htole32(pos));memcpy(EventLoopClientLoginMaster::serverServerList+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size
    memcpy(EventLoopClientLoginMaster::serverServerList+1+4,EventLoopClientLoginMaster::serverPartialServerList,pos);
    EventLoopClientLoginMaster::serverServerListSize=pos+1+4;
    if(EventLoopClientLoginMaster::serverServerListSize==0)
    {
        std::cerr << "EventLoopClientLoginMaster::serverServerListSize==0 (abort)" << std::endl;
        abort();
    }

    //std::cout << "Now the server list is: " << binarytoHexa(EventLoopClientLoginMaster::serverServerList,EventLoopClientLoginMaster::serverServerListSize) << std::endl;
}

void EventLoopServerLoginMaster::doTheReplyCache()
{
    if(EventLoopClientLoginMaster::loginSettingsAndCharactersGroupSize==0)
    {
        std::cerr << "loginSettingsAndCharactersGroupSize==0 (abort)" << std::endl;
        abort();
    }
    if(EventLoopClientLoginMaster::serverLogicalGroupListSize==0)
    {
        std::cerr << "loginSettingsAndCharactersGroupSize==0 (abort)" << std::endl;
        abort();
    }
    if(EventLoopClientLoginMaster::serverServerListSize==0)
    {
        std::cerr << "loginSettingsAndCharactersGroupSize==0 (abort)" << std::endl;
        abort();
    }
    //do the reply cache
    EventLoopClientLoginMaster::loginPreviousToReplyCacheSize=0;
    memcpy(EventLoopClientLoginMaster::loginPreviousToReplyCache+EventLoopClientLoginMaster::loginPreviousToReplyCacheSize,
           EventLoopClientLoginMaster::loginSettingsAndCharactersGroup,
           EventLoopClientLoginMaster::loginSettingsAndCharactersGroupSize);
    EventLoopClientLoginMaster::loginPreviousToReplyCacheSize+=EventLoopClientLoginMaster::loginSettingsAndCharactersGroupSize;

    memcpy(EventLoopClientLoginMaster::loginPreviousToReplyCache+EventLoopClientLoginMaster::loginPreviousToReplyCacheSize,
           EventLoopClientLoginMaster::serverLogicalGroupList,
           EventLoopClientLoginMaster::serverLogicalGroupListSize);
    EventLoopClientLoginMaster::loginPreviousToReplyCacheSize+=EventLoopClientLoginMaster::serverLogicalGroupListSize;

    memcpy(EventLoopClientLoginMaster::loginPreviousToReplyCache+EventLoopClientLoginMaster::loginPreviousToReplyCacheSize,
           EventLoopClientLoginMaster::serverServerList,
           EventLoopClientLoginMaster::serverServerListSize);
    EventLoopClientLoginMaster::loginPreviousToReplyCacheSize+=EventLoopClientLoginMaster::serverServerListSize;
}

bool EventLoopServerLoginMaster::tryListen()
{
        const bool &returnedValue=tryListenInternal(server_ip, server_port);
    if(server_ip!=NULL)
    {
        if(returnedValue)
            std::cout << "Listen on " << server_ip << ":" << server_port << std::endl;
        delete[] server_ip;   //new char[] -> must be delete[], not scalar delete
        server_ip=NULL;
    }
    else
    {
        if(returnedValue)
            std::cout << "Listen on *:" << server_port << std::endl;
    }
    if(server_port!=NULL)
    {
        delete[] server_port;   //new char[] -> must be delete[], not scalar delete
        server_port=NULL;
    }
    return returnedValue;
}

void EventLoopServerLoginMaster::generateToken(TinyXMLSettings &settings)
{
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(EventLoopClientLoginMaster::private_token,1,TOKEN_SIZE_FOR_MASTERAUTH,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_MASTERAUTH)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_MASTERAUTH << " needed to do the token from " << RANDOMFILEDEVICE << std::endl;
        abort();
    }
    settings.setValue("token",binarytoHexa(EventLoopClientLoginMaster::private_token,TOKEN_SIZE_FOR_MASTERAUTH).c_str());
    fclose(fpRandomFile);
}

void EventLoopServerLoginMaster::load_account_max_id()
{
    std::string queryText;
    switch(databaseBaseLogin->databaseType())
    {
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `account` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        #endif
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM account ORDER BY id DESC LIMIT 0,1;";
        break;
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM account ORDER BY id DESC LIMIT 1;";
        break;
        #endif
    default:
        abort();
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText,this,&EventLoopServerLoginMaster::load_account_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseLogin->errorMessage() << std::endl;
        abort();
    }
    EventLoopClientLoginMaster::maxAccountId=0;
}

void EventLoopServerLoginMaster::load_account_max_id_static(void *object)
{
    static_cast<EventLoopServerLoginMaster *>(object)->load_account_max_id_return();
}

void EventLoopServerLoginMaster::load_account_max_id_return()
{
    while(databaseBaseLogin->next())
    {
        bool ok;
        //not +1 because incremented before use
        EventLoopClientLoginMaster::maxAccountId=stringtouint32(databaseBaseLogin->value(0),&ok);
        if(!ok)
        {
            std::cerr << "Max account id is failed to convert to number" << std::endl;
            abort();
        }
    }
    CharactersGroup::serverWaitedToBeReady--;
    //will jump to SQL_common_load_finish()
}

void EventLoopServerLoginMaster::loadTheDatapack()
{
    CommonDatapack::commonDatapack.parseDatapack("datapack/");

    if(databaseBaseBase==NULL)
    {
        std::cerr << "Login databases need be connected to load the dictionary" << std::endl;
        abort();
    }

    baseServerMasterSendDatapack.load("datapack/");
    load(databaseBaseBase);
}

void EventLoopServerLoginMaster::SQL_common_load_finish()
{
    //INSERT INTO dictionary in suspend can't close
    /*databaseBaseLogin->syncDisconnect();
    databaseBaseLogin=NULL;*/
    CharactersGroup::serverWaitedToBeReady--;

    loadTheProfile();
    loadTheDictionary();

    CommonDatapack::commonDatapack.unload();
    baseServerMasterSendDatapack.unload();
    BaseServerMasterLoadDictionary::unload();
}

void EventLoopServerLoginMaster::loadTheProfile()
{
    if(rawServerListForC211.empty())
    {
        std::cerr << "EventLoopServerLoginMaster::loadTheProfile(): rawServerListForC211 empty (abort)" << std::endl;
        abort();
    }

    //send skin
    rawServerListForC211[rawServerListForC211Size]=CommonDatapack::commonDatapack.get_skins().size();
    rawServerListForC211Size+=1;
    unsigned int skinId=0;
    while(skinId<CommonDatapack::commonDatapack.get_skins().size())
    {
        {const uint16_t _tmp_le=(htole16(BaseServerMasterLoadDictionary::dictionary_skin_internal_to_database.at(skinId)));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

        rawServerListForC211Size+=2;
        skinId++;
    }

    //profile list size
    rawServerListForC211[rawServerListForC211Size]=CommonDatapack::commonDatapack.get_profileList().size();
    rawServerListForC211Size+=1;
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.get_profileList().size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.get_profileList().at(index);
        {
            //database id
            {const uint16_t _tmp_le=(htole16(dictionary_starter_internal_to_database.at(index)));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

            rawServerListForC211Size+=sizeof(uint16_t);
            //skin
            rawServerListForC211[rawServerListForC211Size]=profile.forcedskin.size();
            rawServerListForC211Size+=1;
            {
                unsigned int skinListIndex=0;
                while(skinListIndex<profile.forcedskin.size())
                {
                    rawServerListForC211[rawServerListForC211Size]=profile.forcedskin.at(skinListIndex);
                    rawServerListForC211Size+=1;
                    skinListIndex++;
                }
            }
            //cash
            /* crash for unaligned 64Bits on ARM + grsec
            *reinterpret_cast<uint64_t *>(rawServerListForC211.data()+rawServerListForC211Size)=htole64(profile.cash); */
            const uint64_t cashlittleendian=htole64(profile.cash);
            memcpy(rawServerListForC211.data()+rawServerListForC211Size,&cashlittleendian,8);
            rawServerListForC211Size+=sizeof(uint64_t);

            //monster
            {
                rawServerListForC211[rawServerListForC211Size]=profile.monstergroup.size();
                rawServerListForC211Size+=1;
                unsigned int monsterGroupListIndex=0;
                while(monsterGroupListIndex<profile.monstergroup.size())
                {
                    const std::vector<Profile::Monster> &monsters=profile.monstergroup.at(monsterGroupListIndex);
                    rawServerListForC211[rawServerListForC211Size]=monsters.size();
                    rawServerListForC211Size+=1;
                    unsigned int monsterListIndex=0;
                    while(monsterListIndex<monsters.size())
                    {
                        const Profile::Monster &playerMonster=monsters.at(monsterListIndex);

                        //monster
                        {const uint16_t _tmp_le=(htole16(playerMonster.id));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

                        rawServerListForC211Size+=sizeof(uint16_t);
                        //level
                        rawServerListForC211[rawServerListForC211Size]=playerMonster.level;
                        rawServerListForC211Size+=1;
                        //captured with
                        {const uint16_t _tmp_le=(htole16(playerMonster.captured_with));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

                        rawServerListForC211Size+=sizeof(uint16_t);

                        if(!CommonDatapack::commonDatapack.has_monster(playerMonster.id))
                        {
                            std::cerr << "For profile the monster " << playerMonster.id << " is not found (abort)" << std::endl;
                            abort();
                        }
                        const Monster &monster=CommonDatapack::commonDatapack.get_monster(playerMonster.id);
                        const Monster::Stat &monsterStat=CommonFightEngineBase::getStat(monster,playerMonster.level);
                        const std::vector<CatchChallenger::PlayerMonster::PlayerSkill> &skills=CommonFightEngineBase::generateWildSkill(monster,playerMonster.level);

                        //hp
                        {const uint32_t _tmp_le=(htole32(monsterStat.hp));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

                        rawServerListForC211Size+=sizeof(uint32_t);
                        //gender
                        rawServerListForC211[rawServerListForC211Size]=(int8_t)monster.ratio_gender;
                        rawServerListForC211Size+=sizeof(uint8_t);

                        //skill list
                        rawServerListForC211[rawServerListForC211Size]=skills.size();
                        rawServerListForC211Size+=1;
                        unsigned int skillListIndex=0;
                        while(skillListIndex<skills.size())
                        {
                            const CatchChallenger::PlayerMonster::PlayerSkill &skill=skills.at(skillListIndex);
                            if(!CommonDatapack::commonDatapack.has_monsterSkill(skill.skill))
                            {
                                std::cerr << "For profile the monster skill " << skill.skill << " is not found (abort)" << std::endl;
                                abort();
                            }
                            //skill
                            {const uint16_t _tmp_le=(htole16(skill.skill));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

                            rawServerListForC211Size+=sizeof(uint16_t);
                            //skill level
                            rawServerListForC211[rawServerListForC211Size]=skill.level;
                            rawServerListForC211Size+=sizeof(uint8_t);
                            //skill endurance
                            rawServerListForC211[rawServerListForC211Size]=skill.endurance;
                            rawServerListForC211Size+=sizeof(uint8_t);
                            skillListIndex++;
                        }

                        monsterListIndex++;
                    }

                    monsterGroupListIndex++;
                }
            }

            {
                //reputation
                rawServerListForC211[rawServerListForC211Size]=profile.reputations.size();
                rawServerListForC211Size+=sizeof(uint8_t);
                unsigned int reputationIndex=0;
                while(reputationIndex<profile.reputations.size())
                {
                    const Profile::Reputation &reputation=profile.reputations.at(reputationIndex);
                    if(reputation.internalIndex>=CommonDatapack::commonDatapack.get_reputation().size())
                    {
                        std::cerr << "For profile the reputation " << reputation.internalIndex << " is not found (abort)" << std::endl;
                        abort();
                    }
                    //type
                    {const uint16_t _tmp_le=(htole16(CommonDatapack::commonDatapack.get_reputation().at(reputation.internalIndex).reverse_database_id));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

                    rawServerListForC211Size+=sizeof(uint16_t);
                    //level
                    rawServerListForC211[rawServerListForC211Size]=reputation.level;
                    rawServerListForC211Size+=sizeof(uint8_t);
                    //point
                    {const uint32_t _tmp_le=(htole32(reputation.point));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

                    rawServerListForC211Size+=sizeof(uint32_t);
                    reputationIndex++;
                }
            }

            {
                //item
                rawServerListForC211[rawServerListForC211Size]=profile.items.size();
                rawServerListForC211Size+=sizeof(uint8_t);
                unsigned int reputationIndex=0;
                while(reputationIndex<profile.items.size())
                {
                    const Profile::Item &item=profile.items.at(reputationIndex);
                    /* no item list loaded to check here
                    if(CommonDatapack::commonDatapack.items.find(item.id)==CommonDatapack::commonDatapack.items.cend())
                    {
                        std::cerr << "For profile the item " << item.id << " is not found (abort)" << std::endl;
                        abort();
                    }*/
                    //item id
                    {const uint16_t _tmp_le=(htole16(item.id));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

                    rawServerListForC211Size+=sizeof(uint16_t);
                    //quantity
                    {const uint32_t _tmp_le=(htole32(item.quantity));memcpy(rawServerListForC211.data()+rawServerListForC211Size,&_tmp_le,sizeof(_tmp_le));}

                    rawServerListForC211Size+=sizeof(uint32_t);
                    reputationIndex++;
                }
            }
        }
        index++;
    }

    memcpy(rawServerListForC211.data()+rawServerListForC211Size,CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
    rawServerListForC211Size+=CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size();
    //CommonSettingsCommon::commonSettingsCommon.datapackHashBase.clear();no memory gain

    //send the network message
    EventLoopClientLoginMaster::loginSettingsAndCharactersGroup[0x00]=0x46;
    {const uint32_t _tmp_le=(htole32(rawServerListForC211Size));memcpy(EventLoopClientLoginMaster::loginSettingsAndCharactersGroup+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size
    memcpy(EventLoopClientLoginMaster::loginSettingsAndCharactersGroup+1+4,rawServerListForC211.data(),rawServerListForC211Size);
    EventLoopClientLoginMaster::loginSettingsAndCharactersGroupSize=1+4+rawServerListForC211Size;

    //release the 256 KB scratch buffer now it has been copied into the packet
    //(swap-with-empty actually frees capacity; clear() alone would keep it).
    std::vector<char>().swap(rawServerListForC211);
    rawServerListForC211Size=0;

    if(EventLoopClientLoginMaster::loginSettingsAndCharactersGroupSize==0)
    {
        std::cerr << "EventLoopClientLoginMaster::serverLogicalGroupListSize==0 (abort)" << std::endl;
        abort();
    }

    doTheReplyCache();
}

void EventLoopServerLoginMaster::loadTheDictionary()
{
    uint32_t posOutput=0;

    //Max player monsters
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
        posOutput+=1;
    }
    //Max warehouse player monsters
    {
        {const uint16_t _tmp_le=(htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        posOutput+=2;
    }
    //send reputation
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonDatapack::commonDatapack.get_reputation().size();
    posOutput+=1;
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.get_reputation().size())
    {
        const Reputation &reputation=CommonDatapack::commonDatapack.get_reputation().at(index);
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=
                htole16(reputation.reverse_database_id);
        posOutput+=2;
        index++;
    }
    //send skin
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonDatapack::commonDatapack.get_skins().size();
    posOutput+=1;
    index=0;
    while(index<CommonDatapack::commonDatapack.get_skins().size())
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=
                htole16(BaseServerMasterLoadDictionary::dictionary_skin_internal_to_database.at(index));
        posOutput+=2;
        index++;
    }
    //send starter
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonDatapack::commonDatapack.get_profileList().size();
    posOutput+=1;
    index=0;
    while(index<CommonDatapack::commonDatapack.get_profileList().size())
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=
                htole16(BaseServerMasterLoadDictionary::dictionary_starter_internal_to_database.at(index));
        posOutput+=2;
        index++;
    }

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
    posOutput+=CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size();
    std::cout << "datapackHashBase is " << binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase) << std::endl;

    //resize handles both the (re)allocation and freeing any previous cache.
    EventLoopServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver.resize(posOutput+1);
    EventLoopServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize=posOutput;
    memcpy(EventLoopServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver.data(),ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}
