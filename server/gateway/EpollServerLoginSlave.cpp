#include "EpollServerLoginSlave.h"
#include "../epoll/Epoll.h"
#include "../base/PreparedDBQuery.h"

using namespace CatchChallenger;

#include <vector>
#include <string>

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
#include "../base/TinyXMLSettings.h"

EpollServerLoginSlave *EpollServerLoginSlave::epollServerLoginSlave=NULL;

EpollServerLoginSlave::EpollServerLoginSlave() :
    tcpNodelay(false),
    tcpCork(false),
    serverReady(false),
    server_ip(NULL),
    server_port(NULL)
{
    TinyXMLSettings settings(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/gateway.xml");

    srand(time(NULL));

    {
        if(!settings.contains("ip"))
            settings.setValue("ip","");
        const std::string &server_ip_stdstring=settings.value("ip");
        if(!server_ip_stdstring.empty())
        {
            server_ip=new char[server_ip_stdstring.size()+1];
            strcpy(server_ip,server_ip_stdstring.data());
        }
        if(!settings.contains("port"))
            settings.setValue("port",rand()%40000+10000);
        const std::string &server_port_stdstring=settings.value("port");
        server_port=new char[server_port_stdstring.size()+1];
        strcpy(server_port,server_port_stdstring.data());
    }

    {
        if(!settings.contains("destination_ip"))
            settings.setValue("destination_ip","");
        const std::string &destination_server_ip_stdstring=settings.value("destination_ip");
        if(!destination_server_ip_stdstring.empty())
        {
            destination_server_ip=new char[destination_server_ip_stdstring.size()+1];
            strcpy(destination_server_ip,destination_server_ip_stdstring.data());
        }
        else
        {
            settings.sync();
            std::cerr << "destination_ip is empty (abort)" << std::endl;
            abort();
        }
        if(!settings.contains("destination_port"))
            settings.setValue("destination_port",rand()%40000+10000);
        bool ok;
        unsigned int tempPort=stringtouint16(settings.value("destination_port"),&ok);
        if(!ok)
        {
            settings.sync();
            std::cerr << "destination_port not number: " << settings.value("destination_port") << std::endl;
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

    if(!settings.contains("httpDatapackMirrorRewriteBase"))
        settings.setValue("httpDatapackMirrorRewriteBase","");
    LinkToGameServer::httpDatapackMirrorRewriteBase.resize(256+1);
    LinkToGameServer::httpDatapackMirrorRewriteBase.resize(
                FacilityLibGeneral::toUTF8WithHeader(
                    httpMirrorFix(settings.value("httpDatapackMirrorRewriteBase")),
                    LinkToGameServer::httpDatapackMirrorRewriteBase.data()
                    )
                );
    if(LinkToGameServer::httpDatapackMirrorRewriteBase.empty())
    {
        settings.sync();
        std::cerr << "httpDatapackMirrorRewriteBase.isEmpty() abort" << std::endl;
        abort();
    }
    if(!settings.contains("httpDatapackMirrorRewriteMainAndSub"))
        settings.setValue("httpDatapackMirrorRewriteMainAndSub","");
    LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.resize(256+1);
    LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.resize(
                FacilityLibGeneral::toUTF8WithHeader(
                    httpMirrorFix(settings.value("httpDatapackMirrorRewriteMainAndSub")),
                    LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.data()
                    )
                );
    if(LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.empty())
    {
        settings.sync();
        std::cerr << "httpDatapackMirrorRewriteMainAndSub.isEmpty() abort" << std::endl;
        abort();
    }

    //connection
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    if(!settings.contains("compression"))
        settings.setValue("compression","zlib");
    if(!settings.contains("compressionLevel"))
        settings.setValue("compressionLevel","6");
    if(settings.value("compression")=="none")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::None;
    else if(settings.value("compression")=="xz")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Xz;
    else if(settings.value("compression")=="lz4")
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Lz4;
    else
        ProtocolParsing::compressionTypeServer          = ProtocolParsing::CompressionType::Zlib;
    ProtocolParsing::compressionLevel          = stringtouint8(settings.value("compressionLevel"));
    #endif

    settings.beginGroup("Linux");
    if(!settings.contains("tcpCork"))
        settings.setValue("tcpCork",true);
    if(!settings.contains("tcpNodelay"))
        settings.setValue("tcpNodelay",false);
    tcpCork=stringtobool(settings.value("tcpCork"));
    tcpNodelay=stringtobool(settings.value("tcpNodelay"));
    settings.endGroup();

    settings.beginGroup("commandUpdateDatapack");
    if(!settings.contains("base"))
        settings.setValue("base","");
    if(!settings.contains("main"))
        settings.setValue("main","");
    if(!settings.contains("sub"))
        settings.setValue("sub","");
    DatapackDownloaderBase::commandUpdateDatapackBase=settings.value("base");
    DatapackDownloaderMainSub::commandUpdateDatapackMain=settings.value("main");
    DatapackDownloaderMainSub::commandUpdateDatapackSub=settings.value("sub");
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
        if(!regex_search(mirror,httpMatch))
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

