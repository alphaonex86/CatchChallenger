#include "CliEventLoop.hpp"
#include "CliApiClient.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

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
