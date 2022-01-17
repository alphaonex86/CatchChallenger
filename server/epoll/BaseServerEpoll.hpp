#ifndef CATCHCHALLENGER_BASESERVEREPOLL_H
#define CATCHCHALLENGER_BASESERVEREPOLL_H

#include "../base/BaseServer.hpp"
#include "BaseClassSwitch.hpp"

class BaseServerEpoll : public CatchChallenger::BaseServer, public BaseClassSwitch
{
public:
    BaseServerEpoll();
    void loadAndFixSettings() override;
    void setEventTimer(const uint8_t &event,const uint8_t &value,const unsigned int &time,const unsigned int &start) override;
    void preload_the_visibility_algorithm() override;
    void unload_the_visibility_algorithm() override;
    void unload_the_events() override;
    BaseClassSwitch::EpollObjectType getType() const override;
};

#endif
