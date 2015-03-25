#include "EpollServerLoginSlave.h"
#include "CharactersGroupForLogin.h"

using namespace CatchChallenger;

#include <QFile>
#include <QByteArray>

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <iostream>

#include "EpollClientLoginSlave.h"
#include "../../general/base/ProtocolParsing.h"

EpollServerLoginSlave::EpollServerLoginSlave() :
    tcpNodelay(false),
    tcpCork(false),
    serverReady(false),
    server_ip(NULL),
    server_port(NULL)
{
    QSettings settings("login_slave.conf",QSettings::IniFormat);

    {
        memset(EpollClientLoginSlave::serverServerList,0x00,sizeof(EpollClientLoginSlave::serverServerList));
        memset(EpollClientLoginSlave::serverLogicalGroupList,0x00,sizeof(EpollClientLoginSlave::serverLogicalGroupList));
        memset(EpollClientLoginSlave::serverLogicalGroupAndServerList,0x00,sizeof(EpollClientLoginSlave::serverLogicalGroupAndServerList));
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
    if(!settings.contains(QStringLiteral("token")))
        generateToken(settings);
    QString token=settings.value(QStringLiteral("token")).toString();
    if(token.size()!=(TOKEN_SIZE*2))
        generateToken(settings);
    token=settings.value(QStringLiteral("token")).toString();
    memcpy(EpollClientLoginSlave::private_token,QByteArray::fromHex(token.toLatin1()).constData(),TOKEN_SIZE);
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
            EpollClientLoginSlave::proxyMode=EpollClientLoginSlave::ProxyMode::Direct;
        else
            EpollClientLoginSlave::proxyMode=EpollClientLoginSlave::ProxyMode::Proxy;
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

    QString mysql_db;
    QString mysql_host;
    QString mysql_login;
    QString mysql_pass;
    QString type;
    bool ok;
    //here to have by login server an auth
    {
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
        EpollClientLoginSlave::databaseBaseLogin.considerDownAfterNumberOfTry=settings.value(QStringLiteral("considerDownAfterNumberOfTry")).toUInt(&ok);
        if(EpollClientLoginSlave::databaseBaseLogin.considerDownAfterNumberOfTry==0 || !ok)
        {
            std::cerr << "considerDownAfterNumberOfTry==0 (abort)" << std::endl;
            abort();
        }
        mysql_db=settings.value(QStringLiteral("mysql_db")).toString();
        mysql_host=settings.value(QStringLiteral("mysql_host")).toString();
        mysql_login=settings.value(QStringLiteral("mysql_login")).toString();
        mysql_pass=settings.value(QStringLiteral("mysql_pass")).toString();
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
        if(!EpollClientLoginSlave::databaseBaseLogin.syncConnect(mysql_host.toUtf8().constData(),mysql_db.toUtf8().constData(),mysql_login.toUtf8().constData(),mysql_pass.toUtf8().constData()))
        {
            std::cerr << "Connect to login database failed:" << EpollClientLoginSlave::databaseBaseLogin.errorMessage() << std::endl;
            abort();
        }
        settings.endGroup();
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
            if(!settings.contains(QStringLiteral("CharactersGroupForLogin")))
                settings.setValue(QStringLiteral("CharactersGroupForLogin"),QString());
            if(!settings.contains(QStringLiteral("comment")))
                settings.setValue(QStringLiteral("comment"),QStringLiteral("to do maxClanId, maxCharacterId, maxMonsterId"));
        }
        settings.sync();
        if(settings.contains(QStringLiteral("mysql_login")))
        {
            const QString &charactersGroup=settings.value(QStringLiteral("CharactersGroupForLogin")).toString();
            if(!CharactersGroupForLogin::databaseBaseCommonList.contains(charactersGroup))
            {
                CharactersGroupForLogin::serverWaitedToBeReady++;
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
                CharactersGroupForLogin::databaseBaseCommonList[charactersGroup]=new CharactersGroupForLogin(mysql_db.toUtf8().constData(),mysql_host.toUtf8().constData(),mysql_login.toUtf8().constData(),mysql_pass.toUtf8().constData(),considerDownAfterNumberOfTry,tryInterval);
                charactersGroupForLoginList << charactersGroup;
            }
            else
                std::cerr << "CharactersGroupForLogin already found for group " << CharactersGroupForLoginId << std::endl;
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
                std::cerr << "only db type postgresql supported (abort)" << std::endl;
                abort();
            }
            EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup[EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize]=(unsigned char)data.size();
            EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=sizeof(unsigned char);
            memcpy(EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup+EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize,data.constData(),data.size());
            EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize+=data.size();
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
            settings.setValue(QStringLiteral("considerDownAfterNumberOfTry"),30);
        if(!settings.contains(QStringLiteral("tryInterval")))
            settings.setValue(QStringLiteral("tryInterval"),1);
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
        const int &linkfd=LoginLinkToMaster::tryConnect(host.toLocal8Bit().constData(),port,tryInterval,considerDownAfterNumberOfTry);
        if(linkfd<0)
        {
            std::cerr << "Unable to connect on master" << std::endl;
            abort();
        }
        #ifdef SERVERSSL
            EpollClientLoginSlave::linkToMaster=new LoginLinkToMaster(linkfd,ctx);
        #else
            EpollClientLoginSlave::linkToMaster=new LoginLinkToMaster(linkfd);
        #endif
        settings.endGroup();
    }

    settings.sync();
}

EpollServerLoginSlave::~EpollServerLoginSlave()
{
    if(EpollClientLoginSlave::private_token!=NULL)
        memset(EpollClientLoginSlave::private_token,0x00,TOKEN_SIZE);
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
    if(EpollClientLoginSlave::linkToMaster!=NULL)
    {
        delete EpollClientLoginSlave::linkToMaster;
        EpollClientLoginSlave::linkToMaster=NULL;
    }
}

void EpollServerLoginSlave::close()
{
    if(EpollClientLoginSlave::linkToMaster!=NULL)
    {
        delete EpollClientLoginSlave::linkToMaster;
        EpollClientLoginSlave::linkToMaster=NULL;
    }
    EpollGenericServer::close();
}

void EpollServerLoginSlave::SQL_common_load_finish()
{
    serverReady=true;
}

bool EpollServerLoginSlave::tryListen()
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

void EpollServerLoginSlave::generateToken(QSettings &settings)
{
    FILE *fpRandomFile = fopen("/dev/urandom","rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open /dev/urandom to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(EpollClientLoginSlave::private_token,1,TOKEN_SIZE,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE << " needed to do the token from /dev/urandom" << std::endl;
        abort();
    }
    settings.setValue(QStringLiteral("token"),QString(QByteArray(EpollClientLoginSlave::private_token,TOKEN_SIZE).toHex()));
    fclose(fpRandomFile);
}

