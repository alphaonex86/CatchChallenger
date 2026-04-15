#if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
#ifndef CATCHCHALLENGER_SERVER_PREPAREDDBQUERY_H
#define CATCHCHALLENGER_SERVER_PREPAREDDBQUERY_H

#include "DatabaseBase.hpp"
#include "PreparedStatementUnit.hpp"
#include <string>

namespace CatchChallenger {

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
class PreparedDBQueryLogin
{
public:
    static void initDatabaseQueryLogin(const DatabaseBase::DatabaseType &type, DatabaseBase * const database);
public:
    //query
    static PreparedStatementUnit db_query_login;
    static PreparedStatementUnit db_query_insert_login;
};
#endif

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
class PreparedDBQueryCommonForLogin
{
public:
    void initDatabaseQueryCommonForLogin(const DatabaseBase::DatabaseType &type,CatchChallenger::DatabaseBase * const database);
public:
    PreparedStatementUnit db_query_characters;//should be already limited at creation (write access), not need limit on read
    //static PreparedStatementUnit db_query_characters_with_monsters;
    PreparedStatementUnit db_query_select_server_time;
    PreparedStatementUnit db_query_delete_character;
    PreparedStatementUnit db_query_delete_monster_by_character;
    PreparedStatementUnit db_query_select_character_by_pseudo;
    PreparedStatementUnit db_query_get_character_count_by_account;
    PreparedStatementUnit db_query_account_time_to_delete_character_by_id;
    PreparedStatementUnit db_query_update_character_time_to_delete_by_id;
};
#endif

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
class PreparedDBQueryCommon
{
public:
    void initDatabaseQueryCommonWithoutSP(const DatabaseBase::DatabaseType &type,CatchChallenger::DatabaseBase * const database);
    void initDatabaseQueryCommonWithSP(const DatabaseBase::DatabaseType &type,CatchChallenger::DatabaseBase * const database);
public:
    //login and gameserver alone
    PreparedStatementUnit db_query_delete_monster_by_id;
    static StringWithReplacement db_query_insert_monster;
    PreparedStatementUnit db_query_insert_monster_full;/*wild catch*/

    #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
    PreparedStatementUnit db_query_update_character_item;
    PreparedStatementUnit db_query_update_character_item_and_encyclopedia;
    PreparedStatementUnit db_query_update_character_item_warehouse;
    /*PreparedStatementUnit db_query_update_character_monster;
    PreparedStatementUnit db_query_update_character_monster_and_encyclopedia;
    PreparedStatementUnit db_query_update_character_monster_warehouse;*/
    PreparedStatementUnit db_query_update_character_monster_encyclopedia;
    PreparedStatementUnit db_query_update_cash;
    PreparedStatementUnit db_query_update_warehouse_cash;
    PreparedStatementUnit db_query_update_character_recipe;
    PreparedStatementUnit db_query_update_character_allow;
    PreparedStatementUnit db_query_update_character_reputations;
    PreparedStatementUnit db_query_select_clan_by_name;
    PreparedStatementUnit db_query_update_character_clan_to_reset;
    PreparedStatementUnit db_query_update_character_clan_and_leader;
    PreparedStatementUnit db_query_delete_clan;
    PreparedStatementUnit db_query_update_monster_xp_hp_level;
    PreparedStatementUnit db_query_update_monster_hp_only;
    PreparedStatementUnit db_query_update_monster_xp;
    PreparedStatementUnit db_query_insert_clan;
    PreparedStatementUnit db_query_played_time;
    PreparedStatementUnit db_query_monster_update_skill_and_endurance;
    PreparedStatementUnit db_query_monster_update_endurance;
    PreparedStatementUnit db_query_update_monster_buff;
    PreparedStatementUnit db_query_character_by_id;
    PreparedStatementUnit db_query_set_character_time_to_delete_to_zero;
    PreparedStatementUnit db_query_update_character_last_connect;
    PreparedStatementUnit db_query_clan;
    PreparedStatementUnit db_query_change_right;
    PreparedStatementUnit db_query_delete_monster_buff;
    PreparedStatementUnit db_query_insert_warehouse_monster;
    PreparedStatementUnit db_query_update_monster_position;
    PreparedStatementUnit db_query_update_monster_position_and_place;
    PreparedStatementUnit db_query_update_monster_place;
    PreparedStatementUnit db_query_update_monster_and_hp;
    PreparedStatementUnit db_query_update_monster_hp_and_level;
    PreparedStatementUnit db_query_select_monsters_by_player_id;//don't filter by place, dispatched in internal
    PreparedStatementUnit db_query_update_monster_move_to_new_player;
    PreparedStatementUnit db_query_update_monster_owner;
    PreparedStatementUnit db_query_update_gift;

    #if defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
    PreparedStatementUnit db_query_insert_server_time;
    PreparedStatementUnit db_query_update_server_time_played_time;
    PreparedStatementUnit db_query_update_server_time_last_connect;
    #endif
    #endif
};
#endif

#if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
class PreparedDBQueryServer
{
public:
    void initDatabaseQueryServer(const DatabaseBase::DatabaseType &type,CatchChallenger::DatabaseBase * const database);
public:
    PreparedStatementUnit db_query_update_character_forserver_map;
    PreparedStatementUnit db_query_insert_factory;
    PreparedStatementUnit db_query_update_factory;
    PreparedStatementUnit db_query_delete_city;
    PreparedStatementUnit db_query_update_city_clan;
    PreparedStatementUnit db_query_insert_city;
    PreparedStatementUnit db_query_update_character_quests;
    PreparedStatementUnit db_query_character_server_by_id;
    PreparedStatementUnit db_query_delete_character_server_by_id;
    PreparedStatementUnit db_query_select_character_bymap;
    PreparedStatementUnit db_query_update_plant;
    PreparedStatementUnit db_query_update_itemonmap;
    PreparedStatementUnit db_query_update_character_bot_already_beaten;
    PreparedStatementUnit db_query_update_industries;
};
#endif

}

#endif
#elif CATCHCHALLENGER_DB_BLACKHOLE
#elif CATCHCHALLENGER_DB_FILE
#else
#error Define what do here
#endif
