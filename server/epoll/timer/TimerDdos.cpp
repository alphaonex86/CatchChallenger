#include "TimerDdos.hpp"
#include "../Epoll.hpp"

#include "../../base/LocalClientHandlerWithoutSender.hpp"
#include "../../base/BroadCastWithoutSender.hpp"
#include "../../base/ClientNetworkReadWithoutSender.hpp"
#include "../../base/GlobalServerData.hpp"
#include "../../../general/base/GeneralVariable.hpp"

#include <iostream>

TimerDdos::TimerDdos()
{
}

void TimerDdos::exec()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE<=0)
    {
        std::cerr << "CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE out of range:" << CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE << std::endl;
        return;
    }
    #endif
    CatchChallenger::LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doDDOSChat();
    CatchChallenger::BroadCastWithoutSender::broadCastWithoutSender.doDDOSChat();
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    CatchChallenger::ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender.doDDOSAction();
    #endif
}

