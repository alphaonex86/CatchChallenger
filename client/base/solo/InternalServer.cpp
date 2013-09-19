#include "InternalServer.h"
#include "../../../server/base/GlobalServerData.h"
#include "../../../general/base/FacilityLib.h"

using namespace CatchChallenger;

InternalServer::InternalServer() :
    BaseServer()
{
    GlobalServerData::serverPrivateVariables.eventThreaderList << &thread;//broad cast (0)
    GlobalServerData::serverPrivateVariables.eventThreaderList << &thread;//map management (1)
    GlobalServerData::serverPrivateVariables.eventThreaderList << &thread;//network read (2)
    GlobalServerData::serverPrivateVariables.eventThreaderList << &thread;//heavy load (3)
    GlobalServerData::serverPrivateVariables.eventThreaderList << &thread;//local calcule (4)
    GlobalServerData::serverPrivateVariables.eventThreaderList << &thread;//local broad cast (5)

    emit need_be_started();
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
InternalServer::~InternalServer()
{
    thread.quit();
    thread.wait();
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
        DebugClass::debugConsole(QString("Unable to listen the internal server"));
        stat=Down;
        emit error(QString("Unable to listen the internal server"));
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
    BaseServer::start_internal_server();
    preload_the_data();
    stat=Up;
    emit is_started(true);
    return;
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void InternalServer::removeOneClient()
{
    BaseServer::removeOneClient();
    check_if_now_stopped();
}
