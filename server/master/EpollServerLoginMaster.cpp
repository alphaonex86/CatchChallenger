#include "EpollServerLoginMaster.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../VariableServer.h"
#include "../../general/fight/CommonFightEngineBase.h"

using namespace CatchChallenger;

#include <QFile>
#include <QByteArray>
#include <QCryptographicHash>
#include <QRegularExpression>

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

EpollServerLoginMaster::EpollServerLoginMaster() :
    server_ip(NULL),
    server_port(NULL),
    rawServerListForC211(static_cast<char *>(malloc(sizeof(EpollClientLoginMaster::loginSettingsAndCharactersGroup)))),
    rawServerListForC211Size(0),
    databaseBaseLogin(NULL),
    databaseBaseBase(NULL)
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
        memset(EpollClientLoginMaster::replyToRegisterLoginServer,0x00,sizeof(EpollClientLoginMaster::replyToRegisterLoginServer));
        memset(EpollClientLoginMaster::serverServerList,0x00,sizeof(EpollClientLoginMaster::serverServerList));
        memset(EpollClientLoginMaster::serverLogicalGroupList,0x00,sizeof(EpollClientLoginMaster::serverLogicalGroupList));
        memset(rawServerListForC211,0x00,sizeof(EpollClientLoginMaster::loginSettingsAndCharactersGroup));
    }

    QSettings settings("master.conf",QSettings::IniFormat);

    srand(time(NULL));

    loadLoginSettings(settings);
    loadDBLoginSettings(settings);
    QStringList charactersGroupList=loadCharactersGroup(settings);
    charactersGroupListReply(charactersGroupList);
    loadTheDatapack();
    doTheLogicalGroup(settings);
    doTheServerList();

    {
        memset(EpollClientLoginMaster::characterSelectionIsWrongBufferCharacterNotFound,0x00,sizeof(EpollClientLoginMaster::characterSelectionIsWrongBufferCharacterNotFound));
        memset(EpollClientLoginMaster::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline,0x00,sizeof(EpollClientLoginMaster::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline));
        memset(EpollClientLoginMaster::characterSelectionIsWrongBufferServerInternalProblem,0x00,sizeof(EpollClientLoginMaster::characterSelectionIsWrongBufferServerInternalProblem));
        memset(EpollClientLoginMaster::characterSelectionIsWrongBufferServerNotFound,0x00,sizeof(EpollClientLoginMaster::characterSelectionIsWrongBufferServerNotFound));
        char tempBuff;

        tempBuff=2;
        //size will be the same
        EpollClientLoginMaster::characterSelectionIsWrongBufferSize=ProtocolParsingBase::computeReplyData(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    false,
                    #endif
                    EpollClientLoginMaster::characterSelectionIsWrongBufferCharacterNotFound,0,&tempBuff,1,-1/*not fixed size*/
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    ,ProtocolParsing::compressionTypeServer
                    #endif
                    );
        tempBuff=3;
        if(EpollClientLoginMaster::characterSelectionIsWrongBufferSize!=ProtocolParsingBase::computeReplyData(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    false,
                    #endif
                    EpollClientLoginMaster::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline,0,&tempBuff,1,-1/*not fixed size*/
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            ,ProtocolParsing::compressionTypeServer
            #endif
            ))
            abort();
        tempBuff=4;
        if(EpollClientLoginMaster::characterSelectionIsWrongBufferSize!=ProtocolParsingBase::computeReplyData(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    false,
                    #endif
                    EpollClientLoginMaster::characterSelectionIsWrongBufferServerInternalProblem,0,&tempBuff,1,-1/*not fixed size*/
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            ,ProtocolParsing::compressionTypeServer
            #endif
            ))
            abort();
        tempBuff=5;
        if(EpollClientLoginMaster::characterSelectionIsWrongBufferSize!=ProtocolParsingBase::computeReplyData(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    false,
                    #endif
                    EpollClientLoginMaster::characterSelectionIsWrongBufferServerNotFound,0,&tempBuff,1,-1/*not fixed size*/
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            ,ProtocolParsing::compressionTypeServer
            #endif
            ))
            abort();
    }
}

EpollServerLoginMaster::~EpollServerLoginMaster()
{
    if(EpollClientLoginMaster::private_token!=NULL)
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
    QHash<QString,CharactersGroup *>::const_iterator i = CharactersGroup::hash.constBegin();
    while (i != CharactersGroup::hash.constEnd()) {
        delete i.value();
        ++i;
    }
    CharactersGroup::hash.clear();
}

void EpollServerLoginMaster::loadLoginSettings(QSettings &settings)
{
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
    if(!settings.contains(QStringLiteral("token")))
        generateToken(settings);
    QString token=settings.value(QStringLiteral("token")).toString();
    if(token.size()!=(TOKEN_SIZE_FOR_MASTERAUTH*2))
        generateToken(settings);
    token=settings.value(QStringLiteral("token")).toString();
    memcpy(EpollClientLoginMaster::private_token,QByteArray::fromHex(token.toLatin1()).constData(),TOKEN_SIZE_FOR_MASTERAUTH);

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
    if(!settings.contains(QStringLiteral("automatic_account_creation")))
        settings.setValue(QStringLiteral("automatic_account_creation"),false);
    if(!settings.contains(QStringLiteral("character_delete_time")))
        settings.setValue(QStringLiteral("character_delete_time"),604800);
    if(!settings.contains(QStringLiteral("max_pseudo_size")))
        settings.setValue(QStringLiteral("max_pseudo_size"),20);
    if(!settings.contains(QStringLiteral("max_character")))
        settings.setValue(QStringLiteral("max_character"),3);
    if(!settings.contains(QStringLiteral("min_character")))
        settings.setValue(QStringLiteral("min_character"),1);
    CommonSettingsCommon::commonSettingsCommon.automatic_account_creation=settings.value(QStringLiteral("automatic_account_creation")).toBool();
    bool ok;
    CommonSettingsCommon::commonSettingsCommon.character_delete_time=settings.value(QStringLiteral("character_delete_time")).toUInt(&ok);
    if(CommonSettingsCommon::commonSettingsCommon.character_delete_time==0 || !ok)
    {
        std::cerr << "character_delete_time==0 (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.min_character=settings.value(QStringLiteral("min_character")).toUInt(&ok);
    if(!ok)
    {
        std::cerr << "CommonSettingsCommon::commonSettingsCommon.min_character not number (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.max_character=settings.value(QStringLiteral("max_character")).toUInt(&ok);
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
    CommonSettingsCommon::commonSettingsCommon.max_pseudo_size=settings.value(QStringLiteral("max_pseudo_size")).toUInt(&ok);
    if(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size==0 || !ok)
    {
        std::cerr << "max_pseudo_size==0 (abort)" << std::endl;
        abort();
    }
    if(!settings.contains(QStringLiteral("maxPlayerMonsters")))
        settings.setValue(QStringLiteral("maxPlayerMonsters"),8);
    if(!settings.contains(QStringLiteral("maxWarehousePlayerMonsters")))
        settings.setValue(QStringLiteral("maxWarehousePlayerMonsters"),30);
    if(!settings.contains(QStringLiteral("maxPlayerItems")))
        settings.setValue(QStringLiteral("maxPlayerItems"),30);
    if(!settings.contains(QStringLiteral("maxWarehousePlayerItems")))
        settings.setValue(QStringLiteral("maxWarehousePlayerItems"),150);
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters=settings.value(QStringLiteral("maxPlayerMonsters")).toUInt(&ok);
    if(CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters==0 || !ok)
    {
        std::cerr << "maxPlayerMonsters==0 (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters=settings.value(QStringLiteral("maxWarehousePlayerMonsters")).toUInt(&ok);
    if(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters==0 || !ok)
    {
        std::cerr << "maxWarehousePlayerMonsters==0 (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems=settings.value(QStringLiteral("maxPlayerItems")).toUInt(&ok);
    if(CommonSettingsCommon::commonSettingsCommon.maxPlayerItems==0 || !ok)
    {
        std::cerr << "maxPlayerItems==0 (abort)" << std::endl;
        abort();
    }
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems=settings.value(QStringLiteral("maxWarehousePlayerItems")).toUInt(&ok);
    if(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems==0 || !ok)
    {
        std::cerr << "maxWarehousePlayerItems==0 (abort)" << std::endl;
        abort();
    }
}

void EpollServerLoginMaster::loadDBLoginSettings(QSettings &settings)
{
    if(CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
    {
        QString db;
        QString host;
        QString login;
        QString pass;
        QString type;

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
        if(!settings.contains(QStringLiteral("comment")))
            settings.setValue(QStringLiteral("comment"),QStringLiteral("to do maxAccountId"));
        settings.sync();

        bool ok;
        //to load the dictionary
        {
            databaseBaseLogin=new EpollPostgresql();
            //here to have by login server an auth

            databaseBaseLogin->considerDownAfterNumberOfTry=settings.value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
            if(databaseBaseLogin->considerDownAfterNumberOfTry==0 || !ok)
            {
                std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
                abort();
            }
            db=settings.value(QStringLiteral("db")).toString();
            host=settings.value(QStringLiteral("host")).toString();
            login=settings.value(QStringLiteral("login")).toString();
            pass=settings.value(QStringLiteral("pass")).toString();
            databaseBaseLogin->tryInterval=settings.value(QStringLiteral("tryInterval")).toUInt(&ok);
            if(databaseBaseLogin->tryInterval==0 || !ok)
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
            if(!databaseBaseLogin->syncConnect(host.toUtf8().constData(),db.toUtf8().constData(),login.toUtf8().constData(),pass.toUtf8().constData()))
            {
                std::cerr << "Connect to login database failed:" << databaseBaseLogin->errorMessage() << std::endl;
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
        QString db;
        QString host;
        QString login;
        QString pass;
        QString type;

        settings.beginGroup(QStringLiteral("db-base"));
        if(!settings.contains(QStringLiteral("considerDownAfterNumberOfTry")))
            settings.setValue(QStringLiteral("considerDownAfterNumberOfTry"),30);
        if(!settings.contains(QStringLiteral("tryInterval")))
            settings.setValue(QStringLiteral("tryInterval"),1);
        if(!settings.contains(QStringLiteral("db")))
            settings.setValue(QStringLiteral("db"),QStringLiteral("catchchallenger_base"));
        if(!settings.contains(QStringLiteral("host")))
            settings.setValue(QStringLiteral("host"),QStringLiteral("localhost"));
        if(!settings.contains(QStringLiteral("login")))
            settings.setValue(QStringLiteral("login"),QStringLiteral("root"));
        if(!settings.contains(QStringLiteral("pass")))
            settings.setValue(QStringLiteral("pass"),QStringLiteral("root"));
        if(!settings.contains(QStringLiteral("type")))
            settings.setValue(QStringLiteral("type"),QStringLiteral("postgresql"));
        if(!settings.contains(QStringLiteral("comment")))
            settings.setValue(QStringLiteral("comment"),QStringLiteral("to do maxAccountId"));
        settings.sync();

        bool ok;
        //to load the dictionary
        {
            databaseBaseBase=new EpollPostgresql();
            //here to have by login server an auth

            databaseBaseBase->considerDownAfterNumberOfTry=settings.value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
            if(databaseBaseBase->considerDownAfterNumberOfTry==0 || !ok)
            {
                std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
                abort();
            }
            db=settings.value(QStringLiteral("db")).toString();
            host=settings.value(QStringLiteral("host")).toString();
            login=settings.value(QStringLiteral("login")).toString();
            pass=settings.value(QStringLiteral("pass")).toString();
            databaseBaseBase->tryInterval=settings.value(QStringLiteral("tryInterval")).toUInt(&ok);
            if(databaseBaseBase->tryInterval==0 || !ok)
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
            if(!databaseBaseBase->syncConnect(host.toUtf8().constData(),db.toUtf8().constData(),login.toUtf8().constData(),pass.toUtf8().constData()))
            {
                std::cerr << "Connect to login database failed:" << databaseBaseBase->errorMessage() << std::endl;
                abort();
            }
        }

        CharactersGroup::serverWaitedToBeReady++;
        settings.endGroup();
    }
}

QStringList EpollServerLoginMaster::loadCharactersGroup(QSettings &settings)
{
    QString db;
    QString host;
    QString login;
    QString pass;
    QString type;
    bool ok;

    QStringList charactersGroupList;
    bool continueCharactersGroupSettings=true;
    int charactersGroupId=0;
    while(continueCharactersGroupSettings)
    {
        settings.beginGroup(QStringLiteral("db-common-")+QString::number(charactersGroupId));
        if(charactersGroupId==0)
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
        }
        settings.sync();
        if(settings.contains(QStringLiteral("login")))
        {
            const QString &charactersGroup=settings.value(QStringLiteral("charactersGroup")).toString();
            if(!CharactersGroup::hash.contains(charactersGroup))
            {
                CharactersGroup::serverWaitedToBeReady++;
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
                CharactersGroup::hash[charactersGroup]=new CharactersGroup(db.toUtf8().constData(),host.toUtf8().constData(),login.toUtf8().constData(),pass.toUtf8().constData(),considerDownAfterNumberOfTry,tryInterval,charactersGroup);
                charactersGroupList << charactersGroup;
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
    charactersGroupList.sort();
    return charactersGroupList;
}

void EpollServerLoginMaster::charactersGroupListReply(QStringList &charactersGroupList)
{
    rawServerListForC211[0x00]=CommonSettingsCommon::commonSettingsCommon.automatic_account_creation;
    *reinterpret_cast<quint32 *>(rawServerListForC211+0x01)=(quint32)htole32((quint32)CommonSettingsCommon::commonSettingsCommon.character_delete_time);
    rawServerListForC211[0x05]=CommonSettingsCommon::commonSettingsCommon.min_character;
    rawServerListForC211[0x06]=CommonSettingsCommon::commonSettingsCommon.max_character;
    rawServerListForC211[0x07]=CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
    rawServerListForC211[0x08]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
    *reinterpret_cast<quint16 *>(rawServerListForC211+0x09)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters);
    rawServerListForC211[0x0B]=CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
    *reinterpret_cast<quint16 *>(rawServerListForC211+0x0C)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems);
    rawServerListForC211Size=0x0E;
    //do the Characters group
    rawServerListForC211[rawServerListForC211Size]=(unsigned char)charactersGroupList.size();
    rawServerListForC211Size+=sizeof(unsigned char);
    int index=0;
    while(index<charactersGroupList.size())
    {
        const QString &charactersGroupName=charactersGroupList.at(index);
        int newSize=0;
        if(!charactersGroupName.isEmpty())
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
        CharactersGroup::list << CharactersGroup::hash.value(charactersGroupName);
        CharactersGroup::list.last()->index=index;

        index++;
    }
}

void EpollServerLoginMaster::doTheLogicalGroup(QSettings &settings)
{
    //do the LogicalGroup
    char rawServerList[sizeof(EpollClientLoginMaster::serverLogicalGroupList)];
    memset(rawServerList,0x00,sizeof(rawServerList));
    int rawServerListSize=0x01;

    QString textToConvert;
    quint8 logicalGroup=0;
    bool logicalGroupContinue=true;
    while(logicalGroupContinue)
    {
        settings.beginGroup(QStringLiteral("logical-group-")+QString::number(logicalGroup));
        logicalGroupContinue=settings.contains(QStringLiteral("path")) && settings.contains(QStringLiteral("xml")) && logicalGroup<255;
        if(!logicalGroupContinue && logicalGroup==0)
        {
            if(!settings.contains(QStringLiteral("path")))
                settings.setValue(QStringLiteral("path"),QString());
            if(!settings.contains(QStringLiteral("xml")))
                settings.setValue(QStringLiteral("xml"),QStringLiteral("<name>root</name>"));
            logicalGroupContinue=true;
        }
        if(logicalGroupContinue)
        {
            //path
            {
                textToConvert=settings.value(QStringLiteral("path")).toString();
                if(textToConvert.size()>20)
                {
                    std::cerr << "path too hurge (abort)" << std::endl;
                    abort();
                }

                EpollClientLoginMaster::logicalGroupHash[textToConvert]=logicalGroup;

                const QByteArray &utf8data=textToConvert.toUtf8();
                if(utf8data.size()>255)
                {
                    std::cerr << "logical group converted too big (abort)" << std::endl;
                    abort();
                }
                rawServerList[rawServerListSize]=utf8data.size();
                rawServerListSize+=1;
                memcpy(rawServerList+rawServerListSize,utf8data.constData(),utf8data.size());
                rawServerListSize+=utf8data.size();
            }
            //translation
            {
                textToConvert=settings.value(QStringLiteral("xml")).toString();
                if(textToConvert.size()>4*1024)
                {
                    std::cerr << "translation too hurge (abort)" << std::endl;
                    abort();
                }
                int newSize=FacilityLibGeneral::toUTF8With16BitsHeader(textToConvert,rawServerList+rawServerListSize);
                if(newSize==0)
                {
                    std::cerr << "translation null or unable to translate in utf8 (abort)" << std::endl;
                    abort();
                }
                rawServerListSize+=newSize;
            }

            logicalGroup++;
        }
        settings.endGroup();
    }
    rawServerList[0x00]=logicalGroup;

    EpollClientLoginMaster::serverLogicalGroupListSize=ProtocolParsingBase::computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
            EpollClientLoginMaster::serverLogicalGroupList,
            0xC2,0x0F,rawServerList,rawServerListSize);
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
    memset(EpollClientLoginMaster::serverPartialServerList,0x00,sizeof(EpollClientLoginMaster::serverPartialServerList));
    int rawServerListSize=0x00;

    const int &serverListSize=EpollClientLoginMaster::gameServers.size();
    EpollClientLoginMaster::serverPartialServerList[rawServerListSize]=serverListSize;
    rawServerListSize+=1;
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
            EpollClientLoginMaster::serverPartialServerList[rawServerListSize]=gameServerOnEpollClientLoginMaster->charactersGroupForGameServer->index;
            rawServerListSize+=1;
        }
        //key
        {
            *reinterpret_cast<quint32 *>(EpollClientLoginMaster::serverPartialServerList+rawServerListSize)=htole32(gameServerOnCharactersGroup->uniqueKey);
            rawServerListSize+=sizeof(gameServerOnCharactersGroup->uniqueKey);
        }
        //host
        {
            int newSize=FacilityLibGeneral::toUTF8WithHeader(gameServerOnCharactersGroup->host,EpollClientLoginMaster::serverPartialServerList+rawServerListSize);
            if(newSize==0)
            {
                std::cerr << "host null or unable to translate in utf8 (abort)" << std::endl;
                abort();
            }
            rawServerListSize+=newSize;
        }
        //port
        {
            *reinterpret_cast<unsigned short int *>(EpollClientLoginMaster::serverPartialServerList+rawServerListSize)=(unsigned short int)htole16((unsigned short int)gameServerOnCharactersGroup->port);
            rawServerListSize+=sizeof(unsigned short int);
        }
        //metaData
        {
            if(gameServerOnCharactersGroup->metaData.size()>4*1024)
            {
                std::cerr << "metaData too hurge (abort)" << std::endl;
                abort();
            }
            {
                const QByteArray &utf8data=gameServerOnCharactersGroup->metaData.toUtf8();
                if(utf8data.size()>65535)
                {
                    *reinterpret_cast<quint16 *>(EpollClientLoginMaster::serverPartialServerList+rawServerListSize)=0;
                    rawServerListSize+=2;
                }
                else
                {
                    *reinterpret_cast<quint16 *>(EpollClientLoginMaster::serverPartialServerList+rawServerListSize)=(quint16)htole16((quint16)utf8data.size());
                    rawServerListSize+=2;
                    memcpy(EpollClientLoginMaster::serverPartialServerList+rawServerListSize,utf8data.constData(),utf8data.size());
                    rawServerListSize+=utf8data.size();
                }
            }
        }
        //logicalGroup
        {
            EpollClientLoginMaster::serverPartialServerList[rawServerListSize]=gameServerOnCharactersGroup->logicalGroupIndex;
            rawServerListSize+=1;
        }
        //max player
        *reinterpret_cast<unsigned short int *>(EpollClientLoginMaster::serverPartialServerList+rawServerListSize)=(unsigned short int)htole16(gameServerOnCharactersGroup->maxPlayer);
        rawServerListSize+=sizeof(unsigned short int);

        serverListIndex++;
    }
    EpollClientLoginMaster::serverPartialServerListSize=rawServerListSize;

    //Second list part with same size
    serverListIndex=0;
    while(serverListIndex<serverListSize)
    {
        const EpollClientLoginMaster * const gameServerOnEpollClientLoginMaster=EpollClientLoginMaster::gameServers.at(serverListIndex);
        const CharactersGroup::InternalGameServer * const gameServerOnCharactersGroup=gameServerOnEpollClientLoginMaster->charactersGroupForGameServerInformation;
        //connected player
        *reinterpret_cast<unsigned short int *>(EpollClientLoginMaster::serverPartialServerList+rawServerListSize)=(unsigned short int)htole16(gameServerOnCharactersGroup->currentPlayer);
        rawServerListSize+=sizeof(unsigned short int);

        serverListIndex++;
    }

    EpollClientLoginMaster::serverServerListSize=ProtocolParsingBase::computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
            EpollClientLoginMaster::serverServerList,
            0xC2,0x10,EpollClientLoginMaster::serverPartialServerList,rawServerListSize);
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

void EpollServerLoginMaster::generateToken(QSettings &settings)
{
    FILE *fpRandomFile = fopen("/dev/urandom","rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open /dev/urandom to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(EpollClientLoginMaster::private_token,1,TOKEN_SIZE_FOR_MASTERAUTH,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_MASTERAUTH)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_MASTERAUTH << " needed to do the token from /dev/urandom" << std::endl;
        abort();
    }
    settings.setValue(QStringLiteral("token"),QString(QByteArray(EpollClientLoginMaster::private_token,TOKEN_SIZE_FOR_MASTERAUTH).toHex()));
    fclose(fpRandomFile);
}

void EpollServerLoginMaster::load_account_max_id()
{
    QString queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `account` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QLatin1String("SELECT id FROM account ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM account ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText.toLatin1(),this,&EpollServerLoginMaster::load_account_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
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
        EpollClientLoginMaster::maxAccountId=QString(databaseBaseLogin->value(0)).toUInt(&ok);
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
    QTime time;
    time.restart();
    CommonDatapack::commonDatapack.parseDatapack("datapack/");
    qDebug() << QStringLiteral("Loaded the common datapack into %1ms").arg(time.elapsed());

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
    int skinId=0;
    while(skinId<CommonDatapack::commonDatapack.skins.size())
    {
        *reinterpret_cast<quint16 *>(rawServerListForC211+rawServerListForC211Size)=htole16(BaseServerMasterLoadDictionary::dictionary_skin_internal_to_database.value(skinId));
        rawServerListForC211Size+=2;
        skinId++;
    }

    //profile list size
    rawServerListForC211[rawServerListForC211Size]=CommonDatapack::commonDatapack.profileList.size();
    rawServerListForC211Size+=1;
    int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
        {
            //database id
            *reinterpret_cast<quint16 *>(rawServerListForC211+rawServerListForC211Size)=htole16(dictionary_starter_internal_to_database.at(index));
            rawServerListForC211Size+=sizeof(quint16);
            //skin
            rawServerListForC211[rawServerListForC211Size]=profile.forcedskin.size();
            rawServerListForC211Size+=1;
            {
                int skinListIndex=0;
                while(skinListIndex<profile.forcedskin.size())
                {
                    rawServerListForC211[rawServerListForC211Size]=profile.forcedskin.at(skinListIndex);
                    rawServerListForC211Size+=1;
                    skinListIndex++;
                }
            }
            //cash
            *reinterpret_cast<quint64 *>(rawServerListForC211+rawServerListForC211Size)=htole64(profile.cash);
            rawServerListForC211Size+=sizeof(quint64);

            //monster
            rawServerListForC211[rawServerListForC211Size]=profile.monsters.size();
            rawServerListForC211Size+=1;
            {
                int monsterListIndex=0;
                while(monsterListIndex<profile.monsters.size())
                {
                    const Profile::Monster &playerMonster=profile.monsters.at(monsterListIndex);

                    //monster
                    *reinterpret_cast<quint16 *>(rawServerListForC211+rawServerListForC211Size)=htole16(playerMonster.id);
                    rawServerListForC211Size+=sizeof(quint16);
                    //level
                    rawServerListForC211[rawServerListForC211Size]=playerMonster.level;
                    rawServerListForC211Size+=1;
                    //captured with
                    *reinterpret_cast<quint16 *>(rawServerListForC211+rawServerListForC211Size)=htole16(playerMonster.captured_with);
                    rawServerListForC211Size+=sizeof(quint16);

                    const Monster &monster=CommonDatapack::commonDatapack.monsters.value(playerMonster.id);
                    const Monster::Stat &monsterStat=CommonFightEngineBase::getStat(monster,playerMonster.level);
                    const QList<CatchChallenger::PlayerMonster::PlayerSkill> &skills=CommonFightEngineBase::generateWildSkill(monster,playerMonster.level);

                    //hp
                    *reinterpret_cast<quint32 *>(rawServerListForC211+rawServerListForC211Size)=htole32(monsterStat.hp);
                    rawServerListForC211Size+=sizeof(quint32);
                    //gender
                    rawServerListForC211[rawServerListForC211Size]=(qint8)monster.ratio_gender;
                    rawServerListForC211Size+=sizeof(quint8);

                    //skill list
                    rawServerListForC211[rawServerListForC211Size]=skills.size();
                    rawServerListForC211Size+=1;
                    int skillListIndex=0;
                    while(skillListIndex<skills.size())
                    {
                        const CatchChallenger::PlayerMonster::PlayerSkill &skill=skills.at(skillListIndex);
                        //skill
                        *reinterpret_cast<quint16 *>(rawServerListForC211+rawServerListForC211Size)=htole16(skill.skill);
                        rawServerListForC211Size+=sizeof(quint16);
                        //skill level
                        rawServerListForC211[rawServerListForC211Size]=skill.level;
                        rawServerListForC211Size+=sizeof(quint8);
                        //skill endurance
                        rawServerListForC211[rawServerListForC211Size]=skill.endurance;
                        rawServerListForC211Size+=sizeof(quint8);
                        skillListIndex++;
                    }

                    monsterListIndex++;
                }
            }

            {
                //reputation
                rawServerListForC211[rawServerListForC211Size]=profile.reputation.size();
                rawServerListForC211Size+=sizeof(quint8);
                int reputationIndex=0;
                while(reputationIndex<profile.reputation.size())
                {
                    const Profile::Reputation &reputation=profile.reputation.at(reputationIndex);
                    //type
                    *reinterpret_cast<quint16 *>(rawServerListForC211+rawServerListForC211Size)=htole16(CommonDatapack::commonDatapack.reputation[reputation.reputationId].reverse_database_id);
                    rawServerListForC211Size+=sizeof(quint16);
                    //level
                    rawServerListForC211[rawServerListForC211Size]=reputation.level;
                    rawServerListForC211Size+=sizeof(quint8);
                    //point
                    *reinterpret_cast<quint32 *>(rawServerListForC211+rawServerListForC211Size)=htole32(reputation.point);
                    rawServerListForC211Size+=sizeof(quint32);
                    reputationIndex++;
                }
            }

            {
                //item
                rawServerListForC211[rawServerListForC211Size]=profile.items.size();
                rawServerListForC211Size+=sizeof(quint8);
                int reputationIndex=0;
                while(reputationIndex<profile.items.size())
                {
                    const Profile::Item &reputation=profile.items.at(reputationIndex);
                    //item id
                    *reinterpret_cast<quint16 *>(rawServerListForC211+rawServerListForC211Size)=htole16(reputation.id);
                    rawServerListForC211Size+=sizeof(quint16);
                    //quantity
                    *reinterpret_cast<quint32 *>(rawServerListForC211+rawServerListForC211Size)=htole32(reputation.quantity);
                    rawServerListForC211Size+=sizeof(quint32);
                    reputationIndex++;
                }
            }
        }
        index++;
    }

    memcpy(rawServerListForC211+rawServerListForC211Size,CommonSettingsCommon::commonSettingsCommon.datapackHashBase.constData(),CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
    rawServerListForC211Size+=CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size();
    //CommonSettingsCommon::commonSettingsCommon.datapackHashBase.clear();no memory gain

    EpollClientLoginMaster::loginSettingsAndCharactersGroupSize=ProtocolParsingBase::computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
            EpollClientLoginMaster::loginSettingsAndCharactersGroup,
            0xC2,0x11,rawServerListForC211,rawServerListForC211Size);
    if(EpollClientLoginMaster::loginSettingsAndCharactersGroupSize==0)
    {
        std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort)" << std::endl;
        abort();
    }

    CommonDatapack::commonDatapack.unload();
    baseServerMasterSendDatapack.unload();
    BaseServerMasterLoadDictionary::unload();

    doTheReplyCache();
}
