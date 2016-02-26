#include "EpollServerLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../epoll/Epoll.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../base/PreparedDBQuery.h"

using namespace CatchChallenger;

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <iostream>
#include <vector>
#include <algorithm>

#include "EpollClientLoginSlave.h"
#include "../base/DictionaryLogin.h"
#include "../../general/base/ProtocolParsing.h"
#include "../epoll/EpollSocket.h"

EpollServerLoginSlave *EpollServerLoginSlave::epollServerLoginSlave=NULL;

EpollServerLoginSlave::EpollServerLoginSlave() :
    tcpNodelay(false),
    tcpCork(false),
    serverReady(false)
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

    TinyXMLSettings settings(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/login.xml");

    {
        memset(LinkToMaster::private_token_master,0x00,sizeof(LinkToMaster::private_token_master));
        memset(LinkToMaster::private_token_statclient,0x00,sizeof(LinkToMaster::private_token_statclient));
        memset(EpollClientLoginSlave::serverServerList,0x00,sizeof(EpollClientLoginSlave::serverServerList));
        memset(EpollClientLoginSlave::serverLogicalGroupList,0x00,sizeof(EpollClientLoginSlave::serverLogicalGroupList));
        memset(EpollClientLoginSlave::serverLogicalGroupAndServerList,0x00,sizeof(EpollClientLoginSlave::serverLogicalGroupAndServerList));
    }
    {
        static const unsigned char tocopy[]=PROTOCOL_HEADER_MASTERSERVER;
        memcpy(LinkToMaster::header_magic_number+2,tocopy,9);
    }
    {
        static const unsigned char tocopy[]=PROTOCOL_HEADER_GAMESERVER;
        memcpy(LinkToGameServer::protocolHeaderToMatchGameServer+2,tocopy,5);
    }

    srand(time(NULL));

    if(!settings.contains("ip"))
        settings.setValue("ip","");
    server_ip=settings.value("ip");
    if(!settings.contains("port"))
        settings.setValue("port",rand()%40000+10000);
    server_port=settings.value("port");

    //token
    settings.beginGroup("master");
    {
        if(!settings.contains("token"))
            generateToken(settings);
        std::string token=settings.value("token");
        if(token.size()!=TOKEN_SIZE_FOR_MASTERAUTH*2/*String Hexa, not binary*/)
            generateToken(settings);
        token=settings.value("token");
        const std::vector<char> &tokenBinary=hexatoBinary(token);
        if(tokenBinary.empty())
        {
            std::cerr << "convertion to binary for pass failed for: " << token << std::endl;
            abort();
        }
        memcpy(LinkToMaster::private_token_master,tokenBinary.data(),TOKEN_SIZE_FOR_MASTERAUTH);
    }
    settings.endGroup();

    settings.beginGroup("statclient");
    {
        if(!settings.contains("token"))
            generateTokenStatClient(settings);
        std::string token=settings.value("token");
        if(token.size()!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT*2/*String Hexa, not binary*/)
            generateTokenStatClient(settings);
        token=settings.value("token");
        const std::vector<char> &tokenBinary=hexatoBinary(token);
        if(tokenBinary.empty())
        {
            std::cerr << "convertion to binary for pass failed for: " << token << std::endl;
            abort();
        }
        memcpy(LinkToMaster::private_token_statclient,tokenBinary.data(),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
    }
    settings.endGroup();

    //mode
    {
        if(!settings.contains("mode"))
            settings.setValue("mode","direct");//or proxy
        std::string mode=settings.value("mode");
        if(mode!="direct" && mode!="proxy")
        {
            mode="direct";
            settings.setValue("mode",mode.c_str());
        }
        if(mode=="direct")
        {
            EpollClientLoginSlave::proxyMode=EpollClientLoginSlave::ProxyMode::Reconnect;
            EpollClientLoginSlave::serverServerList[0x00]=0x01;//Reconnect mode
        }
        else
        {
            EpollClientLoginSlave::proxyMode=EpollClientLoginSlave::ProxyMode::Proxy;
            EpollClientLoginSlave::serverServerList[0x00]=0x02;//proxy mode
        }
    }

    if(!settings.contains("httpDatapackMirror"))
        settings.setValue("httpDatapackMirror","http://localhost/datapack/");
    std::string httpDatapackMirror=settings.value("httpDatapackMirror");
    if(httpDatapackMirror.empty())
    {
        settings.sync();
        std::cerr << "empty mirror in the settings but not supported from now (abort)" << std::endl;
        abort();

        #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
        qDebug() << "Need mirror because CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION is def, need decompression to datapack list input";
        return EXIT_FAILURE;
        #endif
    }
    else
    {
        std::vector<std::string> newMirrorList;
        std::regex httpMatch("^https?://.+$");
        const std::vector<std::string> &mirrorList=stringsplit(httpDatapackMirror,';');
        unsigned int index=0;
        while(index<mirrorList.size())
        {
            const std::string &mirror=mirrorList.at(index);
            if(!regex_search(mirror,httpMatch))
            {
                std::cerr << "Mirror wrong: " << mirror << std::endl;
                abort();
            }
            if(stringEndsWith(mirror,"/"))
                newMirrorList.push_back(mirror);
            else
                newMirrorList.push_back(mirror+"/");
            index++;
        }
        httpDatapackMirror=stringimplode(newMirrorList,';');
    }

    //connection
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    if(!settings.contains("compression"))
        settings.setValue("compression","zlib");
    if(settings.value("compression")=="none")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::None;
    else if(settings.value("compression")=="xz")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Xz;
    else if(settings.value("compression")=="lz4")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Lz4;
    else
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Zlib;
    ProtocolParsing::compressionLevel          = stringtouint8(settings.value("compressionLevel"));
    #endif

    settings.beginGroup("Linux");
    if(!settings.contains("tcpCork"))
        settings.setValue("tcpCork",true);
    if(!settings.contains("tcpNodelay"))
        settings.setValue("tcpNodelay",false);
    tcpCork=stringtobool(settings.value("tcpCork"));
    tcpNodelay=stringtobool(settings.value("tcpNodelay"));
    settings.endGroup();
    settings.sync();

    std::string db;
    std::string host;
    std::string login;
    std::string pass;
    std::string type;
    bool ok;
    //here to have by login server an auth
    {
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
        settings.sync();
        EpollClientLoginSlave::databaseBaseLogin.considerDownAfterNumberOfTry=stringtouint32(settings.value("considerDownAfterNumberOfTry"),&ok);
        if(EpollClientLoginSlave::databaseBaseLogin.considerDownAfterNumberOfTry==0 || !ok)
        {
            std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
            abort();
        }
        db=settings.value("db");
        host=settings.value("host");
        login=settings.value("login");
        pass=settings.value("pass");
        EpollClientLoginSlave::databaseBaseLogin.tryInterval=stringtouint32(settings.value("tryInterval"),&ok);
        if(EpollClientLoginSlave::databaseBaseLogin.tryInterval==0 || !ok)
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
        if(!EpollClientLoginSlave::databaseBaseLogin.syncConnect(host.c_str(),db.c_str(),login.c_str(),pass.c_str()))
        {
            std::cerr << "Connect to login database failed:" << EpollClientLoginSlave::databaseBaseLogin.errorMessage() << ", host: " << host << ", db: " << db << ", login: " << login << std::endl;
            abort();
        }
        settings.endGroup();
        settings.sync();

        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        PreparedDBQueryLogin::initDatabaseQueryLogin(EpollClientLoginSlave::databaseBaseLogin.databaseType());
        #endif
        //PreparedDBQueryBase::initDatabaseQueryBase(EpollClientLoginSlave::databaseBaseLogin.databaseType());//don't exist, allow dictionary and loaded without cache
    }

    std::vector<std::string> charactersGroupForLoginList;
    bool continueCharactersGroupForLoginSettings=true;
    int CharactersGroupForLoginId=0;
    while(continueCharactersGroupForLoginSettings)
    {
        settings.beginGroup("db-common-"+std::to_string(CharactersGroupForLoginId));
        if(CharactersGroupForLoginId==0)
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
            settings.sync();
        }
        if(settings.contains("login"))
        {
            const std::string &charactersGroup=settings.value("charactersGroup");
            if(CharactersGroupForLogin::hash.find(charactersGroup)==CharactersGroupForLogin::hash.cend())
            {
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
                CharactersGroupForLogin::hash[charactersGroup]=new CharactersGroupForLogin(db.c_str(),host.c_str(),login.c_str(),pass.c_str(),considerDownAfterNumberOfTry,tryInterval);
                charactersGroupForLoginList.push_back(charactersGroup);
            }
            else
            {
                std::cerr << "CharactersGroupForLogin already found for group " << CharactersGroupForLoginId << std::endl;
                abort();
            }
            CharactersGroupForLoginId++;
        }
        else
            continueCharactersGroupForLoginSettings=false;
        settings.endGroup();
    }

    {
        std::sort(charactersGroupForLoginList.begin(),charactersGroupForLoginList.end());
        EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup[EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize]=(unsigned char)charactersGroupForLoginList.size();
        EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=sizeof(unsigned char);
        unsigned int index=0;
        while(index<charactersGroupForLoginList.size())
        {
            const std::string &CharactersGroupForLoginName=charactersGroupForLoginList.at(index);
            if(CharactersGroupForLoginName.size()>20)
            {
                std::cerr << "CharactersGroupForLoginName too big (abort)" << std::endl;
                abort();
            }
            EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup[EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize]=CharactersGroupForLoginName.size();
            EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=1;
            memcpy(EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup+EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize,CharactersGroupForLoginName.data(),CharactersGroupForLoginName.size());
            EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=CharactersGroupForLoginName.size();

            CharactersGroupForLogin::hash[CharactersGroupForLoginName]->index=index;
            CharactersGroupForLogin::list.push_back(CharactersGroupForLogin::hash.at(CharactersGroupForLoginName));

            if(index==0)
                PreparedDBQueryCommon::initDatabaseQueryCommonWithoutSP(CharactersGroupForLogin::list.back()->databaseType());

            index++;
        }
    }

    {
        settings.beginGroup("master");
        if(!settings.contains("host"))
            settings.setValue("host","localhost");
        if(!settings.contains("port"))
            settings.setValue("port",9999);
        if(!settings.contains("considerDownAfterNumberOfTry"))
            settings.setValue("considerDownAfterNumberOfTry",3);
        if(!settings.contains("tryInterval"))
            settings.setValue("tryInterval",5);
        settings.sync();
        const std::string &host=settings.value("host");
        const uint16_t &port=stringtouint16(settings.value("port"),&ok);
        if(port==0 || !ok)
        {
            std::cerr << "Master port not a number or 0:" << settings.value("port") << std::endl;
            abort();
        }
        const uint8_t &tryInterval=stringtouint8(settings.value("tryInterval"),&ok);
        if(tryInterval==0 || !ok)
        {
            std::cerr << "tryInterval==0 (abort)" << std::endl;
            abort();
        }
        const uint8_t &considerDownAfterNumberOfTry=stringtouint8(settings.value("considerDownAfterNumberOfTry"),&ok);
        if(considerDownAfterNumberOfTry==0 || !ok)
        {
            std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
            abort();
        }

        {
            const int &linkfd=LinkToMaster::tryConnect(host.c_str(),port,tryInterval,considerDownAfterNumberOfTry);
            if(linkfd<0)
            {
                std::cerr << "Unable to connect on master" << std::endl;
                abort();
            }
            #ifdef SERVERSSL
            ctx from what?
            linkToMaster::linkToMaster=new linkToMaster(linkfd,ctx);
            #else
            LinkToMaster::linkToMaster=new LinkToMaster(linkfd);
            #endif
            LinkToMaster::linkToMaster->stat=LinkToMaster::Stat::Connected;
            LinkToMaster::linkToMaster->readTheFirstSslHeader();
            LinkToMaster::linkToMaster->setConnexionSettings();
            const int &s = EpollSocket::make_non_blocking(linkfd);
            if(s == -1)
                std::cerr << "unable to make to socket non blocking" << std::endl;
        }

        LinkToMaster::linkToMaster->httpDatapackMirror=httpDatapackMirror;

        if(LinkToMaster::linkToMaster->httpDatapackMirror.empty())
        {
            std::cerr << "EpollClientLoginSlave::linkToMaster->httpDatapackMirror.isEmpty(), not coded for now (abort)" << std::endl;
            abort();
        }
        if(LinkToMaster::linkToMaster->httpDatapackMirror.size()>255)
        {
            std::cerr << "LinkToMaster::linkToMaster->httpDatapackMirror size>255 (abort)" << std::endl;
            abort();
        }

        settings.endGroup();
    }
}

EpollServerLoginSlave::~EpollServerLoginSlave()
{
    memset(LinkToMaster::private_token_master,0x00,sizeof(LinkToMaster::private_token_master));
    memset(LinkToMaster::private_token_statclient,0x00,sizeof(LinkToMaster::private_token_statclient));
    if(LinkToMaster::linkToMaster!=NULL)
    {
        delete LinkToMaster::linkToMaster;
        LinkToMaster::linkToMaster=NULL;
    }
    unsigned int index=0;
    while(index<EpollServerLoginSlave::loginProfileList.size())
    {
        EpollServerLoginSlave::LoginProfile &profile=EpollServerLoginSlave::loginProfileList[index];
        delete profile.preparedQueryChar;
        index++;
    }
}

void EpollServerLoginSlave::close()
{
    if(LinkToMaster::linkToMaster!=NULL)
    {
        delete LinkToMaster::linkToMaster;
        LinkToMaster::linkToMaster=NULL;
    }
    EpollGenericServer::close();
}

void EpollServerLoginSlave::SQL_common_load_finish()
{
    if(!LinkToMaster::linkToMaster->httpDatapackMirror.empty())
        serverReady=true;
    else
    {
        std::cerr << "!httpDatapackMirror.isEmpty() (abort)" << std::endl;
        abort();
    }
}

bool EpollServerLoginSlave::tryListen()
{
    #ifdef SERVERSSL
        const bool &returnedValue=trySslListen(server_ip, server_port,"server.crt", "server.key");
    #else
        const bool &returnedValue=tryListenInternal(server_ip.c_str(), server_port.c_str());
    #endif

    preload_the_randomData();
    preload_profile();

    if(!server_ip.empty())
        std::cout << "Listen on " << server_ip << ":" << server_port << std::endl;
    else
        std::cout << "Listen on *:" << server_port << std::endl;
    server_ip.clear();
    server_port.clear();
    return returnedValue;
}

void EpollServerLoginSlave::generateToken(TinyXMLSettings &settings)
{
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(LinkToMaster::private_token_master,1,TOKEN_SIZE_FOR_MASTERAUTH,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_MASTERAUTH)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_MASTERAUTH << " needed to do the token from " << RANDOMFILEDEVICE << std::endl;
        abort();
    }
    settings.setValue("token",binarytoHexa(reinterpret_cast<char *>(LinkToMaster::private_token_master)
                                           ,TOKEN_SIZE_FOR_MASTERAUTH).c_str());
    fclose(fpRandomFile);
    settings.sync();
}

void EpollServerLoginSlave::generateTokenStatClient(TinyXMLSettings &settings)
{
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(LinkToMaster::private_token_statclient,1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT << " needed to do the token from " << RANDOMFILEDEVICE << std::endl;
        abort();
    }
    settings.setValue("token",binarytoHexa(reinterpret_cast<char *>(LinkToMaster::private_token_statclient)
                                           ,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT).c_str());
    fclose(fpRandomFile);
    settings.sync();
}

void EpollServerLoginSlave::setSkinPair(const uint8_t &internalId,const uint16_t &databaseId)
{
    while((uint16_t)DictionaryLogin::dictionary_skin_database_to_internal.size()<(databaseId+1))
        DictionaryLogin::dictionary_skin_database_to_internal.push_back(0);
    while((uint16_t)DictionaryLogin::dictionary_skin_internal_to_database.size()<(internalId+1))
        DictionaryLogin::dictionary_skin_internal_to_database.push_back(0);
    DictionaryLogin::dictionary_skin_internal_to_database[internalId]=databaseId;
    DictionaryLogin::dictionary_skin_database_to_internal[databaseId]=internalId;
}

void EpollServerLoginSlave::setProfilePair(const uint8_t &internalId,const uint16_t &databaseId)
{
    while((uint16_t)DictionaryLogin::dictionary_starter_database_to_internal.size()<(databaseId+1))
        DictionaryLogin::dictionary_starter_database_to_internal.push_back(0);
    while((uint16_t)DictionaryLogin::dictionary_starter_internal_to_database.size()<(internalId+1))
        DictionaryLogin::dictionary_starter_internal_to_database.push_back(0);
    DictionaryLogin::dictionary_starter_internal_to_database[internalId]=databaseId;
    DictionaryLogin::dictionary_starter_database_to_internal[databaseId]=internalId;
}

void EpollServerLoginSlave::compose04Reply()
{
    EpollClientLoginSlave::loginGood[0x00]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    EpollClientLoginSlave::loginGood[0x01]=0x00;//reply id, unknow
    EpollClientLoginSlave::loginGood[0x02]=0x00;//reply size, unknow
    EpollClientLoginSlave::loginGood[0x03]=0x00;//reply size, unknow
    EpollClientLoginSlave::loginGood[0x04]=0x00;//reply size, unknow
    EpollClientLoginSlave::loginGood[0x05]=0x00;//reply size, unknow

    EpollClientLoginSlave::loginGood[0x06]=0x01;//good

    *reinterpret_cast<uint32_t *>(EpollClientLoginSlave::loginGood+0x07)=htole32(CommonSettingsCommon::commonSettingsCommon.character_delete_time);
    EpollClientLoginSlave::loginGood[0x0B]=CommonSettingsCommon::commonSettingsCommon.max_character;
    EpollClientLoginSlave::loginGood[0x0C]=CommonSettingsCommon::commonSettingsCommon.min_character;
    EpollClientLoginSlave::loginGood[0x0D]=CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
    EpollClientLoginSlave::loginGood[0x0E]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
    *reinterpret_cast<uint16_t *>(EpollClientLoginSlave::loginGood+0x0F)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters);
    EpollClientLoginSlave::loginGood[0x11]=CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
    *reinterpret_cast<uint16_t *>(EpollClientLoginSlave::loginGood+0x12)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems);
    EpollClientLoginSlave::loginGoodSize=0x14;

    memcpy(EpollClientLoginSlave::loginGood+EpollClientLoginSlave::loginGoodSize,EpollClientLoginSlave::baseDatapackSum,sizeof(EpollClientLoginSlave::baseDatapackSum));
    EpollClientLoginSlave::loginGoodSize+=sizeof(EpollClientLoginSlave::baseDatapackSum);

    if(LinkToMaster::linkToMaster->httpDatapackMirror.empty())
    {
        std::cerr << "EpollClientLoginSlave::linkToMaster->httpDatapackMirror.isEmpty(), not coded for now (abort)" << std::endl;
        abort();
    }
    if(LinkToMaster::linkToMaster->httpDatapackMirror.size()>255)
    {
        std::cerr << "LinkToMaster::linkToMaster->httpDatapackMirror size>255 (abort)" << std::endl;
        abort();
    }
    EpollClientLoginSlave::loginGood[EpollClientLoginSlave::loginGoodSize]=(uint8_t)LinkToMaster::linkToMaster->httpDatapackMirror.size();
    EpollClientLoginSlave::loginGoodSize+=1;
    memcpy(EpollClientLoginSlave::loginGood+EpollClientLoginSlave::loginGoodSize,LinkToMaster::linkToMaster->httpDatapackMirror.data(),LinkToMaster::linkToMaster->httpDatapackMirror.size());
    EpollClientLoginSlave::loginGoodSize+=LinkToMaster::linkToMaster->httpDatapackMirror.size();
}

void EpollServerLoginSlave::preload_profile()
{
    if(CharactersGroupForLogin::list.empty())
    {
        std::cerr << "EpollServerLoginSlave::preload_profile() CharactersGroupForLogin::list.isEmpty() (abort)" << std::endl;
        abort();
    }
    const DatabaseBase::DatabaseType &type=CharactersGroupForLogin::list.at(0)->databaseType();
    std::vector<std::string> tempStringList;

    unsigned int index=0;
    while(index<EpollServerLoginSlave::loginProfileList.size())
    {
        EpollServerLoginSlave::LoginProfile &profile=EpollServerLoginSlave::loginProfileList[index];
        //assume here all is the same type
        switch(type)
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                tempStringList.push_back("INSERT INTO `character`(`id`,`account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`date`,`warehouse_cash`,`clan_leader`,`time_to_delete`,`played_time`,`last_connect`,`starter`) VALUES(");
                tempStringList.push_back(",");
                tempStringList.push_back(",'");
                tempStringList.push_back("',");
                tempStringList.push_back(",0,0,"+
                        std::to_string(profile.cash)+",");
                tempStringList.push_back(",0,0,0,0,0,"+
                        std::to_string(profile.databaseId)+");");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                tempStringList.push_back("INSERT INTO character(id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,time_to_delete,played_time,last_connect,starter) VALUES(");
                tempStringList.push_back(",");
                tempStringList.push_back(",'");
                tempStringList.push_back("',");
                tempStringList.push_back(",0,0,"+
                        std::to_string(profile.cash)+",");
                tempStringList.push_back(",0,0,0,0,0,"+
                        std::to_string(index)+");");
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                tempStringList.push_back("INSERT INTO character(id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,time_to_delete,played_time,last_connect,starter) VALUES(");
                tempStringList.push_back(",");
                tempStringList.push_back(",'");
                tempStringList.push_back("',");
                tempStringList.push_back(",0,0,"+
                        std::to_string(profile.cash)+",");
                tempStringList.push_back(",0,FALSE,0,0,0,"+
                        std::to_string(index)+");");
            break;
        }
        unsigned int preparedQueryCharTempSize=0;
        //reservate the memory space
        {
            unsigned int sub_index=0;
            while(sub_index<tempStringList.size())
            {
                preparedQueryCharTempSize+=tempStringList.at(sub_index).size();
                sub_index++;
            }
            profile.preparedQueryChar=(char *)malloc(preparedQueryCharTempSize);
        }
        //set the new memory space
        {
            unsigned int sub_index=0;
            while(sub_index<tempStringList.size())
            {
                profile.preparedQuerySize[sub_index]=tempStringList.at(sub_index).size();
                if(sub_index>0)
                    profile.preparedQueryPos[sub_index]=profile.preparedQueryPos[sub_index-1]+profile.preparedQuerySize[sub_index-1];
                memcpy(profile.preparedQueryChar+profile.preparedQueryPos[sub_index],tempStringList.at(sub_index).data(),tempStringList.at(sub_index).size());
                sub_index++;
            }
        }
        tempStringList.clear();

        index++;
    }

    if(EpollServerLoginSlave::loginProfileList.size()==0 && CommonSettingsCommon::commonSettingsCommon.min_character!=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        std::cout << "no profile loaded! C211 never send? (abort)" << std::endl;
        abort();
    }
    std::cout << EpollServerLoginSlave::loginProfileList.size() << " profile loaded" << std::endl;
}
