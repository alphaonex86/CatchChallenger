#if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
#include "PreparedDBQuery.hpp"
#include <iostream>

using namespace CatchChallenger;

#if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
void PreparedDBQueryServer::initDatabaseQueryServer(const DatabaseBase::DatabaseType &type, CatchChallenger::DatabaseBase * const database)
{
    switch(type)
    {
        default:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        std::cerr << "PreparedDBQuery: Unknown database type" << std::endl;
        #else
        std::cerr << "PreparedDBQuery: Unknown database type in epoll mode" << std::endl;
        #endif
        abort();
        return;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER)) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::Mysql:
        PreparedDBQueryServer::db_query_update_character_forserver_map=PreparedStatementUnit("UPDATE `character_forserver` SET `map`=%1,`x`=%2,`y`=%3,`orientation`=%4,`rescue_map`=%5,`rescue_x`=%6,`rescue_y`=%7,`rescue_orientation`=%8,`unvalidated_rescue_map`=%9,`unvalidated_rescue_x`=%10,`unvalidated_rescue_y`=%11,`unvalidated_rescue_orientation`=%12 WHERE `character`=%13",database);
        PreparedDBQueryServer::db_query_character_server_by_id=PreparedStatementUnit("SELECT `map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,LOWER(HEX(`botfight_id`)),LOWER(HEX(`itemonmap`)),LOWER(HEX(`quest`)),`blob_version`,`date`,LOWER(HEX(`plants`)) FROM `character_forserver` WHERE `character`=%1",database);
        PreparedDBQueryServer::db_query_delete_character_server_by_id=PreparedStatementUnit("DELETE FROM `character_forserver` WHERE `character`=%1",database);
        PreparedDBQueryServer::db_query_insert_factory=PreparedStatementUnit("INSERT INTO `factory`(`id`,`resources`,`products`,`last_update`) VALUES(%1,'%2','%3',%4)",database);
        PreparedDBQueryServer::db_query_update_factory=PreparedStatementUnit("UPDATE `factory` SET `resources`='%1',`products`='%2',`last_update`=%3 WHERE `id`=%4",database);
        PreparedDBQueryServer::db_query_delete_city=PreparedStatementUnit("DELETE FROM `city` WHERE `city`='%1'",database);
        PreparedDBQueryServer::db_query_update_city_clan=PreparedStatementUnit("UPDATE `city` SET `clan`=%1 WHERE `city`='%2';",database);
        PreparedDBQueryServer::db_query_insert_city=PreparedStatementUnit("INSERT INTO `city`(`clan`,`city`) VALUES(%1,'%2');",database);
        PreparedDBQueryServer::db_query_update_character_quests=PreparedStatementUnit("UPDATE character_forserver SET quest=UNHEX('%1') WHERE character=%2",database);
        PreparedDBQueryServer::db_query_update_plant=PreparedStatementUnit("UPDATE character_forserver SET plants=UNHEX('%1') WHERE character=%2",database);
        PreparedDBQueryServer::db_query_update_itemonmap=PreparedStatementUnit("UPDATE character_forserver SET itemonmap=UNHEX('%1') WHERE character=%2",database);
        PreparedDBQueryServer::db_query_update_character_bot_already_beaten=PreparedStatementUnit("UPDATE character_forserver SET botfight_id=UNHEX('%1') WHERE character=%2",database);
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        PreparedDBQueryServer::db_query_update_character_forserver_map=PreparedStatementUnit("UPDATE character_forserver SET map=%1,x=%2,y=%3,orientation=%4,rescue_map=%5,rescue_x=%6,rescue_y=%7,rescue_orientation=%8,unvalidated_rescue_map=%9,unvalidated_rescue_x=%10,unvalidated_rescue_y=%11,unvalidated_rescue_orientation=%12 WHERE character=%13",database);

        PreparedDBQueryServer::db_query_character_server_by_id=PreparedStatementUnit("SELECT map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,botfight_id,itemonmap,quest,blob_version,date,plants FROM character_forserver WHERE character=%1",database);
        PreparedDBQueryServer::db_query_delete_character_server_by_id=PreparedStatementUnit("DELETE FROM character_forserver WHERE character=%1",database);
        PreparedDBQueryServer::db_query_insert_factory=PreparedStatementUnit("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)",database);
        PreparedDBQueryServer::db_query_update_factory=PreparedStatementUnit("UPDATE factory SET resources='%1',products='%2',last_update=%3 WHERE id=%4",database);
        PreparedDBQueryServer::db_query_delete_city=PreparedStatementUnit("DELETE FROM city WHERE city='%1'",database);
        PreparedDBQueryServer::db_query_update_city_clan=PreparedStatementUnit("UPDATE city SET clan=%1 WHERE city='%2';",database);
        PreparedDBQueryServer::db_query_insert_city=PreparedStatementUnit("INSERT INTO city(clan,city) VALUES(%1,'%2');",database);
        PreparedDBQueryServer::db_query_update_character_quests=PreparedStatementUnit("UPDATE character_forserver SET quest='%1' WHERE character=%2",database);
        PreparedDBQueryServer::db_query_update_plant=PreparedStatementUnit("UPDATE character_forserver SET plants='%1' WHERE character=%2",database);
        PreparedDBQueryServer::db_query_update_itemonmap=PreparedStatementUnit("UPDATE character_forserver SET itemonmap='%1' WHERE character=%2",database);
        PreparedDBQueryServer::db_query_update_character_bot_already_beaten=PreparedStatementUnit("UPDATE character_forserver SET botfight_id='%1' WHERE character=%2",database);
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
        PreparedDBQueryServer::db_query_update_character_forserver_map=PreparedStatementUnit("UPDATE character_forserver SET map=%1,x=%2,y=%3,orientation=%4,rescue_map=%5,rescue_x=%6,rescue_y=%7,rescue_orientation=%8,unvalidated_rescue_map=%9,unvalidated_rescue_x=%10,unvalidated_rescue_y=%11,unvalidated_rescue_orientation=%12 WHERE character=%13",database);

        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        PreparedDBQueryServer::db_query_insert_factory=PreparedStatementUnit("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,%2,%3,%4)",database);
        PreparedDBQueryServer::db_query_update_factory=PreparedStatementUnit("UPDATE factory SET resources=%1,products=%2,last_update=%3 WHERE id=%4",database);
        #else
        PreparedDBQueryServer::db_query_insert_factory=PreparedStatementUnit("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)",database);
        PreparedDBQueryServer::db_query_update_factory=PreparedStatementUnit("UPDATE factory SET resources='%1',products='%2',last_update=%3 WHERE id=%4",database);
        #endif
        PreparedDBQueryServer::db_query_delete_city=PreparedStatementUnit("DELETE FROM city WHERE city='%1'",database);
        PreparedDBQueryServer::db_query_update_city_clan=PreparedStatementUnit("UPDATE city SET clan=%1 WHERE city='%2';",database);
        PreparedDBQueryServer::db_query_insert_city=PreparedStatementUnit("INSERT INTO city(clan,city) VALUES(%1,'%2');",database);

        PreparedDBQueryServer::db_query_update_character_quests=PreparedStatementUnit("UPDATE character_forserver SET quest=decode('%1','hex') WHERE character=%2",database);
        PreparedDBQueryServer::db_query_character_server_by_id=PreparedStatementUnit("SELECT map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,encode(botfight_id,'hex'),encode(itemonmap,'hex'),encode(quest,'hex'),blob_version,date,encode(plants,'hex') FROM character_forserver WHERE character=%1",database);
        PreparedDBQueryServer::db_query_delete_character_server_by_id=PreparedStatementUnit("DELETE FROM character_forserver WHERE character=%1",database);
        PreparedDBQueryServer::db_query_update_plant=PreparedStatementUnit("UPDATE character_forserver SET plants=decode('%1','hex') WHERE character=%2",database);
        PreparedDBQueryServer::db_query_update_itemonmap=PreparedStatementUnit("UPDATE character_forserver SET itemonmap=decode('%1','hex') WHERE character=%2",database);
        PreparedDBQueryServer::db_query_update_character_bot_already_beaten=PreparedStatementUnit("UPDATE character_forserver SET botfight_id=decode('%1','hex') WHERE character=%2",database);
        break;
        #endif
    }
}
#endif
#elif CATCHCHALLENGER_DB_BLACKHOLE
#elif CATCHCHALLENGER_DB_FILE
#else
#error Define what do here
#endif
