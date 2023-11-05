#ifndef CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H
#define CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H

#include "QtClient.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_None.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.hpp"

class QtMapVisibilityAlgorithm_None : public QObject, public CatchChallenger::QtClient, public CatchChallenger::MapVisibilityAlgorithm_None
{
    Q_OBJECT
public:
    QtMapVisibilityAlgorithm_None(QIODevice *qtSocket);
    void closeSocket() override;
    bool isValid() override;
    ssize_t readFromSocket(char * data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
    virtual void parseIncommingData() override;
};
class QtMapVisibilityAlgorithm_WithBorder_StoreOnSender : public QObject, public CatchChallenger::QtClient, public CatchChallenger::MapVisibilityAlgorithm_WithBorder_StoreOnSender
{
    Q_OBJECT
public:
    QtMapVisibilityAlgorithm_WithBorder_StoreOnSender(QIODevice *qtSocket);
    void closeSocket() override;
    bool isValid() override;
    ssize_t readFromSocket(char * data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
    virtual void parseIncommingData() override;
};
class QtMapVisibilityAlgorithm_Simple_StoreOnSender : public QObject, public CatchChallenger::QtClient, public CatchChallenger::MapVisibilityAlgorithm_Simple_StoreOnSender
{
    Q_OBJECT
public:
    QtMapVisibilityAlgorithm_Simple_StoreOnSender(QIODevice *qtSocket);
    void closeSocket() override;
    bool isValid() override;
    ssize_t readFromSocket(char * data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
    virtual void parseIncommingData() override;
};

#endif // CLIENTMAPMANAGEMENT_H
