// tools/bot-bench/main.cpp — Qt-free connect/login benchmark client.
//
// Connects N bots to a CatchChallenger login server, walks the two-stage
// login (login server -> token -> game server), selects or creates a
// character, waits until each bot is on the map, prints every state
// transition and finally "ON_MAP <n>/<N>". Exit code 0 only when every bot
// reached the map before the timeout.
//
// --list-only stops one step earlier: it walks the login only, prints
// "CHARACTERS <n>" (n = characters the server listed for the account) and
// exits, without selecting nor creating a character. One bot in that mode can
// therefore never collide with another bot on a character, which is what makes
// it usable as a PROBE of what the account really holds.
//
// The whole client core is in tools/libbot/cli/: POSIX sockets, one concrete
// Qt-free CatchChallenger::Api_protocol subclass and a select() loop. The
// protocol parsing itself is the PRODUCTION code from
// client/libcatchchallenger/, so a wire-format change is picked up here at
// the next build with no edit in this tool.

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "../libbot/cli/CliApiClient.hpp"
#include "../libbot/cli/CliEventLoop.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../../general/base/cpp11addition.hpp"

struct BenchOptions
{
    std::string host;
    uint16_t port;
    std::string login;
    std::string pass;
    std::string datapackPath;
    uint32_t bots;
    uint32_t timeoutMs;
    bool verbose;
    bool listOnly;
};

static void printUsage(const char * const programName)
{
    std::cout << "Usage: " << programName << " [options]" << std::endl
              << "  --host <host>       login server host (default localhost)" << std::endl
              << "  --port <port>       login server port (default 42489)" << std::endl
              << "  --login <login>     account login (default bench)" << std::endl
              << "  --pass <pass>       account password (default bench)" << std::endl
              << "  --bots <n>          number of bots to connect (default 1)" << std::endl
              << "  --datapack <path>   local datapack directory (default datapack/)" << std::endl
              << "  --timeout <ms>      wall-clock budget for the whole run (default 60000)" << std::endl
              << "  --verbose           print the protocol messages of every bot" << std::endl
              << "  --list-only         stop after the login reply: print" << std::endl
              << "                      \"CHARACTERS <n>\" (n = characters the" << std::endl
              << "                      server listed for the account) and exit," << std::endl
              << "                      without selecting or creating one" << std::endl
              << "Exit code: 0 when every bot reached the map (or, with --list-only," << std::endl
              << "when every bot got its character list), 1 otherwise." << std::endl;
}

/// \brief accepts both "--key value" and "--key=value"
/// \return false on a missing or malformed value (already reported)
static bool takeValue(const int &argc,char * const argv[],int &index,
                      const std::string &name,std::string &value)
{
    const std::string argument(argv[index]);
    const size_t equalPosition=argument.find('=');
    if(equalPosition!=std::string::npos)
    {
        value=argument.substr(equalPosition+1);
        if(value.empty())
        {
            std::cerr << name << " needs a value" << std::endl;
            return false;
        }
        return true;
    }
    if((index+1)>=argc)
    {
        std::cerr << name << " needs a value" << std::endl;
        return false;
    }
    index++;
    value=std::string(argv[index]);
    return true;
}

static bool matchOption(const char * const argument,const std::string &name)
{
    const std::string text(argument);
    if(text==name)
        return true;
    return text.compare(0,name.size()+1,name+"=")==0;
}

static bool parseOptions(const int &argc,char * const argv[],BenchOptions &options)
{
    int index=1;
    while(index<argc)
    {
        std::string value;
        if(matchOption(argv[index],"--host"))
        {
            if(!takeValue(argc,argv,index,"--host",options.host))
                return false;
        }
        else if(matchOption(argv[index],"--port"))
        {
            if(!takeValue(argc,argv,index,"--port",value))
                return false;
            bool ok=false;
            options.port=stringtouint16(value,&ok);
            if(!ok || options.port==0)
            {
                std::cerr << "--port is not a valid port: " << value << std::endl;
                return false;
            }
        }
        else if(matchOption(argv[index],"--login"))
        {
            if(!takeValue(argc,argv,index,"--login",options.login))
                return false;
        }
        else if(matchOption(argv[index],"--pass"))
        {
            if(!takeValue(argc,argv,index,"--pass",options.pass))
                return false;
        }
        else if(matchOption(argv[index],"--bots"))
        {
            if(!takeValue(argc,argv,index,"--bots",value))
                return false;
            bool ok=false;
            options.bots=stringtouint32(value,&ok);
            if(!ok || options.bots==0)
            {
                std::cerr << "--bots is not a positive number: " << value << std::endl;
                return false;
            }
        }
        else if(matchOption(argv[index],"--datapack"))
        {
            if(!takeValue(argc,argv,index,"--datapack",options.datapackPath))
                return false;
        }
        else if(matchOption(argv[index],"--timeout"))
        {
            if(!takeValue(argc,argv,index,"--timeout",value))
                return false;
            bool ok=false;
            options.timeoutMs=stringtouint32(value,&ok);
            if(!ok || options.timeoutMs==0)
            {
                std::cerr << "--timeout is not a positive number of ms: " << value << std::endl;
                return false;
            }
        }
        else if(matchOption(argv[index],"--verbose"))
            options.verbose=true;
        else if(matchOption(argv[index],"--list-only"))
            options.listOnly=true;
        else
        {
            if(matchOption(argv[index],"--help") || matchOption(argv[index],"-h"))
            {
                printUsage(argv[0]);
                return false;
            }
            else
            {
                std::cerr << "unknown option: " << argv[index] << std::endl;
                printUsage(argv[0]);
                return false;
            }
        }
        index++;
    }
    return true;
}

int main(int argc,char *argv[])
{
    BenchOptions options;
    options.host="localhost";
    options.port=42489;
    options.login="bench";
    options.pass="bench";
    options.datapackPath="datapack/";
    options.bots=1;
    options.timeoutMs=60000;
    options.verbose=false;
    options.listOnly=false;
    if(!parseOptions(argc,argv,options))
        return 1;

    //packetFixedSize[] is a static table shared by every client: build it once
    //here. defineMaxPlayers() refines the player-index width when the server
    //announces its own limit.
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    std::cout << "bot-bench: " << options.bots << " bot(s) -> " << options.host
              << ":" << options.port << " login=" << options.login
              << " datapack=" << options.datapackPath << std::endl;

    std::vector<CatchChallenger::CliApiClient *> clients;
    CatchChallenger::CliEventLoop loop;
    uint32_t botIndex=0;
    while(botIndex<options.bots)
    {
        CatchChallenger::CliApiClient * const client=new CatchChallenger::CliApiClient();
        client->setLabel("bot"+std::to_string(botIndex));
        client->setVerbose(options.verbose);
        client->setCharacterSlot(botIndex);
        client->setListOnly(options.listOnly);
        client->setIdentity(options.login,options.pass,options.datapackPath);
        clients.push_back(client);
        loop.addClient(client);
        //a failed connect leaves the bot in State_Failed; the loop reports it
        //and the final count stays honest, so keep starting the others.
        if(!client->connectToLoginServer(options.host,options.port))
            std::cerr << "[" << client->getLabel() << "] " << client->getFailReason() << std::endl;
        botIndex++;
    }

    const bool allFinished=loop.run(options.timeoutMs);
    if(!loop.getError().empty())
        std::cerr << "event loop error: " << loop.getError() << std::endl;
    else
    {
        if(!allFinished)
            std::cerr << "timeout after " << options.timeoutMs << "ms" << std::endl;
    }

    //the state each bot had to reach: the map, or just the login reply
    const CatchChallenger::CliApiClient::State wantedState=options.listOnly?
                CatchChallenger::CliApiClient::State_Listed:
                CatchChallenger::CliApiClient::State_OnMap;

    //report every bot that did not make it, so a failure is diagnosable
    size_t reached=0;
    size_t index=0;
    while(index<clients.size())
    {
        const CatchChallenger::CliApiClient * const client=clients.at(index);
        if(client->getState()==wantedState)
            reached++;
        else
            std::cerr << "[" << client->getLabel() << "] stuck in "
                      << CatchChallenger::CliApiClient::stateToString(client->getState())
                      << (client->getFailReason().empty()?std::string():(": "+client->getFailReason()))
                      << std::endl;
        index++;
    }

    if(options.listOnly)
    {
        //one line per bot; a --list-only run normally uses a single bot
        index=0;
        while(index<clients.size())
        {
            const CatchChallenger::CliApiClient * const client=clients.at(index);
            if(client->getState()==CatchChallenger::CliApiClient::State_Listed)
                std::cout << "CHARACTERS " << client->getCharacterCount() << std::endl;
            index++;
        }
    }
    else
        std::cout << "ON_MAP " << loop.onMapCount() << "/" << options.bots << std::endl;

    index=0;
    while(index<clients.size())
    {
        delete clients.at(index);
        index++;
    }
    clients.clear();

    if(reached==options.bots)
        return 0;
    return 1;
}
