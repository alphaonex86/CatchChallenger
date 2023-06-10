#if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
#ifndef CATCHCHALLENGER_EpollDatabase_H
#define CATCHCHALLENGER_EpollDatabase_H

#include "../../base/DatabaseBase.hpp"
#include "../BaseClassSwitch.hpp"

class EpollDatabase : public BaseClassSwitch, public CatchChallenger::DatabaseBase
{
};

#endif
#endif
