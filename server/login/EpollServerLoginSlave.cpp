#include "EpollServerLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../epoll/Epoll.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../base/PreparedDBQuery.h"

using namespace CatchChallenger;

#include <QFile>
#include <QByteArray>
#include <QCoreApplication>

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <iostream>

#include "EpollClientLoginSlave.h"
#include "../base/DictionaryLogin.h"
#include "../../general/base/ProtocolParsing.h"

EpollServerLoginSlave *EpollServerLoginSlave::epollServerLoginSlave=NULL;

EpollServerLoginSlave::EpollServerLoginSlave() :
    tcpNodelay(false),
    tcpCork(false),
    serverReady(false),
    server_ip(NULL),
    server_port(NULL)
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

    QSettings settings(QCoreApplication::applicationDirPath()+"/login.conf",QSettings::IniFormat);

    {
        memset(EpollClientLoginSlave::serverServerList,0x00,sizeof(EpollClientLoginSlave::serverServerList));
        memset(EpollClientLoginSlave::serverLogicalGroupList,0x00,sizeof(EpollClientLoginSlave::serverLogicalGroupList));
        memset(EpollClientLoginSlave::serverLogicalGroupAndServerList,0x00,sizeof(EpollClientLoginSlave::serverLogicalGroupAndServerList));
    }
    {
        memset(EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterNotFound,0x00,sizeof(EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterNotFound));
        memset(EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline,0x00,sizeof(EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline));
        memset(EpollClientLoginSlave::characterSelectionIsWrongBufferServerInternalProblem,0x00,sizeof(EpollClientLoginSlave::characterSelectionIsWrongBufferServerInternalProblem));
        memset(EpollClientLoginSlave::characterSelectionIsWrongBufferServerNotFound,0x00,sizeof(EpollClientLoginSlave::characterSelectionIsWrongBufferServerNotFound));
        char tempBuff;

        tempBuff=2;
        //size will be the same
        EpollClientLoginSlave::characterSelectionIsWrongBufferSize=ProtocolParsingBase::computeReplyData(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    false,
                    #endif
                    EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterNotFound,0,&tempBuff,1,-1/*not fixed size*/
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            ,ProtocolParsing::compressionTypeServer
            #endif
            );
        tempBuff=3;
        if(EpollClientLoginSlave::characterSelectionIsWrongBufferSize!=ProtocolParsingBase::computeReplyData(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    false,
                    #endif
                    EpollClientLoginSlave::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline,0,&tempBuff,1,-1/*not fixed size*/
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            ,ProtocolParsing::compressionTypeServer
            #endif
            ))
            abort();
        tempBuff=4;
        if(EpollClientLoginSlave::characterSelectionIsWrongBufferSize!=ProtocolParsingBase::computeReplyData(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    false,
                    #endif
                    EpollClientLoginSlave::characterSelectionIsWrongBufferServerInternalProblem,0,&tempBuff,1,-1/*not fixed size*/
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            ,ProtocolParsing::compressionTypeServer
            #endif
            ))
            abort();
        tempBuff=5;
        if(EpollClientLoginSlave::characterSelectionIsWrongBufferSize!=ProtocolParsingBase::computeReplyData(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    false,
                    #endif
                    EpollClientLoginSlave::characterSelectionIsWrongBufferServerNotFound,0,&tempBuff,1,-1/*not fixed size*/
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            ,ProtocolParsing::compressionTypeServer
            #endif
            ))
            abort();
    }

    srand(time(NULL));

    if(!settings.contains(QStringLiteral("ip")))
        settings.setValue(QStringLiteral("ip"),QString());
    const QString &server_ip_string=settings.value(QStringLiteral("ip")).toString();
    const QByteArray &server_ip_data=server_ip_string.toLocal8Bit();
    if(!server_ip_string.isEmpty())
    {
        server_ip=new char[server_ip_data.size()+1];
        strcpy(server_ip,server_ip_data.constData());
    }
    if(!settings.contains(QStringLiteral("port")))
        settings.setValue(QStringLiteral("port"),rand()%40000+10000);
    const QByteArray &server_port_data=settings.value(QStringLiteral("port")).toString().toLocal8Bit();
    server_port=new char[server_port_data.size()+1];
    strcpy(server_port,server_port_data.constData());

    //token
    settings.beginGroup(QStringLiteral("master"));
    if(!settings.contains(QStringLiteral("token")))
        generateToken(settings);
    QString token=settings.value(QStringLiteral("token")).toString();
    if(token.size()!=TOKEN_SIZE_FOR_MASTERAUTH*2/*String Hexa, not binary*/)
        generateToken(settings);
    token=settings.value(QStringLiteral("token")).toString();
    memcpy(LinkToMaster::header_magic_number_and_private_token+9,QByteArray::fromHex(token.toLatin1()).constData(),TOKEN_SIZE_FOR_MASTERAUTH);
    settings.endGroup();

    //mode
    {
        if(!settings.contains(QStringLiteral("mode")))
            settings.setValue(QStringLiteral("mode"),QStringLiteral("direct"));//or proxy
        QString mode=settings.value(QStringLiteral("mode")).toString();
        if(mode!=QStringLiteral("direct") && mode!=QStringLiteral("proxy"))
        {
            mode=QStringLiteral("direct");
            settings.setValue(QStringLiteral("mode"),mode);
        }
        if(mode==QStringLiteral("direct"))
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

    if(!settings.contains(QStringLiteral("httpDatapackMirror")))
        settings.setValue(QStringLiteral("httpDatapackMirror"),QString());
    QString httpDatapackMirror=settings.value(QStringLiteral("httpDatapackMirror")).toString();
    httpDatapackMirror=settings.value(QStringLiteral("httpDatapackMirror")).toString();
    if(httpDatapackMirror.isEmpty())
    {
        std::cerr << "empty mirror in the settings but not supported from now (abort)" << std::endl;
        abort();

        #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
        qDebug() << "Need mirror because CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION is def, need decompression to datapack list input";
        return EXIT_FAILURE;
        #endif
    }
    else
    {
        QStringList newMirrorList;
        QRegularExpression httpMatch("^https?://.+$");
        const QStringList &mirrorList=httpDatapackMirror.split(";");
        int index=0;
        while(index<mirrorList.size())
        {
            const QString &mirror=mirrorList.at(index);
            if(!mirror.contains(httpMatch))
            {
                std::cerr << "Mirror wrong: " << mirror.toStdString() << std::endl;
                abort();
            }
            if(mirror.endsWith("/"))
                newMirrorList << mirror;
            else
                newMirrorList << mirror+"/";
            index++;
        }
        httpDatapackMirror=newMirrorList.join(";");
    }

    //connection
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    if(!settings.contains(QStringLiteral("compression")))
        settings.setValue(QStringLiteral("compression"),QStringLiteral("zlib"));
    if(settings.value(QStringLiteral("compression")).toString()==QStringLiteral("none"))
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::None;
    else if(settings.value(QStringLiteral("compression")).toString()==QStringLiteral("xz"))
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Xz;
    else
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Zlib;
    #endif

    settings.beginGroup(QStringLiteral("Linux"));
    if(!settings.contains(QStringLiteral("tcpCork")))
        settings.setValue(QStringLiteral("tcpCork"),true);
    if(!settings.contains(QStringLiteral("tcpNodelay")))
        settings.setValue(QStringLiteral("tcpNodelay"),false);
    tcpCork=settings.value(QStringLiteral("tcpCork")).toBool();
    tcpNodelay=settings.value(QStringLiteral("tcpNodelay")).toBool();
    settings.endGroup();
    settings.sync();

    QString db;
    QString host;
    QString login;
    QString pass;
    QString type;
    bool ok;
    //here to have by login server an auth
    {
        settings.beginGroup(QStringLiteral("db-login"));
        if(!settings.contains(QStringLiteral("considerDownAfterNumberOfTry")))
            settings.setValue(QStringLiteral("considerDownAfterNumberOfTry"),30);
        if(!settings.contains(QStringLiteral("tryInterval")))
            settings.setValue(QStringLiteral("tryInterval"),1);
        if(!settings.contains(QStringLiteral("db")))
            settings.setValue(QStringLiteral("db"),QStringLiteral("catchchallenger_login"));
        if(!settings.contains(QStringLiteral("host")))
            settings.setValue(QStringLiteral("host"),QStringLiteral("localhost"));
        if(!settings.contains(QStringLiteral("login")))
            settings.setValue(QStringLiteral("login"),QStringLiteral("root"));
        if(!settings.contains(QStringLiteral("pass")))
            settings.setValue(QStringLiteral("pass"),QStringLiteral("root"));
        if(!settings.contains(QStringLiteral("type")))
            settings.setValue(QStringLiteral("type"),QStringLiteral("postgresql"));
        settings.sync();
        EpollClientLoginSlave::databaseBaseLogin.considerDownAfterNumberOfTry=settings.value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
        if(EpollClientLoginSlave::databaseBaseLogin.considerDownAfterNumberOfTry==0 || !ok)
        {
            std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
            abort();
        }
        db=settings.value(QStringLiteral("db")).toString();
        host=settings.value(QStringLiteral("host")).toString();
        login=settings.value(QStringLiteral("login")).toString();
        pass=settings.value(QStringLiteral("pass")).toString();
        EpollClientLoginSlave::databaseBaseLogin.tryInterval=settings.value(QStringLiteral("tryInterval")).toUInt(&ok);
        if(EpollClientLoginSlave::databaseBaseLogin.tryInterval==0 || !ok)
        {
            std::cerr << "tryInterval==0 (abort)" << std::endl;
            abort();
        }
        type=settings.value(QStringLiteral("type")).toString();
        if(type!=QStringLiteral("postgresql"))
        {
            std::cerr << "only db type postgresql supported (abort)" << std::endl;
            abort();
        }
        if(!EpollClientLoginSlave::databaseBaseLogin.syncConnect(host.toUtf8().constData(),db.toUtf8().constData(),login.toUtf8().constData(),pass.toUtf8().constData()))
        {
            std::cerr << "Connect to login database failed:" << EpollClientLoginSlave::databaseBaseLogin.errorMessage() << std::endl;
            abort();
        }
        settings.endGroup();
        settings.sync();

        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        PreparedDBQueryLogin::initDatabaseQueryLogin(EpollClientLoginSlave::databaseBaseLogin.databaseType());
        #endif
        //PreparedDBQueryBase::initDatabaseQueryBase(EpollClientLoginSlave::databaseBaseLogin.databaseType());//don't exist, allow dictionary and loaded without cache
    }

    QStringList charactersGroupForLoginList;
    bool continueCharactersGroupForLoginSettings=true;
    int CharactersGroupForLoginId=0;
    while(continueCharactersGroupForLoginSettings)
    {
        settings.beginGroup(QStringLiteral("db-common-")+QString::number(CharactersGroupForLoginId));
        if(CharactersGroupForLoginId==0)
        {
            if(!settings.contains(QStringLiteral("considerDownAfterNumberOfTry")))
                settings.setValue(QStringLiteral("considerDownAfterNumberOfTry"),3);
            if(!settings.contains(QStringLiteral("tryInterval")))
                settings.setValue(QStringLiteral("tryInterval"),5);
            if(!settings.contains(QStringLiteral("db")))
                settings.setValue(QStringLiteral("db"),QStringLiteral("catchchallenger_common"));
            if(!settings.contains(QStringLiteral("host")))
                settings.setValue(QStringLiteral("host"),QStringLiteral("localhost"));
            if(!settings.contains(QStringLiteral("login")))
                settings.setValue(QStringLiteral("login"),QStringLiteral("root"));
            if(!settings.contains(QStringLiteral("pass")))
                settings.setValue(QStringLiteral("pass"),QStringLiteral("root"));
            if(!settings.contains(QStringLiteral("type")))
                settings.setValue(QStringLiteral("type"),QStringLiteral("postgresql"));
            if(!settings.contains(QStringLiteral("charactersGroup")))
                settings.setValue(QStringLiteral("charactersGroup"),QString());
            if(!settings.contains(QStringLiteral("comment")))
                settings.setValue(QStringLiteral("comment"),QStringLiteral("to do maxClanId, maxCharacterId, maxMonsterId"));
            settings.sync();
        }
        if(settings.contains(QStringLiteral("login")))
        {
            const QString &charactersGroup=settings.value(QStringLiteral("charactersGroup")).toString();
            if(!CharactersGroupForLogin::hash.contains(charactersGroup))
            {
                const quint8 &considerDownAfterNumberOfTry=settings.value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
                if(considerDownAfterNumberOfTry==0 || !ok)
                {
                    std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
                    abort();
                }
                db=settings.value(QStringLiteral("db")).toString();
                host=settings.value(QStringLiteral("host")).toString();
                login=settings.value(QStringLiteral("login")).toString();
                pass=settings.value(QStringLiteral("pass")).toString();
                const quint8 &tryInterval=settings.value(QStringLiteral("tryInterval")).toUInt(&ok);
                if(tryInterval==0 || !ok)
                {
                    std::cerr << "tryInterval==0 (abort)" << std::endl;
                    abort();
                }
                type=settings.value(QStringLiteral("type")).toString();
                if(type!=QStringLiteral("postgresql"))
                {
                    std::cerr << "only db type postgresql supported (abort)" << std::endl;
                    abort();
                }
                CharactersGroupForLogin::hash[charactersGroup]=new CharactersGroupForLogin(db.toUtf8().constData(),host.toUtf8().constData(),login.toUtf8().constData(),pass.toUtf8().constData(),considerDownAfterNumberOfTry,tryInterval);
                charactersGroupForLoginList << charactersGroup;
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
        charactersGroupForLoginList.sort();
        EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup[EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize]=(unsigned char)charactersGroupForLoginList.size();
        EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=sizeof(unsigned char);
        int index=0;
        while(index<charactersGroupForLoginList.size())
        {
            const QString &CharactersGroupForLoginName=charactersGroupForLoginList.at(index);
            const QByteArray &data=CharactersGroupForLoginName.toUtf8();
            if(data.size()>20)
            {
                std::cerr << "CharactersGroupForLoginName too big (abort)" << std::endl;
                abort();
            }
            EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup[EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize]=data.size();
            EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=1;
            memcpy(EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup+EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize,data.constData(),data.size());
            EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=data.size();

            CharactersGroupForLogin::hash[CharactersGroupForLoginName]->index=index;
            CharactersGroupForLogin::list << CharactersGroupForLogin::hash[CharactersGroupForLoginName];

            if(index==0)
                PreparedDBQueryCommon::initDatabaseQueryCommonWithoutSP(CharactersGroupForLogin::list.last()->databaseType());

            index++;
        }
    }

    {
        settings.beginGroup(QStringLiteral("master"));
        if(!settings.contains(QStringLiteral("host")))
            settings.setValue(QStringLiteral("host"),QStringLiteral("localhost"));
        if(!settings.contains(QStringLiteral("port")))
            settings.setValue(QStringLiteral("port"),9999);
        if(!settings.contains(QStringLiteral("considerDownAfterNumberOfTry")))
            settings.setValue(QStringLiteral("considerDownAfterNumberOfTry"),3);
        if(!settings.contains(QStringLiteral("tryInterval")))
            settings.setValue(QStringLiteral("tryInterval"),5);
        settings.sync();
        const QString &host=settings.value(QStringLiteral("host")).toString();
        const quint16 &port=settings.value(QStringLiteral("port")).toUInt(&ok);
        if(port==0 || !ok)
        {
            std::cerr << "Master port not a number or 0:" << settings.value(QStringLiteral("port")).toString().toStdString() << std::endl;
            abort();
        }
        const quint8 &tryInterval=settings.value(QStringLiteral("tryInterval")).toUInt(&ok);
        if(tryInterval==0 || !ok)
        {
            std::cerr << "tryInterval==0 (abort)" << std::endl;
            abort();
        }
        const quint8 &considerDownAfterNumberOfTry=settings.value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
        if(considerDownAfterNumberOfTry==0 || !ok)
        {
            std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
            abort();
        }

        {
            const int &linkfd=LinkToMaster::tryConnect(host.toLocal8Bit().constData(),port,tryInterval,considerDownAfterNumberOfTry);
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
        }

        LinkToMaster::linkToMaster->httpDatapackMirror=httpDatapackMirror;
        settings.endGroup();
    }
}

EpollServerLoginSlave::~EpollServerLoginSlave()
{
    memset(LinkToMaster::header_magic_number_and_private_token,0x00,sizeof(LinkToMaster::header_magic_number_and_private_token));
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
    if(!LinkToMaster::linkToMaster->httpDatapackMirror.isEmpty())
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
        const bool &returnedValue=tryListenInternal(server_ip, server_port);
    #endif

    preload_the_randomData();
    preload_profile();

    if(server_ip!=NULL)
    {
        std::cout << "Listen on " << server_ip << ":" << server_port << std::endl;
        delete server_ip;
        server_ip=NULL;
    }
    else
        std::cout << "Listen on *:" << server_port << std::endl;
    if(server_port!=NULL)
    {
        delete server_port;
        server_port=NULL;
    }
    return returnedValue;
}

void EpollServerLoginSlave::generateToken(QSettings &settings)
{
    FILE *fpRandomFile = fopen("/dev/urandom","rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open /dev/urandom to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(LinkToMaster::header_magic_number_and_private_token+9,1,TOKEN_SIZE_FOR_MASTERAUTH,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_MASTERAUTH)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_MASTERAUTH << " needed to do the token from /dev/urandom" << std::endl;
        abort();
    }
    settings.setValue(QStringLiteral("token"),QString(
                          QByteArray(
                              reinterpret_cast<char *>(LinkToMaster::header_magic_number_and_private_token)
                              +9,TOKEN_SIZE_FOR_MASTERAUTH)
                          .toHex()));
    fclose(fpRandomFile);
    settings.sync();
}

void EpollServerLoginSlave::setSkinPair(const quint8 &internalId,const quint16 &databaseId)
{
    while(DictionaryLogin::dictionary_skin_database_to_internal.size()<(databaseId+1))
        DictionaryLogin::dictionary_skin_database_to_internal << 0;
    while(DictionaryLogin::dictionary_skin_internal_to_database.size()<(internalId+1))
        DictionaryLogin::dictionary_skin_internal_to_database << 0;
    DictionaryLogin::dictionary_skin_internal_to_database[internalId]=databaseId;
    DictionaryLogin::dictionary_skin_database_to_internal[databaseId]=internalId;
}

void EpollServerLoginSlave::setProfilePair(const quint8 &internalId,const quint16 &databaseId)
{
    while(DictionaryLogin::dictionary_starter_database_to_internal.size()<(databaseId+1))
        DictionaryLogin::dictionary_starter_database_to_internal << 0;
    while(DictionaryLogin::dictionary_starter_internal_to_database.size()<(internalId+1))
        DictionaryLogin::dictionary_starter_internal_to_database << 0;
    DictionaryLogin::dictionary_starter_internal_to_database[internalId]=databaseId;
    DictionaryLogin::dictionary_starter_database_to_internal[databaseId]=internalId;
}

void EpollServerLoginSlave::compose04Reply()
{
    EpollClientLoginSlave::loginGood[0x00]=0x01;//good

    *reinterpret_cast<quint32 *>(EpollClientLoginSlave::loginGood+0x01)=htole32(CommonSettingsCommon::commonSettingsCommon.character_delete_time);
    EpollClientLoginSlave::loginGood[0x05]=CommonSettingsCommon::commonSettingsCommon.max_character;
    EpollClientLoginSlave::loginGood[0x06]=CommonSettingsCommon::commonSettingsCommon.min_character;
    EpollClientLoginSlave::loginGood[0x07]=CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
    EpollClientLoginSlave::loginGood[0x08]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
    *reinterpret_cast<quint16 *>(EpollClientLoginSlave::loginGood+0x09)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters);
    EpollClientLoginSlave::loginGood[0x0B]=CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
    *reinterpret_cast<quint16 *>(EpollClientLoginSlave::loginGood+0x0C)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems);
    EpollClientLoginSlave::loginGoodSize=0x0E;

    memcpy(EpollClientLoginSlave::loginGood+EpollClientLoginSlave::loginGoodSize,EpollClientLoginSlave::baseDatapackSum,sizeof(EpollClientLoginSlave::baseDatapackSum));
    EpollClientLoginSlave::loginGoodSize+=sizeof(EpollClientLoginSlave::baseDatapackSum);

    const QByteArray &httpDatapackMirrorData=LinkToMaster::linkToMaster->httpDatapackMirror.toUtf8();
    if(LinkToMaster::linkToMaster->httpDatapackMirror.isEmpty())
    {
        std::cerr << "EpollClientLoginSlave::linkToMaster->httpDatapackMirror.isEmpty(), not coded for now (abort)" << std::endl;
        abort();
    }
    if(httpDatapackMirrorData.size()>255)
    {
        std::cerr << "httpDatapackMirrorData size>255 (abort)" << std::endl;
        abort();
    }
    EpollClientLoginSlave::loginGood[EpollClientLoginSlave::loginGoodSize]=(quint8)httpDatapackMirrorData.size();
    EpollClientLoginSlave::loginGoodSize+=1;
    memcpy(EpollClientLoginSlave::loginGood+EpollClientLoginSlave::loginGoodSize,httpDatapackMirrorData.constData(),httpDatapackMirrorData.size());
    EpollClientLoginSlave::loginGoodSize+=httpDatapackMirrorData.size();
}

void EpollServerLoginSlave::preload_profile()
{
    if(CharactersGroupForLogin::list.isEmpty())
    {
        std::cerr << "EpollServerLoginSlave::preload_profile() CharactersGroupForLogin::list.isEmpty() (abort)" << std::endl;
        abort();
    }
    const DatabaseBase::Type &type=CharactersGroupForLogin::list.at(0)->databaseType();
    QStringList tempStringList;

    unsigned int index=0;
    while(index<EpollServerLoginSlave::loginProfileList.size())
    {
        EpollServerLoginSlave::LoginProfile &profile=EpollServerLoginSlave::loginProfileList[index];
        //assume here all is the same type
        switch(type)
        {
            default:
            case DatabaseBase::Type::Mysql:
                tempStringList << QStringLiteral("INSERT INTO `character`(`id`,`account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`date`,`warehouse_cash`,`clan_leader`,`time_to_delete`,`played_time`,`last_connect`,`starter`) VALUES(");
                tempStringList << QLatin1String(",");
                tempStringList << QLatin1String(",'");
                tempStringList << QLatin1String("',");
                tempStringList << QLatin1String(",0,0,")+
                        QString::number(profile.cash)+QLatin1String(",");
                tempStringList << QLatin1String(",0,0,0,0,0,")+
                        QString::number(profile.databaseId)+QLatin1String(");");
            break;
            case DatabaseBase::Type::SQLite:
                tempStringList << QStringLiteral("INSERT INTO character(id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,time_to_delete,played_time,last_connect,starter) VALUES(");
                tempStringList << QLatin1String(",");
                tempStringList << QLatin1String(",'");
                tempStringList << QLatin1String("',");
                tempStringList << QLatin1String(",0,0,")+
                        QString::number(profile.cash)+QLatin1String(",");
                tempStringList << QLatin1String(",0,0,0,0,0,")+
                        QString::number(index)+QLatin1String(");");
            break;
            case DatabaseBase::Type::PostgreSQL:
                tempStringList << QStringLiteral("INSERT INTO character(id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,time_to_delete,played_time,last_connect,starter) VALUES(");
                tempStringList << QLatin1String(",");
                tempStringList << QLatin1String(",'");
                tempStringList << QLatin1String("',");
                tempStringList << QLatin1String(",0,0,")+
                        QString::number(profile.cash)+QLatin1String(",");
                tempStringList << QLatin1String(",0,FALSE,0,0,0,")+
                        QString::number(index)+QLatin1String(");");
            break;
        }
        unsigned int preparedQueryCharTempSize=0;
        //reservate the memory space
        {
            int sub_index=0;
            while(sub_index<tempStringList.size())
            {
                const QByteArray &tempStringData=tempStringList.at(sub_index).toUtf8();
                preparedQueryCharTempSize+=tempStringData.size();
                sub_index++;
            }
            profile.preparedQueryChar=(char *)malloc(preparedQueryCharTempSize);
        }
        //set the new memory space
        {
            int sub_index=0;
            while(sub_index<tempStringList.size())
            {
                const QByteArray &tempStringData=tempStringList.at(sub_index).toUtf8();
                profile.preparedQuerySize[sub_index]=tempStringData.size();
                if(sub_index>0)
                    profile.preparedQueryPos[sub_index]=profile.preparedQueryPos[sub_index-1]+profile.preparedQuerySize[sub_index-1];
                memcpy(profile.preparedQueryChar+profile.preparedQueryPos[sub_index],tempStringData.constData(),tempStringData.size());
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
