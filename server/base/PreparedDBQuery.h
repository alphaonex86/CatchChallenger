#ifndef CATCHCHALLENGER_SERVER_PREPAREDDBQUERY_H
#define CATCHCHALLENGER_SERVER_PREPAREDDBQUERY_H

#include "DatabaseBase.h"
#include <string>

namespace CatchChallenger {

class PreparedDBQuery
{
public:
    PreparedDBQuery();
    void initDatabaseQuery(const DatabaseBase::Type &type,const bool &useSP);
public:
    static char * db_type_string;

    //query
    static char * db_query_select_allow;
    static char * db_query_login;
    static char * db_query_insert_login;
    static char * db_query_characters;
    static char * db_query_played_time;
    static char * db_query_monster_skill;
    static char * db_query_monster;
    static char * db_query_character_by_id;
    static char * db_query_update_character_time_to_delete;
    static char * db_query_update_character_last_connect;
    static char * db_query_clan;

    static char * db_query_monster_by_character_id;
    static char * db_query_delete_monster_buff;
    static char * db_query_delete_monster_specific_buff;
    static char * db_query_delete_monster_skill;
    static char * db_query_delete_bot_already_beaten;
    static char * db_query_delete_character;
    static char * db_query_delete_all_item;
    static char * db_query_delete_all_item_warehouse;
    static char * db_query_delete_all_item_market;
    static char * db_query_delete_monster_by_character;
    static char * db_query_delete_monster_warehouse_by_character;
    static char * db_query_delete_monster_market_by_character;
    static char * db_query_delete_monster_by_id;
    static char * db_query_delete_monster_warehouse_by_id;
    static char * db_query_delete_monster_market_by_id;
    static char * db_query_delete_plant;
    static char * db_query_delete_plant_by_id;
    static char * db_query_delete_quest;
    static char * db_query_delete_recipes;
    static char * db_query_delete_reputation;
    static char * db_query_delete_allow;

    static char * db_query_select_clan_by_name;
    static char * db_query_select_character_by_pseudo;
    static char * db_query_insert_monster;
    static char * db_query_insert_monster_full;
    static char * db_query_insert_warehouse_monster_full;
    static char * db_query_insert_monster_skill;
    static char * db_query_insert_reputation;
    static char * db_query_insert_item;
    static char * db_query_insert_item_warehouse;
    static char * db_query_insert_item_market;
    static char * db_query_account_time_to_delete_character_by_id;
    static char * db_query_update_character_time_to_delete_by_id;
    static char * db_query_select_reputation_by_id;
    static char * db_query_select_quest_by_id;
    static char * db_query_select_recipes_by_player_id;
    static char * db_query_select_items_by_player_id;
    static char * db_query_select_items_warehouse_by_player_id;
    static char * db_query_select_monsters_by_player_id;
    static char * db_query_select_monsters_warehouse_by_player_id;
    static char * db_query_select_monstersSkill_by_id;
    static char * db_query_select_monstersBuff_by_id;
    static char * db_query_select_bot_beaten;
    static char * db_query_select_itemOnMap;
    static char * db_query_insert_itemonmap;
    static char * db_query_change_right;
    static char * db_query_update_item;
    static char * db_query_update_item_warehouse;
    static char * db_query_delete_item;
    static char * db_query_delete_item_warehouse;
    static char * db_query_update_cash;
    static char * db_query_update_warehouse_cash;
    static char * db_query_insert_recipe;
    static char * db_query_insert_factory;
    static char * db_query_update_factory;
    static char * db_query_insert_character_allow;
    static char * db_query_delete_character_allow;
    static char * db_query_update_reputation;
    static char * db_query_update_character_clan;
    static char * db_query_update_character_clan_and_leader;
    static char * db_query_delete_clan;
    static char * db_query_delete_city;
    static char * db_query_update_character_clan_by_pseudo;
    static char * db_query_update_monster_xp_hp_level;
    static char * db_query_update_monster_hp_only;
    static char * db_query_update_monster_sp_only;
    static char * db_query_update_monster_skill_level;
    static char * db_query_insert_monster_catch;
    static char * db_query_update_monster_xp;
    static char * db_query_insert_bot_already_beaten;
    static char * db_query_insert_monster_buff;
    static char * db_query_update_monster_level;
    static char * db_query_update_monster_position;
    static char * db_query_update_monster_and_hp;
    static char * db_query_update_monster_level_only;
    static char * db_query_delete_monster_specific_skill;
    static char * db_query_insert_clan;
    static char * db_query_insert_plant;
    static char * db_query_update_monster_owner;
    static char * db_query_update_quest_finish;
    static char * db_query_update_quest_step;
    static char * db_query_update_quest_restart;
    static char * db_query_insert_quest;
    static char * db_query_update_city_clan;
    static char * db_query_insert_city;
    static char * db_query_delete_item_market;
    static char * db_query_update_item_market;
    static char * db_query_update_item_market_and_price;
    static char * db_query_update_charaters_market_cash;
    static char * db_query_insert_monster_market;
    static char * db_query_get_market_cash;
};

}

#endif
