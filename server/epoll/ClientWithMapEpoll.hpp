#ifndef CATCHCHALLENGER_CLIENTMAPMANAGEMENTEPOLL_H
#define CATCHCHALLENGER_CLIENTMAPMANAGEMENTEPOLL_H

#include "EpollClient.hpp"
#include "../base/ClientMapManagement/ClientWithMap.hpp"

class ClientWithMapEpoll : public CatchChallenger::EpollClient, public CatchChallenger::ClientWithMap
{
public:
        /* static allocation, with holes, see doc/algo/visibility/constant-time-player-visibility.png
     * can add carbage collector to not search free holes
     IMPLEMENT INTO HIGHTER CLASS, like vector<Epoll> */
    static std::vector<ClientWithMapEpoll> clients;
    static std::vector<SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> clients_removed_index;
public:
    ClientWithMapEpoll(const int &infd);
    void closeSocket() override;
    bool isValid() override;
    ssize_t readFromSocket(char * data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
};

#endif // CLIENTMAPMANAGEMENT_H
