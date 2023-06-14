#ifndef CATCHCHALLENGER_BASESERVERMASTERLOGINDICTIONARY_H
#define CATCHCHALLENGER_BASESERVERMASTERLOGINDICTIONARY_H

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include <vector>
#include <regex>
#include "DatabaseBase.hpp"

namespace CatchChallenger {
class BaseServerMasterLoadDictionary
{
public:
    explicit BaseServerMasterLoadDictionary();
    virtual ~BaseServerMasterLoadDictionary();
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    void load(CatchChallenger::DatabaseBase * const databaseBase);
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif
public:
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    DatabaseBase *databaseBaseBase;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif

    std::vector<int> dictionary_reputation_database_to_internal;//negative == not found
    std::vector<uint8_t> dictionary_skin_database_to_internal;
    std::vector<uint32_t> dictionary_skin_internal_to_database;
    std::vector<uint8_t> dictionary_starter_database_to_internal;
    std::vector<uint32_t> dictionary_starter_internal_to_database;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
private:
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif
    void preload_dictionary_reputation();
private:
    virtual void SQL_common_load_finish() = 0;

    static void preload_dictionary_reputation_static(void *object);
    void preload_dictionary_reputation_return();
    void preload_dictionary_skin();
    static void preload_dictionary_skin_static(void *object);
    void preload_dictionary_skin_return();
    void preload_dictionary_starter();
    static void preload_dictionary_starter_static(void *object);
    void preload_dictionary_starter_return();
protected:
    void unload();
};
}
#endif

#endif
