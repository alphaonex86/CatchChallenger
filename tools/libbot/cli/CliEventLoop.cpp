#include "CliEventLoop.hpp"
#include "CliApiClient.hpp"
#include "CliLatency.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include <ctime>
#include <sys/select.h>
#include <sys/time.h>

using namespace CatchChallenger;

CliEventLoop::CliEventLoop() :
    clients(),
    errorString()
{
}

CliEventLoop::~CliEventLoop()
{
}

void CliEventLoop::addClient(CliApiClient *client)
{
    clients.push_back(client);
}

const std::string &CliEventLoop::getError() const
{
    return errorString;
}

size_t CliEventLoop::onMapCount() const
{
    size_t count=0;
    size_t index=0;
    while(index<clients.size())
    {
        if(clients.at(index)->getState()==CliApiClient::State_OnMap)
            count++;
        index++;
    }
    return count;
}

void CliEventLoop::runPendingWork()
{
    size_t index=0;
    while(index<clients.size())
    {
        CliApiClient * const client=clients.at(index);
        //A client can flag PendingWork_LoadDatapack and still reach the map in
        //the same parse when another bot already parsed the (process-wide)
        //datapack: nothing is queued then, so drop the stale work.
        if(client->getPendingWork()!=CliApiClient::PendingWork_None && !client->isFinished())
        {
            if(!client->runPendingWork())
                std::cerr << "[" << client->getLabel() << "] " << client->getFailReason() << std::endl;
        }
        index++;
    }
}

void CliEventLoop::reportStateChanges()
{
    size_t index=0;
    while(index<clients.size())
    {
        CliApiClient * const client=clients.at(index);
        if(client->takeStateChanged())
        {
            std::cout << "[" << client->getLabel() << "] "
                      << CliApiClient::stateToString(client->getState());
            if(client->getState()==CliApiClient::State_OnMap)
                std::cout << " map=" << client->getMapIndex()
                          << " x=" << static_cast<uint32_t>(client->getX())
                          << " y=" << static_cast<uint32_t>(client->getY());
            else
            {
                if(client->getState()==CliApiClient::State_Failed)
                    std::cout << " " << client->getFailReason();
            }
            std::cout << std::endl;
        }
        index++;
    }
}

/// \brief moves one client hands to the socket per loop round.
/// Big enough that select() is not called once per 3-byte packet, small enough
/// that one bot cannot starve the others while the server still drains it.
#define CATCHCHALLENGER_SPAM_MOVES_PER_ROUND 64

bool CliEventLoop::runSpam(const uint32_t &seconds,uint64_t &moves,uint64_t &elapsedNs,size_t &survivors)
{
    moves=0;
    elapsedNs=0;
    survivors=0;
    //Arm the bots BEFORE the clock starts: picking the two tiles reads the
    //local map and sends nothing, so it does not belong in the measured window.
    size_t index=0;
    size_t armed=0;
    while(index<clients.size())
    {
        CliApiClient * const client=clients.at(index);
        if(client->getState()==CliApiClient::State_OnMap)
        {
            client->setSpamMode(true);
            if(client->prepareSpam())
                armed++;
            else
                std::cerr << "[" << client->getLabel() << "] " << client->getFailReason() << std::endl;
        }
        index++;
    }
    if(armed==0)
    {
        errorString="no bot could be armed for the move loop";
        return false;
    }

    struct timespec startTime;
    if(::clock_gettime(CLOCK_MONOTONIC,&startTime)<0)
    {
        errorString=std::string("clock_gettime(CLOCK_MONOTONIC) failed: ")+strerror(errno);
        return false;
    }
    const uint64_t windowNs=static_cast<uint64_t>(seconds)*1000000000ULL;
    struct timespec now=startTime;
    while(true)
    {
        //1. push moves. Every client stops on its own as soon as the kernel
        //   refuses bytes, so this is paced by the server, not by a timer.
        size_t alive=0;
        bool morePossible=false;
        int maxFd=-1;
        fd_set readSet;
        fd_set writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        index=0;
        while(index<clients.size())
        {
            CliApiClient * const client=clients.at(index);
            if(client->getState()==CliApiClient::State_OnMap && client->getSocket().isValid())
            {
                moves+=client->pushSpamMoves(CATCHCHALLENGER_SPAM_MOVES_PER_ROUND);
                //pushSpamMoves() can kill the bot (kick during the write), so
                //re-test before adding it to the sets
                if(client->getState()==CliApiClient::State_OnMap && client->getSocket().isValid())
                {
                    alive++;
                    const int fd=client->getSocket().fd();
                    if(fd>=FD_SETSIZE)
                    {
                        errorString="fd "+std::to_string(fd)+" >= FD_SETSIZE ("+
                                    std::to_string(FD_SETSIZE)+"): too many bots for select(), "
                                    "lower --bots or raise the fd limit";
                        return false;
                    }
                    //always read: the server answers a move with the visibility
                    //broadcast of the other bots and with its ping queries, and
                    //not draining that would back-pressure the SERVER instead.
                    FD_SET(fd,&readSet);
                    if(client->wantWrite())
                        FD_SET(fd,&writeSet);
                    else
                    {
                        if(client->isSpamming())
                            morePossible=true;
                    }
                    if(fd>maxFd)
                        maxFd=fd;
                }
            }
            index++;
        }

        //2. deadline / fleet checks
        if(::clock_gettime(CLOCK_MONOTONIC,&now)<0)
        {
            errorString=std::string("clock_gettime(CLOCK_MONOTONIC) failed: ")+strerror(errno);
            return false;
        }
        elapsedNs=static_cast<uint64_t>(now.tv_sec-startTime.tv_sec)*1000000000ULL+
                  static_cast<uint64_t>(now.tv_nsec)-static_cast<uint64_t>(startTime.tv_nsec);
        if(elapsedNs>=windowNs)
            break;
        //every bot lost its socket: report what was measured, do not spin
        if(alive==0 || maxFd<0)
            break;

        //3. wait. Zero timeout when at least one bot can send right now, so the
        //   loop never sleeps while the server is still keeping up.
        struct timeval waitTime;
        if(morePossible)
        {
            waitTime.tv_sec=0;
            waitTime.tv_usec=0;
        }
        else
        {
            const uint64_t remainingNs=windowNs-elapsedNs;
            uint64_t waitNs=20000000ULL;//20ms: only reached when every bot is write-blocked
            if(remainingNs<waitNs)
                waitNs=remainingNs;
            waitTime.tv_sec=static_cast<time_t>(waitNs/1000000000ULL);
            waitTime.tv_usec=static_cast<suseconds_t>((waitNs%1000000000ULL)/1000);
        }
        const int readyCount=::select(maxFd+1,&readSet,&writeSet,NULL,&waitTime);
        if(readyCount<0)
        {
            if(errno!=EINTR)
            {
                errorString=std::string("select() failed: ")+strerror(errno);
                return false;
            }
        }
        else
        {
            if(readyCount>0)
            {
                index=0;
                while(index<clients.size())
                {
                    CliApiClient * const client=clients.at(index);
                    if(client->getState()==CliApiClient::State_OnMap && client->getSocket().isValid())
                    {
                        const int fd=client->getSocket().fd();
                        if(FD_ISSET(fd,&writeSet))
                        {
                            if(!client->flushOutput())
                                std::cerr << "[" << client->getLabel() << "] flush failed" << std::endl;
                        }
                        //read AFTER the flush and re-test: parsing can kick us
                        if(client->getSocket().isValid() && FD_ISSET(fd,&readSet))
                            client->socketReadyRead();
                    }
                    index++;
                }
            }
        }
    }

    //A survivor is a bot that really took part (it was armed) and that the
    //server never dropped: still selected on the map with a live socket. A bot
    //that stopped moving (fight) but stayed connected counts; a kicked one, and
    //one that could never be armed, do not.
    index=0;
    while(index<clients.size())
    {
        CliApiClient * const client=clients.at(index);
        if(client->wasSpamArmed() && client->getState()==CliApiClient::State_OnMap &&
           client->getSocket().isValid())
            survivors++;
        else
        {
            if(!client->getFailReason().empty())
                std::cerr << "[" << client->getLabel() << "] did not survive the spam window: "
                          << client->getFailReason() << std::endl;
        }
        index++;
    }
    return true;
}

#ifdef CATCHCHALLENGER_BENCHMARK
/// \brief longest select() nap of the latency loop, ms. Short enough that a
/// probe due in the next tick is not delayed past its 1.5s cooldown, long
/// enough that an idle window costs almost no CPU (which would otherwise
/// compete with the server on a single-core board).
#define CATCHCHALLENGER_LATENCY_TICK_MS 10

bool CliEventLoop::runLatency(const uint32_t &seconds,uint64_t &elapsedNs)
{
    elapsedNs=0;
    CliLatency * const recorder=CliLatency::instance();
    if(recorder==NULL)
    {
        errorString="no latency recorder installed";
        return false;
    }
    struct timespec startTime;
    if(::clock_gettime(CLOCK_MONOTONIC,&startTime)<0)
    {
        errorString=std::string("clock_gettime(CLOCK_MONOTONIC) failed: ")+strerror(errno);
        return false;
    }
    const uint64_t windowNs=static_cast<uint64_t>(seconds)*1000000000ULL;
    struct timespec now=startTime;
    while(true)
    {
        //1. probes first: the send is what the echo is timed against
        recorder->sendDueProbes();

        //2. deadline
        if(::clock_gettime(CLOCK_MONOTONIC,&now)<0)
        {
            errorString=std::string("clock_gettime(CLOCK_MONOTONIC) failed: ")+strerror(errno);
            return false;
        }
        elapsedNs=static_cast<uint64_t>(now.tv_sec-startTime.tv_sec)*1000000000ULL+
                  static_cast<uint64_t>(now.tv_nsec)-static_cast<uint64_t>(startTime.tv_nsec);
        if(elapsedNs>=windowNs)
            break;

        //3. collect the sockets still worth waiting on
        int maxFd=-1;
        size_t alive=0;
        fd_set readSet;
        fd_set writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        size_t index=0;
        while(index<clients.size())
        {
            CliApiClient * const client=clients.at(index);
            if(client->getState()==CliApiClient::State_OnMap && client->getSocket().isValid())
            {
                const int fd=client->getSocket().fd();
                if(fd>=FD_SETSIZE)
                {
                    errorString="fd "+std::to_string(fd)+" >= FD_SETSIZE ("+
                                std::to_string(FD_SETSIZE)+"): too many bots for select(), "
                                "lower --bots or raise the fd limit";
                    return false;
                }
                alive++;
                //always read: the echo of our own probe, the other bots' probes
                //and the server's ping queries all arrive here, and not draining
                //them would back-pressure the SERVER instead of measuring it.
                FD_SET(fd,&readSet);
                if(client->wantWrite())
                    FD_SET(fd,&writeSet);
                if(fd>maxFd)
                    maxFd=fd;
            }
            index++;
        }
        //every bot lost its socket: report the window measured, do not spin
        if(alive==0 || maxFd<0)
            break;

        //4. wait, but never past the end of the window
        uint64_t waitNs=static_cast<uint64_t>(CATCHCHALLENGER_LATENCY_TICK_MS)*1000000ULL;
        const uint64_t remainingNs=windowNs-elapsedNs;
        if(remainingNs<waitNs)
            waitNs=remainingNs;
        struct timeval waitTime;
        waitTime.tv_sec=static_cast<time_t>(waitNs/1000000000ULL);
        waitTime.tv_usec=static_cast<suseconds_t>((waitNs%1000000000ULL)/1000);
        const int readyCount=::select(maxFd+1,&readSet,&writeSet,NULL,&waitTime);
        if(readyCount<0)
        {
            if(errno!=EINTR)
            {
                errorString=std::string("select() failed: ")+strerror(errno);
                return false;
            }
        }
        else
        {
            if(readyCount>0)
            {
                index=0;
                while(index<clients.size())
                {
                    CliApiClient * const client=clients.at(index);
                    if(client->getState()==CliApiClient::State_OnMap && client->getSocket().isValid())
                    {
                        const int fd=client->getSocket().fd();
                        if(FD_ISSET(fd,&writeSet))
                        {
                            if(!client->flushOutput())
                                std::cerr << "[" << client->getLabel() << "] flush failed" << std::endl;
                        }
                        //read AFTER the flush and re-test: parsing can kick us
                        if(client->getSocket().isValid() && FD_ISSET(fd,&readSet))
                            client->socketReadyRead();
                    }
                    index++;
                }
            }
        }
    }
    return true;
}
#endif // CATCHCHALLENGER_BENCHMARK

bool CliEventLoop::run(const uint32_t &timeoutMs)
{
    struct timeval startTime;
    if(::gettimeofday(&startTime,NULL)<0)
    {
        errorString=std::string("gettimeofday() failed: ")+strerror(errno);
        return false;
    }
    while(true)
    {
        //Report BEFORE the deferred work, then again after: NEED_RECONNECT and
        //NEED_DATAPACK are set inside the parser and consumed by
        //runPendingWork() in the same iteration, so reporting only afterwards
        //would silently swallow both transitions.
        reportStateChanges();
        //deferred work: it is what turns NEED_RECONNECT into a new fd and
        //NEED_DATAPACK into ON_MAP, so a client can finish without any further
        //socket event.
        runPendingWork();
        reportStateChanges();

        size_t unfinished=0;
        int maxFd=-1;
        fd_set readSet;
        fd_set writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        size_t index=0;
        while(index<clients.size())
        {
            CliApiClient * const client=clients.at(index);
            if(!client->isFinished())
            {
                unfinished++;
                CliSocket &socket=client->getSocket();
                if(socket.isValid())
                {
                    const int fd=socket.fd();
                    if(fd>=FD_SETSIZE)
                    {
                        errorString="fd "+std::to_string(fd)+" >= FD_SETSIZE ("+
                                    std::to_string(FD_SETSIZE)+"): too many bots for select(), "
                                    "lower --bots or raise the fd limit";
                        return false;
                    }
                    if(socket.isConnecting())
                        FD_SET(fd,&writeSet);
                    else
                        FD_SET(fd,&readSet);
                    if(fd>maxFd)
                        maxFd=fd;
                }
            }
            index++;
        }
        if(unfinished==0)
            return true;
        if(maxFd<0)
        {
            //every unfinished client lost its socket without reporting a
            //reason; that would spin the loop forever.
            errorString="no socket left to wait on while "+std::to_string(unfinished)+
                        " bot(s) are still unfinished";
            return false;
        }

        struct timeval now;
        if(::gettimeofday(&now,NULL)<0)
        {
            errorString=std::string("gettimeofday() failed: ")+strerror(errno);
            return false;
        }
        const int64_t elapsedMs=static_cast<int64_t>(now.tv_sec-startTime.tv_sec)*1000+
                                static_cast<int64_t>(now.tv_usec-startTime.tv_usec)/1000;
        if(elapsedMs>=static_cast<int64_t>(timeoutMs))
            return false;
        int64_t remainingMs=static_cast<int64_t>(timeoutMs)-elapsedMs;
        //cap the wait so the deadline is re-checked regularly even when the
        //server goes quiet
        if(remainingMs>200)
            remainingMs=200;
        struct timeval waitTime;
        waitTime.tv_sec=remainingMs/1000;
        waitTime.tv_usec=(remainingMs%1000)*1000;

        const int readyCount=::select(maxFd+1,&readSet,&writeSet,NULL,&waitTime);
        if(readyCount<0)
        {
            if(errno!=EINTR)
            {
                errorString=std::string("select() failed: ")+strerror(errno);
                return false;
            }
        }
        else
        {
            if(readyCount>0)
            {
                index=0;
                while(index<clients.size())
                {
                    CliApiClient * const client=clients.at(index);
                    //re-test isValid()/isFinished() on every step: handling one
                    //client can close its own socket (reconnect) but never
                    //another's, so the sets stay meaningful.
                    if(!client->isFinished() && client->getSocket().isValid())
                    {
                        const int fd=client->getSocket().fd();
                        if(FD_ISSET(fd,&writeSet))
                            client->socketWritable();
                        else
                        {
                            if(FD_ISSET(fd,&readSet))
                                client->socketReadyRead();
                        }
                    }
                    index++;
                }
            }
        }
    }
}
