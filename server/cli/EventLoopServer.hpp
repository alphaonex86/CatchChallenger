#ifndef EVENT_LOOP_SERVER_H
#define EVENT_LOOP_SERVER_H

#include "EventLoopGenericServer.hpp"
#include "BaseServerEventLoop.hpp"
#include "../base/ServerStructures.hpp"

namespace CatchChallenger {
class EventLoopServer : public CatchChallenger::EventLoopGenericServer, public BaseServerEventLoop
{
public:
    EventLoopServer();
    ~EventLoopServer();
    void initTheDatabase();
    bool tryListen();
    void preload_1_the_data();
    void unload_the_data();
    void setNormalSettings(const NormalServerSettings &settings);
    NormalServerSettings getNormalSettings() const;
    void loadAndFixSettings();
    void preload_finish();
    bool isReady();
    void quitForCriticalDatabaseQueryFailed();
private:
    NormalServerSettings normalServerSettings;
    bool ready;
};
}

#endif // EVENT_LOOP_SERVER_H
