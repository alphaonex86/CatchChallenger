#include "EventLoopServerLoginSlave.hpp"
#include <cstring>
#include "CharactersGroupForLogin.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/ProtocolVersion.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "LinkToMaster.hpp"
#include "LinkToGameServer.hpp"
#include "../base/PreparedDBQuery.hpp"
#include "../base/GlobalServerData.hpp"

using namespace CatchChallenger;

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <iostream>
#include <vector>
#include <algorithm>

#include "EventLoopClientLoginSlave.hpp"
#include "../base/DictionaryLogin.hpp"
#include "../../general/base/ProtocolParsing.hpp"

EventLoopServerLoginSlave *EventLoopServerLoginSlave::unixServerLoginSlave=NULL;

EventLoopServerLoginSlave::EventLoopServerLoginSlave() :
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

    TinyXMLSettings settings(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/login.xml");

    {
        memset(LinkToMaster::private_token_master,0x00,sizeof(LinkToMaster::private_token_master));
        memset(LinkToMaster::private_token_statclient,0x00,sizeof(LinkToMaster::private_token_statclient));
        memset(EventLoopClientLoginSlave::serverServerList,0x00,sizeof(EventLoopClientLoginSlave::serverServerList));
        memset(EventLoopClientLoginSlave::serverLogicalGroupList,0x00,sizeof(EventLoopClientLoginSlave::serverLogicalGroupList));
        memset(EventLoopClientLoginSlave::serverLogicalGroupAndServerList,0x00,sizeof(EventLoopClientLoginSlave::serverLogicalGroupAndServerList));
    }
    {
        static const unsigned char tocopy[]=PROTOCOL_HEADER_MASTERSERVER;
        memcpy(LinkToMaster::header_magic_number+2,tocopy,9);
    }
    {
        static const unsigned char tocopy[]=PROTOCOL_HEADER_GAMESERVER;
        memcpy(LinkToGameServer::protocolHeaderToMatchGameServer+2,tocopy,5);
    }

    bool ok;
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
        if(token.size()!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER*2/*String Hexa, not binary*/)
            generateTokenStatClient(settings);
        token=settings.value("token");
        const std::vector<char> &tokenBinary=hexatoBinary(token);
        if(tokenBinary.empty())
        {
            std::cerr << "convertion to binary for pass failed for: " << token << std::endl;
            abort();
        }
        memcpy(LinkToMaster::private_token_statclient,tokenBinary.data(),CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
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
            EventLoopClientLoginSlave::proxyMode=EventLoopClientLoginSlave::ProxyMode::Reconnect;
            EventLoopClientLoginSlave::serverServerList[0x00]=0x01;//Reconnect mode
        }
        else
        {
            EventLoopClientLoginSlave::proxyMode=EventLoopClientLoginSlave::ProxyMode::Proxy;
            EventLoopClientLoginSlave::serverServerList[0x00]=0x02;//proxy mode
        }
    }

    if(!settings.contains("httpDatapackMirror"))
        settings.setValue("httpDatapackMirror","http://localhost/datapack/");
    std::string httpDatapackMirror=settings.value("httpDatapackMirror");
    if(httpDatapackMirror.empty())
    {
        //Empty mirror == "no HTTP mirror; push datapack inline over
        //the gameplay protocol". The handler at
        //ClientNetworkReadQuery.cpp packetCode 0xA1 (compiled in
        //because login isn't a CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
        //binary) reads from datapack_basePath, set by BaseServer2.cpp
        //to <applicationDirPath>/datapack/, i.e. a `datapack/` dir
        //next to the login binary. Operators wanting the push path
        //must therefore stage the datapack at ./datapack/ relative
        //to catchchallenger-server-login.
        #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
        std::cerr << "Need mirror because CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION is def, need decompression to datapack list input (abort)" << std::endl;
        abort();
        #endif
        settings.sync();
        std::cout << "httpDatapackMirror is empty: clients will pull the datapack inline over the protocol from ./datapack/ next to the login binary" << std::endl;
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
    #ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
    if(!settings.contains("compression"))
        settings.setValue("compression","zstd");
    if(settings.value("compression")=="none")
        CompressionProtocol::compressionTypeServer          = CompressionProtocol::CompressionType::None;
    else if(settings.value("compression")=="zstd")
        CompressionProtocol::compressionTypeServer          = CompressionProtocol::CompressionType::Zstandard;
    else
    {
        std::cerr << "compression not supported: " << settings.value("compression") << std::endl;
        CompressionProtocol::compressionTypeServer          = CompressionProtocol::CompressionType::Zstandard;
    }
    if(!settings.contains("compressionLevel"))
        settings.setValue("compressionLevel","6");
    CompressionProtocol::compressionLevel          = stringtouint8(settings.value("compressionLevel"),&ok);
    if(!ok)
    {
        std::cerr << "Compression level not a number fixed by 6" << std::endl;
        CompressionProtocol::compressionLevel=6;
    }
    #endif

    settings.beginGroup("Linux");
    if(!settings.contains("tcpCork"))
        settings.setValue("tcpCork",false);
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
        settings.sync();
        EventLoopClientLoginSlave::databaseBaseLogin.considerDownAfterNumberOfTry=stringtouint32(settings.value("considerDownAfterNumberOfTry"),&ok);
        if(!ok)
        {
            std::cerr << "considerDownAfterNumberOfTry not number" << std::endl;
            abort();
        }
        db=settings.value("db");
        host=settings.value("host");
        login=settings.value("login");
        pass=settings.value("pass");
        EventLoopClientLoginSlave::databaseBaseLogin.tryInterval=stringtouint32(settings.value("tryInterval"),&ok);
        if(EventLoopClientLoginSlave::databaseBaseLogin.tryInterval==0 || !ok)
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
        if(!EventLoopClientLoginSlave::databaseBaseLogin.syncConnect(host.c_str(),db.c_str(),login.c_str(),pass.c_str()))
        {
            std::cerr << "Connect to login database failed:" << EventLoopClientLoginSlave::databaseBaseLogin.errorMessage() << ", host: " << host << ", db: " << db << ", login: " << login << std::endl;
            abort();
        }
        settings.endGroup();
        settings.sync();

        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        PreparedDBQueryLogin::initDatabaseQueryLogin(EventLoopClientLoginSlave::databaseBaseLogin.databaseType(),&EventLoopClientLoginSlave::databaseBaseLogin);
        #endif
        //PreparedDBQueryBase::initDatabaseQueryBase(EventLoopClientLoginSlave::databaseBaseLogin.databaseType());//don't exist, allow dictionary and loaded without cache
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
            settings.sync();
        }
        if(settings.contains("login"))
        {
            const std::string &charactersGroup=settings.value("charactersGroup");
            if(CharactersGroupForLogin::hash.find(charactersGroup)==CharactersGroupForLogin::hash.cend())
            {
                const uint8_t &considerDownAfterNumberOfTry=stringtouint8(settings.value("considerDownAfterNumberOfTry"),&ok);
                if(!ok)
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
        EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroup[EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize]=(unsigned char)charactersGroupForLoginList.size();
        EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=sizeof(unsigned char);
        unsigned int index=0;
        while(index<charactersGroupForLoginList.size())
        {
            const std::string &CharactersGroupForLoginName=charactersGroupForLoginList.at(index);
            if(CharactersGroupForLoginName.size()>20)
            {
                std::cerr << "CharactersGroupForLoginName too big (abort)" << std::endl;
                abort();
            }
            EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroup[EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize]=CharactersGroupForLoginName.size();
            EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=1;
            memcpy(EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroup+EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize,CharactersGroupForLoginName.data(),CharactersGroupForLoginName.size());
            EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=CharactersGroupForLoginName.size();

            CharactersGroupForLogin::hash[CharactersGroupForLoginName]->charactersGroupIndex=index;
            CharactersGroupForLogin::list.push_back(CharactersGroupForLogin::hash.at(CharactersGroupForLoginName));

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
        if(!ok)
        {
            std::cerr << "considerDownAfterNumberOfTry not number (abort)" << std::endl;
            abort();
        }

        {
            const int &linkfd=LinkToMaster::tryConnect(host.c_str(),port,tryInterval,considerDownAfterNumberOfTry);
            if(linkfd<0)
            {
                std::cerr << "Unable to connect on master" << std::endl;
                abort();
            }
            LinkToMaster::linkToMaster=new LinkToMaster(linkfd);
            LinkToMaster::linkToMaster->stat=LinkToMaster::Stat::Connected;
            // SSL/cleartext preamble byte was removed; jump straight
            // to the protocol-header send.
            LinkToMaster::linkToMaster->sendProtocolHeader();
            LinkToMaster::linkToMaster->setConnexionSettings();
            /*const int &s = SocketUtil::make_non_blocking(linkfd);
            if(s == -1)
                std::cerr << "unable to make to socket non blocking" << std::endl;*/
        }

        LinkToMaster::linkToMaster->httpDatapackMirror=httpDatapackMirror;

        //Empty mirror is allowed — the client receives a 0-length
        //mirror string in the loginGood packet and falls back to
        //the in-protocol datapack pull (Api_protocol_reply.cpp
        //branch where mirrorSize==0). Only the >255 bound is fatal.
        if(LinkToMaster::linkToMaster->httpDatapackMirror.size()>255)
        {
            std::cerr << "LinkToMaster::linkToMaster->httpDatapackMirror size>255 (abort)" << std::endl;
            abort();
        }

        settings.endGroup();
    }
}

EventLoopServerLoginSlave::~EventLoopServerLoginSlave()
{
    memset(LinkToMaster::private_token_master,0x00,sizeof(LinkToMaster::private_token_master));
    memset(LinkToMaster::private_token_statclient,0x00,sizeof(LinkToMaster::private_token_statclient));
    if(LinkToMaster::linkToMaster!=NULL)
    {
        delete LinkToMaster::linkToMaster;
        LinkToMaster::linkToMaster=NULL;
    }
}

void EventLoopServerLoginSlave::close()
{
    std::cerr << "EventLoopServerLoginSlave::close()" << std::endl;
    if(LinkToMaster::linkToMaster!=NULL)
    {
        delete LinkToMaster::linkToMaster;
        LinkToMaster::linkToMaster=NULL;
    }
    EventLoopGenericServer::close();
}

void EventLoopServerLoginSlave::SQL_common_load_finish()
{
    //Empty mirror means clients pull the datapack inline over the
    //protocol; the SQL common-load finishing condition is the same
    //either way, so just flip serverReady on.
    serverReady=true;
}

bool EventLoopServerLoginSlave::tryListen()
{
        const bool &returnedValue=tryListenInternal(server_ip.c_str(), server_port.c_str());

    preload_2_sync_the_randomData();
    preload_profile();

    if(server_port.empty())
    {
        std::cout << "EventLoopServerLoginSlave::tryListen() port can't be empty (abort)" << std::endl;
        abort();
    }
    if(!server_ip.empty())
        std::cout << "Listen on " << server_ip << ":" << server_port << std::endl;
    else
        std::cout << "Listen on *:" << server_port << std::endl;
    /*no, to resume it server_ip.clear();
    server_port.clear();*/
    return returnedValue;
}

void EventLoopServerLoginSlave::generateToken(TinyXMLSettings &settings)
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

void EventLoopServerLoginSlave::generateTokenStatClient(TinyXMLSettings &settings)
{
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(LinkToMaster::private_token_statclient,1,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,fpRandomFile);
    if(returnedSize!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
    {
        std::cerr << "Unable to read the " << CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER << " needed to do the token from " << RANDOMFILEDEVICE << std::endl;
        abort();
    }
    settings.setValue("token",binarytoHexa(reinterpret_cast<char *>(LinkToMaster::private_token_statclient)
                                           ,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER).c_str());
    fclose(fpRandomFile);
    settings.sync();
}

void EventLoopServerLoginSlave::setSkinPair(const uint8_t &internalId,const uint16_t &databaseId)
{
    while((uint16_t)DictionaryLogin::dictionary_skin_database_to_internal.size()<(databaseId+1))
        DictionaryLogin::dictionary_skin_database_to_internal.push_back(255);
    while((uint16_t)DictionaryLogin::dictionary_skin_internal_to_database.size()<(internalId+1))
        DictionaryLogin::dictionary_skin_internal_to_database.push_back(0);
    DictionaryLogin::dictionary_skin_internal_to_database[internalId]=databaseId;
    DictionaryLogin::dictionary_skin_database_to_internal[databaseId]=internalId;
}

void EventLoopServerLoginSlave::setProfilePair(const uint8_t &internalId,const uint16_t &databaseId)
{
    while((uint16_t)DictionaryLogin::dictionary_starter_database_to_internal.size()<(databaseId+1))
        DictionaryLogin::dictionary_starter_database_to_internal.push_back(255);
    while((uint16_t)DictionaryLogin::dictionary_starter_internal_to_database.size()<(internalId+1))
        DictionaryLogin::dictionary_starter_internal_to_database.push_back(0);
    DictionaryLogin::dictionary_starter_internal_to_database[internalId]=databaseId;
    DictionaryLogin::dictionary_starter_database_to_internal[databaseId]=internalId;
}

void EventLoopServerLoginSlave::compose04Reply()
{
    EventLoopClientLoginSlave::loginGood[0x00]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    EventLoopClientLoginSlave::loginGood[0x01]=0x00;//reply id, unknow
    EventLoopClientLoginSlave::loginGood[0x02]=0x00;//reply size, unknow
    EventLoopClientLoginSlave::loginGood[0x03]=0x00;//reply size, unknow
    EventLoopClientLoginSlave::loginGood[0x04]=0x00;//reply size, unknow
    EventLoopClientLoginSlave::loginGood[0x05]=0x00;//reply size, unknow

    EventLoopClientLoginSlave::loginGood[0x06]=0x01;//good

    {const uint32_t _tmp_le=(htole32(CommonSettingsCommon::commonSettingsCommon.character_delete_time));memcpy(EventLoopClientLoginSlave::loginGood+0x07,&_tmp_le,sizeof(_tmp_le));}

    EventLoopClientLoginSlave::loginGood[0x0B]=CommonSettingsCommon::commonSettingsCommon.max_character;
    EventLoopClientLoginSlave::loginGood[0x0C]=CommonSettingsCommon::commonSettingsCommon.min_character;
    EventLoopClientLoginSlave::loginGood[0x0D]=CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
    EventLoopClientLoginSlave::loginGood[0x0E]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
    {const uint16_t _tmp_le=(htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters));memcpy(EventLoopClientLoginSlave::loginGood+0x0F,&_tmp_le,sizeof(_tmp_le));}

    EventLoopClientLoginSlave::loginGoodSize=0x11;

    memcpy(EventLoopClientLoginSlave::loginGood+EventLoopClientLoginSlave::loginGoodSize,EventLoopClientLoginSlave::baseDatapackSum,sizeof(EventLoopClientLoginSlave::baseDatapackSum));
    EventLoopClientLoginSlave::loginGoodSize+=sizeof(EventLoopClientLoginSlave::baseDatapackSum);

    //Empty mirror is fine: write size=0 and no body, the client
    //(Api_protocol_reply.cpp, mirrorSize==0 branch) clears its
    //own httpDatapackMirrorBase and switches to the in-protocol
    //datapack-pull path served from datapack/ next to this binary.
    if(LinkToMaster::linkToMaster->httpDatapackMirror.size()>255)
    {
        std::cerr << "LinkToMaster::linkToMaster->httpDatapackMirror size>255 (abort)" << std::endl;
        abort();
    }
    EventLoopClientLoginSlave::loginGood[EventLoopClientLoginSlave::loginGoodSize]=(uint8_t)LinkToMaster::linkToMaster->httpDatapackMirror.size();
    EventLoopClientLoginSlave::loginGoodSize+=1;
    if(!LinkToMaster::linkToMaster->httpDatapackMirror.empty())
    {
        memcpy(EventLoopClientLoginSlave::loginGood+EventLoopClientLoginSlave::loginGoodSize,LinkToMaster::linkToMaster->httpDatapackMirror.data(),LinkToMaster::linkToMaster->httpDatapackMirror.size());
        EventLoopClientLoginSlave::loginGoodSize+=LinkToMaster::linkToMaster->httpDatapackMirror.size();
    }
}

void EventLoopServerLoginSlave::preload_profile()
{
    if(CharactersGroupForLogin::list.empty())
    {
        std::cerr << "EventLoopServerLoginSlave::preload_profile() CharactersGroupForLogin::list.isEmpty() (abort)" << std::endl;
        abort();
    }

    unsigned int profileIndex=0;
    while(profileIndex<EventLoopServerLoginSlave::loginProfileList.size())
    {
        EventLoopServerLoginSlave::LoginProfile &profile=EventLoopServerLoginSlave::loginProfileList[profileIndex];

        std::string encyclopedia_item,item;
        if(!profile.items.empty())
        {
            uint32_t lastItemId=0;
            std::vector<LoginProfile::Item> item_list=profile.items;
            std::sort(item_list.begin(),item_list.end(),[](const LoginProfile::Item &a, const LoginProfile::Item &b) {
                return a.id<b.id;
            });
            CATCHCHALLENGER_TYPE_ITEM max=item_list.at(item_list.size()-1).id;
            uint32_t pos=0;
            char item_raw[(2+4)*item_list.size()];
            unsigned int index=0;
            while(index<item_list.size())
            {
                const LoginProfile::Item &item=item_list.at(index);
                {const uint16_t _tmp_le=(htole16(item.id-lastItemId));memcpy(item_raw+pos,&_tmp_le,sizeof(_tmp_le));}

                pos+=2;
                lastItemId=item.id;
                {const uint32_t _tmp_le=(htole32(item.quantity));memcpy(item_raw+pos,&_tmp_le,sizeof(_tmp_le));}

                pos+=4;
                index++;
            }
            item=binarytoHexa(item_raw,sizeof(item_raw));

            const size_t size=max/8+1;
            char bitlist[size];
            memset(bitlist,0,size);
            index=0;
            while(index<item_list.size())
            {
                const LoginProfile::Item &item=item_list.at(index);
                uint16_t bittoUp=item.id;
                bitlist[bittoUp/8]|=(1<<(7-bittoUp%8));
                index++;
            }
            encyclopedia_item=binarytoHexa(bitlist,sizeof(bitlist));
        }
        std::string reputations;
        if(!profile.reputations.empty())
        {
            uint32_t lastReputationId=0;
            std::vector<LoginProfile::Reputation> reputations_list=profile.reputations;
            std::sort(reputations_list.begin(),reputations_list.end(),[](const LoginProfile::Reputation &a, const LoginProfile::Reputation &b) {
                return a.reputationDatabaseId<b.reputationDatabaseId;
            });
            uint32_t pos=0;
            char reputation_raw[(1+4+1)*reputations_list.size()];
            unsigned int index=0;
            while(index<reputations_list.size())
            {
                const LoginProfile::Reputation &reputation=reputations_list.at(index);
                {const uint32_t _tmp_le=(htole32(reputation.point));memcpy(reputation_raw+pos,&_tmp_le,sizeof(_tmp_le));}

                pos+=4;
                reputation_raw[pos]=reputation.reputationDatabaseId-lastReputationId;
                pos+=1;
                lastReputationId=reputation.reputationDatabaseId;
                reputation_raw[pos]=reputation.level;
                pos+=1;
                index++;
            }
            reputations=binarytoHexa(reputation_raw,sizeof(reputation_raw));
        }

        unsigned int indexDatabaseCommon=0;
        while(indexDatabaseCommon<CharactersGroupForLogin::list.size())
        {
            DatabaseBase * const database=CharactersGroupForLogin::list.at(indexDatabaseCommon)->database();
            const DatabaseBase::DatabaseType &databaseType=CharactersGroupForLogin::list.at(indexDatabaseCommon)->databaseType();
            LoginProfile::PreparedStatementForCreation &preparedStatementForCreation=profile.preparedStatementForCreationByCommon[database];
            if(preparedStatementForCreation.type.size()<=profileIndex)
                preparedStatementForCreation.type.resize(profileIndex+1);
            LoginProfile::PreparedStatementForCreationType &preparedStatementForCreationType=preparedStatementForCreation.type[profileIndex];
            //assume here all is the same type
            {
                unsigned int monsterGroupIndex=0;
                //profile.monster_insert.resize(profile.monstergroup.size());
                while(monsterGroupIndex<profile.monstergroup.size())
                {
                    const std::vector<LoginProfile::Monster> &monsters=profile.monstergroup.at(monsterGroupIndex);
                    if(preparedStatementForCreationType.monsterGroup.size()<=monsterGroupIndex)
                        preparedStatementForCreationType.monsterGroup.resize(monsterGroupIndex+1);
                    LoginProfile::PreparedStatementForCreationMonsterGroup &preparedStatementForCreationMonsterGroup=preparedStatementForCreationType.monsterGroup[monsterGroupIndex];
                    std::vector<uint16_t> monsterForEncyclopedia;
                    unsigned int monsterIndex=0;
                    while(monsterIndex<monsters.size())
                    {
                        const LoginProfile::Monster &monster=monsters.at(monsterIndex);
                        if(monster.skills.empty())
                        {
                            std::cerr << "monster.skills.empty() for some profile" << std::endl;
                            std::cerr << "check if datapack is in " << GlobalServerData::serverSettings.datapack_basePath << std::endl;
                            std::cerr << "check if datapack check if this monster id have skill " << monster.id << std::endl;
                            std::cerr << "check the profile number " << profileIndex << std::endl;
                            abort();
                        }
                        uint32_t lastSkillId=0;
                        std::vector<LoginProfile::Monster::Skill> skills_list=monster.skills;
                        std::sort(skills_list.begin(),skills_list.end(),[](const LoginProfile::Monster::Skill &a, const LoginProfile::Monster::Skill &b) {
                            return a.id<b.id;
                        });

                        char raw_skill[(2+1)*skills_list.size()],raw_skill_endurance[1*skills_list.size()];
                        //the skills
                        unsigned int sub_index=0;
                        while(sub_index<skills_list.size())
                        {
                            const LoginProfile::Monster::Skill &skill=skills_list.at(sub_index);
                            {const uint16_t _tmp_le=(htole16(skill.id-lastSkillId));memcpy(raw_skill+sub_index*(2+1),&_tmp_le,sizeof(_tmp_le));}

                            lastSkillId=skill.id;
                            raw_skill[sub_index*(2+1)+2]=skill.level;
                            raw_skill_endurance[sub_index]=skill.endurance;
                            sub_index++;
                        }
                        //dynamic part
                        {
                            //id,character,place,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,skills,skills_endurance
                            if(monster.ratio_gender!=-1)
                            {
                                const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster.compose({
                                        "%1",
                                        "%2",
                                        "1",
                                        std::to_string(monster.hp),
                                        std::to_string(monster.id),
                                        std::to_string(monster.level),
                                        std::to_string(monster.captured_with),
                                        "%3",
                                        "%4",
                                        std::to_string(monsterIndex+1),
                                        binarytoHexa(raw_skill,sizeof(raw_skill)),
                                        binarytoHexa(raw_skill_endurance,sizeof(raw_skill_endurance))
                                        });
                                preparedStatementForCreationMonsterGroup.monster_insert.push_back(PreparedStatementUnit(queryText,database));
                            }
                            else
                            {
                                const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster.compose({
                                        "%1",
                                        "%2",
                                        "1",
                                        std::to_string(monster.hp),
                                        std::to_string(monster.id),
                                        std::to_string(monster.level),
                                        std::to_string(monster.captured_with),
                                        "3",
                                        "%3",
                                        std::to_string(monsterIndex+1),
                                        binarytoHexa(raw_skill,sizeof(raw_skill)),
                                        binarytoHexa(raw_skill_endurance,sizeof(raw_skill_endurance))
                                        });
                                preparedStatementForCreationMonsterGroup.monster_insert.push_back(PreparedStatementUnit(queryText,database));
                            }
                        }
                        monsterForEncyclopedia.push_back(monster.id);
                        monsterIndex++;
                    }
                    //do the encyclopedia monster
                    const std::vector<uint16_t>::iterator result=std::max_element(monsterForEncyclopedia.begin(),monsterForEncyclopedia.end());
                    const size_t size=*result/8+1;
                    char bitlist[size];
                    memset(bitlist,0,size);
                    monsterIndex=0;
                    while(monsterIndex<monsterForEncyclopedia.size())
                    {
                        uint16_t bittoUp=monsterForEncyclopedia.at(monsterIndex);
                        bitlist[bittoUp/8]|=(1<<(7-bittoUp%8));
                        monsterIndex++;
                    }

                    switch(databaseType)
                    {
                        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
                        case DatabaseBase::DatabaseType::Mysql:
                            preparedStatementForCreationMonsterGroup.character_insert=PreparedStatementUnit(std::string("INSERT INTO `character`("
                                    "`id`,`account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`date`,`warehouse_cash`,`clan_leader`,"
                                    "`time_to_delete`,`played_time`,`last_connect`,`starter`,`item`,`reputations`,`encyclopedia_monster`,`encyclopedia_item`"
                                    ") VALUES(%1,%2,"+
                                                                                            #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                                                                                            std::string("%3")+
                                                                                            #else
                                                                                            std::string("'%3'")+
                                                                                            #endif
                                                                                            ",%4,0,0,"+
                                    std::to_string(profile.cash)+",%5,0,0,"
                                    "0,0,0,"+
                                    std::to_string(profile.databaseId/*starter*/)+",UNHEX('"+item+"'),UNHEX('"+reputations+"'),UNHEX('"+binarytoHexa(bitlist,sizeof(bitlist))+"'),UNHEX('"+encyclopedia_item+"'));"),database);
                        break;
                        #endif
                        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
                        case DatabaseBase::DatabaseType::SQLite:
                            preparedStatementForCreationMonsterGroup.character_insert=PreparedStatementUnit(std::string("INSERT INTO character("
                                    "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                                    "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                                    ") VALUES(%1,%2,"+
                                                                                            #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                                                                                            std::string("%3")+
                                                                                            #else
                                                                                            std::string("'%3'")+
                                                                                            #endif
                                                                                            ",%4,0,0,"+
                                    std::to_string(profile.cash)+",%5,0,0,"
                                    "0,0,0,"+
                                    std::to_string(profile.databaseId/*starter*/)+",'"+item+"','"+reputations+"','"+binarytoHexa(bitlist,sizeof(bitlist))+"','"+encyclopedia_item+"');"),database);
                        break;
                        #endif
                        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
                        case DatabaseBase::DatabaseType::PostgreSQL:
                            preparedStatementForCreationMonsterGroup.character_insert=PreparedStatementUnit(std::string("INSERT INTO character("
                                    "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                                    "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                                    ") VALUES(%1,%2,"+
                                                                                            #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                                                                                            std::string("%3")+
                                                                                            #else
                                                                                            std::string("'%3'")+
                                                                                            #endif
                                                                                            ",%4,0,0,"+
                                    std::to_string(profile.cash)+",%5,0,FALSE,"
                                    "0,0,0,"+
                                    std::to_string(profile.databaseId/*starter*/)+",'\\x"+item+"','\\x"+reputations+"','\\x"+binarytoHexa(bitlist,sizeof(bitlist))+"','\\x"+encyclopedia_item+"');"),database);
                        break;
                        #endif
                    default:
                        abort();
                        break;
                    }

                    monsterGroupIndex++;
                }
            }
            indexDatabaseCommon++;
        }

        profileIndex++;
    }

    if(EventLoopServerLoginSlave::loginProfileList.size()==0 && CommonSettingsCommon::commonSettingsCommon.min_character!=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        std::cout << "no profile loaded! C211 never send? (abort)" << std::endl;
        abort();
    }
    std::cout << EventLoopServerLoginSlave::loginProfileList.size() << " profile loaded" << std::endl;
}
