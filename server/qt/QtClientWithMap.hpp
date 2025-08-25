#ifndef CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H
#define CATCHCHALLENGER_QTCLIENTMAPMANAGEMENT_H

#include "QtClient.hpp"
#include "../base/ClientMapManagement/ClientWithMap.hpp"

class QtClientWithMap : public QObject, public CatchChallenger::QtClient, public CatchChallenger::ClientWithMap
{
    Q_OBJECT
public:
    /* static allocation, with holes, see doc/algo/visibility/constant-time-player-visibility.png
     * can add carbage collector to not search free holes
     IMPLEMENT INTO HIGHTER CLASS, like vector<Epoll> */
    static std::vector<QtClientWithMap> clients;
    static std::vector<SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> clients_removed_index;
public:
    QtMapVisibilityAlgorithm_Simple_StoreOnSender(QIODevice *qtSocket);
    void closeSocket() override;
    bool isValid() override;
    ssize_t readFromSocket(char * data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
    virtual void parseIncommingData() override;
};

#endif // CLIENTMAPMANAGEMENT_H
