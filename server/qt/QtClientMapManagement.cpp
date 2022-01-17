#include "QtClientMapManagement.hpp"

QtMapVisibilityAlgorithm_None::QtMapVisibilityAlgorithm_NoneEpoll(const int &infd) :
    CatchChallenger::::CatchChallenger::QtClient()
{
}

void QtMapVisibilityAlgorithm_None::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool QtMapVisibilityAlgorithm_None::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t QtMapVisibilityAlgorithm_None::read(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t QtMapVisibilityAlgorithm_None::write(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}

QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::QtMapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll(const int &infd) :
    CatchChallenger::::CatchChallenger::QtClient()
{
}

void QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::read(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::write(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}

QtMapVisibilityAlgorithm_Simple_StoreOnSender::QtMapVisibilityAlgorithm_Simple_StoreOnSenderEpoll(const int &infd) :
    CatchChallenger::::CatchChallenger::QtClient()
{
}

void QtMapVisibilityAlgorithm_Simple_StoreOnSender::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool QtMapVisibilityAlgorithm_Simple_StoreOnSender::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t QtMapVisibilityAlgorithm_Simple_StoreOnSender::read(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t QtMapVisibilityAlgorithm_Simple_StoreOnSender::write(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}
