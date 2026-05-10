#if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
#ifndef CATCHCHALLENGER_EVENT_LOOP_DATABASE_H
#define CATCHCHALLENGER_EVENT_LOOP_DATABASE_H

#include "../../base/DatabaseBase.hpp"
#include "../BaseClassSwitch.hpp"

class EventLoopDatabase : public BaseClassSwitch, public CatchChallenger::DatabaseBase
{
};

#endif
#endif
