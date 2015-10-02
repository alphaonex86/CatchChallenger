#include "EpollServerLoginSlave.h"
#include "../epoll/Epoll.h"
#include "../base/PreparedDBQuery.h"

using namespace CatchChallenger;

#include <QFile>
#include <std::vector<char>>
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

EpollServerLoginSlave::EpollServerLoginSlave() :
    tcpNodelay(false),
    tcpCork(false),
    serverReady(false),
    server_ip(NULL),
    server_port(NULL)
{
    std::unordered_settings settings(QCoreApplication::applicationDirPath()+"/gateway.conf",std::unordered_settings::IniFormat);

    srand(time(NULL));

    {
        if(!settings.contains(std::stringLiteral("ip")))
            settings.setValue(std::stringLiteral("ip"),std::string());
        const std::string &server_ip_string=settings.value(std::stringLiteral("ip")).toString();
        const std::vector<char> &server_ip_data=server_ip_string.toLocal8Bit();
        if(!server_ip_string.isEmpty())
        {
            server_ip=new char[server_ip_data.size()+1];
            strcpy(server_ip,server_ip_data.constData());
        }
        if(!settings.contains(std::stringLiteral("port")))
            settings.setValue(std::stringLiteral("port"),rand()%40000+10000);
        const std::vector<char> &server_port_data=settings.value(std::stringLiteral("port")).toString().toLocal8Bit();
        server_port=new char[server_port_data.size()+1];
        strcpy(server_port,server_port_data.constData());
    }

    {
        if(!settings.contains(std::stringLiteral("destination_ip")))
            settings.setValue(std::stringLiteral("destination_ip"),std::string());
        const std::string &destination_server_ip_string=settings.value(std::stringLiteral("destination_ip")).toString();
        const std::vector<char> &destination_server_ip_data=destination_server_ip_string.toLocal8Bit();
        if(!destination_server_ip_string.isEmpty())
        {
            destination_server_ip=new char[destination_server_ip_data.size()+1];
            strcpy(destination_server_ip,destination_server_ip_data.constData());
        }
        else
        {
            settings.sync();
            std::cerr << "destination_ip is empty (abort)" << std::endl;
            abort();
        }
        if(!settings.contains(std::stringLiteral("destination_port")))
            settings.setValue(std::stringLiteral("destination_port"),rand()%40000+10000);
        bool ok;
        unsigned int tempPort=settings.value(std::stringLiteral("destination_port")).toUInt(&ok);
        if(!ok)
        {
            settings.sync();
            std::cerr << "destination_port not number: " << settings.value(std::stringLiteral("destination_port")).toString().toStdString() << std::endl;
            abort();
        }
        if(tempPort==0 || tempPort>65535)
        {
            settings.sync();
            std::cerr << "destination_port ==0 || >65535: " << tempPort << std::endl;
            abort();
        }
        destination_server_port=tempPort;
    }

    if(!settings.contains(std::stringLiteral("httpDatapackMirrorRewriteBase")))
        settings.setValue(std::stringLiteral("httpDatapackMirrorRewriteBase"),std::string());
    LinkToGameServer::httpDatapackMirrorRewriteBase=std::vector<char>(ProtocolParsingBase::tempBigBufferForOutput,
                                                               FacilityLibGeneral::toUTF8WithHeader(httpMirrorFix(settings.value("httpDatapackMirrorRewriteBase").toString().toStdString()),ProtocolParsingBase::tempBigBufferForOutput));
    if(LinkToGameServer::httpDatapackMirrorRewriteBase.isEmpty())
    {
        settings.sync();
        std::cerr << "httpDatapackMirrorRewriteBase.isEmpty() abort" << std::endl;
        abort();
    }
    if(!settings.contains(std::stringLiteral("httpDatapackMirrorRewriteMainAndSub")))
        settings.setValue(std::stringLiteral("httpDatapackMirrorRewriteMainAndSub"),std::string());
    LinkToGameServer::httpDatapackMirrorRewriteMainAndSub=std::vector<char>(ProtocolParsingBase::tempBigBufferForOutput,
                                                                     FacilityLibGeneral::toUTF8WithHeader(httpMirrorFix(settings.value("httpDatapackMirrorRewriteMainAndSub").toString().toStdString()),ProtocolParsingBase::tempBigBufferForOutput));
    if(LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.isEmpty())
    {
        settings.sync();
        std::cerr << "httpDatapackMirrorRewriteMainAndSub.isEmpty() abort" << std::endl;
        abort();
    }

    //connection
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    if(!settings.contains(std::stringLiteral("compression")))
        settings.setValue(std::stringLiteral("compression"),std::stringLiteral("zlib"));
    if(settings.value(std::stringLiteral("compression")).toString()==std::stringLiteral("none"))
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::None;
    else if(settings.value(std::stringLiteral("compression")).toString()==std::stringLiteral("xz"))
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Xz;
    else if(settings.value(std::stringLiteral("compression")).toString()==std::stringLiteral("lz4"))
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Lz4;
    else
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Zlib;
    ProtocolParsing::compressionLevel          = settings.value(std::stringLiteral("compressionLevel")).toUInt();
    #endif

    settings.beginGroup(std::stringLiteral("Linux"));
    if(!settings.contains(std::stringLiteral("tcpCork")))
        settings.setValue(std::stringLiteral("tcpCork"),true);
    if(!settings.contains(std::stringLiteral("tcpNodelay")))
        settings.setValue(std::stringLiteral("tcpNodelay"),false);
    tcpCork=settings.value(std::stringLiteral("tcpCork")).toBool();
    tcpNodelay=settings.value(std::stringLiteral("tcpNodelay")).toBool();
    settings.endGroup();

    settings.beginGroup(std::stringLiteral("commandUpdateDatapack"));
    if(!settings.contains(std::stringLiteral("base")))
        settings.setValue(std::stringLiteral("base"),std::string());
    if(!settings.contains(std::stringLiteral("main")))
        settings.setValue(std::stringLiteral("main"),std::string());
    if(!settings.contains(std::stringLiteral("sub")))
        settings.setValue(std::stringLiteral("sub"),std::string());
    DatapackDownloaderBase::commandUpdateDatapackBase=settings.value(std::stringLiteral("base")).toString().toStdString();
    DatapackDownloaderMainSub::commandUpdateDatapackMain=settings.value(std::stringLiteral("main")).toString().toStdString();
    DatapackDownloaderMainSub::commandUpdateDatapackSub=settings.value(std::stringLiteral("sub")).toString().toStdString();
    settings.endGroup();

    settings.sync();

    tryListen();

    {
        const std::vector<std::string> &extensionAllowedTemp=stringsplit(CATCHCHALLENGER_EXTENSION_ALLOWED,';');
        unsigned int index=0;
        while(index<extensionAllowedTemp.size())
        {
            DatapackDownloaderMainSub::extensionAllowed.insert(extensionAllowedTemp.at(index));
            index++;
        }
    }
    {
        const std::vector<std::string> &extensionAllowedTemp=stringsplit(CATCHCHALLENGER_EXTENSION_COMPRESSED,';');
        unsigned int index=0;
        while(index<extensionAllowedTemp.size())
        {
            EpollClientLoginSlave::compressedExtension.insert(extensionAllowedTemp.at(index));
            index++;
        }
    }
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

size_t EpollServerLoginSlave::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = static_cast<char *>(realloc(mem->memory, mem->size + realsize + 1));
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

std::string EpollServerLoginSlave::httpMirrorFix(const std::string & mirrors)
{
    if(mirrors.empty())
        return std::string();
    std::vector<std::string> newMirrorList;
    std::regex httpMatch("^https?://.+$");
    const std::vector<std::string> &mirrorList=stringsplit(mirrors,';');
    unsigned int index=0;
    while(index<mirrorList.size())
    {
        const std::string &mirror=mirrorList.at(index);
        if(!std::regex_match(mirror,httpMatch))
        {
            std::cerr << "Mirror wrong: " << mirror << std::endl;
            //abort();//prevent the server crash/close the gateway
        }
        else
        {
            if(stringEndsWith(mirror,"/"))
                newMirrorList.push_back(mirror);
            else
                newMirrorList.push_back(mirror+"/");
        }
        index++;
    }
    return stringimplode(newMirrorList,';');
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

