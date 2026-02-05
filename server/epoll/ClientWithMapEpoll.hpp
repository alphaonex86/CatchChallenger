#ifndef CATCHCHALLENGER_CLIENTMAPMANAGEMENTEPOLL_H
#define CATCHCHALLENGER_CLIENTMAPMANAGEMENTEPOLL_H

#include "EpollClient.hpp"
#include "../base/MapManagement/ClientWithMap.hpp"

class ClientWithMapEpoll : public CatchChallenger::EpollClient, public CatchChallenger::ClientWithMap
{
public:
    ClientWithMapEpoll(const PLAYER_INDEX_FOR_CONNECTED &index_connected_player);
    void closeSocket() override;
    bool isValid() override;
    ssize_t readFromSocket(char * data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
    void reset(int infd);
};

#endif // CLIENTMAPMANAGEMENT_H
