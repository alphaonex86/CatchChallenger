#ifndef CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H
#define CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H

#include "QtClient.hpp"
#include "../base/ClientMapManagement/ClientWithMap.hpp"

class QtClientWithMap : public QObject, public CatchChallenger::QtClient, public CatchChallenger::ClientWithMap
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
