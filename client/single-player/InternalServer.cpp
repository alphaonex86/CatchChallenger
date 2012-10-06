#include "InternalServer.h"
#include "../../server/base/GlobalData.h"
#include "../../general/base/FacilityLib.h"

using namespace Pokecraft;

InternalServer::InternalServer(const QString &dbPath) :
    BaseServer()
{
    this->dbPath=dbPath;

    GlobalData::serverSettings.max_players=1;
    GlobalData::serverSettings.commmonServerSettings.sendPlayerNumber = false;

    GlobalData::serverSettings.database.type=ServerSettings::Database::DatabaseType_SQLite;
    GlobalData::serverSettings.mapVisibility.mapVisibilityAlgorithm	= MapVisibilityAlgorithm_none;

    GlobalData::serverPrivateVariables.eventThreaderList << &thread;//broad cast (0)
    GlobalData::serverPrivateVariables.eventThreaderList << &thread;//map management (1)
    GlobalData::serverPrivateVariables.eventThreaderList << &thread;//network read (2)
    GlobalData::serverPrivateVariables.eventThreaderList << &thread;//heavy load (3)
    GlobalData::serverPrivateVariables.eventThreaderList << &thread;//local calcule (4)

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

    if(!QFakeServer::server.listen())
    {
        DebugClass::debugConsole(QString("Unable to listen the internal server"));
        stat=Down;
        emit error(QString("Unable to listen the internal server"));
        return;
    }

    if(!initialize_the_database())
    {
        QFakeServer::server.close();
        stat=Down;
        return;
    }

    preload_the_data();
    stat=Up;
    emit is_started(true);
    return;
}

QString InternalServer::sqlitePath()
{
    return dbPath;
}
