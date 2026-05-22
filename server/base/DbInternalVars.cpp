// DbInternalVars.cpp -- storage + lifecycle for CATCHCHALLENGER_DB_INTERNAL_VARS
// (renamed from CATCHCHALLENGER_DB_FILE_RAM). Empty TU in every other
// build so the link cost is zero in production. See DbInternalVars.hpp.

#ifdef CATCHCHALLENGER_DB_INTERNAL_VARS

#include "DbInternalVars.hpp"
#include <iostream>

namespace CatchChallenger {

std::unordered_map<std::string, std::vector<uint8_t> > dbInternalVarsStore;

void DbInternalVars::init()
{
    dbInternalVarsStore.clear();
    std::cerr << "DB_INTERNAL_VARS: in-memory database (no disk, no file syscall)"
              << std::endl;
}

void DbInternalVars::shutdown()
{
    dbInternalVarsStore.clear();
}

}   // namespace CatchChallenger

#endif // CATCHCHALLENGER_DB_INTERNAL_VARS
