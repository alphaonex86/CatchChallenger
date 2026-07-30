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
// --spam --seconds N adds a SATURATION phase after every bot is on the map:
// each bot walks back and forth as fast as the server drains its socket for N
// seconds, then the run prints
//     SURVIVORS <bots still connected>/<bots>
//     MOVES <move packets handed to a socket, all bots>
//     REQ_PER_S <MOVES / the measured window>
// and exits 0 as long as at least one bot ever reached the map. The window is
// measured with CLOCK_MONOTONIC around the phase only, so connect/login/
// character/datapack time is excluded. There is no sleep and no rate limit:
// the pacing is TCP back-pressure from the server, which is what puts a
// single-threaded event loop at 100% CPU.
//
// NOTE: a server built WITHOUT -DCATCHCHALLENGER_BENCHMARK=ON compiles in the
// anti-flood filter (CATCHCHALLENGER_DDOS_FILTER, kickLimitMove defaults to 60
// moves per window) and kicks every bot in the first fraction of a second. That
// is not a bug in either side — measuring saturation requires the benchmark
// build, exactly as benchmark/CLAUDE.md describes. --spam against a filtered
// server reports SURVIVORS 0 and a MOVES count in the tens.
//
// The whole client core is in tools/libbot/cli/: POSIX sockets, one concrete
// Qt-free CatchChallenger::Api_protocol subclass and a select() loop. The
// protocol parsing itself is the PRODUCTION code from
// client/libcatchchallenger/, so a wire-format change is picked up here at
// the next build with no edit in this tool.

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "../libbot/cli/CliApiClient.hpp"
#include "../libbot/cli/CliEventLoop.hpp"
//expands to nothing unless CATCHCHALLENGER_BENCHMARK is set
#include "../libbot/cli/CliLatency.hpp"
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
    bool spam;
    bool latency;
    /// \brief length of the measured window, shared by --spam and --latency
    /// (they are mutually exclusive, so one knob is enough)
    uint32_t windowSeconds;
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
              << "  --spam              saturation phase once every bot is on the map:" << std::endl
              << "                      each bot moves as fast as the server drains its" << std::endl
              << "                      socket (no sleep, no rate limit), then print" << std::endl
              << "                      SURVIVORS/MOVES/REQ_PER_S. Implies one account" << std::endl
              << "                      per bot: the login becomes \"<login><index>\", so" << std::endl
              << "                      the server's per-account character cap can never" << std::endl
              << "                      limit the fleet size" << std::endl
              << "  --seconds <n>       length of the --spam / --latency window (default 10)" << std::endl
              << "  --latency           latency phase once every bot is on the map:" << std::endl
              << "                      each bot sends a tagged local chat probe every" << std::endl
              << "                      1.5s and the echoes are timestamped, then print" << std::endl
              << "                      \"BENCH <chat|join|rtt>_lat_hist_<i>=<n>\" log2-ns" << std::endl
              << "                      histograms. Needs a CATCHCHALLENGER_BENCHMARK" << std::endl
              << "                      build; implies one account per bot like --spam." << std::endl
              << "                      chat and join need at least 2 bots; rtt needs 1" << std::endl
              << "Exit code: 0 when every bot reached the map (or, with --list-only," << std::endl
              << "when every bot got its character list), 1 otherwise. With --spam: 0" << std::endl
              << "as soon as one bot reached the map, 1 when none did." << std::endl;
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
        else if(matchOption(argv[index],"--seconds"))
        {
            if(!takeValue(argc,argv,index,"--seconds",value))
                return false;
            bool ok=false;
            options.windowSeconds=stringtouint32(value,&ok);
            if(!ok || options.windowSeconds==0)
            {
                std::cerr << "--seconds is not a positive number of seconds: " << value << std::endl;
                return false;
            }
        }
        else if(matchOption(argv[index],"--verbose"))
            options.verbose=true;
        else if(matchOption(argv[index],"--list-only"))
            options.listOnly=true;
        else if(matchOption(argv[index],"--spam"))
            options.spam=true;
        else if(matchOption(argv[index],"--latency"))
            options.latency=true;
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

/** \brief bots onboarded per wave.
 *
 * The all-in-one server keeps at most CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION
 * (50, server/base/VariableServer.hpp) logins in flight and, past that, SILENTLY
 * DISCONNECTS THE OLDEST pending one to make room for the newcomer
 * (server/base/ClientNetworkRead.cpp, the 0xAC character-select path). 200 bots
 * connecting in one burst therefore lose the first 150 with no server-side log
 * line at all. Staying well under the cap onboards every bot; this paces only
 * the ONBOARDING, never the measured window.
 */
#define CATCHCHALLENGER_BOTBENCH_ONBOARD_WAVE 24

/// \brief milliseconds since an arbitrary origin, immune to clock changes
static uint64_t monotonicMs()
{
    struct timespec now;
    if(::clock_gettime(CLOCK_MONOTONIC,&now)<0)
    {
        std::cerr << "clock_gettime(CLOCK_MONOTONIC) failed: " << strerror(errno) << std::endl;
        return 0;
    }
    return static_cast<uint64_t>(now.tv_sec)*1000ULL+
           static_cast<uint64_t>(now.tv_nsec)/1000000ULL;
}

/// \brief create, register and connect the bots [firstIndex;lastIndex[
static void startBots(const BenchOptions &options,
                      std::vector<CatchChallenger::CliApiClient *> &clients,
                      CatchChallenger::CliEventLoop &loop,
                      const uint32_t &firstIndex,const uint32_t &lastIndex)
{
    uint32_t botIndex=firstIndex;
    while(botIndex<lastIndex)
    {
        CatchChallenger::CliApiClient * const client=new CatchChallenger::CliApiClient();
        client->setLabel("bot"+std::to_string(botIndex));
        client->setVerbose(options.verbose);
        client->setListOnly(options.listOnly);
        //Shared account (default): the N bots split ONE account and bot i takes
        //its i-th character. That is what the character-list test needs, but the
        //server caps characters per account (max_character, default 3), so it
        //cannot scale a saturation run.
        //--spam and --latency therefore give every bot its OWN login
        //"<login><index>" with a single character: no cap, and no create-account
        //race between bots since no two of them open the same login.
        if(options.spam || options.latency)
        {
            client->setCharacterSlot(0);
            client->setIdentity(options.login+std::to_string(botIndex),options.pass,
                                options.datapackPath);
        }
        else
        {
            client->setCharacterSlot(botIndex);
            client->setIdentity(options.login,options.pass,options.datapackPath);
        }
        clients.push_back(client);
        loop.addClient(client);
#ifdef CATCHCHALLENGER_BENCHMARK
        //enrol for the chat driver BEFORE the login: the recorder's clock is
        //already running, so this bot's join is timed too.
        if(CatchChallenger::CliLatency::instance()!=NULL)
            CatchChallenger::CliLatency::instance()->registerClient(client);
#endif
        //a failed connect leaves the bot in State_Failed; the loop reports it
        //and the final count stays honest, so keep starting the others.
        if(!client->connectToLoginServer(options.host,options.port))
            std::cerr << "[" << client->getLabel() << "] " << client->getFailReason() << std::endl;
        botIndex++;
    }
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
    options.spam=false;
    options.latency=false;
    options.windowSeconds=10;
    if(!parseOptions(argc,argv,options))
        return 1;
    if(options.spam && options.listOnly)
    {
        std::cerr << "--spam needs a character on the map, so it cannot be combined "
                     "with --list-only" << std::endl;
        return 1;
    }
    if(options.latency && options.listOnly)
    {
        std::cerr << "--latency needs a character on the map, so it cannot be combined "
                     "with --list-only" << std::endl;
        return 1;
    }
    if(options.latency && options.spam)
    {
        std::cerr << "--latency measures a QUIET path and --spam saturates the server: "
                     "running both at once would report queueing delay, not latency. "
                     "Pick one." << std::endl;
        return 1;
    }
#ifndef CATCHCHALLENGER_BENCHMARK
    if(options.latency)
    {
        std::cerr << "--latency needs a build configured with "
                     "-DCATCHCHALLENGER_BENCHMARK=ON (the recorder is compiled out here)"
                  << std::endl;
        return 1;
    }
#endif

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
#ifdef CATCHCHALLENGER_BENCHMARK
    //Constructed here so its monotonic clock starts BEFORE the first login: the
    //join metric is sampled while the bots onboard. It is only INSTALLED when
    //--latency was asked for, and the hooks in CliApiClient test for that, so a
    //--spam run pays nothing.
    CatchChallenger::CliLatency latencyRecorder;
    if(options.latency)
        CatchChallenger::CliLatency::setInstance(&latencyRecorder);
#endif
    //Bots sharing ONE account must not open a brand-new login together: they all
    //get "unknown login, you may create it" (0x07) and the losers of the
    //create-account race are answered 0x02 AND DISCONNECTED
    //(server/base/ClientLoad/ClientHeavyLoadSelectChar.cpp loginIsWrong() sends
    //the reply then calls errorOutput()), so they can never reach the map. Run
    //bot 0 alone first: it creates the account, every later bot just logs in.
    //Not needed with --spam (one login per bot, nobody races anybody).
    const bool serialiseFirstLogin=(!options.spam && !options.latency && options.bots>1);

    //run() returns when every client it KNOWS ABOUT is finished, so each wave is
    //created only after the previous one is done.
    const uint64_t onboardStartMs=monotonicMs();
    bool allFinished=true;
    uint32_t started=0;
    while(started<options.bots)
    {
        uint32_t wave=CATCHCHALLENGER_BOTBENCH_ONBOARD_WAVE;
        if(started==0 && serialiseFirstLogin)
            wave=1;
        if(wave>(options.bots-started))
            wave=options.bots-started;
        startBots(options,clients,loop,started,started+wave);
        const uint64_t elapsedMs=monotonicMs()-onboardStartMs;
        if(elapsedMs>=options.timeoutMs)
        {
            std::cerr << "onboarding budget of " << options.timeoutMs << "ms spent after "
                      << started << "/" << options.bots << " bot(s)" << std::endl;
            allFinished=false;
            started+=wave;
            break;
        }
        allFinished=loop.run(static_cast<uint32_t>(options.timeoutMs-elapsedMs));
        started+=wave;
        //A serialised first login exists to CREATE the shared account: fanning
        //out before it succeeded would put the whole fleet back in the race.
        if(serialiseFirstLogin && started==1)
        {
            if(clients.at(0)->getState()!=CatchChallenger::CliApiClient::State_OnMap &&
               clients.at(0)->getState()!=CatchChallenger::CliApiClient::State_Listed)
            {
                std::cerr << "the first bot did not get through the login, not starting the "
                          << (options.bots-1) << " others" << std::endl;
                break;
            }
        }
    }
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
    //captured BEFORE the saturation phase: a bot kicked during the window is no
    //longer on the map, but it DID reach it, which is what the exit code is about
    const size_t onMapReached=loop.onMapCount();
    if(!options.listOnly)
        std::cout << "ON_MAP " << onMapReached << "/" << options.bots << std::endl;

    //---- saturation phase ----------------------------------------------
    if(options.spam && onMapReached>0)
    {
        uint64_t moves=0;
        uint64_t elapsedNs=0;
        size_t survivors=0;
        if(!loop.runSpam(options.windowSeconds,moves,elapsedNs,survivors))
            std::cerr << "spam phase error: " << loop.getError() << std::endl;
        //Print the numbers even when the window ended early (every bot kicked,
        //loop error): a truncated window with its real duration is a result, a
        //missing line is not.
        std::cout << "SURVIVORS " << survivors << "/" << options.bots << std::endl;
        std::cout << "MOVES " << moves << std::endl;
        //REQ_PER_S is grepped as ^REQ_PER_S ([0-9.]+)$: plain fixed notation,
        //never an exponent, never a locale comma (the C locale is the default
        //and this binary never calls setlocale()).
        double seconds=static_cast<double>(elapsedNs)/1000000000.0;
        double perSecond=0.0;
        if(seconds>0.0)
            perSecond=static_cast<double>(moves)/seconds;
        char formatted[64];
        const int written=snprintf(formatted,sizeof(formatted),"%.3f",perSecond);
        if(written>0 && written<static_cast<int>(sizeof(formatted)))
            std::cout << "REQ_PER_S " << formatted << std::endl;
        else
        {
            std::cerr << "REQ_PER_S formatting failed for " << perSecond << std::endl;
            std::cout << "REQ_PER_S 0.000" << std::endl;
        }
        std::cerr << "spam window: " << seconds << "s" << std::endl;
    }

    //---- latency phase -------------------------------------------------
#ifdef CATCHCHALLENGER_BENCHMARK
    if(options.latency && onMapReached>0)
    {
        uint64_t elapsedNs=0;
        if(!loop.runLatency(options.windowSeconds,elapsedNs))
            std::cerr << "latency phase error: " << loop.getError() << std::endl;
        //Who was still able to probe at the end. A bot that leaves the map
        //during the window stops sending, and the histogram alone cannot show
        //that -- it just holds fewer samples, which reads as "the platform is
        //quiet" instead of "the fleet fell apart". Same contract as
        //SURVIVORS for --spam.
        size_t latencySurvivors=0;
        index=0;
        while(index<clients.size())
        {
            CatchChallenger::CliApiClient * const client=clients.at(index);
            if(client->getState()==CatchChallenger::CliApiClient::State_OnMap &&
               client->getSocket().isValid())
                latencySurvivors++;
            else
                std::cerr << "[" << client->getLabel() << "] left the map during the "
                          << "latency window: "
                          << CatchChallenger::CliApiClient::stateToString(client->getState())
                          << (client->getFailReason().empty()?std::string():
                              (": "+client->getFailReason())) << std::endl;
            index++;
        }
        std::cout << "SURVIVORS " << latencySurvivors << "/" << options.bots << std::endl;
        //Dump even when the window ended early (every bot kicked, loop error):
        //a truncated histogram with its real sample count is a result, a missing
        //one is not. The join buckets are already filled from the onboarding.
        latencyRecorder.dumpBench();
        std::cerr << "latency window: "
                  << (static_cast<double>(elapsedNs)/1000000000.0) << "s" << std::endl;
    }
    //The recorder outlives the bots as a stack object, but its maps key on the
    //bot pointers: uninstall it before they are deleted so a notification fired
    //during teardown cannot reach a freed client.
    CatchChallenger::CliLatency::setInstance(NULL);
#endif

    index=0;
    while(index<clients.size())
    {
        delete clients.at(index);
        index++;
    }
    clients.clear();

    if(options.spam || options.latency)
    {
        //A saturation or latency run is about the measured window, not
        //onboarding: it only fails when NO bot ever reached the map (nothing was
        //measured at all).
        if(onMapReached>0)
            return 0;
        return 1;
    }
    if(reached==options.bots)
        return 0;
    return 1;
}
