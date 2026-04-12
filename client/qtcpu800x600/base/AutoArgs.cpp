#include "AutoArgs.h"

#include <cstring>
#include <iostream>

QString AutoArgs::server;
bool AutoArgs::autologin=false;
QString AutoArgs::character;
bool AutoArgs::closeWhenOnMap=false;
bool AutoArgs::dropSendDataAfterOnMap=false;

void AutoArgs::parse(int &argc, char *argv[])
{
    int writeIndex=1;
    int i=1;
    while(i<argc)
    {
        const char * const arg=argv[i];
        if(std::strcmp(arg,"--autologin")==0)
        {
            autologin=true;
            i++;
            continue;
        }
        if(std::strcmp(arg,"--closewhenonmap")==0)
        {
            closeWhenOnMap=true;
            i++;
            continue;
        }
        if(std::strcmp(arg,"--dropsenddataafteronmap")==0)
        {
            dropSendDataAfterOnMap=true;
            i++;
            continue;
        }
        if(std::strcmp(arg,"--server")==0 && (i+1)<argc)
        {
            server=QString::fromUtf8(argv[i+1]);
            i+=2;
            continue;
        }
        if(std::strncmp(arg,"--server=",9)==0)
        {
            server=QString::fromUtf8(arg+9);
            i++;
            continue;
        }
        if(std::strcmp(arg,"--character")==0 && (i+1)<argc)
        {
            character=QString::fromUtf8(argv[i+1]);
            i+=2;
            continue;
        }
        if(std::strncmp(arg,"--character=",12)==0)
        {
            character=QString::fromUtf8(arg+12);
            i++;
            continue;
        }
        argv[writeIndex++]=argv[i];
        i++;
    }
    argc=writeIndex;
    argv[argc]=nullptr;
    std::cerr << "AutoArgs: server=\"" << server.toStdString()
              << "\" autologin=" << (autologin?"true":"false")
              << " character=\"" << character.toStdString() << "\""
              << " closeWhenOnMap=" << (closeWhenOnMap?"true":"false")
              << " dropSendDataAfterOnMap=" << (dropSendDataAfterOnMap?"true":"false") << std::endl;
}
