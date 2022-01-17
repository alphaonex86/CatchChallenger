#ifndef CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H
#define CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H

#include "QtClient.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_None.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.hpp"

class QtMapVisibilityAlgorithm_None : public CatchChallenger::QtClient, public CatchChallenger::MapVisibilityAlgorithm_None
{
public:
    QtMapVisibilityAlgorithm_None(const int &infd);
    void closeSocket() override;
    bool isValid() override;
    ssize_t read(char * data, const size_t &size) override;
    ssize_t write(const char * const data, const size_t &size) override;
};
class QtMapVisibilityAlgorithm_WithBorder_StoreOnSender : public CatchChallenger::QtClient, public CatchChallenger::MapVisibilityAlgorithm_WithBorder_StoreOnSender
{
public:
    QtMapVisibilityAlgorithm_WithBorder_StoreOnSender(const int &infd);
    void closeSocket() override;
    bool isValid() override;
    ssize_t read(char * data, const size_t &size) override;
    ssize_t write(const char * const data, const size_t &size) override;
};
class QtMapVisibilityAlgorithm_Simple_StoreOnSender : public CatchChallenger::QtClient, public CatchChallenger::MapVisibilityAlgorithm_Simple_StoreOnSender
{
public:
    QtMapVisibilityAlgorithm_Simple_StoreOnSender(const int &infd);
    void closeSocket() override;
    bool isValid() override;
    ssize_t read(char * data, const size_t &size) override;
    ssize_t write(const char * const data, const size_t &size) override;
};

#endif // CLIENTMAPMANAGEMENT_H
