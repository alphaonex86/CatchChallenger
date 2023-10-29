#ifndef CATCHCHALLENGER_CLIENTMAPMANAGEMENTEPOLL_H
#define CATCHCHALLENGER_CLIENTMAPMANAGEMENTEPOLL_H

#include "EpollClientAdvanced.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_None.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.hpp"

class MapVisibilityAlgorithm_NoneEpoll : public CatchChallenger::EpollClient, public CatchChallenger::MapVisibilityAlgorithm_None
{
public:
    MapVisibilityAlgorithm_NoneEpoll(const int &infd);
    void closeSocket() override;
    bool isValid() override;
    ssize_t read(char * data, const size_t &size) override;
    ssize_t write(const char * const data, const size_t &size) override;
};
class MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll : public CatchChallenger::EpollClient, public CatchChallenger::MapVisibilityAlgorithm_WithBorder_StoreOnSender
{
public:
    MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll(const int &infd);
    void closeSocket() override;
    bool isValid() override;
    ssize_t read(char * data, const size_t &size) override;
    ssize_t write(const char * const data, const size_t &size) override;
};
class MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll : public CatchChallenger::EpollClient, public CatchChallenger::MapVisibilityAlgorithm_Simple_StoreOnSender
{
public:
    MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll(const int &infd);
    void closeSocket() override;
    bool isValid() override;
    ssize_t read(char * data, const size_t &size) override;
    ssize_t write(const char * const data, const size_t &size) override;
};

#endif // CLIENTMAPMANAGEMENT_H
