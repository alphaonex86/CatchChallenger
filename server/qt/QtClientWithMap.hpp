#ifndef CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H
#define CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H

#include "QtClient.hpp"
#include "../base/MapManagement/ClientWithMap.hpp"

class QtClientWithMap : public QObject, public CatchChallenger::QtClient, public CatchChallenger::ClientWithMap
{
    Q_OBJECT
public:
    QtClientWithMap(QIODevice *qtSocket, const PLAYER_INDEX_FOR_CONNECTED &index_connected_player);
    void closeSocket() override;
    bool isValid() override;
    ssize_t readFromSocket(char * data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
    virtual void parseIncommingData() override;
};

#endif // CLIENTMAPMANAGEMENT_H
