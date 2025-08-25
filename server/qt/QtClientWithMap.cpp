#include "QtClientMapManagement.hpp"

QtMapVisibilityAlgorithm_None::QtMapVisibilityAlgorithm_None(QIODevice *qtSocket) :
    CatchChallenger::QtClient(qtSocket)
{
}

void QtMapVisibilityAlgorithm_None::parseIncommingData()
{
    Client::parseIncommingData();
}

void QtMapVisibilityAlgorithm_None::closeSocket()
{
    CatchChallenger::QtClient::closeSocket();
}

bool QtMapVisibilityAlgorithm_None::isValid()
{
    return CatchChallenger::QtClient::isValid();
}

ssize_t QtMapVisibilityAlgorithm_None::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::QtClient::read(data,size);
}

ssize_t QtMapVisibilityAlgorithm_None::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::QtClient::write(data,size);
}

QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::QtMapVisibilityAlgorithm_WithBorder_StoreOnSender(QIODevice *qtSocket) :
    CatchChallenger::QtClient(qtSocket)
{
}

void QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::parseIncommingData()
{
    Client::parseIncommingData();
}

void QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::closeSocket()
{
    CatchChallenger::QtClient::closeSocket();
}

bool QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::isValid()
{
    return CatchChallenger::QtClient::isValid();
}

ssize_t QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::QtClient::read(data,size);
}

ssize_t QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::QtClient::write(data,size);
}

QtMapVisibilityAlgorithm_Simple_StoreOnSender::QtMapVisibilityAlgorithm_Simple_StoreOnSender(QIODevice *qtSocket) :
    CatchChallenger::QtClient(qtSocket)
{
}

void QtMapVisibilityAlgorithm_Simple_StoreOnSender::parseIncommingData()
{
    Client::parseIncommingData();
}

void QtMapVisibilityAlgorithm_Simple_StoreOnSender::closeSocket()
{
    CatchChallenger::QtClient::closeSocket();
}

bool QtMapVisibilityAlgorithm_Simple_StoreOnSender::isValid()
{
    return CatchChallenger::QtClient::isValid();
}

ssize_t QtMapVisibilityAlgorithm_Simple_StoreOnSender::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::QtClient::read(data,size);
}

ssize_t QtMapVisibilityAlgorithm_Simple_StoreOnSender::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::QtClient::write(data,size);
}
