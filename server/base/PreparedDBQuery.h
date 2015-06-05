#ifndef CATCHCHALLENGER_SERVER_PREPAREDDBQUERY_H
#define CATCHCHALLENGER_SERVER_PREPAREDDBQUERY_H

#include "DatabaseBase.h"
#include <QString>

namespace CatchChallenger {

class PreparedDBQueryLogin
{
public:
    static void initDatabaseQueryLogin(const DatabaseBase::Type &type);
public:
    //query
    static QString db_query_login;
    static QString db_query_insert_login;
};

class PreparedDBQueryCommon
{
public:
    static void initDatabaseQueryCommonWithoutSP(const DatabaseBase::Type &type);
    static void initDatabaseQueryCommonWithSP(const DatabaseBase::Type &type,const bool &useSP);
public:
    //query
    static QString db_query_select_allow;
    static QString db_query_characters;
    static QString db_query_played_time;
    static QString db_query_monster_skill;
    static QString db_query_monster;
    static QString db_query_character_by_id;
    static QString db_query_update_character_time_to_delete;
    static QString db_query_update_character_last_connect;
    static QString db_query_clan;
    static QString db_query_select_server_time;
    static QString db_query_insert_server_time;
    static QString db_query_update_server_time_played_time;
    static QString db_query_update_server_time_last_connect;

    static QString db_query_select_monstersBuff_by_id;
    static QString db_query_monster_by_character_id;
    static QString db_query_delete_monster_buff;
    static QString db_query_delete_monster_specific_buff;
    static QString db_query_delete_monster_skill;
    static QString db_query_delete_character;
    static QString db_query_delete_all_item;
    static QString db_query_delete_all_item_warehouse;
    static QString db_query_delete_monster_by_character;
    static QString db_query_delete_monster_warehouse_by_character;
    static QString db_query_delete_monster_by_id;
    static QString db_query_delete_monster_warehouse_by_id;
    static QString db_query_delete_recipes;
    static QString db_query_delete_reputation;
    static QString db_query_delete_allow;

    static QString db_query_update_monster_move_to_player;
    static QString db_query_update_monster_move_to_new_player;
    static QString db_query_update_monster_move_to_warehouse;
    static QString db_query_update_monster_move_to_market;
    static QString db_query_select_clan_by_name;
    static QString db_query_select_character_by_pseudo;
    #ifdef CATCHCHALLENGER_CLASS_LOGIN
    static QString db_query_get_character_count_by_account;
    #endif
    static QString db_query_insert_monster;
    static QString db_query_insert_monster_full;
    static QString db_query_insert_warehouse_monster_full;
    static QString db_query_insert_monster_skill;
    static QString db_query_insert_reputation;
    static QString db_query_insert_item;
    static QString db_query_insert_item_warehouse;
    static QString db_query_account_time_to_delete_character_by_id;
    static QString db_query_update_character_time_to_delete_by_id;
    static QString db_query_select_reputation_by_id;
    static QString db_query_select_recipes_by_player_id;
    static QString db_query_select_items_by_player_id;
    static QString db_query_select_items_warehouse_by_player_id;
    static QString db_query_select_monsters_by_player_id;
    static QString db_query_select_monsters_warehouse_by_player_id;
    static QString db_query_select_monstersSkill_by_id;
    static QString db_query_change_right;
    static QString db_query_update_item;
    static QString db_query_update_item_warehouse;
    static QString db_query_delete_item;
    static QString db_query_delete_item_warehouse;
    static QString db_query_update_cash;
    static QString db_query_update_warehouse_cash;
    static QString db_query_insert_recipe;
    static QString db_query_insert_character_allow;
    static QString db_query_delete_character_allow;
    static QString db_query_update_reputation;
    static QString db_query_update_character_clan;
    static QString db_query_update_character_clan_and_leader;
    static QString db_query_delete_clan;
    static QString db_query_update_character_clan_by_pseudo;
    static QString db_query_update_monster_xp_hp_level;
    static QString db_query_update_monster_hp_only;
    static QString db_query_update_monster_sp_only;
    static QString db_query_update_monster_skill_level;
    static QString db_query_update_monster_xp;
    static QString db_query_insert_monster_buff;
    static QString db_query_update_monster_level;
    static QString db_query_update_monster_position;
    static QString db_query_update_monster_and_hp;
    static QString db_query_update_monster_level_only;
    static QString db_query_delete_monster_specific_skill;
    static QString db_query_insert_clan;
    static QString db_query_update_monster_owner;
};

class PreparedDBQueryServer
{
public:
    static void initDatabaseQueryServer(const DatabaseBase::Type &type);
public:
    static QString db_query_insert_bot_already_beaten;
    static QString db_query_character_server_by_id;
    static QString db_query_select_bot_beaten;
    static QString db_query_select_itemOnMap;
    static QString db_query_insert_itemonmap;
    static QString db_query_insert_recipe;
    static QString db_query_insert_factory;
    static QString db_query_update_factory;
    static QString db_query_delete_quest;
    static QString db_query_select_quest_by_id;
    static QString db_query_update_quest_finish;
    static QString db_query_update_quest_step;
    static QString db_query_update_quest_restart;
    static QString db_query_insert_quest;
    static QString db_query_delete_city;
    static QString db_query_update_city_clan;
    static QString db_query_insert_city;
    static QString db_query_delete_plant;
    static QString db_query_delete_plant_by_id;
    static QString db_query_insert_plant;
    static QString db_query_delete_bot_already_beaten;
    static QString db_query_insert_monster_market_price;
    static QString db_query_delete_monster_market_price;

    static QString db_query_delete_all_item_market;
    static QString db_query_insert_item_market;
    static QString db_query_delete_item_market;
    static QString db_query_update_item_market;
    static QString db_query_update_item_market_and_price;
    static QString db_query_update_charaters_market_cash;
    static QString db_query_get_market_cash;
    static QString db_query_update_character_forserver_map_part1;
    static QString db_query_update_character_forserver_map_part2;
};

}

#endif
