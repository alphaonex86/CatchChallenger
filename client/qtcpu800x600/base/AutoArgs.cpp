#include "AutoArgs.h"

#include <cstring>
#include <cstdlib>
#include <iostream>

QString AutoArgs::server;
QString AutoArgs::host;
uint16_t AutoArgs::port=0;
bool AutoArgs::autologin=false;
QString AutoArgs::character;
bool AutoArgs::closeWhenOnMap=false;
int AutoArgs::closeWhenOnMapAfter=0;
bool AutoArgs::dropSendDataAfterOnMap=false;
bool AutoArgs::autosolo=false;

void AutoArgs::printHelp(const char *progName)
{
    const char *name=(progName!=nullptr && progName[0]!='\0')?progName:"client-qtcpu800x600";
    std::cout
        << "Usage: " << name << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --help                     Show this help and exit.\n"
        << "  --server=NAME              Select the server with the given name.\n"
        << "  --server NAME              Same as above.\n"
        << "  --host HOST                Host or IP to connect (requires --port).\n"
        << "  --port PORT                Port number to connect (requires --host).\n"
        << "  --autologin                Automatically press the login button.\n"
        << "  --character=NAME           Auto-select a character by pseudo.\n"
        << "  --character NAME           Same as above.\n"
        << "  --closewhenonmap           Quit 1s after the first map is loaded.\n"
        << "  --closewhenonmapafter=N    On map: toggle direction each 1s, quit after N seconds.\n"
        << "  --dropsenddataafteronmap   Drop outgoing traffic after the first map is loaded.\n"
        << "  --autosolo                 Load the first savegame, enter the game; on 10s\n"
        << "                             timeout dump character/current map and close.\n"
        << "  quit                       Request all running instances to quit.\n";
    std::cout.flush();
}

void AutoArgs::parse(int &argc, char *argv[])
{
    const char * const progName=(argc>0)?argv[0]:"";
    int writeIndex=1;
    int i=1;
    while(i<argc)
    {
        const char * const arg=argv[i];
        if(std::strcmp(arg,"--help")==0 || std::strcmp(arg,"-h")==0)
        {
            printHelp(progName);
            std::exit(0);
        }
        if(std::strcmp(arg,"--autosolo")==0)
        {
            autosolo=true;
            i++;
            continue;
        }
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
        if(std::strncmp(arg,"--closewhenonmapafter=",22)==0)
        {
            closeWhenOnMapAfter=std::atoi(arg+22);
            if(closeWhenOnMapAfter<1)
                closeWhenOnMapAfter=1;
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
        if(std::strcmp(arg,"--host")==0 && (i+1)<argc)
        {
            host=QString::fromUtf8(argv[i+1]);
            i+=2;
            continue;
        }
        if(std::strncmp(arg,"--host=",7)==0)
        {
            host=QString::fromUtf8(arg+7);
            i++;
            continue;
        }
        if(std::strcmp(arg,"--port")==0 && (i+1)<argc)
        {
            port=static_cast<uint16_t>(std::atoi(argv[i+1]));
            i+=2;
            continue;
        }
        if(std::strncmp(arg,"--port=",7)==0)
        {
            port=static_cast<uint16_t>(std::atoi(arg+7));
            i++;
            continue;
        }
        argv[writeIndex++]=argv[i];
        i++;
    }
    argc=writeIndex;
    argv[argc]=nullptr;
    std::cerr << "AutoArgs: server=\"" << server.toStdString() << "\""
              << " host=\"" << host.toStdString() << "\""
              << " port=" << port
              << " autologin=" << (autologin?"true":"false")
              << " character=\"" << character.toStdString() << "\""
              << " closeWhenOnMap=" << (closeWhenOnMap?"true":"false")
              << " closeWhenOnMapAfter=" << closeWhenOnMapAfter
              << " dropSendDataAfterOnMap=" << (dropSendDataAfterOnMap?"true":"false")
              << " autosolo=" << (autosolo?"true":"false") << std::endl;
}
