#include "AutoArgs.h"

#include <cstring>
#include <cstdlib>
#include <iostream>

QString AutoArgs::server;
QString AutoArgs::host;
uint16_t AutoArgs::port=0;
QString AutoArgs::url;
bool AutoArgs::autologin=false;
QString AutoArgs::character;
bool AutoArgs::closeWhenOnMap=false;
int AutoArgs::closeWhenOnMapAfter=0;
bool AutoArgs::dropSendDataAfterOnMap=false;
bool AutoArgs::autosolo=false;
QString AutoArgs::mainDatapackCodeOverride;

void AutoArgs::printHelp(const char *progName)
{
    const char *name=(progName!=nullptr && progName[0]!='\0')?progName:"client-qtcpu800x600";
    std::cout
        << "Usage: " << name << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --help                     Show this help and exit.\n"
        << "  --server=NAME              Select an existing server-list entry by name (TCP or WS).\n"
        << "  --server NAME              Same as above.\n"
#ifndef CATCHCHALLENGER_NO_TCPSOCKET
        << "  --host HOST                TCP direct-connect host/IP (requires --port).\n"
        << "  --port PORT                TCP direct-connect port (requires --host).\n"
#endif
#ifndef CATCHCHALLENGER_NO_WEBSOCKET
        << "  --url URL                  WebSocket direct-connect ws:// or wss:// URL.\n"
#endif
        << "  --autologin                Automatically press the login button.\n"
        << "  --character=NAME           Auto-select a character by pseudo.\n"
        << "  --character NAME           Same as above.\n"
        << "  --closewhenonmap           Quit 1s after the first map is loaded.\n"
        << "  --closewhenonmapafter=N    On map: toggle direction each 1s, quit after N seconds.\n"
        << "  --dropsenddataafteronmap   Drop outgoing traffic after the first map is loaded.\n"
        << "  --autosolo                 Load the first savegame, enter the game; on 10s\n"
        << "                             timeout dump character/current map and close.\n"
        << "  --main-datapack-code=CODE  Override the autosolo maincode under\n"
        << "                             datapack/internal/map/main/ (default: first dir).\n"
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
#ifndef CATCHCHALLENGER_NO_TCPSOCKET
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
#endif
#ifndef CATCHCHALLENGER_NO_WEBSOCKET
        if(std::strcmp(arg,"--url")==0 && (i+1)<argc)
        {
            url=QString::fromUtf8(argv[i+1]);
            i+=2;
            continue;
        }
        if(std::strncmp(arg,"--url=",6)==0)
        {
            url=QString::fromUtf8(arg+6);
            i++;
            continue;
        }
#endif
        if(std::strcmp(arg,"--main-datapack-code")==0 && (i+1)<argc)
        {
            mainDatapackCodeOverride=QString::fromUtf8(argv[i+1]);
            i+=2;
            continue;
        }
        if(std::strncmp(arg,"--main-datapack-code=",21)==0)
        {
            mainDatapackCodeOverride=QString::fromUtf8(arg+21);
            i++;
            continue;
        }
        argv[writeIndex++]=argv[i];
        i++;
    }
    argc=writeIndex;
    argv[argc]=nullptr;

    // Enforce mutual exclusion between the three server-selection groups
    // (see CLAUDE.md "Client server-selection CLI flags — pick exactly
    // ONE"). When the user supplies more than one, drop everything and
    // surface a clear error rather than silently picking a winner that
    // depends on the auto-connect priority order in mainwindow.cpp.
    {
        const bool group_list = !server.isEmpty();
        const bool group_tcp  = !host.isEmpty() || port!=0;
        const bool group_ws   = !url.isEmpty();
        const int set_groups  = (group_list?1:0)+(group_tcp?1:0)+(group_ws?1:0);
        if(set_groups>1)
        {
            std::cerr << "ERROR: --server, --host/--port, and --url are mutually "
                         "exclusive — pass exactly one group. Ignoring all three."
                      << std::endl;
            server.clear();
            host.clear();
            port=0;
            url.clear();
        }
    }

    std::cerr << "AutoArgs: server=\"" << server.toStdString() << "\""
              << " host=\"" << host.toStdString() << "\""
              << " port=" << port
              << " url=\"" << url.toStdString() << "\""
              << " autologin=" << (autologin?"true":"false")
              << " character=\"" << character.toStdString() << "\""
              << " closeWhenOnMap=" << (closeWhenOnMap?"true":"false")
              << " closeWhenOnMapAfter=" << closeWhenOnMapAfter
              << " dropSendDataAfterOnMap=" << (dropSendDataAfterOnMap?"true":"false")
              << " autosolo=" << (autosolo?"true":"false")
              << " mainDatapackCodeOverride=\"" << mainDatapackCodeOverride.toStdString() << "\"" << std::endl;
}
