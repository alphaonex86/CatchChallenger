#include "EpollMapManagement.h"

using namespace CatchChallenger;

EpollMapVisibilityAlgorithm_Simple_StoreOnSender::EpollMapVisibilityAlgorithm_Simple_StoreOnSender(
        const int infd
        #ifdef SERVERSSL
        ,server->getCtx()
        #endif
        ) : EpollClient(infd)
{
}

EpollMapVisibilityAlgorithm_None::EpollMapVisibilityAlgorithm_None(
        const int infd
        #ifdef SERVERSSL
        ,server->getCtx()
        #endif
        ) : EpollClient(infd)
{
}

EpollMapVisibilityAlgorithm_WithBorder_StoreOnSender::EpollMapVisibilityAlgorithm_WithBorder_StoreOnSender(
        const int infd
        #ifdef SERVERSSL
        ,server->getCtx()
        #endif
        ) : EpollClient(infd)
{
}
