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
#include "DatapackDownloaderBase.h"
#include "DatapackDownloaderMainSub.h"
#include "../base/DictionaryLogin.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/FacilityLibGeneral.h"

EpollServerLoginSlave *EpollServerLoginSlave::epollServerLoginSlave=NULL;
CURL *EpollServerLoginSlave::curl=NULL;

EpollServerLoginSlave::EpollServerLoginSlave() :
    tcpNodelay(false),
    tcpCork(false),
    serverReady(false),
    server_ip(NULL),
    server_port(NULL)
{
    QSettings settings(QCoreApplication::applicationDirPath()+"/login.conf",QSettings::IniFormat);

    srand(time(NULL));

    EpollServerLoginSlave::curl=curl_easy_init();
    if(!EpollServerLoginSlave::curl)
    {
        std::cerr << "curl_easy_init() failed abort" << std::endl;
        abort();
    }

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
    }

    {
        if(!settings.contains(QStringLiteral("destination_ip")))
            settings.setValue(QStringLiteral("destination_ip"),QString());
        const QString &destination_server_ip_string=settings.value(QStringLiteral("destination_ip")).toString();
        const QByteArray &destination_server_ip_data=destination_server_ip_string.toLocal8Bit();
        if(!destination_server_ip_string.isEmpty())
        {
            destination_server_ip=new char[destination_server_ip_data.size()+1];
            strcpy(destination_server_ip,destination_server_ip_data.constData());
        }
        if(!settings.contains(QStringLiteral("destination_port")))
            settings.setValue(QStringLiteral("destination_port"),rand()%40000+10000);
        bool ok;
        unsigned int tempPort=settings.value(QStringLiteral("destination_port")).toUInt(&ok);
        if(!ok)
        {
            std::cerr << "destination_port not number: " << settings.value(QStringLiteral("destination_port")).toString().toStdString() << std::endl;
            abort();
        }
        if(tempPort==0 || tempPort>65535)
        {
            std::cerr << "destination_port ==0 || >65535: " << tempPort << std::endl;
            abort();
        }
        destination_server_port=tempPort;
    }

    if(!settings.contains(QStringLiteral("httpDatapackMirrorRewriteBase")))
        settings.setValue(QStringLiteral("httpDatapackMirrorRewriteBase"),QString());
    LinkToGameServer::httpDatapackMirrorRewriteBase=FacilityLibGeneral::toUTF8WithHeader(httpMirrorFix(settings.value(QStringLiteral("httpDatapackMirrorRewriteBase")).toString()));
    if(LinkToGameServer::httpDatapackMirrorRewriteBase.isEmpty())
    {
        std::cerr << "httpDatapackMirrorRewriteBase.isEmpty() abort" << std::endl;
        abort();
    }
    if(!settings.contains(QStringLiteral("httpDatapackMirrorRewriteMainAndSub")))
        settings.setValue(QStringLiteral("httpDatapackMirrorRewriteMainAndSub"),QString());
    LinkToGameServer::httpDatapackMirrorRewriteMainAndSub=FacilityLibGeneral::toUTF8WithHeader(httpMirrorFix(settings.value(QStringLiteral("httpDatapackMirrorRewriteMainAndSub")).toString()));
    if(LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.isEmpty())
    {
        std::cerr << "httpDatapackMirrorRewriteMainAndSub.isEmpty() abort" << std::endl;
        abort();
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

    settings.beginGroup(QStringLiteral("commandUpdateDatapack"));
    if(!settings.contains(QStringLiteral("base")))
        settings.setValue(QStringLiteral("base"),QString());
    if(!settings.contains(QStringLiteral("main")))
        settings.setValue(QStringLiteral("main"),QString());
    if(!settings.contains(QStringLiteral("sub")))
        settings.setValue(QStringLiteral("sub"),QString());
    DatapackDownloaderBase::commandUpdateDatapackBase=settings.value(QStringLiteral("base")).toString();
    DatapackDownloaderMainSub::commandUpdateDatapackMain=settings.value(QStringLiteral("main")).toString();
    DatapackDownloaderMainSub::commandUpdateDatapackSub=settings.value(QStringLiteral("sub")).toString();
    settings.endGroup();

    settings.sync();

    tryListen();

    DatapackDownloaderMainSub::extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    DatapackDownloaderBase::extensionAllowed=DatapackDownloaderMainSub::extensionAllowed;
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
    if(mirrors.isEmpty())
        return QString();
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

