#ifndef CATCHCHALLENGER_PLAYERUPDATERTOMASTER_H
#define CATCHCHALLENGER_PLAYERUPDATERTOMASTER_H

#ifdef EPOLLCATCHCHALLENGERSERVER

namespace CatchChallenger {
class PlayerUpdaterToMaster
        : public EpollTimer
{
public:
    explicit PlayerUpdaterToMaster();
private:
    void exec();
private:
    unsigned short int sended_connected_players;
    enum FrequencyUpdate
    {
        FrequencyUpdate_slow,
        FrequencyUpdate_medium,
        FrequencyUpdate_fast,
    };
    FrequencyUpdate frequencyUpdate;
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOMASTER_H
