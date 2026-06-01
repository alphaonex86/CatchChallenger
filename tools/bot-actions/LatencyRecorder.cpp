#include "LatencyRecorder.h"

#ifdef CATCHCHALLENGER_BENCHMARK

#include <iostream>
#include <cstdlib>
#include <QCoreApplication>

// Globals declared in the header.
bool g_latencyMode=false;
LatencyRecorder *g_latencyRecorder=NULL;
volatile std::sig_atomic_t g_latencySigint=0;
int g_latencySeconds=15;

// The driver tick; per tick every bot whose chat cooldown elapsed sends one
// chat probe. The PER-BOT cooldown keeps us under the server chat anti-DDOS
// limit (default 5 / 5s) -- never the tick rate.
#define LATENCY_SEND_INTERVAL_MS 100
#define LATENCY_CHAT_PERIOD_NS   1500000000LL   // 1.5s/bot -> 3.3 per 5s < 5
// Marks our deterministic chat probes so other chatter is never mistaken for
// a probe; the body carries the monotonic send time.
#define LATENCY_CHAT_TAG "CCLAT|"

LatencyRecorder::LatencyRecorder(QObject *parent) :
    QObject(parent)
{
    int m=0;
    while(m<Metric_COUNT)
    {
        count[m]=0;
        int b=0;
        while(b<48)
        {
            hist[m][b]=0;
            b++;
        }
        m++;
    }
    if(!connect(&sendTimer,&QTimer::timeout,this,&LatencyRecorder::doSend))
        abort();
    finishTimer.setSingleShot(true);
    if(!connect(&finishTimer,&QTimer::timeout,this,&LatencyRecorder::onMeasureDone))
        abort();
    //start the clock immediately: join latency is sampled as bots arrive on
    //the map, which happens BEFORE the driver (start()) kicks in.
    clock.start();
    g_latencyRecorder=this;
}

int LatencyRecorder::bucketOf(int64_t ns) const
{
    if(ns<=1)
        return 0;
    //same definition as the server BENCH histogram: bucket=floor(log2(ns)).
    int b=63-__builtin_clzll((unsigned long long)ns);
    if(b<0)
        b=0;
    if(b>47)
        b=47;
    return b;
}

void LatencyRecorder::record(Metric m, int64_t deltaNs)
{
    if(deltaNs<0)
        return;
    hist[m][bucketOf(deltaNs)]++;
    count[m]++;
}

void LatencyRecorder::registerApi(CatchChallenger::Api_client_real *api)
{
    if(api==NULL)
        return;
    apis.push_back(api);
    if(!connect(api,&CatchChallenger::Api_protocol_Qt::QthaveCharacter,this,&LatencyRecorder::onHaveCharacter,Qt::QueuedConnection))
        abort();
    if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtinsert_player,this,&LatencyRecorder::onInsertPlayer,Qt::QueuedConnection))
        abort();
    if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtnew_chat_text,this,&LatencyRecorder::onChat,Qt::QueuedConnection))
        abort();
}

void LatencyRecorder::start(int seconds)
{
    sendTimer.start(LATENCY_SEND_INTERVAL_MS);
    //fixed measurement window that EXCLUDES startup: it begins only now (all
    //bots on the map), runs for `seconds`, then dumps + exits on its own.
    if(seconds>0)
        finishTimer.start(seconds*1000);
}

void LatencyRecorder::onMeasureDone()
{
    //dump then _exit WITHOUT unwinding: tearing down QApplication while bots
    //are still connected aborts in Qt destructors. The histogram is already
    //flushed to stdout, so a hard exit is clean for a benchmark (same trick
    //the server's SIGINT dump uses).
    dumpBench();
    std::cout.flush();
    std::_Exit(0);
}

void LatencyRecorder::onHaveCharacter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,
                                      const COORD_TYPE &x, const COORD_TYPE &y,
                                      const CatchChallenger::Direction &last_direction)
{
    (void)mapIndex;
    (void)x;
    (void)y;
    (void)last_direction;
    CatchChallenger::Api_client_real *api=qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(api==NULL)
        return;
    //this bot just reached the map: t0 for its own join visibility.
    const std::string self=api->getPseudo();
    if(joinSendNs.find(self)==joinSendNs.cend())
        joinSendNs[self]=clock.nsecsElapsed();
}

void LatencyRecorder::onInsertPlayer(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,
                                     const CatchChallenger::Player_public_informations &player,
                                     const SIMPLIFIED_PLAYER_ID_FOR_MAP &mapId,
                                     const uint8_t &x, const uint8_t &y,
                                     const CatchChallenger::Direction &direction)
{
    (void)simplifiedIndex;
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
    CatchChallenger::Api_client_real *api=qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(api==NULL)
        return;
    //another player appeared in this bot's view: time it against that player's
    //own map-placement instant (join visibility latency).
    if(player.pseudo!=api->getPseudo())
    {
        const std::map<std::string,int64_t>::const_iterator it=joinSendNs.find(player.pseudo);
        if(it!=joinSendNs.cend())
            record(Metric_join,clock.nsecsElapsed()-it->second);
    }
}

void LatencyRecorder::onChat(const CatchChallenger::Chat_type &chat_type, const std::string &text,
                             const std::string &pseudo, const CatchChallenger::Player_type &player_type)
{
    (void)chat_type;
    (void)player_type;
    CatchChallenger::Api_client_real *api=qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(api==NULL)
        return;
    const std::string tag=LATENCY_CHAT_TAG;
    if(text.size()<=tag.size() || text.compare(0,tag.size(),tag)!=0)
        return;//not one of our probes
    const int64_t sendNs=(int64_t)strtoll(text.c_str()+tag.size(),NULL,10);
    const int64_t delta=clock.nsecsElapsed()-sendNs;
    if(pseudo==api->getPseudo())
        record(Metric_rtt,delta);   //own echo -> client/server/client round trip
    else
        record(Metric_chat,delta);  //heard by another bot -> map relay
}

void LatencyRecorder::doSend()
{
    const int64_t now=clock.nsecsElapsed();
    size_t i=0;
    while(i<apis.size())
    {
        CatchChallenger::Api_client_real *api=apis.at(i);
        i++;
        if(api!=NULL && api->getCaracterSelected())
        {
            //chat probe (chat + rtt). Cooldown keeps us under the chat DDOS limit.
            const std::map<CatchChallenger::Api_protocol_Qt *,int64_t>::const_iterator cit=nextChatNs.find(api);
            if(cit==nextChatNs.cend() || now>=cit->second)
            {
                nextChatNs[api]=now+LATENCY_CHAT_PERIOD_NS;
                //embed our monotonic send time; the server echoes it to the
                //sender (rtt) and to nearby bots (chat).
                const std::string text=std::string(LATENCY_CHAT_TAG)+std::to_string((long long)now);
                api->sendChatText(CatchChallenger::Chat_type_local,text);
            }
        }
    }
}

void LatencyRecorder::dumpOne(const char *prefix, Metric m) const
{
    int b=0;
    while(b<48)
    {
        if(hist[m][b]>0)
            std::cout << "BENCH " << prefix << "_lat_hist_" << b << "=" << hist[m][b] << std::endl;
        b++;
    }
    std::cout << "BENCH " << prefix << "_count=" << count[m] << std::endl;
}

void LatencyRecorder::dumpBench() const
{
    dumpOne("chat",Metric_chat);
    dumpOne("join",Metric_join);
    dumpOne("rtt",Metric_rtt);
    std::cout.flush();
}

#endif // CATCHCHALLENGER_BENCHMARK
