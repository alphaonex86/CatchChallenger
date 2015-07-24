#include "EpollServerLoginSlave.h"
#include "../epoll/Epoll.h"
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
    QSettings settings(QCoreApplication::applicationDirPath()+"/login.conf",QSettings::IniFormat);

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

    if(!settings.contains(QStringLiteral("httpDatapackMirrorRewriteBase")))
        settings.setValue(QStringLiteral("httpDatapackMirrorRewriteBase"),QString());
    QString httpDatapackMirrorRewriteBase=httpMirrorFix(settings.value(QStringLiteral("httpDatapackMirrorRewriteBase")).toString());
    if(!settings.contains(QStringLiteral("httpDatapackMirrorRewriteMainAndSub")))
        settings.setValue(QStringLiteral("httpDatapackMirrorRewriteMainAndSub"),QString());
    QString httpDatapackMirrorRewriteMainAndSub=httpMirrorFix(settings.value(QStringLiteral("httpDatapackMirrorRewriteMainAndSub")).toString());

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
}

EpollServerLoginSlave::~EpollServerLoginSlave()
{
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
}

QString EpollServerLoginSlave::httpMirrorFix(QString mirrors)
{
    QStringList newMirrorList;
    QRegularExpression httpMatch("^https?://.+$");
    const QStringList &mirrorList=mirrors.split(";");
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
    mirrors=newMirrorList.join(";");
    return mirrors;
}

void EpollServerLoginSlave::close()
{
    EpollGenericServer::close();
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
