#if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
#ifndef CATCHCHALLENGER_EVENT_LOOP_DATABASE_H
#define CATCHCHALLENGER_EVENT_LOOP_DATABASE_H

#include "../../base/DatabaseBase.hpp"
#include "../BaseClassSwitch.hpp"

class EventLoopDatabase : public BaseClassSwitch, public CatchChallenger::DatabaseBase
{
};

// EventLoopDb: switchable alias used by the cluster binaries
// (master, login, gateway, game-server-alone). One DB backend per
// binary; selected at configure time via CATCHCHALLENGER_DB_POSTGRESQL
// or CATCHCHALLENGER_DB_MYSQL. The cli (all-in-one) binary does NOT
// use this alias — it stays on the explicit class names because its
// CMakeLists adds source files conditionally.
#if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(CATCHCHALLENGER_DB_MYSQL)
#error Both CATCHCHALLENGER_DB_POSTGRESQL and CATCHCHALLENGER_DB_MYSQL are defined; pick one for cluster binaries.
#endif

// Forward declarations of the concrete classes — full definitions live
// in EventLoopPostgresql.hpp / EventLoopMySQL.hpp. Cluster source files
// that need to call methods (`new EventLoopDb()`, member access) must
// include the matching concrete header. CharactersGroup.{hpp,cpp},
// EventLoopServerLoginMaster.{hpp,cpp} and friends already do, because
// the implementing .cpp pulls in either header transitively via the
// concrete-backend translation unit.
#if defined(CATCHCHALLENGER_DB_POSTGRESQL)
class EventLoopPostgresql;
typedef EventLoopPostgresql EventLoopDb;
#elif defined(CATCHCHALLENGER_DB_MYSQL)
class EventLoopMySQL;
typedef EventLoopMySQL EventLoopDb;
#endif

#endif // CATCHCHALLENGER_EVENT_LOOP_DATABASE_H
#endif // outer DB-backend guard
