#include "PreparedDBQuery.hpp"
#include <iostream>

using namespace CatchChallenger;

//query
#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
PreparedStatementUnit PreparedDBQueryLogin::db_query_login;
PreparedStatementUnit PreparedDBQueryLogin::db_query_insert_login;
#endif

StringWithReplacement PreparedDBQueryCommon::db_query_insert_monster;

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
void PreparedDBQueryLogin::initDatabaseQueryLogin(const DatabaseBase::DatabaseType &type, CatchChallenger::DatabaseBase * const database)
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
        PreparedDBQueryLogin::db_query_login=PreparedStatementUnit("SELECT `id`,LOWER(HEX(`password`)) FROM `account` WHERE `login`=UNHEX('%1')",database);
        PreparedDBQueryLogin::db_query_insert_login=PreparedStatementUnit("INSERT INTO account(id,login,password,date) VALUES(%1,UNHEX('%2'),UNHEX('%3'),%4)",database);
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        PreparedDBQueryLogin::db_query_login=PreparedStatementUnit("SELECT id,password FROM account WHERE login='%1'",database);
        PreparedDBQueryLogin::db_query_insert_login=PreparedStatementUnit("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4)",database);
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
        PreparedDBQueryLogin::db_query_login=PreparedStatementUnit("SELECT id,encode(password,'hex') FROM account WHERE login=decode('%1','hex')",database);
        PreparedDBQueryLogin::db_query_insert_login=PreparedStatementUnit("INSERT INTO account(id,login,password,date) VALUES(%1,decode('%2','hex'),decode('%3','hex'),%4)",database);
        break;
        #endif
    }
}
#endif

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
void PreparedDBQueryCommonForLogin::initDatabaseQueryCommonForLogin(const DatabaseBase::DatabaseType &type, CatchChallenger::DatabaseBase * const database)
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
        PreparedDBQueryCommonForLogin::db_query_characters=PreparedStatementUnit("SELECT `id`,`pseudo`,`skin`,`time_to_delete`,`played_time`,`last_connect` FROM `character` WHERE `account`=%1",database);
        PreparedDBQueryCommonForLogin::db_query_select_server_time=PreparedStatementUnit("SELECT `server`,`played_time`,`last_connect` FROM `server_time` WHERE `account`=%1",database);//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommonForLogin::db_query_delete_character=PreparedStatementUnit("DELETE FROM `character` WHERE `id`=%1",database);
        PreparedDBQueryCommonForLogin::db_query_delete_monster_by_character=PreparedStatementUnit("DELETE FROM `monster` WHERE `character`=%1",database);
        PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo=PreparedStatementUnit("SELECT `id` FROM `character` WHERE `pseudo`='%1'",database);
        PreparedDBQueryCommonForLogin::db_query_get_character_count_by_account=PreparedStatementUnit("SELECT COUNT(*) FROM `character` WHERE `account`=%1",database);
        PreparedDBQueryCommonForLogin::db_query_account_time_to_delete_character_by_id=PreparedStatementUnit("SELECT `account`,`time_to_delete` FROM `character` WHERE `id`=%1",database);
        PreparedDBQueryCommonForLogin::db_query_update_character_time_to_delete_by_id=PreparedStatementUnit("UPDATE `character` SET `time_to_delete`=%1 WHERE `id`=%2",database);
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        PreparedDBQueryCommonForLogin::db_query_characters=PreparedStatementUnit("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect FROM character WHERE account=%1",database);
        PreparedDBQueryCommonForLogin::db_query_select_server_time=PreparedStatementUnit("SELECT server,played_time,last_connect FROM server_time WHERE account=%1",database);//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommonForLogin::db_query_delete_character=PreparedStatementUnit("DELETE FROM character WHERE id=%1",database);
        PreparedDBQueryCommonForLogin::db_query_delete_monster_by_character=PreparedStatementUnit("DELETE FROM monster WHERE character=%1",database);
        PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo=PreparedStatementUnit("SELECT id FROM character WHERE pseudo='%1'",database);
        PreparedDBQueryCommonForLogin::db_query_get_character_count_by_account=PreparedStatementUnit("SELECT COUNT(*) FROM character WHERE account=%1",database);
        PreparedDBQueryCommonForLogin::db_query_account_time_to_delete_character_by_id=PreparedStatementUnit("SELECT account,time_to_delete FROM character WHERE id=%1",database);
        PreparedDBQueryCommonForLogin::db_query_update_character_time_to_delete_by_id=PreparedStatementUnit("UPDATE character SET time_to_delete=%1 WHERE id=%2",database);
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
        PreparedDBQueryCommonForLogin::db_query_characters=PreparedStatementUnit("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect FROM character WHERE account=%1",database);
        PreparedDBQueryCommonForLogin::db_query_select_server_time=PreparedStatementUnit("SELECT server,played_time,last_connect FROM server_time WHERE account=%1",database);//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommonForLogin::db_query_delete_character=PreparedStatementUnit("DELETE FROM character WHERE id=%1",database);
        PreparedDBQueryCommonForLogin::db_query_delete_monster_by_character=PreparedStatementUnit("DELETE FROM monster WHERE character=%1",database);
        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo=PreparedStatementUnit("SELECT id FROM character WHERE pseudo=%1",database);
        #else
        PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo=PreparedStatementUnit("SELECT id FROM character WHERE pseudo='%1'",database);
        #endif
        PreparedDBQueryCommonForLogin::db_query_get_character_count_by_account=PreparedStatementUnit("SELECT COUNT(*) FROM character WHERE account=%1",database);
        PreparedDBQueryCommonForLogin::db_query_account_time_to_delete_character_by_id=PreparedStatementUnit("SELECT account,time_to_delete FROM character WHERE id=%1",database);
        PreparedDBQueryCommonForLogin::db_query_update_character_time_to_delete_by_id=PreparedStatementUnit("UPDATE character SET time_to_delete=%1 WHERE id=%2",database);
        break;
        #endif
    }
}
#endif
