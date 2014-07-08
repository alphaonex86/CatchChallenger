#include "InternalServer.h"
#include "../../../server/base/GlobalServerData.h"
#include "../../../general/base/FacilityLib.h"

using namespace CatchChallenger;

InternalServer::InternalServer() :
    QtServer()
{
    connect(this,&QtServer::need_be_started,this,&InternalServer::start_internal_server,Qt::QueuedConnection);
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
InternalServer::~InternalServer()
{
}

//////////////////////////////////////////// server starting //////////////////////////////////////

//start with allow real player to connect
void InternalServer::start_internal_server()
{
    if(stat!=Down)
    {
        DebugClass::debugConsole("In wrong stat");
        return;
    }
    stat=InUp;
    loadAndFixSettings();

    if(!QFakeServer::server.listen())
    {
        DebugClass::debugConsole(QStringLiteral("Unable to listen the internal server"));
        stat=Down;
        emit error(QStringLiteral("Unable to listen the internal server"));
        emit is_started(false);
        return;
    }

    if(!initialize_the_database())
    {
        QFakeServer::server.close();
        stat=Down;
        emit is_started(false);
        return;
    }
    preload_the_data();
    stat=Up;
    return;
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void InternalServer::removeOneClient()
{
    QtServer::removeOneClient();
    check_if_now_stopped();
}
