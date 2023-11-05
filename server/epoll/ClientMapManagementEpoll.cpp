#include "ClientMapManagementEpoll.hpp"

MapVisibilityAlgorithm_NoneEpoll::MapVisibilityAlgorithm_NoneEpoll(const int &infd) :
    CatchChallenger::EpollClient(infd)
{
}

void MapVisibilityAlgorithm_NoneEpoll::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool MapVisibilityAlgorithm_NoneEpoll::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t MapVisibilityAlgorithm_NoneEpoll::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t MapVisibilityAlgorithm_NoneEpoll::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}

MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll::MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll(const int &infd) :
    CatchChallenger::EpollClient(infd)
{
}

void MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}

MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll::MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll(const int &infd) :
    CatchChallenger::EpollClient(infd)
{
}

void MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}
