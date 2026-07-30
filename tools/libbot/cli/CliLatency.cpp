#include "CliLatency.hpp"

#ifdef CATCHCHALLENGER_BENCHMARK

#include "CliApiClient.hpp"

#include <cstdlib>
#include <ctime>
#include <iostream>

using namespace CatchChallenger;

// Per-BOT cooldown between two probes. 1.5s -> 3.3 probes per 5s window, under
// the server's default chat limit (5 / 5s). It paces each bot, never the loop.
#define CATCHCHALLENGER_CLILATENCY_CHAT_PERIOD_NS 1500000000LL
// Marks our deterministic probes so ordinary chatter is never mistaken for one;
// the body carries the monotonic send time.
#define CATCHCHALLENGER_CLILATENCY_TAG "CCLAT|"

/// \brief the single recorder (NULL unless --latency). A file-static pointer
/// rather than a member of the bot: the hooks live in CliApiClient, which must
/// stay free of latency state in a normal build.
static CliLatency *cliLatencyInstance=NULL;

CliLatency *CliLatency::instance()
{
    return cliLatencyInstance;
}

void CliLatency::setInstance(CliLatency *recorder)
{
    cliLatencyInstance=recorder;
}

CliLatency::CliLatency() :
    startNs(0),
    clients(),
    nextChatNs(),
    joinSendNs()
{
    uint8_t metric=0;
    while(metric<Metric_COUNT)
    {
        count[metric]=0;
        uint8_t bucket=0;
        while(bucket<48)
        {
            hist[metric][bucket]=0;
            bucket++;
        }
        metric++;
    }
    struct timespec now;
    if(::clock_gettime(CLOCK_MONOTONIC,&now)<0)
    {
        //A dead monotonic clock cannot be worked around, but it must not take
        //the run down: every delta then reads 0 and the histogram is empty,
        //which the harness already reports as "metric absent".
        std::cerr << "CliLatency: clock_gettime(CLOCK_MONOTONIC) failed, "
                     "latency will not be measured" << std::endl;
        startNs=0;
    }
    else
        startNs=static_cast<int64_t>(now.tv_sec)*1000000000LL+
                static_cast<int64_t>(now.tv_nsec);
}

CliLatency::~CliLatency()
{
    if(cliLatencyInstance==this)
        cliLatencyInstance=NULL;
}

int64_t CliLatency::nowNs() const
{
    struct timespec now;
    if(::clock_gettime(CLOCK_MONOTONIC,&now)<0)
        return 0;
    return static_cast<int64_t>(now.tv_sec)*1000000000LL+
           static_cast<int64_t>(now.tv_nsec)-startNs;
}

int CliLatency::bucketOf(const int64_t &ns) const
{
    if(ns<=1)
        return 0;
    //same definition as the server BENCH histogram: bucket=floor(log2(ns))
    int bucket=63-__builtin_clzll(static_cast<unsigned long long>(ns));
    if(bucket<0)
        bucket=0;
    if(bucket>47)
        bucket=47;
    return bucket;
}

void CliLatency::record(const Metric &metric,const int64_t &deltaNs)
{
    if(deltaNs>=0)
    {
        hist[metric][bucketOf(deltaNs)]++;
        count[metric]++;
    }
}

void CliLatency::registerClient(CliApiClient *client)
{
    if(client!=NULL)
        clients.push_back(client);
}

void CliLatency::onOwnMapPlacement(CliApiClient *client)
{
    if(client!=NULL)
    {
        //this bot just reached the map: t0 for its own join visibility
        const std::string self=client->getPseudo();
        if(joinSendNs.find(self)==joinSendNs.cend())
            joinSendNs[self]=nowNs();
    }
}

void CliLatency::onOtherPlayerInserted(CliApiClient *client,const std::string &pseudo)
{
    if(client!=NULL)
    {
        //another player appeared in this bot's view: time it against that
        //player's own map-placement instant
        if(pseudo!=client->getPseudo())
        {
            const std::map<std::string,int64_t>::const_iterator it=joinSendNs.find(pseudo);
            if(it!=joinSendNs.cend())
                record(Metric_join,nowNs()-it->second);
        }
    }
}

void CliLatency::onChat(CliApiClient *client,const std::string &text,const std::string &pseudo)
{
    if(client!=NULL)
    {
        const std::string tag=CATCHCHALLENGER_CLILATENCY_TAG;
        if(text.size()>tag.size() && text.compare(0,tag.size(),tag)==0)
        {
            const int64_t sendNs=static_cast<int64_t>(strtoll(text.c_str()+tag.size(),NULL,10));
            const int64_t delta=nowNs()-sendNs;
            if(pseudo==client->getPseudo())
                record(Metric_rtt,delta);   //own echo -> client/server/client round trip
            else
                record(Metric_chat,delta);  //heard by another bot -> map relay
        }
    }
}

void CliLatency::sendDueProbes()
{
    const int64_t now=nowNs();
    size_t index=0;
    while(index<clients.size())
    {
        CliApiClient * const client=clients.at(index);
        index++;
        if(client!=NULL && client->getState()==CliApiClient::State_OnMap &&
           client->getSocket().isValid())
        {
            const std::map<CliApiClient *,int64_t>::const_iterator it=nextChatNs.find(client);
            if(it==nextChatNs.cend() || now>=it->second)
            {
                nextChatNs[client]=now+CATCHCHALLENGER_CLILATENCY_CHAT_PERIOD_NS;
                //embed our monotonic send time; the server echoes it to the
                //sender (rtt) and to the bots sharing the map (chat)
                const std::string text=std::string(CATCHCHALLENGER_CLILATENCY_TAG)+
                                       std::to_string(static_cast<long long>(now));
                client->sendChatText(Chat_type_local,text);
                //the probe is a few bytes, but a saturated socket still refuses
                //them: push what the kernel takes now, the loop retries the rest
                if(client->wantWrite())
                {
                    if(!client->flushOutput())
                        std::cerr << "[" << client->getLabel()
                                  << "] latency probe flush failed" << std::endl;
                }
            }
        }
    }
}

void CliLatency::dumpOne(const char * const prefix,const Metric &metric) const
{
    uint8_t bucket=0;
    while(bucket<48)
    {
        if(hist[metric][bucket]>0)
            std::cout << "BENCH " << prefix << "_lat_hist_"
                      << static_cast<uint32_t>(bucket) << "=" << hist[metric][bucket]
                      << std::endl;
        bucket++;
    }
    std::cout << "BENCH " << prefix << "_count=" << count[metric] << std::endl;
}

void CliLatency::dumpBench() const
{
    dumpOne("chat",Metric_chat);
    dumpOne("join",Metric_join);
    dumpOne("rtt",Metric_rtt);
    std::cout.flush();
}

#endif // CATCHCHALLENGER_BENCHMARK
