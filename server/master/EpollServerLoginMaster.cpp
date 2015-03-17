#include "EpollServerLoginMaster.h"

using namespace CatchChallenger;

#include <QFile>
#include <QByteArray>

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <iostream>

/// \todo the back id get

#include "EpollClientLoginMaster.h"

EpollServerLoginMaster::EpollServerLoginMaster() :
    server_ip(NULL),
    server_port(NULL),
    databaseBaseLogin(NULL),
    databaseBaseCommon(NULL)
{
    QSettings settings("login_master.conf",QSettings::IniFormat);

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
    if(!settings.contains(QStringLiteral("token")))
        generateToken(settings);
    QString token=settings.value(QStringLiteral("token")).toString();
    if(token.size()!=(TOKEN_SIZE*2))
        generateToken(settings);
    token=settings.value(QStringLiteral("token")).toString();
    memcpy(EpollClientLoginMaster::private_token,QByteArray::fromHex(token.toLatin1()).constData(),TOKEN_SIZE);

    //connection
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
    EpollClientLoginMaster::replyToRegisterLoginServer[0x02]=settings.value(QStringLiteral("automatic_account_creation")).toBool();
    const quint32 &character_delete_time=settings.value(QStringLiteral("character_delete_time")).toUInt();
    if(character_delete_time==0)
    {
        std::cerr << "character_delete_time==0 (abort)" << std::endl;
        abort();
    }
    *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToRegisterLoginServer+0x03)=(quint32)htobe32((quint32)character_delete_time);
    const quint8 &min_character=settings.value(QStringLiteral("min_character")).toUInt();
    EpollClientLoginMaster::replyToRegisterLoginServer[0x07]=min_character;
    const quint8 &max_character=settings.value(QStringLiteral("max_character")).toUInt();
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
    EpollClientLoginMaster::replyToRegisterLoginServer[0x08]=max_character;
    const quint8 &max_pseudo_size=settings.value(QStringLiteral("max_pseudo_size")).toUInt();
    if(max_pseudo_size==0)
    {
        std::cerr << "max_pseudo_size==0 (abort)" << std::endl;
        abort();
    }
    EpollClientLoginMaster::replyToRegisterLoginServer[0x09]=max_pseudo_size;

    databaseBaseLogin=new EpollPostgresql();
    databaseBaseCommon=new EpollPostgresql();
    QString mysql_db;
    QString mysql_host;
    QString mysql_login;
    QString mysql_pass;
    QString type;
    bool ok;
    //here to have by login server an auth
    settings.beginGroup(QStringLiteral("db-login"));
    if(!settings.contains(QStringLiteral("considerDownAfterNumberOfTry")))
        settings.setValue(QStringLiteral("considerDownAfterNumberOfTry"),3);
    if(!settings.contains(QStringLiteral("tryInterval")))
        settings.setValue(QStringLiteral("tryInterval"),5);
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

    settings.beginGroup(QStringLiteral("db-common"));
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
    settings.sync();
    databaseBaseCommon->considerDownAfterNumberOfTry=settings.value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
    if(databaseBaseCommon->considerDownAfterNumberOfTry==0 || !ok)
    {
        std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
        abort();
    }
    mysql_db=settings.value(QStringLiteral("mysql_db")).toString();
    mysql_host=settings.value(QStringLiteral("mysql_host")).toString();
    mysql_login=settings.value(QStringLiteral("mysql_login")).toString();
    mysql_pass=settings.value(QStringLiteral("mysql_pass")).toString();
    databaseBaseCommon->tryInterval=settings.value(QStringLiteral("tryInterval")).toUInt(&ok);
    if(databaseBaseCommon->tryInterval==0 || !ok)
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
    if(!databaseBaseCommon->syncConnect(mysql_host.toUtf8().constData(),mysql_db.toUtf8().constData(),mysql_login.toUtf8().constData(),mysql_pass.toUtf8().constData()))
    {
        std::cerr << "Connect to login database failed:" << databaseBaseCommon->errorMessage() << std::endl;
        abort();
    }
    settings.endGroup();

    load_clan_max_id();
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
    if(databaseBaseCommon!=NULL)
    {
        delete databaseBaseCommon;
        databaseBaseCommon=NULL;
    }
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

void EpollServerLoginMaster::load_clan_max_id()
{
    EpollClientLoginMaster::maxClanId=0;
    QString queryText;
    switch(databaseBaseCommon->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `clan` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&EpollServerLoginMaster::load_clan_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        abort();
    }
}

void EpollServerLoginMaster::load_clan_max_id_static(void *object)
{
    static_cast<EpollServerLoginMaster *>(object)->load_clan_max_id_return();
}

void EpollServerLoginMaster::load_clan_max_id_return()
{
    EpollClientLoginMaster::maxClanId=0;
    while(databaseBaseCommon->next())
    {
        bool ok;
        //not +1 because incremented before use
        EpollClientLoginMaster::maxClanId=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(!ok)
        {
            std::cerr << "Max clan id is failed to convert to number" << std::endl;
            abort();
        }
    }
    load_account_max_id();
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
    load_character_max_id();
}

void EpollServerLoginMaster::load_character_max_id()
{
    QString queryText;
    switch(databaseBaseCommon->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `character` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&EpollServerLoginMaster::load_character_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        abort();
    }
    EpollClientLoginMaster::maxCharacterId=0;
}

void EpollServerLoginMaster::load_character_max_id_static(void *object)
{
    static_cast<EpollServerLoginMaster *>(object)->load_character_max_id_return();
}

void EpollServerLoginMaster::load_character_max_id_return()
{
    while(databaseBaseCommon->next())
    {
        bool ok;
        //not +1 because incremented before use
        EpollClientLoginMaster::maxCharacterId=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(!ok)
        {
            std::cerr << "Max character id is failed to convert to number" << std::endl;
            abort();
        }
    }
    load_monsters_max_id();
}

void EpollServerLoginMaster::load_monsters_max_id()
{
    EpollClientLoginMaster::maxMonsterId=1;
    QString queryText;
    switch(databaseBaseCommon->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `monster` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QLatin1String("SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM monster ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&EpollServerLoginMaster::load_monsters_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        abort();//stop because can't do the first db access
    }
    return;
}

void EpollServerLoginMaster::load_monsters_max_id_static(void *object)
{
    static_cast<EpollServerLoginMaster *>(object)->load_monsters_max_id_return();
}

void EpollServerLoginMaster::load_monsters_max_id_return()
{
    while(databaseBaseCommon->next())
    {
        bool ok;
        //not +1 because incremented before use
        EpollClientLoginMaster::maxMonsterId=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(!ok)
        {
            std::cerr << "Max monster id is failed to convert to number" << std::endl;
            abort();
        }
    }
    tryListen();
}
