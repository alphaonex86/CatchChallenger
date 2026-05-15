// DbFileRam.hpp -- lifecycle hooks for CATCHCHALLENGER_DB_FILE_RAM.
//
// init()     -- creates the tmpfs prefix tree under /dev/shm/cc-server-<pid>/.
//               Idempotent. Registers an atexit() cleanup hook on first call.
// shutdown() -- recursive remove of the prefix; safe to call from any
//               server-shutdown path. Idempotent.
//
// Both are no-ops in production builds (the macro guard collapses the
// TU to nothing, see DbFileRam.cpp).

#ifndef CATCHCHALLENGER_DB_FILE_RAM_HPP
#define CATCHCHALLENGER_DB_FILE_RAM_HPP

#ifdef CATCHCHALLENGER_DB_FILE_RAM
namespace CatchChallenger {
class DbFileRam
{
public:
    static void init();
    static void shutdown();
};
}
#endif

#endif // CATCHCHALLENGER_DB_FILE_RAM_HPP
