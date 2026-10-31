#include "CliClientOptions.hpp"

#include <QCoreApplication>
#include <cstdlib>
#include <cstring>
#include <iostream>

QString CliClientOptions::serverName;
QString CliClientOptions::host;
uint16_t CliClientOptions::port=0;
QString CliClientOptions::url;
bool CliClientOptions::autologin=false;
QString CliClientOptions::characterName;
bool CliClientOptions::closeWhenOnMap=false;
int CliClientOptions::closeWhenOnMapAfter=0;
bool CliClientOptions::dropSendDataAfterOnMap=false;
bool CliClientOptions::autosolo=false;
QString CliClientOptions::takeScreenshotPath;
QString CliClientOptions::mainDatapackCodeOverride;
bool CliClientOptions::autosoloClick=false;
int CliClientOptions::autosoloClickDx=0;
int CliClientOptions::autosoloClickDy=0;
bool CliClientOptions::clickSignTest=false;
bool CliClientOptions::clickDoorTest=false;
bool CliClientOptions::keyboardTest=false;
bool CliClientOptions::dialogOverflowTest=false;
bool CliClientOptions::fixedBackground=false;
bool CliClientOptions::remoteControl=false;

void CliClientOptions::printHelp(const char *progName)
{
    const char *name=(progName!=nullptr && progName[0]!='\0')?progName:"catchchallenger";
    std::cout
        << "Usage: " << name << " [options]\n"
        << "\n"
        << "Options (VALUE flags accept both \"--flag VALUE\" and \"--flag=VALUE\"):\n"
        << "  --help, -h                 Show this help and exit.\n"
        << "  --server NAME              Select an existing server-list entry by name (TCP or WS).\n"
#ifndef CATCHCHALLENGER_NO_TCPSOCKET
        << "  --host HOST                TCP direct-connect host/IP (requires --port).\n"
        << "  --port PORT                TCP direct-connect port (requires --host).\n"
#endif
#ifndef CATCHCHALLENGER_NO_WEBSOCKET
        << "  --url URL                  WebSocket direct-connect ws:// or wss:// URL.\n"
#endif
        << "                             --server, --host/--port and --url: pass only one.\n"
        << "  --autologin                Automatically press the login button.\n"
        << "  --character NAME           Auto-select a character by pseudo.\n"
        << "  --closewhenonmap           Quit 1s after the first map is loaded.\n"
        << "  --closewhenonmapafter=N    On map: toggle direction each 1s, quit after N seconds.\n"
        << "  --dropsenddataafteronmap   Drop outgoing traffic after the first map is loaded.\n"
        << "  --remote-control           Open the QLocalServer automation channel (see client/dev.md).\n"
        << "  --autosolo                 Load the first savegame and enter the game.\n"
        << "  --autosolo=DX,DY           Like --autosolo, then click the tile at player+(DX,DY)\n"
        << "                             (e.g. --autosolo=+1,-5) — walk to it, face & open a sign there.\n"
        << "  --test-clicksign           TEST: click the nearest sign, walk+face+open it, then quit.\n"
        << "  --test-clickdoor           TEST: click a door->other map, click next to the return\n"
        << "                             teleport (push)->back on the original map, then quit.\n"
        << "  --test-keyboard            TEST: ARROW-key walk to a sign + ENTER to open (ESCAPE to\n"
        << "                             close), then arrow-walk into a door and back to the city.\n"
        << "  --test-dialogoverflow      TEST: show a long dialog text, assert no overflow, then quit.\n"
        << "  --fixed                    Freeze the animated background (qtopengl) so\n"
        << "                             --take-screenshot produces deterministic PNGs.\n"
        << "  --take-screenshot PATH     On map (or title screen without --autosolo): write the\n"
        << "                             rendered viewport to PATH and exit (pins srand(42)).\n"
        << "  --main-datapack-code CODE  Override the autosolo maincode under\n"
        << "                             datapack/internal/map/main/ (default: first dir).\n"
        << "  quit                       Request all running instances to quit.\n";
    std::cout.flush();
}

bool CliClientOptions::takeValue(const char *name,int &i,const int argc,char *argv[],QString &value)
{
    const char * const arg=argv[i];
    const size_t nameSize=std::strlen(name);
    if(std::strncmp(arg,name,nameSize)!=0)
        return false;
    if(arg[nameSize]=='=')
    {
        value=QString::fromUtf8(arg+nameSize+1);
        return true;
    }
    if(arg[nameSize]!='\0')
        return false;
    if((i+1)<argc)
    {
        i++;
        value=QString::fromUtf8(argv[i]);
    }
    else
        std::cerr << name << " requires a value" << std::endl;
    return true;
}

bool CliClientOptions::parse(int &argc,char *argv[])
{
    serverName.clear();
    host.clear();
    port=0;
    url.clear();
    autologin=false;
    characterName.clear();
    closeWhenOnMap=false;
    closeWhenOnMapAfter=0;
    dropSendDataAfterOnMap=false;
    autosolo=false;
    takeScreenshotPath.clear();
    mainDatapackCodeOverride.clear();
    autosoloClick=false;
    autosoloClickDx=0;
    autosoloClickDy=0;
    clickSignTest=false;
    clickDoorTest=false;
    keyboardTest=false;
    dialogOverflowTest=false;
    fixedBackground=false;
    remoteControl=false;
    if(argc<1)
        return true;

    int writeIndex=1;
    int i=1;
    while(i<argc)
    {
        const char * const arg=argv[i];
        QString value;
        if(std::strcmp(arg,"--help")==0 || std::strcmp(arg,"-h")==0)
        {
            printHelp(argc>0?argv[0]:nullptr);
            return false;
        }
        else if(std::strcmp(arg,"--autosolo")==0)
            autosolo=true;
        else if(std::strncmp(arg,"--autosolo=",11)==0)
        {
            autosolo=true;
            const char * const comma=std::strchr(arg+11,',');
            if(comma!=nullptr)
            {
                autosoloClick=true;
                autosoloClickDx=std::atoi(arg+11);
                autosoloClickDy=std::atoi(comma+1);
            }
            else
                std::cerr << "--autosolo=DX,DY requires two comma-separated integers" << std::endl;
        }
        else if(std::strcmp(arg,"--test-clicksign")==0)
            clickSignTest=true;
        else if(std::strcmp(arg,"--test-clickdoor")==0)
            clickDoorTest=true;
        else if(std::strcmp(arg,"--test-keyboard")==0)
            keyboardTest=true;
        else if(std::strcmp(arg,"--test-dialogoverflow")==0)
            dialogOverflowTest=true;
        else if(std::strcmp(arg,"--autologin")==0)
            autologin=true;
        else if(std::strcmp(arg,"--closewhenonmap")==0)
            closeWhenOnMap=true;
        else if(std::strncmp(arg,"--closewhenonmapafter=",22)==0)
        {
            closeWhenOnMapAfter=std::atoi(arg+22);
            if(closeWhenOnMapAfter<1)
                closeWhenOnMapAfter=1;
        }
        else if(std::strcmp(arg,"--dropsenddataafteronmap")==0)
            dropSendDataAfterOnMap=true;
        else if(std::strcmp(arg,"--remote-control")==0)
            remoteControl=true;
        else if(std::strcmp(arg,"--fixed")==0)
            fixedBackground=true;
        else if(takeValue("--server",i,argc,argv,serverName))
        {}
        else if(takeValue("--character",i,argc,argv,characterName))
        {}
#ifndef CATCHCHALLENGER_NO_TCPSOCKET
        else if(takeValue("--host",i,argc,argv,host))
        {}
        else if(takeValue("--port",i,argc,argv,value))
        {
            bool ok=false;
            port=value.toUShort(&ok);
            if(!ok)
                std::cerr << "--port: \"" << value.toStdString() << "\" is not a port number" << std::endl;
        }
#endif
#ifndef CATCHCHALLENGER_NO_WEBSOCKET
        else if(takeValue("--url",i,argc,argv,url))
        {}
#endif
        else if(takeValue("--main-datapack-code",i,argc,argv,mainDatapackCodeOverride))
        {}
        else if(takeValue("--take-screenshot",i,argc,argv,takeScreenshotPath))
        {}
        else
        {
            //not ours: left for Qt (-platform ...) and the "quit" request
            argv[writeIndex]=argv[i];
            writeIndex++;
        }
        i++;
    }
    argc=writeIndex;
    argv[argc]=nullptr;

    const bool groupList=!serverName.isEmpty();
    const bool groupTcp=!host.isEmpty() || port!=0;
    const bool groupWs=!url.isEmpty();
    if((groupList?1:0)+(groupTcp?1:0)+(groupWs?1:0)>1)
    {
        std::cerr << "ERROR: --server, --host/--port, and --url are mutually "
                     "exclusive — pass exactly one group. Ignoring all three." << std::endl;
        serverName.clear();
        host.clear();
        port=0;
        url.clear();
    }
    return true;
}

bool CliClientOptions::exitOnErrorIfAutomated(const std::string &error)
{
    if(!closeWhenOnMap && closeWhenOnMapAfter<=0)
        return false;
    std::cerr << "CliClientOptions: error with --closewhenonmap[after], exiting with rc=2: " << error << std::endl;
    QCoreApplication::exit(2);
    return true;
}
