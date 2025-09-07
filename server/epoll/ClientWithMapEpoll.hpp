#ifndef CATCHCHALLENGER_CLIENTMAPMANAGEMENTEPOLL_H
#define CATCHCHALLENGER_CLIENTMAPMANAGEMENTEPOLL_H

#include "EpollClient.hpp"
#include "../base/ClientMapManagement/ClientWithMap.hpp"

class ClientWithMapEpoll : public CatchChallenger::EpollClient, public CatchChallenger::ClientWithMap
{
public:
    ClientWithMapEpoll(const int &infd);
    void closeSocket() override;
    bool isValid() override;
    ssize_t readFromSocket(char * data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
};

#endif // CLIENTMAPMANAGEMENT_H
