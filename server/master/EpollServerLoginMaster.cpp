#include "EpollServerLoginMaster.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonDatapack.h"
#include "../VariableServer.h"

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

EpollServerLoginMaster::EpollServerLoginMaster() :
    server_ip(NULL),
    server_port(NULL),
    rawServerListForC20011(static_cast<char *>(malloc(sizeof(EpollClientLoginMaster::loginSettingsAndCharactersGroup)))),
    rawServerListForC20011Size(0),
    databaseBaseLogin(NULL),
    character_delete_time(3600),
    min_character(0),
    max_character(3),
    max_pseudo_size(20)
{
    {
        //empty buffer
        memset(EpollClientLoginMaster::replyToRegisterLoginServer+2,0x00,sizeof(EpollClientLoginMaster::replyToRegisterLoginServer)-2);
        memset(EpollClientLoginMaster::serverServerList,0x00,sizeof(EpollClientLoginMaster::serverServerList));
        memset(EpollClientLoginMaster::serverLogicalGroupList,0x00,sizeof(EpollClientLoginMaster::serverLogicalGroupList));
        memset(rawServerListForC20011,0x00,sizeof(EpollClientLoginMaster::loginSettingsAndCharactersGroup));
    }

    QSettings settings("login_master.conf",QSettings::IniFormat);

    srand(time(NULL));

    loadLoginSettings(settings);
    loadDBLoginSettings(settings);
    QStringList charactersGroupList=loadCharactersGroup(settings);
    charactersGroupListReply(charactersGroupList);
    doTheLogicalGroup(settings);
    doTheServerList();
    doTheReplyCache();
    loadTheProfile();
}

EpollServerLoginMaster::~EpollServerLoginMaster()
{
    if(EpollClientLoginMaster::private_token!=NULL)
        memset(EpollClientLoginMaster::private_token,0x00,TOKEN_SIZE);
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
        delete databaseBaseLogin;
        databaseBaseLogin=NULL;
    }
    if(rawServerListForC20011!=NULL)
    {
        delete rawServerListForC20011;
        rawServerListForC20011=NULL;
        rawServerListForC20011Size=0;
    }
    QHash<QString,CharactersGroup *>::const_iterator i = CharactersGroup::charactersGroupHash.constBegin();
    while (i != CharactersGroup::charactersGroupHash.constEnd()) {
        delete i.value();
        ++i;
    }
    CharactersGroup::charactersGroupHash.clear();
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
    if(token.size()!=(TOKEN_SIZE*2))
        generateToken(settings);
    token=settings.value(QStringLiteral("token")).toString();
    memcpy(EpollClientLoginMaster::private_token,QByteArray::fromHex(token.toLatin1()).constData(),TOKEN_SIZE);

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
    EpollClientLoginMaster::automatic_account_creation=settings.value(QStringLiteral("automatic_account_creation")).toBool();
    character_delete_time=settings.value(QStringLiteral("character_delete_time")).toUInt();
    if(character_delete_time==0)
    {
        std::cerr << "character_delete_time==0 (abort)" << std::endl;
        abort();
    }
    min_character=settings.value(QStringLiteral("min_character")).toUInt();
    max_character=settings.value(QStringLiteral("max_character")).toUInt();
    if(max_character<=0)
    {
        std::cerr << "max_character<=0 (abort)" << std::endl;
        abort();
    }
    if(max_character<min_character)
    {
        std::cerr << "max_character<min_character (abort)" << std::endl;
        abort();
    }
    max_pseudo_size=settings.value(QStringLiteral("max_pseudo_size")).toUInt();
    if(max_pseudo_size==0)
    {
        std::cerr << "max_pseudo_size==0 (abort)" << std::endl;
        abort();
    }
}

void EpollServerLoginMaster::loadDBLoginSettings(QSettings &settings)
{
    QString mysql_db;
    QString mysql_host;
    QString mysql_login;
    QString mysql_pass;
    QString type;
    bool ok;
    if(EpollClientLoginMaster::automatic_account_creation)
    {
        CharactersGroup::serverWaitedToBeReady++;
        databaseBaseLogin=new EpollPostgresql();
        //here to have by login server an auth
        settings.beginGroup(QStringLiteral("db-login"));
        if(!settings.contains(QStringLiteral("considerDownAfterNumberOfTry")))
            settings.setValue(QStringLiteral("considerDownAfterNumberOfTry"),30);
        if(!settings.contains(QStringLiteral("tryInterval")))
            settings.setValue(QStringLiteral("tryInterval"),1);
        if(!settings.contains(QStringLiteral("mysql_db")))
            settings.setValue(QStringLiteral("mysql_db"),QStringLiteral("catchchallenger_login"));
        if(!settings.contains(QStringLiteral("mysql_host")))
            settings.setValue(QStringLiteral("mysql_host"),QStringLiteral("localhost"));
        if(!settings.contains(QStringLiteral("mysql_login")))
            settings.setValue(QStringLiteral("mysql_login"),QStringLiteral("root"));
        if(!settings.contains(QStringLiteral("mysql_pass")))
            settings.setValue(QStringLiteral("mysql_pass"),QStringLiteral("root"));
        if(!settings.contains(QStringLiteral("type")))
            settings.setValue(QStringLiteral("type"),QStringLiteral("postgresql"));
        if(!settings.contains(QStringLiteral("comment")))
            settings.setValue(QStringLiteral("comment"),QStringLiteral("to do maxAccountId"));
        settings.sync();
        databaseBaseLogin->considerDownAfterNumberOfTry=settings.value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
        if(databaseBaseLogin->considerDownAfterNumberOfTry==0 || !ok)
        {
            std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
            abort();
        }
        mysql_db=settings.value(QStringLiteral("mysql_db")).toString();
        mysql_host=settings.value(QStringLiteral("mysql_host")).toString();
        mysql_login=settings.value(QStringLiteral("mysql_login")).toString();
        mysql_pass=settings.value(QStringLiteral("mysql_pass")).toString();
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
        if(!databaseBaseLogin->syncConnect(mysql_host.toUtf8().constData(),mysql_db.toUtf8().constData(),mysql_login.toUtf8().constData(),mysql_pass.toUtf8().constData()))
        {
            std::cerr << "Connect to login database failed:" << databaseBaseLogin->errorMessage() << std::endl;
            abort();
        }
        settings.endGroup();
        load_account_max_id();
    }
}

QStringList EpollServerLoginMaster::loadCharactersGroup(QSettings &settings)
{
    QString mysql_db;
    QString mysql_host;
    QString mysql_login;
    QString mysql_pass;
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
            if(!settings.contains(QStringLiteral("mysql_db")))
                settings.setValue(QStringLiteral("mysql_db"),QStringLiteral("catchchallenger_common"));
            if(!settings.contains(QStringLiteral("mysql_host")))
                settings.setValue(QStringLiteral("mysql_host"),QStringLiteral("localhost"));
            if(!settings.contains(QStringLiteral("mysql_login")))
                settings.setValue(QStringLiteral("mysql_login"),QStringLiteral("root"));
            if(!settings.contains(QStringLiteral("mysql_pass")))
                settings.setValue(QStringLiteral("mysql_pass"),QStringLiteral("root"));
            if(!settings.contains(QStringLiteral("type")))
                settings.setValue(QStringLiteral("type"),QStringLiteral("postgresql"));
            if(!settings.contains(QStringLiteral("charactersGroup")))
                settings.setValue(QStringLiteral("charactersGroup"),QString());
            if(!settings.contains(QStringLiteral("comment")))
                settings.setValue(QStringLiteral("comment"),QStringLiteral("to do maxClanId, maxCharacterId, maxMonsterId"));
        }
        settings.sync();
        if(settings.contains(QStringLiteral("mysql_login")))
        {
            const QString &charactersGroup=settings.value(QStringLiteral("charactersGroup")).toString();
            if(!CharactersGroup::charactersGroupHash.contains(charactersGroup))
            {
                CharactersGroup::serverWaitedToBeReady++;
                const quint8 &considerDownAfterNumberOfTry=settings.value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
                if(considerDownAfterNumberOfTry==0 || !ok)
                {
                    std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
                    abort();
                }
                mysql_db=settings.value(QStringLiteral("mysql_db")).toString();
                mysql_host=settings.value(QStringLiteral("mysql_host")).toString();
                mysql_login=settings.value(QStringLiteral("mysql_login")).toString();
                mysql_pass=settings.value(QStringLiteral("mysql_pass")).toString();
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
                CharactersGroup::charactersGroupHash[charactersGroup]=new CharactersGroup(mysql_db.toUtf8().constData(),mysql_host.toUtf8().constData(),mysql_login.toUtf8().constData(),mysql_pass.toUtf8().constData(),considerDownAfterNumberOfTry,tryInterval);
                charactersGroupList << charactersGroup;
            }
            else
                std::cerr << "charactersGroup already found for group " << charactersGroupId << std::endl;
            charactersGroupId++;
        }
        else
            continueCharactersGroupSettings=false;
        settings.endGroup();
    }
    return charactersGroupList;
}

void EpollServerLoginMaster::charactersGroupListReply(QStringList &charactersGroupList)
{
    charactersGroupList.sort();

    rawServerListForC20011[0x00]=EpollClientLoginMaster::automatic_account_creation;
    *reinterpret_cast<quint32 *>(rawServerListForC20011+0x03)=(quint32)htobe32((quint32)character_delete_time);
    rawServerListForC20011[0x05]=min_character;
    rawServerListForC20011[0x06]=max_character;
    rawServerListForC20011[0x07]=max_pseudo_size;
    rawServerListForC20011Size=0x08;
    //do the Characters group
    rawServerListForC20011[rawServerListForC20011Size]=(unsigned char)charactersGroupList.size();
    rawServerListForC20011Size+=sizeof(unsigned char);
    int index=0;
    while(index<charactersGroupList.size())
    {
        const QString &charactersGroupName=charactersGroupList.at(index);
        const int &newSize=FacilityLibGeneral::toUTF8WithHeader(charactersGroupName,rawServerListForC20011);
        if(newSize==0 || charactersGroupName.size()>20)
        {
            std::cerr << "charactersGroupName to hurge, null or unable to translate in utf8 (abort)" << std::endl;
            abort();
        }
        rawServerListForC20011Size+=newSize;
        index++;
        CharactersGroup::charactersGroupList << CharactersGroup::charactersGroupHash.value(charactersGroupName);
    }
}

void EpollServerLoginMaster::doTheLogicalGroup(QSettings &settings)
{
    //do the LogicalGroup
    char rawServerList[sizeof(EpollClientLoginMaster::serverLogicalGroupList)];
    memset(rawServerList,0x00,sizeof(rawServerList));
    int rawServerListSize=0x01;

    QString textToConvert;
    int logicalGroup=0;
    bool logicalGroupContinue=true;
    while(logicalGroupContinue)
    {
        settings.beginGroup(QStringLiteral("logical-group-")+QString::number(logicalGroup));
        logicalGroupContinue=settings.contains(QStringLiteral("path")) && settings.contains(QStringLiteral("translation")) && logicalGroup<255;
        if(logicalGroupContinue)
        {
            //path
            textToConvert=settings.value(QStringLiteral("path")).toString();
            if(textToConvert.size()>20)
            {
                std::cerr << "path too hurge (abort)" << std::endl;
                abort();
            }
            int newSize=FacilityLibGeneral::toUTF8WithHeader(textToConvert,rawServerList+rawServerListSize);
            if(newSize==0)
            {
                std::cerr << "path null or unable to translate in utf8 (abort)" << std::endl;
                abort();
            }
            rawServerListSize+=newSize;
            //translation
            textToConvert=settings.value(QStringLiteral("translation")).toString();
            if(textToConvert.size()>4*1024)
            {
                std::cerr << "translation too hurge (abort)" << std::endl;
                abort();
            }
            newSize=FacilityLibGeneral::toUTF8With16BitsHeader(textToConvert,rawServerList+rawServerListSize);
            if(newSize==0)
            {
                std::cerr << "translation null or unable to translate in utf8 (abort)" << std::endl;
                abort();
            }
            rawServerListSize+=newSize;
        }
        logicalGroup++;
        settings.endGroup();
    }
    rawServerList[0x00]=logicalGroup;

    EpollClientLoginMaster::serverLogicalGroupListSize=ProtocolParsingBase::computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
            EpollClientLoginMaster::serverLogicalGroupList,
            0xC2,0x000F,rawServerList,rawServerListSize);
    if(EpollClientLoginMaster::serverLogicalGroupListSize==0)
    {
        std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort)" << std::endl;
        abort();
    }
}

void EpollServerLoginMaster::doTheServerList()
{
    //do the server list
    char rawServerList[sizeof(EpollClientLoginMaster::serverServerList)];
    memset(rawServerList,0x00,sizeof(rawServerList));
    int rawServerListSize=0x00;

    const int &serverListSize=0x01;
    rawServerList[rawServerListSize]=serverListSize;
    rawServerListSize+=1;
    int serverListIndex=0;
    while(serverListIndex<serverListSize)
    {
        const QString &charactersGroup="Test";
        const char key[]={0x00,0x00,0x00,0x00};
        const QString &host="localhost";
        const unsigned short int port=9999;
        const QString &metaData="<name>Server test 4</name><name lang=\"fr\">Serveur de teste 4</name><description>Description in english</description><description lang=\"fr\">Description en français</description> (xml attached)";
        const QString &logicalGroup="folder/path (group)";

        //charactersGroup
        if(charactersGroup.size()>20)
        {
            std::cerr << "charactersGroup too hurge (abort)" << std::endl;
            abort();
        }
        int newSize=FacilityLibGeneral::toUTF8WithHeader(charactersGroup,rawServerList+rawServerListSize);
        if(newSize==0)
        {
            std::cerr << "charactersGroup null or unable to translate in utf8 (abort)" << std::endl;
            abort();
        }
        rawServerListSize+=newSize;
        //key
        memcpy(rawServerList+rawServerListSize,key,sizeof(key));
        rawServerListSize+=sizeof(key);
        //host
        newSize=FacilityLibGeneral::toUTF8WithHeader(host,rawServerList+rawServerListSize);
        if(newSize==0)
        {
            std::cerr << "host null or unable to translate in utf8 (abort)" << std::endl;
            abort();
        }
        rawServerListSize+=newSize;
        //port
        *reinterpret_cast<unsigned short int *>(rawServerList+rawServerListSize)=(unsigned short int)htobe16((unsigned short int)port);
        rawServerListSize+=sizeof(unsigned short int);
        //metaData
        if(metaData.size()>4*1024)
        {
            std::cerr << "metaData too hurge (abort)" << std::endl;
            abort();
        }
        newSize=FacilityLibGeneral::toUTF8With16BitsHeader(metaData,rawServerList+rawServerListSize);
        if(newSize==0)
        {
            std::cerr << "translation null or unable to translate in utf8 (abort)" << std::endl;
            abort();
        }
        rawServerListSize+=newSize;
        //logicalGroup
        if(logicalGroup.size()>20)
        {
            std::cerr << "logicalGroup too hurge (abort)" << std::endl;
            abort();
        }
        newSize=FacilityLibGeneral::toUTF8WithHeader(charactersGroup,rawServerList+rawServerListSize);
        if(newSize==0)
        {
            std::cerr << "charactersGroup null or unable to translate in utf8 (abort)" << std::endl;
            abort();
        }
        rawServerListSize+=newSize;

        serverListIndex++;
    }

    EpollClientLoginMaster::serverServerListSize=ProtocolParsingBase::computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
            EpollClientLoginMaster::serverServerList,
            0xC2,0x0010,rawServerList,rawServerListSize);
    if(EpollClientLoginMaster::serverServerListSize==0)
    {
        std::cerr << "EpollClientLoginMaster::serverServerListSize==0 (abort)" << std::endl;
        abort();
    }
}

void EpollServerLoginMaster::doTheReplyCache()
{
    //do the reply cache
    EpollClientLoginMaster::loginPreviousToReplyCacheSize=0;
    memcpy(EpollClientLoginMaster::loginPreviousToReplyCache,
           EpollClientLoginMaster::loginSettingsAndCharactersGroup,
           EpollClientLoginMaster::loginSettingsAndCharactersGroupSize);
    EpollClientLoginMaster::loginPreviousToReplyCacheSize+=EpollClientLoginMaster::loginSettingsAndCharactersGroupSize;
    memcpy(EpollClientLoginMaster::loginPreviousToReplyCache,
           EpollClientLoginMaster::serverLogicalGroupList,
           EpollClientLoginMaster::serverLogicalGroupListSize);
    EpollClientLoginMaster::loginPreviousToReplyCacheSize+=EpollClientLoginMaster::serverLogicalGroupListSize;
    memcpy(EpollClientLoginMaster::loginPreviousToReplyCache,
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
    const int &returnedSize=fread(EpollClientLoginMaster::private_token,1,TOKEN_SIZE,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE << " needed to do the token from /dev/urandom" << std::endl;
        abort();
    }
    settings.setValue(QStringLiteral("token"),QString(QByteArray(EpollClientLoginMaster::private_token,TOKEN_SIZE).toHex()));
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
    loadTheDatapack();
    /*databaseBaseLogin->syncDisconnect();
    databaseBaseLogin=NULL;
    CharactersGroup::serverWaitedToBeReady--;*/
}

void EpollServerLoginMaster::loadTheDatapack()
{
    QTime time;
    time.restart();
    CommonDatapack::commonDatapack.parseDatapack("datapack/");
    qDebug() << QStringLiteral("Loaded the common datapack into %1ms").arg(time.elapsed());

    SQL_common_load_start();
}

void EpollServerLoginMaster::SQL_common_load_finish()
{
    loadTheDatapackFileList();
}

void EpollServerLoginMaster::loadTheDatapackFileList()
{
    QStringList extensionAllowedTemp=(QString(CATCHCHALLENGER_EXTENSION_ALLOWED)+QString(";")+QString(CATCHCHALLENGER_EXTENSION_COMPRESSED)).split(";");
    EpollClientLoginMaster::extensionAllowed=extensionAllowedTemp.toSet();
    QStringList compressedExtensionAllowedTemp=QString(CATCHCHALLENGER_EXTENSION_COMPRESSED).split(";");
    EpollClientLoginMaster::compressedExtension=compressedExtensionAllowedTemp.toSet();
    EpollClientLoginMaster::datapack_file_list_cache=datapack_file_list();
    QRegularExpression datapack_rightFileName = QRegularExpression(DATAPACK_FILE_REGEX);

    QCryptographicHash hash(QCryptographicHash::Sha224);
    QStringList datapack_file_temp=EpollClientLoginMaster::datapack_file_list_cache.keys();
    datapack_file_temp.sort();
    int index=0;
    while(index<datapack_file_temp.size()) {
        QFile file("datapack/"+datapack_file_temp.at(index));
        if(datapack_file_temp.at(index).contains(datapack_rightFileName))
        {
            if(file.open(QIODevice::ReadOnly))
            {
                const QByteArray &data=file.readAll();
                {
                    QCryptographicHash hashFile(QCryptographicHash::Sha224);
                    hashFile.addData(data);
                    EpollClientLoginMaster::DatapackCacheFile cacheFile;
                    cacheFile.mtime=QFileInfo(file).lastModified().toTime_t();
                    cacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                    EpollClientLoginMaster::datapack_file_hash_cache[datapack_file_temp.at(index)]=cacheFile;
                }
                hash.addData(data);
                file.close();
            }
            else
            {
                std::cerr << "Stop now! Unable to open the file " << file.fileName().toStdString() << " to do the datapack checksum for the mirror" << std::endl;
                abort();
            }
        }
        else
            std::cerr << "File excluded because don't match the regex: " << file.fileName() << std::endl;
        index++;
    }
    datapackHash=hash.result();
    std::cout << datapack_file_temp.size() << "file for datapack loaded" << std::endl;

    loadTheProfile();
}

QHash<QString,quint32> EpollServerLoginMaster::datapack_file_list()
{
    QHash<QString,quint32> filesList;

    const QStringList &returnList=FacilityLibGeneral::listFolder("datapack/");
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
                QFile file("datapack/"+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        #ifdef Q_OS_WIN32
                        fileName.replace(EpollClientLoginMaster::text_antislash,EpollClientLoginMaster::text_slash);//remplace if is under windows server
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

void EpollServerLoginMaster::loadTheProfile()
{


    //profile list size
    rawServerListForC20011[rawServerListForC20011Size]=CommonDatapack::commonDatapack.profileList.size();
    rawServerListForC20011Size+=1;

    put in cache the reply

    EpollClientLoginMaster::loginSettingsAndCharactersGroupSize=ProtocolParsingBase::computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
            EpollClientLoginMaster::loginSettingsAndCharactersGroup,
            0xC2,0x0011,rawServerListForC20011,rawServerListForC20011Size);
    if(EpollClientLoginMaster::loginSettingsAndCharactersGroupSize==0)
    {
        std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort)" << std::endl;
        abort();
    }

    if(rawServerListForC20011!=NULL)
    {
        delete rawServerListForC20011;
        rawServerListForC20011=NULL;
        rawServerListForC20011Size=0;
    }
}
