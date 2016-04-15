#ifndef CATCHCHALLENGER_SERVER_PREPAREDDBQUERY_H
#define CATCHCHALLENGER_SERVER_PREPAREDDBQUERY_H

#include "DatabaseBase.h"
#include "StringWithReplacement.h"
#include <string>

namespace CatchChallenger {

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
class PreparedDBQueryLogin
{
public:
    static void initDatabaseQueryLogin(const DatabaseBase::DatabaseType &type);
public:
    //query
    static StringWithReplacement db_query_login;
    static StringWithReplacement db_query_insert_login;
};
#endif

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
class PreparedDBQueryCommonForLogin
{
public:
    static void initDatabaseQueryCommonForLogin(const DatabaseBase::DatabaseType &type);
public:
    static StringWithReplacement db_query_characters;
    static StringWithReplacement db_query_characters_with_monsters;
    static StringWithReplacement db_query_select_server_time;
    static StringWithReplacement db_query_delete_character;
    static StringWithReplacement db_query_select_character_by_pseudo;
    static StringWithReplacement db_query_get_character_count_by_account;
    static StringWithReplacement db_query_account_time_to_delete_character_by_id;
    static StringWithReplacement db_query_update_character_time_to_delete_by_id;
};
#endif

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
class PreparedDBQueryCommon
{
public:
    static void initDatabaseQueryCommonWithoutSP(const DatabaseBase::DatabaseType &type);
    static void initDatabaseQueryCommonWithSP(const DatabaseBase::DatabaseType &type,const bool &useSP);
public:
    //login and gameserver alone
    static StringWithReplacement db_query_delete_monster_by_id;
    static StringWithReplacement db_query_insert_monster;

    #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
    static StringWithReplacement db_query_update_character_item;
    static StringWithReplacement db_query_update_character_item_and_encyclopedia;
    static StringWithReplacement db_query_update_character_item_warehouse;
    static StringWithReplacement db_query_update_character_monster;
    static StringWithReplacement db_query_update_character_monster_and_encyclopedia;
    static StringWithReplacement db_query_update_character_monster_warehouse;
    static StringWithReplacement db_query_update_cash;
    static StringWithReplacement db_query_update_warehouse_cash;
    static StringWithReplacement db_query_update_character_recipe;
    static StringWithReplacement db_query_update_character_allow;
    static StringWithReplacement db_query_update_character_reputations;
    static StringWithReplacement db_query_select_clan_by_name;
    static StringWithReplacement db_query_update_character_clan_to_reset;
    static StringWithReplacement db_query_update_character_clan_and_leader;
    static StringWithReplacement db_query_delete_clan;
    static StringWithReplacement db_query_update_monster_xp_hp_level;
    static StringWithReplacement db_query_update_monster_hp_only;
    static StringWithReplacement db_query_update_monster_sp_only;
    static StringWithReplacement db_query_update_monster_xp;
    static StringWithReplacement db_query_insert_clan;
    static StringWithReplacement db_query_played_time;
    static StringWithReplacement db_query_monster;
    static StringWithReplacement db_query_monster_skill_and_endurance;

    //query
    /*static StringWithReplacement db_query_select_allow;
    static StringWithReplacement db_query_character_by_id;
    static StringWithReplacement db_query_update_character_time_to_delete;
    static StringWithReplacement db_query_update_character_last_connect;
    static StringWithReplacement db_query_clan;
    static StringWithReplacement db_query_insert_server_time;
    static StringWithReplacement db_query_update_server_time_played_time;
    static StringWithReplacement db_query_update_server_time_last_connect;

    static StringWithReplacement db_query_select_monstersBuff_by_id;
    static StringWithReplacement db_query_monster_by_character_id;
    static StringWithReplacement db_query_delete_monster_buff;
    static StringWithReplacement db_query_delete_monster_specific_buff;
    static StringWithReplacement db_query_delete_monster_skill;
    static StringWithReplacement db_query_delete_all_item;
    static StringWithReplacement db_query_delete_all_item_warehouse;
    static StringWithReplacement db_query_delete_monster_by_character;
    static StringWithReplacement db_query_delete_monster_warehouse_by_id;
    static StringWithReplacement db_query_delete_recipes;
    static StringWithReplacement db_query_delete_reputation;
    static StringWithReplacement db_query_delete_allow;
    static StringWithReplacement db_query_update_monster_buff_level;

    static StringWithReplacement db_query_update_monster_move_to_player;
    static StringWithReplacement db_query_update_monster_move_to_new_player;
    static StringWithReplacement db_query_update_monster_move_to_warehouse;
    static StringWithReplacement db_query_update_monster_move_to_market;
    static StringWithReplacement db_query_insert_monster_full;
    static StringWithReplacement db_query_insert_warehouse_monster_full;
    static StringWithReplacement db_query_insert_monster_skill;
    static StringWithReplacement db_query_insert_reputation;
    static StringWithReplacement db_query_select_reputation_by_id;
    static StringWithReplacement db_query_select_recipes_by_player_id;
    static StringWithReplacement db_query_select_items_by_player_id;
    static StringWithReplacement db_query_select_items_warehouse_by_player_id;
    static StringWithReplacement db_query_select_monsters_by_player_id;
    static StringWithReplacement db_query_select_monsters_warehouse_by_player_id;
    static StringWithReplacement db_query_select_monstersSkill_by_id;
    static StringWithReplacement db_query_change_right;
    static StringWithReplacement db_query_insert_item;
    static StringWithReplacement db_query_insert_item_warehouse;
    static StringWithReplacement db_query_update_item;
    static StringWithReplacement db_query_update_item_warehouse;
    static StringWithReplacement db_query_delete_item;
    static StringWithReplacement db_query_delete_item_warehouse;
    static StringWithReplacement db_query_insert_recipe;

    static StringWithReplacement db_query_delete_character_allow;
    static StringWithReplacement db_query_update_reputation;

    static StringWithReplacement db_query_update_monster_skill_level;
    static StringWithReplacement db_query_insert_monster_buff;
    static StringWithReplacement db_query_update_monster_position;
    static StringWithReplacement db_query_update_monster_and_hp;
    static StringWithReplacement db_query_delete_monster_specific_skill;
    static StringWithReplacement db_query_update_monster_owner;*/
    #endif
};
#endif

#if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
class PreparedDBQueryServer
{
public:
    static void initDatabaseQueryServer(const DatabaseBase::DatabaseType &type);
public:
    static StringWithReplacement db_query_update_character_forserver_map;
    static StringWithReplacement db_query_insert_factory;
    static StringWithReplacement db_query_update_factory;
    static StringWithReplacement db_query_delete_city;
    static StringWithReplacement db_query_update_city_clan;
    static StringWithReplacement db_query_insert_city;
    static StringWithReplacement db_query_insert_monster_market_price;
    static StringWithReplacement db_query_delete_monster_market_price;
    static StringWithReplacement db_query_delete_all_item_market;
    static StringWithReplacement db_query_insert_item_market;
    static StringWithReplacement db_query_delete_item_market;
    static StringWithReplacement db_query_update_item_market;
    static StringWithReplacement db_query_update_item_market_and_price;
    static StringWithReplacement db_query_update_charaters_market_cash;
    static StringWithReplacement db_query_get_market_cash;
    static StringWithReplacement db_query_insert_monster_market;
    static StringWithReplacement db_query_update_character_quests;

    /*
    static StringWithReplacement db_query_insert_bot_already_beaten;
    static StringWithReplacement db_query_character_server_by_id;
    static StringWithReplacement db_query_select_bot_beaten;
    static StringWithReplacement db_query_select_itemOnMap;
    static StringWithReplacement db_query_insert_itemonmap;
    static StringWithReplacement db_query_insert_recipe;
    static StringWithReplacement db_query_delete_quest;
    static StringWithReplacement db_query_select_quest_by_id;
    static StringWithReplacement db_query_update_quest_finish;
    static StringWithReplacement db_query_update_quest_step;
    static StringWithReplacement db_query_update_quest_restart;
    static StringWithReplacement db_query_insert_quest;
    static StringWithReplacement db_query_select_plant;
    static StringWithReplacement db_query_delete_plant;
    static StringWithReplacement db_query_delete_plant_by_index;
    static StringWithReplacement db_query_insert_plant;
    static StringWithReplacement db_query_delete_bot_already_beaten;
*/
};
#endif

}

#endif
