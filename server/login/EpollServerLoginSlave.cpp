#include "EpollServerLoginSlave.hpp"
#include "CharactersGroupForLogin.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/ProtocolVersion.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "LinkToMaster.hpp"
#include "LinkToGameServer.hpp"
#include "../base/PreparedDBQuery.hpp"

using namespace CatchChallenger;

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <iostream>
#include <vector>
#include <algorithm>

#include "EpollClientLoginSlave.hpp"
#include "../base/DictionaryLogin.hpp"
#include "../../general/base/ProtocolParsing.hpp"

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

    bool ok;
    srand(time(NULL));

    if(!settings.contains("ip"))
        settings.setValue("ip","");
    server_ip=settings.value("ip");
    if(!settings.contains("port"))
        settings.setValue("port",rand()%40000+10000);
    server_port=settings.value("port");

    #if CATCHCHALLENGER_SERVER_DATABASE_COMMON_BLOBVERSION > 15
    #error CATCHCHALLENGER_SERVER_DATABASE_COMMON_BLOBVERSION can t be greater than 15
    #endif
    if(!settings.contains("common_blobversion_datapack"))
        settings.setValue("common_blobversion_datapack",0);
    common_blobversion_datapack=stringtouint8(settings.value("common_blobversion_datapack"),&ok);
    if(!ok)
    {
        std::cerr << "common_blobversion_datapack is not a number" << std::endl;
        abort();
    }
    if(common_blobversion_datapack>15)
    {
        std::cerr << "common_blobversion_datapack > 15" << std::endl;
        abort();
    }
    common_blobversion_datapack*=16;
    common_blobversion_datapack|=CATCHCHALLENGER_SERVER_DATABASE_COMMON_BLOBVERSION;

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
        EpollClientLoginSlave::databaseBaseLogin.considerDownAfterNumberOfTry=stringtouint32(settings.value("considerDownAfterNumberOfTry"),&ok);
        if(!ok)
        {
            std::cerr << "considerDownAfterNumberOfTry not number" << std::endl;
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
        PreparedDBQueryLogin::initDatabaseQueryLogin(EpollClientLoginSlave::databaseBaseLogin.databaseType(),&EpollClientLoginSlave::databaseBaseLogin);
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
            #ifdef SERVERSSL
            ctx from what?
            linkToMaster::linkToMaster=new linkToMaster(linkfd,ctx);
            #else
            LinkToMaster::linkToMaster=new LinkToMaster(linkfd);
            #endif
            LinkToMaster::linkToMaster->stat=LinkToMaster::Stat::Connected;
            LinkToMaster::linkToMaster->readTheFirstSslHeader();
            LinkToMaster::linkToMaster->setConnexionSettings();
            /*const int &s = EpollSocket::make_non_blocking(linkfd);
            if(s == -1)
                std::cerr << "unable to make to socket non blocking" << std::endl;*/
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
}

void EpollServerLoginSlave::close()
{
    std::cerr << "EpollServerLoginSlave::close()" << std::endl;
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

    if(server_port.empty())
    {
        std::cout << "EpollServerLoginSlave::tryListen() port can't be empty (abort)" << std::endl;
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

    unsigned int index=0;
    while(index<EpollServerLoginSlave::loginProfileList.size())
    {
        EpollServerLoginSlave::LoginProfile &profile=EpollServerLoginSlave::loginProfileList[index];

        std::string encyclopedia_item,item;
        if(!profile.items.empty())
        {
            uint32_t lastItemId=0;
            auto item_list=profile.items;
            std::sort(item_list.begin(),item_list.end(),[](const LoginProfile::Item &a, const LoginProfile::Item &b) {
                return a.id<b.id;
            });
            auto max=item_list.at(item_list.size()-1).id;
            uint32_t pos=0;
            char item_raw[(2+4)*item_list.size()];
            unsigned int index=0;
            while(index<item_list.size())
            {
                const auto &item=item_list.at(index);
                *reinterpret_cast<uint16_t *>(item_raw+pos)=htole16(item.id-lastItemId);
                pos+=2;
                lastItemId=item.id;
                *reinterpret_cast<uint32_t *>(item_raw+pos)=htole32(item.quantity);
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
                const auto &item=item_list.at(index);
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
            auto reputations_list=profile.reputations;
            std::sort(reputations_list.begin(),reputations_list.end(),[](const LoginProfile::Reputation &a, const LoginProfile::Reputation &b) {
                return a.reputationDatabaseId<b.reputationDatabaseId;
            });
            uint32_t pos=0;
            char reputation_raw[(1+4+1)*reputations_list.size()];
            unsigned int index=0;
            while(index<reputations_list.size())
            {
                const auto &reputation=reputations_list.at(index);
                *reinterpret_cast<uint32_t *>(reputation_raw+pos)=htole32(reputation.point);
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
            if(preparedStatementForCreation.type.size()<=index)
                preparedStatementForCreation.type.resize(index+1);
            LoginProfile::PreparedStatementForCreationType &preparedStatementForCreationType=preparedStatementForCreation.type[index];
            //assume here all is the same type
            {
                unsigned int monsterGroupIndex=0;
                //profile.monster_insert.resize(profile.monstergroup.size());
                while(monsterGroupIndex<profile.monstergroup.size())
                {
                    const auto &monsters=profile.monstergroup.at(monsterGroupIndex);
                    if(preparedStatementForCreationType.monsterGroup.size()<=monsterGroupIndex)
                        preparedStatementForCreationType.monsterGroup.resize(monsterGroupIndex+1);
                    LoginProfile::PreparedStatementForCreationMonsterGroup &preparedStatementForCreationMonsterGroup=preparedStatementForCreationType.monsterGroup[monsterGroupIndex];
                    std::vector<uint16_t> monsterForEncyclopedia;
                    unsigned int monsterIndex=0;
                    while(monsterIndex<monsters.size())
                    {
                        const auto &monster=monsters.at(monsterIndex);
                        if(monster.skills.empty())
                        {
                            std::cerr << "monster.skills.empty() for some profile" << std::endl;
                            abort();
                        }
                        uint32_t lastSkillId=0;
                        auto skills_list=monster.skills;
                        std::sort(skills_list.begin(),skills_list.end(),[](const LoginProfile::Monster::Skill &a, const LoginProfile::Monster::Skill &b) {
                            return a.id<b.id;
                        });

                        char raw_skill[(2+1)*skills_list.size()],raw_skill_endurance[1*skills_list.size()];
                        //the skills
                        unsigned int sub_index=0;
                        while(sub_index<skills_list.size())
                        {
                            const auto &skill=skills_list.at(sub_index);
                            *reinterpret_cast<uint16_t *>(raw_skill+sub_index*(2+1))=htole16(skill.id-lastSkillId);
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
                    const auto &result=std::max_element(monsterForEncyclopedia.begin(),monsterForEncyclopedia.end());
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
                                    ",`blob_version`) VALUES(%1,%2,"+
                                                                                            #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                                                                                            std::string("%3")+
                                                                                            #else
                                                                                            std::string("'%3'")+
                                                                                            #endif
                                                                                            ",%4,0,0,"+
                                    std::to_string(profile.cash)+",%5,0,0,"
                                    "0,0,0,"+
                                    std::to_string(profile.databaseId/*starter*/)+",UNHEX('"+item+"'),UNHEX('"+reputations+"'),UNHEX('"+binarytoHexa(bitlist,sizeof(bitlist))+"'),UNHEX('"+encyclopedia_item+"'),"+std::to_string(common_blobversion_datapack)+");"),database);
                        break;
                        #endif
                        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
                        case DatabaseBase::DatabaseType::SQLite:
                            preparedStatementForCreationMonsterGroup.character_insert=PreparedStatementUnit(std::string("INSERT INTO character("
                                    "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                                    "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                                    ",blob_version) VALUES(%1,%2,"+
                                                                                            #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                                                                                            std::string("%3")+
                                                                                            #else
                                                                                            std::string("'%3'")+
                                                                                            #endif
                                                                                            ",%4,0,0,"+
                                    std::to_string(profile.cash)+",%5,0,0,"
                                    "0,0,0,"+
                                    std::to_string(profile.databaseId/*starter*/)+",'"+item+"','"+reputations+"','"+binarytoHexa(bitlist,sizeof(bitlist))+"','"+encyclopedia_item+"',"+std::to_string(common_blobversion_datapack)+");"),database);
                        break;
                        #endif
                        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
                        case DatabaseBase::DatabaseType::PostgreSQL:
                            preparedStatementForCreationMonsterGroup.character_insert=PreparedStatementUnit(std::string("INSERT INTO character("
                                    "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                                    "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                                    ",blob_version) VALUES(%1,%2,"+
                                                                                            #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                                                                                            std::string("%3")+
                                                                                            #else
                                                                                            std::string("'%3'")+
                                                                                            #endif
                                                                                            ",%4,0,0,"+
                                    std::to_string(profile.cash)+",%5,0,FALSE,"
                                    "0,0,0,"+
                                    std::to_string(profile.databaseId/*starter*/)+",'\\x"+item+"','\\x"+reputations+"','\\x"+binarytoHexa(bitlist,sizeof(bitlist))+"','\\x"+encyclopedia_item+"',"+std::to_string(common_blobversion_datapack)+");"),database);
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

        index++;
    }

    if(EpollServerLoginSlave::loginProfileList.size()==0 && CommonSettingsCommon::commonSettingsCommon.min_character!=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        std::cout << "no profile loaded! C211 never send? (abort)" << std::endl;
        abort();
    }
    std::cout << EpollServerLoginSlave::loginProfileList.size() << " profile loaded" << std::endl;
}
