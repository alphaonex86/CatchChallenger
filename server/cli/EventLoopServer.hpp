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
    #ifdef CATCHCHALLENGER_DATAPACK_CPP_EMIT
    // stage1 datapack-cpp emit: expose the listen settings to BaseServer's
    // emitDatapackCpp() so they ride along in the generated C++ cache.
    NormalServerSettings getNormalSettingsForCacheEmit() const override;
    #endif
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
