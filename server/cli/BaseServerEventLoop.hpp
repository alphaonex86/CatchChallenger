#ifndef CATCHCHALLENGER_BASESERVER_EVENT_LOOP_H
#define CATCHCHALLENGER_BASESERVER_EVENT_LOOP_H

#include "../base/BaseServer/BaseServer.hpp"
#include "BaseClassSwitch.hpp"

class BaseServerEventLoop : public CatchChallenger::BaseServer, public BaseClassSwitch
{
public:
    BaseServerEventLoop();
    void loadAndFixSettings() override;
    void setEventTimer(const uint8_t &event,const uint8_t &value,const unsigned int &time,const unsigned int &start) override;
    void preload_the_visibility_algorithm() override;
    void unload_the_visibility_algorithm() override;
    void unload_the_events() override;
    BaseClassSwitch::EventLoopObjectType getType() const override;
};

#endif
