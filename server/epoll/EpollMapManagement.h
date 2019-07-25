#ifndef EPOLLMAPMANAGEMENT_H
#define EPOLLMAPMANAGEMENT_H

#include "../base/ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "EpollClient.h"

namespace CatchChallenger {
class EpollMapVisibilityAlgorithm_Simple_StoreOnSender : public MapVisibilityAlgorithm_Simple_StoreOnSender, public EpollClient
{
public:
    EpollMapVisibilityAlgorithm_Simple_StoreOnSender(const int infd
                                                 #ifdef SERVERSSL
                                                 ,server->getCtx()
                                                 #endif
                                                 );
};
class EpollMapVisibilityAlgorithm_None : public MapVisibilityAlgorithm_None, public EpollClient
{
public:
    EpollMapVisibilityAlgorithm_None(const int infd
                                                 #ifdef SERVERSSL
                                                 ,server->getCtx()
                                                 #endif
                                                 );
};
class EpollMapVisibilityAlgorithm_WithBorder_StoreOnSender : public MapVisibilityAlgorithm_WithBorder_StoreOnSender, public EpollClient
{
public:
    EpollMapVisibilityAlgorithm_WithBorder_StoreOnSender(const int infd
                                                 #ifdef SERVERSSL
                                                 ,server->getCtx()
                                                 #endif
                                                 );
};
}

#endif // EPOLLMAPMANAGEMENT_H
