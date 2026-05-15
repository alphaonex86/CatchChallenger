// DbFileRam.cpp -- runtime setup/teardown for CATCHCHALLENGER_DB_FILE_RAM.
//
// Compiled into the server binary only when CATCHCHALLENGER_DB_FILE_RAM
// is defined. The TU is empty otherwise so the link cost is zero in
// production builds. See server/base/ServerStructures.hpp for the
// macro that points every FileDB path-handling site at the prefix
// initialised here.

#ifdef CATCHCHALLENGER_DB_FILE_RAM

#include "ServerStructures.hpp"
#include "DbFileRam.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace CatchChallenger {

// Defined as `extern` in ServerStructures.hpp. Empty until init().
std::string dbFileRamPrefix;

static bool s_dbFileRamRegistered = false;

static void rm_rf(const std::string &path)
{
    // Best-effort recursive delete; refuses anything outside /dev/shm/.
    if(path.size() < strlen("/dev/shm/") ||
       path.compare(0, strlen("/dev/shm/"), "/dev/shm/") != 0)
        return;
    DIR *dir = ::opendir(path.c_str());
    if(dir != nullptr)
    {
        struct dirent *entry;
        while((entry = ::readdir(dir)) != nullptr)
        {
            const std::string name = entry->d_name;
            if(name == "." || name == "..") continue;
            const std::string sub = path + "/" + name;
            struct stat sb;
            if(::lstat(sub.c_str(), &sb) != 0) continue;
            if(S_ISDIR(sb.st_mode)) rm_rf(sub);
            else                    ::unlink(sub.c_str());
        }
        ::closedir(dir);
    }
    ::rmdir(path.c_str());
}

static void cleanup_atexit()
{
    if(dbFileRamPrefix.empty()) return;
    rm_rf(dbFileRamPrefix);
}

void DbFileRam::init()
{
    if(!dbFileRamPrefix.empty()) return;   // already initialised
    char prefix[64];
    ::snprintf(prefix, sizeof(prefix), "/dev/shm/cc-server-%d", (int)::getpid());
    dbFileRamPrefix = prefix;

    // Wipe any stale tree from a previous run of the same pid (pid
    // reuse after a crash). Then create the directory hierarchy the
    // FileDB code expects -- mirrors the CC_MKDIR block in
    // server/cli/main-unix.cpp, but rooted at the tmpfs prefix.
    rm_rf(dbFileRamPrefix);
    const mode_t m = 0700;
    ::mkdir(dbFileRamPrefix.c_str(),                  m);
    ::mkdir((dbFileRamPrefix + "/database").c_str(),  m);
    ::mkdir((dbFileRamPrefix + "/database/login").c_str(),                m);
    ::mkdir((dbFileRamPrefix + "/database/common").c_str(),               m);
    ::mkdir((dbFileRamPrefix + "/database/common/accounts").c_str(),      m);
    ::mkdir((dbFileRamPrefix + "/database/common/characters").c_str(),    m);
    ::mkdir((dbFileRamPrefix + "/database/server").c_str(),               m);
    ::mkdir((dbFileRamPrefix + "/database/server/characters").c_str(),    m);
    ::mkdir((dbFileRamPrefix + "/database/server/clans").c_str(),         m);
    ::mkdir((dbFileRamPrefix + "/database/server/zone").c_str(),          m);

    if(!s_dbFileRamRegistered)
    {
        std::atexit(cleanup_atexit);
        s_dbFileRamRegistered = true;
    }
    std::cerr << "DB_FILE_RAM: in-RAM database rooted at "
              << dbFileRamPrefix << std::endl;
}

void DbFileRam::shutdown()
{
    cleanup_atexit();
    dbFileRamPrefix.clear();
}

}   // namespace CatchChallenger

#endif // CATCHCHALLENGER_DB_FILE_RAM
