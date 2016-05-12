#include "EpollServerLoginMaster.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/cpp11addition.h"
#include "../VariableServer.h"
#include "../../general/fight/CommonFightEngineBase.h"

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

#include "EpollClientLoginMaster.h"

EpollServerLoginMaster *EpollServerLoginMaster::epollServerLoginMaster=NULL;
char * EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver=NULL;
int EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize=0;

EpollServerLoginMaster::EpollServerLoginMaster() :
    purgeTheLockedAccount(NULL),
    checkTimeoutGameServer(NULL),
    automaticPingSend(NULL),
    server_ip(NULL),
    server_port(NULL),
    rawServerListForC211(static_cast<char *>(malloc(sizeof(EpollClientLoginMaster::loginSettingsAndCharactersGroup)))),
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
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems               = 30;
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems      = 150;
    {
        //empty buffer
        memset(EpollClientLoginMaster::serverServerList,0x00,sizeof(EpollClientLoginMaster::serverServerList));
        memset(EpollClientLoginMaster::serverLogicalGroupList,0x00,sizeof(EpollClientLoginMaster::serverLogicalGroupList));
        memset(rawServerListForC211,0x00,sizeof(EpollClientLoginMaster::loginSettingsAndCharactersGroup));
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

    EpollClientLoginMaster::fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(EpollClientLoginMaster::fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
}

EpollServerLoginMaster::~EpollServerLoginMaster()
{
    fclose(EpollClientLoginMaster::fpRandomFile);

    memset(EpollClientLoginMaster::private_token,0x00,sizeof(EpollClientLoginMaster::private_token));
    if(server_ip!=NULL)
    {
        delete server_ip;
        server_ip=NULL;
    }
    if(server_port!=NULL)
    {
        delete server_port;
        server_port=NULL;
    }
    if(databaseBaseLogin!=NULL)
    {
        EpollPostgresql *databaseBasePg=static_cast<EpollPostgresql *>(databaseBaseLogin);
        delete databaseBasePg;
        databaseBaseLogin=NULL;
    }
    if(databaseBaseBase!=NULL)
    {
        EpollPostgresql *databaseBasePg=static_cast<EpollPostgresql *>(databaseBaseBase);
        delete databaseBasePg;
        databaseBaseBase=NULL;
    }
    if(rawServerListForC211!=NULL)
    {
        delete rawServerListForC211;
        rawServerListForC211=NULL;
        rawServerListForC211Size=0;
    }
    if(EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver!=NULL)
    {
        delete EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver;
        EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver=NULL;
        EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize=0;
    }
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

void EpollServerLoginMaster::loadLoginSettings(TinyXMLSettings &settings)
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
    memcpy(EpollClientLoginMaster::private_token,tokenbinary.data(),TOKEN_SIZE_FOR_MASTERAUTH);

    //connection
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    if(!settings.contains("compression"))
        settings.setValue("compression","zlib");
    if(settings.value("compression").toString()=="none")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::None;
    else if(settings.value("compression").toString()=="xz")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Xz;
    else if(settings.value("compression").toString()=="lz4")
            ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Lz4;
    else
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Zlib;
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
        settings.setValue("maxLockAge",10*60);
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
    if(!settings.contains("maxPlayerItems"))
        settings.setValue("maxPlayerItems",30);
    if(!settings.contains("maxWarehousePlayerItems"))
        settings.setValue("maxWarehousePlayerItems",150);
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
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems=stringtouint8(settings.value("maxPlayerItems"),&ok);
    if(CommonSettingsCommon::commonSettingsCommon.maxPlayerItems==0 || !ok)
    {
        std::cerr << "maxPlayerItems==0 (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems=stringtouint16(settings.value("maxWarehousePlayerItems"),&ok);
    if(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems==0 || !ok)
    {
        std::cerr << "maxWarehousePlayerItems==0 (abort)" << std::endl;
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

void EpollServerLoginMaster::loadDBLoginSettings(TinyXMLSettings &settings)
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
            settings.setValue("type","postgresql");
        if(!settings.contains("comment"))
            settings.setValue("comment","to do maxAccountId");
        settings.sync();

        bool ok;
        //to load the dictionary
        {
            databaseBaseLogin=new EpollPostgresql();
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
            if(type!="postgresql")
            {
                std::cerr << "only db type postgresql supported (abort)" << std::endl;
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
        EpollClientLoginMaster::maxAccountId=1;
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
            settings.setValue("type","postgresql");
        if(!settings.contains("comment"))
            settings.setValue("comment","to do maxAccountId");
        settings.sync();

        bool ok;
        //to load the dictionary
        {
            databaseBaseBase=new EpollPostgresql();
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
            if(type!="postgresql")
            {
                std::cerr << "only db type postgresql supported (abort)" << std::endl;
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

std::vector<std::string> EpollServerLoginMaster::loadCharactersGroup(TinyXMLSettings &settings)
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
                settings.setValue("type","postgresql");
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
                if(type!="postgresql")
                {
                    std::cerr << "only db type postgresql supported (abort)" << std::endl;
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

void EpollServerLoginMaster::charactersGroupListReply(std::vector<std::string> &charactersGroupList)
{
    rawServerListForC211[0x00]=CommonSettingsCommon::commonSettingsCommon.automatic_account_creation;
    *reinterpret_cast<uint32_t *>(rawServerListForC211+0x01)=htole32(CommonSettingsCommon::commonSettingsCommon.character_delete_time);
    rawServerListForC211[0x05]=CommonSettingsCommon::commonSettingsCommon.min_character;
    rawServerListForC211[0x06]=CommonSettingsCommon::commonSettingsCommon.max_character;
    rawServerListForC211[0x07]=CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
    rawServerListForC211[0x08]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
    *reinterpret_cast<uint16_t *>(rawServerListForC211+0x09)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters);
    rawServerListForC211[0x0B]=CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
    *reinterpret_cast<uint16_t *>(rawServerListForC211+0x0C)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems);
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
            newSize=FacilityLibGeneral::toUTF8WithHeader(charactersGroupName,rawServerListForC211+rawServerListForC211Size);
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

void EpollServerLoginMaster::doTheLogicalGroup(TinyXMLSettings &settings)
{
    //do the LogicalGroup
    memset(EpollClientLoginMaster::serverLogicalGroupList,0x00,sizeof(EpollClientLoginMaster::serverLogicalGroupList));
    EpollClientLoginMaster::serverLogicalGroupList[0x00]=0x44;
    EpollClientLoginMaster::serverLogicalGroupListSize=1+4+0x01/*logicalGroupSize*/;

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

                EpollClientLoginMaster::logicalGroupHash[textToConvert]=logicalGroup;

                if(textToConvert.size()>255)
                {
                    std::cerr << "logical group converted too big (abort)" << std::endl;
                    abort();
                }
                EpollClientLoginMaster::serverLogicalGroupList[EpollClientLoginMaster::serverLogicalGroupListSize]=textToConvert.size();
                EpollClientLoginMaster::serverLogicalGroupListSize+=1;
                memcpy(EpollClientLoginMaster::serverLogicalGroupList+EpollClientLoginMaster::serverLogicalGroupListSize,textToConvert.data(),textToConvert.size());
                EpollClientLoginMaster::serverLogicalGroupListSize+=textToConvert.size();
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
                    int newSize=FacilityLibGeneral::toUTF8With16BitsHeader(textToConvert,EpollClientLoginMaster::serverLogicalGroupList+EpollClientLoginMaster::serverLogicalGroupListSize);
                    if(newSize==0)
                    {
                        std::cerr << "translation null or unable to translate in utf8 (abort)" << std::endl;
                        abort();
                    }
                    EpollClientLoginMaster::serverLogicalGroupListSize+=newSize;
                }
                else
                {
                    *reinterpret_cast<uint16_t *>(EpollClientLoginMaster::serverLogicalGroupList+EpollClientLoginMaster::serverLogicalGroupListSize)=0;//not convert to le16... 0 remain 0
                    EpollClientLoginMaster::serverLogicalGroupListSize+=2;
                }
            }

            logicalGroup++;
        }
        settings.endGroup();
    }
    *reinterpret_cast<uint32_t *>(EpollClientLoginMaster::serverLogicalGroupList+1)=htole32(EpollClientLoginMaster::serverLogicalGroupListSize-1-4);//set the dynamic size
    EpollClientLoginMaster::serverLogicalGroupList[1+4]=logicalGroup;

    if(EpollClientLoginMaster::serverLogicalGroupListSize==0)
    {
        std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort)" << std::endl;
        abort();
    }
}

void EpollServerLoginMaster::doTheServerList()
{
    //do the server list
    EpollClientLoginMaster::serverPartialServerListSize=0;
    //memset(EpollClientLoginMaster::serverPartialServerList,0x00,sizeof(EpollClientLoginMaster::serverPartialServerList));//improve the performance
    unsigned int pos=0x00;

    const int &serverListSize=EpollClientLoginMaster::gameServers.size();
    EpollClientLoginMaster::serverPartialServerList[pos]=serverListSize;
    pos+=1;
    int serverListIndex=0;
    while(serverListIndex<serverListSize)
    {
        const EpollClientLoginMaster * const gameServerOnEpollClientLoginMaster=EpollClientLoginMaster::gameServers.at(serverListIndex);
        const CharactersGroup::InternalGameServer * const gameServerOnCharactersGroup=gameServerOnEpollClientLoginMaster->charactersGroupForGameServerInformation;
        if(gameServerOnCharactersGroup==NULL)
        {
            std::cerr << "charactersGroup==NULL (abort)" << std::endl;
            abort();
        }

        //charactersGroup
        {
            EpollClientLoginMaster::serverPartialServerList[pos]=gameServerOnEpollClientLoginMaster->charactersGroupForGameServer->index;
            pos+=1;
        }
        //key
        {
            *reinterpret_cast<uint32_t *>(EpollClientLoginMaster::serverPartialServerList+pos)=htole32(gameServerOnCharactersGroup->uniqueKey);
            pos+=sizeof(gameServerOnCharactersGroup->uniqueKey);
        }
        //host
        {
            int newSize=FacilityLibGeneral::toUTF8WithHeader(gameServerOnCharactersGroup->host,EpollClientLoginMaster::serverPartialServerList+pos);
            if(newSize==0)
            {
                std::cerr << "host null or unable to translate in utf8 (abort)" << std::endl;
                abort();
            }
            pos+=newSize;
        }
        //port
        {
            *reinterpret_cast<unsigned short int *>(EpollClientLoginMaster::serverPartialServerList+pos)=(unsigned short int)htole16((unsigned short int)gameServerOnCharactersGroup->port);
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
                    *reinterpret_cast<uint16_t *>(EpollClientLoginMaster::serverPartialServerList+pos)=0;
                    pos+=2;
                }
                else
                {
                    *reinterpret_cast<uint16_t *>(EpollClientLoginMaster::serverPartialServerList+pos)=htole16(gameServerOnCharactersGroup->metaData.size());
                    pos+=2;
                    memcpy(EpollClientLoginMaster::serverPartialServerList+pos,gameServerOnCharactersGroup->metaData.data(),gameServerOnCharactersGroup->metaData.size());
                    pos+=gameServerOnCharactersGroup->metaData.size();
                }
            }
        }
        //logicalGroup
        {
            EpollClientLoginMaster::serverPartialServerList[pos]=gameServerOnCharactersGroup->logicalGroupIndex;
            pos+=1;
        }
        //max player
        *reinterpret_cast<unsigned short int *>(EpollClientLoginMaster::serverPartialServerList+pos)=(unsigned short int)htole16(gameServerOnCharactersGroup->maxPlayer);
        pos+=sizeof(unsigned short int);

        serverListIndex++;
    }
    EpollClientLoginMaster::serverPartialServerListSize=pos;

    //Second list part with same size
    serverListIndex=0;
    while(serverListIndex<serverListSize)
    {
        const EpollClientLoginMaster * const gameServerOnEpollClientLoginMaster=EpollClientLoginMaster::gameServers.at(serverListIndex);
        const CharactersGroup::InternalGameServer * const gameServerOnCharactersGroup=gameServerOnEpollClientLoginMaster->charactersGroupForGameServerInformation;
        //connected player
        *reinterpret_cast<unsigned short int *>(EpollClientLoginMaster::serverPartialServerList+pos)=(unsigned short int)htole16(gameServerOnCharactersGroup->currentPlayer);
        pos+=sizeof(unsigned short int);

        serverListIndex++;
    }

    //send the network message
    EpollClientLoginMaster::serverServerList[0x00]=0x45;
    *reinterpret_cast<uint32_t *>(EpollClientLoginMaster::serverServerList+1)=htole32(pos);//set the dynamic size
    memcpy(EpollClientLoginMaster::serverServerList+1+4,EpollClientLoginMaster::serverPartialServerList,pos);
    EpollClientLoginMaster::serverServerListSize=pos+1+4;
    if(EpollClientLoginMaster::serverServerListSize==0)
    {
        std::cerr << "EpollClientLoginMaster::serverServerListSize==0 (abort)" << std::endl;
        abort();
    }
}

void EpollServerLoginMaster::doTheReplyCache()
{
    if(EpollClientLoginMaster::loginSettingsAndCharactersGroupSize==0)
    {
        std::cerr << "loginSettingsAndCharactersGroupSize==0 (abort)" << std::endl;
        abort();
    }
    if(EpollClientLoginMaster::serverLogicalGroupListSize==0)
    {
        std::cerr << "loginSettingsAndCharactersGroupSize==0 (abort)" << std::endl;
        abort();
    }
    if(EpollClientLoginMaster::serverServerListSize==0)
    {
        std::cerr << "loginSettingsAndCharactersGroupSize==0 (abort)" << std::endl;
        abort();
    }
    //do the reply cache
    EpollClientLoginMaster::loginPreviousToReplyCacheSize=0;
    memcpy(EpollClientLoginMaster::loginPreviousToReplyCache+EpollClientLoginMaster::loginPreviousToReplyCacheSize,
           EpollClientLoginMaster::loginSettingsAndCharactersGroup,
           EpollClientLoginMaster::loginSettingsAndCharactersGroupSize);
    EpollClientLoginMaster::loginPreviousToReplyCacheSize+=EpollClientLoginMaster::loginSettingsAndCharactersGroupSize;

    memcpy(EpollClientLoginMaster::loginPreviousToReplyCache+EpollClientLoginMaster::loginPreviousToReplyCacheSize,
           EpollClientLoginMaster::serverLogicalGroupList,
           EpollClientLoginMaster::serverLogicalGroupListSize);
    EpollClientLoginMaster::loginPreviousToReplyCacheSize+=EpollClientLoginMaster::serverLogicalGroupListSize;

    memcpy(EpollClientLoginMaster::loginPreviousToReplyCache+EpollClientLoginMaster::loginPreviousToReplyCacheSize,
           EpollClientLoginMaster::serverServerList,
           EpollClientLoginMaster::serverServerListSize);
    EpollClientLoginMaster::loginPreviousToReplyCacheSize+=EpollClientLoginMaster::serverServerListSize;
}

bool EpollServerLoginMaster::tryListen()
{
    #ifdef SERVERSSL
        const bool &returnedValue=trySslListen(server_ip, server_port,"server.crt", "server.key");
    #else
        const bool &returnedValue=tryListenInternal(server_ip, server_port);
    #endif
    if(server_ip!=NULL)
    {
        if(returnedValue)
            std::cout << "Listen on " << server_ip << ":" << server_port << std::endl;
        delete server_ip;
        server_ip=NULL;
    }
    else
    {
        if(returnedValue)
            std::cout << "Listen on *:" << server_port << std::endl;
    }
    if(server_port!=NULL)
    {
        delete server_port;
        server_port=NULL;
    }
    return returnedValue;
}

void EpollServerLoginMaster::generateToken(TinyXMLSettings &settings)
{
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(EpollClientLoginMaster::private_token,1,TOKEN_SIZE_FOR_MASTERAUTH,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_MASTERAUTH)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_MASTERAUTH << " needed to do the token from " << RANDOMFILEDEVICE << std::endl;
        abort();
    }
    settings.setValue("token",binarytoHexa(EpollClientLoginMaster::private_token,TOKEN_SIZE_FOR_MASTERAUTH).c_str());
    fclose(fpRandomFile);
}

void EpollServerLoginMaster::load_account_max_id()
{
    std::string queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `account` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM account ORDER BY id DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM account ORDER BY id DESC LIMIT 1;";
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText,this,&EpollServerLoginMaster::load_account_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseLogin->errorMessage() << std::endl;
        abort();
    }
    EpollClientLoginMaster::maxAccountId=0;
}

void EpollServerLoginMaster::load_account_max_id_static(void *object)
{
    static_cast<EpollServerLoginMaster *>(object)->load_account_max_id_return();
}

void EpollServerLoginMaster::load_account_max_id_return()
{
    while(databaseBaseLogin->next())
    {
        bool ok;
        //not +1 because incremented before use
        EpollClientLoginMaster::maxAccountId=stringtouint32(databaseBaseLogin->value(0),&ok);
        if(!ok)
        {
            std::cerr << "Max account id is failed to convert to number" << std::endl;
            abort();
        }
    }
    CharactersGroup::serverWaitedToBeReady--;
    //will jump to SQL_common_load_finish()
}

void EpollServerLoginMaster::loadTheDatapack()
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

void EpollServerLoginMaster::SQL_common_load_finish()
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

void EpollServerLoginMaster::loadTheProfile()
{
    if(rawServerListForC211==NULL)
    {
        std::cerr << "EpollServerLoginMaster::loadTheProfile(): rawServerListForC20011==NULL (abort)" << std::endl;
        abort();
    }

    //send skin
    rawServerListForC211[rawServerListForC211Size]=CommonDatapack::commonDatapack.skins.size();
    rawServerListForC211Size+=1;
    unsigned int skinId=0;
    while(skinId<CommonDatapack::commonDatapack.skins.size())
    {
        *reinterpret_cast<uint16_t *>(rawServerListForC211+rawServerListForC211Size)=htole16(BaseServerMasterLoadDictionary::dictionary_skin_internal_to_database.at(skinId));
        rawServerListForC211Size+=2;
        skinId++;
    }

    //profile list size
    rawServerListForC211[rawServerListForC211Size]=CommonDatapack::commonDatapack.profileList.size();
    rawServerListForC211Size+=1;
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
        {
            //database id
            *reinterpret_cast<uint16_t *>(rawServerListForC211+rawServerListForC211Size)=htole16(dictionary_starter_internal_to_database.at(index));
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
            *reinterpret_cast<uint64_t *>(rawServerListForC211+rawServerListForC211Size)=htole64(profile.cash); */
            memcpy(rawServerListForC211+rawServerListForC211Size,&htole64(profile.cash),8);
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
                        *reinterpret_cast<uint16_t *>(rawServerListForC211+rawServerListForC211Size)=htole16(playerMonster.id);
                        rawServerListForC211Size+=sizeof(uint16_t);
                        //level
                        rawServerListForC211[rawServerListForC211Size]=playerMonster.level;
                        rawServerListForC211Size+=1;
                        //captured with
                        *reinterpret_cast<uint16_t *>(rawServerListForC211+rawServerListForC211Size)=htole16(playerMonster.captured_with);
                        rawServerListForC211Size+=sizeof(uint16_t);

                        if(CommonDatapack::commonDatapack.monsters.find(playerMonster.id)==CommonDatapack::commonDatapack.monsters.cend())
                        {
                            std::cerr << "For profile the monster " << playerMonster.id << " is not found (abort)" << std::endl;
                            abort();
                        }
                        const Monster &monster=CommonDatapack::commonDatapack.monsters.at(playerMonster.id);
                        const Monster::Stat &monsterStat=CommonFightEngineBase::getStat(monster,playerMonster.level);
                        const std::vector<CatchChallenger::PlayerMonster::PlayerSkill> &skills=CommonFightEngineBase::generateWildSkill(monster,playerMonster.level);

                        //hp
                        *reinterpret_cast<uint32_t *>(rawServerListForC211+rawServerListForC211Size)=htole32(monsterStat.hp);
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
                            if(CommonDatapack::commonDatapack.monsterSkills.find(skill.skill)==CommonDatapack::commonDatapack.monsterSkills.cend())
                            {
                                std::cerr << "For profile the monster skill " << skill.skill << " is not found (abort)" << std::endl;
                                abort();
                            }
                            //skill
                            *reinterpret_cast<uint16_t *>(rawServerListForC211+rawServerListForC211Size)=htole16(skill.skill);
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
                    if(reputation.reputationDatabaseId>=CommonDatapack::commonDatapack.reputation.size())
                    {
                        std::cerr << "For profile the reputation " << reputation.reputationDatabaseId << " is not found (abort)" << std::endl;
                        abort();
                    }
                    //type
                    *reinterpret_cast<uint16_t *>(rawServerListForC211+rawServerListForC211Size)=htole16(CommonDatapack::commonDatapack.reputation.at(reputation.reputationDatabaseId).reverse_database_id);
                    rawServerListForC211Size+=sizeof(uint16_t);
                    //level
                    rawServerListForC211[rawServerListForC211Size]=reputation.level;
                    rawServerListForC211Size+=sizeof(uint8_t);
                    //point
                    *reinterpret_cast<uint32_t *>(rawServerListForC211+rawServerListForC211Size)=htole32(reputation.point);
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
                    *reinterpret_cast<uint16_t *>(rawServerListForC211+rawServerListForC211Size)=htole16(item.id);
                    rawServerListForC211Size+=sizeof(uint16_t);
                    //quantity
                    *reinterpret_cast<uint32_t *>(rawServerListForC211+rawServerListForC211Size)=htole32(item.quantity);
                    rawServerListForC211Size+=sizeof(uint32_t);
                    reputationIndex++;
                }
            }
        }
        index++;
    }

    memcpy(rawServerListForC211+rawServerListForC211Size,CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
    rawServerListForC211Size+=CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size();
    //CommonSettingsCommon::commonSettingsCommon.datapackHashBase.clear();no memory gain

    //send the network message
    EpollClientLoginMaster::loginSettingsAndCharactersGroup[0x00]=0x46;
    *reinterpret_cast<uint32_t *>(EpollClientLoginMaster::loginSettingsAndCharactersGroup+1)=htole32(rawServerListForC211Size);//set the dynamic size
    memcpy(EpollClientLoginMaster::loginSettingsAndCharactersGroup+1+4,rawServerListForC211,rawServerListForC211Size);
    EpollClientLoginMaster::loginSettingsAndCharactersGroupSize=1+4+rawServerListForC211Size;

    delete rawServerListForC211;
    rawServerListForC211=NULL;
    rawServerListForC211Size=0;

    if(EpollClientLoginMaster::loginSettingsAndCharactersGroupSize==0)
    {
        std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort)" << std::endl;
        abort();
    }

    doTheReplyCache();
}

void EpollServerLoginMaster::loadTheDictionary()
{
    uint32_t posOutput=0;

    //Max player monsters
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
        posOutput+=1;
    }
    //Max warehouse player monsters
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters);
        posOutput+=2;
    }
    //Max player items
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
        posOutput+=1;
    }
    //Max warehouse player monsters
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems);
        posOutput+=2;
    }
    //send reputation
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonDatapack::commonDatapack.reputation.size();
    posOutput+=1;
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.reputation.size())
    {
        const Reputation &reputation=CommonDatapack::commonDatapack.reputation.at(index);
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=
                htole16(reputation.reverse_database_id);
        posOutput+=2;
        index++;
    }
    //send skin
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonDatapack::commonDatapack.skins.size();
    posOutput+=1;
    index=0;
    while(index<CommonDatapack::commonDatapack.skins.size())
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=
                htole16(BaseServerMasterLoadDictionary::dictionary_skin_internal_to_database.at(index));
        posOutput+=2;
        index++;
    }
    //send starter
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonDatapack::commonDatapack.profileList.size();
    posOutput+=1;
    index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=
                htole16(BaseServerMasterLoadDictionary::dictionary_starter_internal_to_database.at(index));
        posOutput+=2;
        index++;
    }

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
    posOutput+=CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size();

    if(EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver!=NULL)
        delete EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver;
    EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver=(char *)malloc(posOutput+1);
    EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize=posOutput;
    memcpy(EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver,ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}
