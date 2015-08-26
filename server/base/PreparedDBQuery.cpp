#include "PreparedDBQuery.h"

using namespace CatchChallenger;

//query
std::string PreparedDBQueryLogin::db_query_login=NULL;
std::string PreparedDBQueryLogin::db_query_insert_login=NULL;

std::string PreparedDBQueryServer::db_query_character_server_by_id=NULL;
std::string PreparedDBQueryServer::db_query_delete_all_item_market=NULL;
std::string PreparedDBQueryServer::db_query_insert_item_market=NULL;
std::string PreparedDBQueryServer::db_query_delete_item_market=NULL;
std::string PreparedDBQueryServer::db_query_update_item_market=NULL;
std::string PreparedDBQueryServer::db_query_update_item_market_and_price=NULL;
std::string PreparedDBQueryServer::db_query_update_charaters_market_cash=NULL;
std::string PreparedDBQueryServer::db_query_get_market_cash=NULL;
std::string PreparedDBQueryServer::db_query_insert_monster_market_price=NULL;
std::string PreparedDBQueryServer::db_query_delete_monster_market_price=NULL;

std::string PreparedDBQueryServer::db_query_insert_plant=NULL;
std::string PreparedDBQueryServer::db_query_update_quest_finish=NULL;
std::string PreparedDBQueryServer::db_query_update_quest_step=NULL;
std::string PreparedDBQueryServer::db_query_update_quest_restart=NULL;
std::string PreparedDBQueryServer::db_query_insert_quest=NULL;
std::string PreparedDBQueryServer::db_query_update_city_clan=NULL;
std::string PreparedDBQueryServer::db_query_insert_city=NULL;
std::string PreparedDBQueryServer::db_query_select_plant=NULL;
std::string PreparedDBQueryServer::db_query_delete_plant=NULL;
std::string PreparedDBQueryServer::db_query_delete_plant_by_index=NULL;
std::string PreparedDBQueryServer::db_query_delete_quest=NULL;
std::string PreparedDBQueryServer::db_query_select_quest_by_id=NULL;
std::string PreparedDBQueryServer::db_query_select_bot_beaten=NULL;
std::string PreparedDBQueryServer::db_query_select_itemOnMap=NULL;
std::string PreparedDBQueryServer::db_query_insert_itemonmap=NULL;
std::string PreparedDBQueryServer::db_query_insert_factory=NULL;
std::string PreparedDBQueryServer::db_query_update_factory=NULL;
std::string PreparedDBQueryServer::db_query_delete_city=NULL;
std::string PreparedDBQueryServer::db_query_insert_bot_already_beaten=NULL;
std::string PreparedDBQueryServer::db_query_delete_bot_already_beaten=NULL;

std::string PreparedDBQueryCommon::db_query_select_monstersBuff_by_id=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_move_to_player;
std::string PreparedDBQueryCommon::db_query_update_monster_move_to_new_player;
std::string PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse;
std::string PreparedDBQueryCommon::db_query_update_monster_move_to_market;
std::string PreparedDBQueryCommon::db_query_select_allow=NULL;
std::string PreparedDBQueryCommon::db_query_characters=NULL;
std::string PreparedDBQueryCommon::db_query_played_time=NULL;
std::string PreparedDBQueryCommon::db_query_monster_skill=NULL;
std::string PreparedDBQueryCommon::db_query_monster=NULL;
std::string PreparedDBQueryCommon::db_query_character_by_id=NULL;
std::string PreparedDBQueryCommon::db_query_update_character_time_to_delete=NULL;
std::string PreparedDBQueryCommon::db_query_update_character_last_connect=NULL;
std::string PreparedDBQueryCommon::db_query_clan=NULL;

std::string PreparedDBQueryCommon::db_query_monster_by_character_id=NULL;
std::string PreparedDBQueryCommon::db_query_delete_monster_buff=NULL;
std::string PreparedDBQueryCommon::db_query_delete_monster_specific_buff=NULL;
std::string PreparedDBQueryCommon::db_query_delete_monster_skill=NULL;
std::string PreparedDBQueryCommon::db_query_delete_character=NULL;
std::string PreparedDBQueryCommon::db_query_delete_all_item=NULL;
std::string PreparedDBQueryCommon::db_query_delete_all_item_warehouse=NULL;
std::string PreparedDBQueryCommon::db_query_delete_monster_by_character=NULL;
std::string PreparedDBQueryCommon::db_query_delete_monster_by_id=NULL;
std::string PreparedDBQueryCommon::db_query_delete_monster_warehouse_by_id=NULL;
std::string PreparedDBQueryCommon::db_query_delete_recipes=NULL;
std::string PreparedDBQueryCommon::db_query_delete_reputation=NULL;
std::string PreparedDBQueryCommon::db_query_delete_allow=NULL;

std::string PreparedDBQueryCommon::db_query_select_clan_by_name=NULL;
std::string PreparedDBQueryCommon::db_query_select_character_by_pseudo=NULL;
#ifdef CATCHCHALLENGER_CLASS_LOGIN
std::string PreparedDBQueryCommon::db_query_get_character_count_by_account;
#endif
std::string PreparedDBQueryCommon::db_query_insert_monster=NULL;
std::string PreparedDBQueryCommon::db_query_insert_monster_full=NULL;
std::string PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=NULL;
std::string PreparedDBQueryCommon::db_query_insert_monster_skill=NULL;
std::string PreparedDBQueryCommon::db_query_insert_reputation=NULL;
std::string PreparedDBQueryCommon::db_query_insert_item=NULL;
std::string PreparedDBQueryCommon::db_query_insert_item_warehouse=NULL;
std::string PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id=NULL;
std::string PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id=NULL;
std::string PreparedDBQueryCommon::db_query_select_reputation_by_id=NULL;
std::string PreparedDBQueryCommon::db_query_select_recipes_by_player_id=NULL;
std::string PreparedDBQueryCommon::db_query_select_items_by_player_id=NULL;
std::string PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id=NULL;
std::string PreparedDBQueryCommon::db_query_select_monsters_by_player_id=NULL;
std::string PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=NULL;
std::string PreparedDBQueryCommon::db_query_select_monstersSkill_by_id=NULL;
std::string PreparedDBQueryCommon::db_query_change_right=NULL;
std::string PreparedDBQueryCommon::db_query_update_item=NULL;
std::string PreparedDBQueryCommon::db_query_update_item_warehouse=NULL;
std::string PreparedDBQueryCommon::db_query_delete_item=NULL;
std::string PreparedDBQueryCommon::db_query_delete_item_warehouse=NULL;
std::string PreparedDBQueryCommon::db_query_update_cash=NULL;
std::string PreparedDBQueryCommon::db_query_update_warehouse_cash=NULL;
std::string PreparedDBQueryCommon::db_query_insert_recipe=NULL;
std::string PreparedDBQueryCommon::db_query_insert_character_allow=NULL;
std::string PreparedDBQueryCommon::db_query_delete_character_allow=NULL;
std::string PreparedDBQueryCommon::db_query_update_reputation=NULL;
std::string PreparedDBQueryCommon::db_query_update_character_clan=NULL;
std::string PreparedDBQueryCommon::db_query_update_character_clan_and_leader=NULL;
std::string PreparedDBQueryCommon::db_query_delete_clan=NULL;
std::string PreparedDBQueryCommon::db_query_update_character_clan_by_pseudo=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_hp_only=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_sp_only=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_skill_level=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_xp=NULL;
std::string PreparedDBQueryCommon::db_query_insert_monster_buff=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_level=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_position=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_and_hp=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_level_only=NULL;
std::string PreparedDBQueryCommon::db_query_delete_monster_specific_skill=NULL;
std::string PreparedDBQueryCommon::db_query_insert_clan=NULL;
std::string PreparedDBQueryCommon::db_query_update_monster_owner=NULL;
std::string PreparedDBQueryCommon::db_query_select_server_time=NULL;
std::string PreparedDBQueryCommon::db_query_insert_server_time=NULL;
std::string PreparedDBQueryCommon::db_query_update_server_time_played_time=NULL;
std::string PreparedDBQueryCommon::db_query_update_server_time_last_connect=NULL;
std::string PreparedDBQueryServer::db_query_update_character_forserver_map_part1=NULL;
std::string PreparedDBQueryServer::db_query_update_character_forserver_map_part2=NULL;

void PreparedDBQueryLogin::initDatabaseQueryLogin(const DatabaseBase::DatabaseType &type)
{
    switch(type)
    {
        default:
        return;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        PreparedDBQueryLogin::db_query_login=std::stringLiteral("SELECT `id`,LOWER(HEX(`password`)) FROM `account` WHERE `login`=UNHEX('%1')");
        PreparedDBQueryLogin::db_query_insert_login=std::stringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,UNHEX('%2'),UNHEX('%3'),%4)");
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        PreparedDBQueryLogin::db_query_login=std::stringLiteral("SELECT id,password FROM account WHERE login='%1'");
        PreparedDBQueryLogin::db_query_insert_login=std::stringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4)");
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        PreparedDBQueryLogin::db_query_login=std::stringLiteral("SELECT id,password FROM account WHERE login='%1'");
        PreparedDBQueryLogin::db_query_insert_login=std::stringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4)");
        break;
    }
}

void PreparedDBQueryCommon::initDatabaseQueryCommonWithoutSP(const DatabaseBase::DatabaseType &type)
{
    switch(type)
    {
        default:
        return;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        PreparedDBQueryCommon::db_query_select_allow=std::stringLiteral("SELECT `allow` FROM `character_allow` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_characters=std::stringLiteral("SELECT `id`,`pseudo`,`skin`,`time_to_delete`,`played_time`,`last_connect` FROM `character` WHERE `account`=%1 ORDER BY `played_time` LIMIT 0,%2");
        PreparedDBQueryCommon::db_query_played_time=std::stringLiteral("UPDATE `character` SET `played_time`=`played_time`+%2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_monster_skill=std::stringLiteral("UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3");
        PreparedDBQueryCommon::db_query_character_by_id=std::stringLiteral("SELECT `account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`warehouse_cash`,`clan_leader`,`time_to_delete`,`starter` FROM `character` WHERE `id`=%1");

        PreparedDBQueryCommon::db_query_update_character_time_to_delete=std::stringLiteral("UPDATE `character` SET `time_to_delete`=0 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_character_last_connect=std::stringLiteral("UPDATE `character` SET `last_connect`=%2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_clan=std::stringLiteral("SELECT `name`,`cash` FROM `clan` WHERE `id`=%1");

        PreparedDBQueryCommon::db_query_monster_by_character_id=std::stringLiteral("SELECT `id` FROM `monster` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_monster_buff=std::stringLiteral("DELETE FROM `monster_buff` WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_monster_skill=std::stringLiteral("DELETE FROM `monster_skill` WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_character=std::stringLiteral("DELETE FROM `character` WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_delete_all_item=std::stringLiteral("DELETE FROM `item` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_all_item_warehouse=std::stringLiteral("DELETE FROM `item_warehouse` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_character=std::stringLiteral("DELETE FROM `monster` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_id=std::stringLiteral("DELETE FROM `monster` WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_delete_recipes=std::stringLiteral("DELETE FROM `recipe` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_reputation=std::stringLiteral("DELETE FROM `reputation` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_allow=std::stringLiteral("DELETE FROM `character_allow` WHERE `character`=%1");

        PreparedDBQueryCommon::db_query_select_character_by_pseudo=std::stringLiteral("SELECT `id` FROM `character` WHERE `pseudo`='%1'");
        #ifdef CATCHCHALLENGER_CLASS_LOGIN
        PreparedDBQueryCommon::db_query_get_character_count_by_account=std::stringLiteral("SELECT COUNT(*) FROM `character` WHERE `account`=%1");
        #endif
        PreparedDBQueryCommon::db_query_select_clan_by_name=std::stringLiteral("SELECT `id` FROM `clan` WHERE `name`='%1'");
        PreparedDBQueryCommon::db_query_insert_monster=std::stringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,%3,%4,%5,0,0,%6,%7,0,%3,%8,0)");
        PreparedDBQueryCommon::db_query_insert_monster_full=std::stringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,0)");
        PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=std::stringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,1)");

        PreparedDBQueryCommon::db_query_update_monster_move_to_player=std::stringLiteral("UPDATE `monster` SET `place`=0,`position`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player=std::stringLiteral("UPDATE `monster` SET `place`=0,`character`=%1,`position`=%2 WHERE `id`=%3");
        PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse=std::stringLiteral("UPDATE `monster` SET `place`=1,`position`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_market=std::stringLiteral("UPDATE `monster` SET `place`=2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_insert_monster_skill=std::stringLiteral("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryCommon::db_query_insert_item=std::stringLiteral("INSERT INTO `item`(`item`,`character`,`quantity`) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_insert_item_warehouse=std::stringLiteral("INSERT INTO `item_warehouse`(`item`,`character`,`quantity`) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id=std::stringLiteral("SELECT `account`,`time_to_delete` FROM `character` WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id=std::stringLiteral("UPDATE `character` SET `time_to_delete`=%2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_select_reputation_by_id=std::stringLiteral("SELECT `type`,`point`,`level` FROM `reputation` WHERE `character`=%1 ORDER BY `type`");
        PreparedDBQueryCommon::db_query_select_recipes_by_player_id=std::stringLiteral("SELECT `recipe` FROM `recipe` WHERE `character`=%1 ORDER BY `recipe`");
        PreparedDBQueryCommon::db_query_select_items_by_player_id=std::stringLiteral("SELECT `item`,`quantity` FROM `item` WHERE `character`=%1 ORDER BY `item`");
        PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id=std::stringLiteral("SELECT `item`,`quantity` FROM `item_warehouse` WHERE `character`=%1 ORDER BY `item`");
        PreparedDBQueryCommon::db_query_select_monstersSkill_by_id=std::stringLiteral("SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`=%1 ORDER BY `skill`");
        PreparedDBQueryCommon::db_query_select_monstersBuff_by_id=std::stringLiteral("SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`=%1 ORDER BY `buff`");
        PreparedDBQueryCommon::db_query_change_right=std::stringLiteral("UPDATE `character` SET `type`=%2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_item=std::stringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3");
        PreparedDBQueryCommon::db_query_update_item_warehouse=std::stringLiteral("UPDATE `item_warehouse` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3");
        PreparedDBQueryCommon::db_query_delete_item=std::stringLiteral("DELETE FROM `item` WHERE `item`=%1 AND `character`=%2");
        PreparedDBQueryCommon::db_query_delete_item_warehouse=std::stringLiteral("DELETE FROM `item_warehouse` WHERE `item`=%1 AND `character`=%2");
        PreparedDBQueryCommon::db_query_update_cash=std::stringLiteral("UPDATE `character` SET `cash`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_warehouse_cash=std::stringLiteral("UPDATE `character` SET `warehouse_cash`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_insert_recipe=std::stringLiteral("INSERT INTO `recipe`(`character`,`recipe`) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_insert_character_allow=std::stringLiteral("INSERT INTO `character_allow`(`character`,`allow`) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_delete_character_allow=std::stringLiteral("DELETE FROM `character_allow` WHERE `character`=%1 AND `allow`=%2");
        PreparedDBQueryCommon::db_query_insert_reputation=std::stringLiteral("INSERT INTO `reputation`(`character`,`type`,`point`,`level`) VALUES(%1,'%2',%3,%4)");
        PreparedDBQueryCommon::db_query_update_reputation=std::stringLiteral("UPDATE `reputation` SET `point`=%3,`level`=%4 WHERE `character`=%1 AND `type`='%2'");
        PreparedDBQueryCommon::db_query_update_character_clan=std::stringLiteral("UPDATE `character` SET `clan`=0 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader=std::stringLiteral("UPDATE `character` SET `clan`=%1,`clan_leader`=%2 WHERE `id`=%3;");
        PreparedDBQueryCommon::db_query_delete_clan=std::stringLiteral("DELETE FROM `clan` WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_by_pseudo=std::stringLiteral("UPDATE `character` SET `clan`=0 WHERE `pseudo`=%1 AND `clan`=%2");
        PreparedDBQueryCommon::db_query_update_monster_hp_only=std::stringLiteral("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_sp_only=std::stringLiteral("UPDATE `monster` SET `sp`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_skill_level=std::stringLiteral("UPDATE `monster_skill` SET `level`=%1 WHERE `monster`=%2 AND `skill`=%3");
        PreparedDBQueryCommon::db_query_insert_monster_buff=std::stringLiteral("INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_update_monster_level=std::stringLiteral("UPDATE `monster` SET `level`=%3 WHERE `monster`=%1 AND `buff`=%2");
        PreparedDBQueryCommon::db_query_update_monster_position=std::stringLiteral("UPDATE `monster` SET `position`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_and_hp=std::stringLiteral("UPDATE `monster` SET `hp`=%1,`monster`=%2 WHERE `id`=%3");
        PreparedDBQueryCommon::db_query_delete_monster_specific_buff=std::stringLiteral("DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2");
        PreparedDBQueryCommon::db_query_update_monster_level_only=std::stringLiteral("DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2");
        PreparedDBQueryCommon::db_query_delete_monster_specific_skill=std::stringLiteral("DELETE FROM `monster_skill` WHERE `monster`=%1 AND `skill`=%2");
        PreparedDBQueryCommon::db_query_insert_clan=std::stringLiteral("INSERT INTO `clan`(`id`,`name`,`date`) VALUES(%1,'%2',%3);");
        PreparedDBQueryCommon::db_query_update_monster_owner=std::stringLiteral("UPDATE `monster` SET `character`=%2 WHERE `id`=%1;");
        PreparedDBQueryCommon::db_query_select_server_time=std::stringLiteral("SELECT `server`,`played_time`,`last_connect` FROM `server_time` WHERE `account`=%1");//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommon::db_query_insert_server_time=std::stringLiteral("INSERT INTO `server_time`(`server`,`account`,`played_time`,`last_connect`) VALUES(%1,%2,0,%3);");
        PreparedDBQueryCommon::db_query_update_server_time_played_time=std::stringLiteral("UPDATE `server_time` SET `played_time`=`played_time`+%1 WHERE `server`=%2 AND `account`=%3;");
        PreparedDBQueryCommon::db_query_update_server_time_last_connect=std::stringLiteral("UPDATE `server_time` SET `last_connect`=%1 WHERE `server`=%2 AND `account`=%3;");
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        PreparedDBQueryCommon::db_query_select_allow=std::stringLiteral("SELECT allow FROM character_allow WHERE character=%1");
        PreparedDBQueryCommon::db_query_characters=std::stringLiteral("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect FROM character WHERE account=%1 ORDER BY played_time LIMIT 0,%2");
        PreparedDBQueryCommon::db_query_played_time=std::stringLiteral("UPDATE character SET played_time=played_time+%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_monster_skill=std::stringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3");
        PreparedDBQueryCommon::db_query_character_by_id=std::stringLiteral("SELECT account,pseudo,skin,type,clan,cash,warehouse_cash,clan_leader,time_to_delete,starter FROM character WHERE id=%1");

        PreparedDBQueryCommon::db_query_update_character_time_to_delete=std::stringLiteral("UPDATE character SET time_to_delete=0 WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_last_connect=std::stringLiteral("UPDATE character SET last_connect=%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_clan=std::stringLiteral("SELECT name,cash FROM clan WHERE id=%1");

        PreparedDBQueryCommon::db_query_monster_by_character_id=std::stringLiteral("SELECT id FROM monster WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_buff=std::stringLiteral("DELETE FROM monster_buff WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_monster_skill=std::stringLiteral("DELETE FROM monster_skill WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_character=std::stringLiteral("DELETE FROM character WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_all_item=std::stringLiteral("DELETE FROM item WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_all_item_warehouse=std::stringLiteral("DELETE FROM item_warehouse WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_character=std::stringLiteral("DELETE FROM monster WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_id=std::stringLiteral("DELETE FROM monster WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_recipes=std::stringLiteral("DELETE FROM recipe WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_reputation=std::stringLiteral("DELETE FROM reputation WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_allow=std::stringLiteral("DELETE FROM character_allow WHERE character=%1");

        PreparedDBQueryCommon::db_query_select_clan_by_name=std::stringLiteral("SELECT id FROM clan WHERE name='%1'");
        PreparedDBQueryCommon::db_query_select_character_by_pseudo=std::stringLiteral("SELECT id FROM character WHERE pseudo='%1'");
        #ifdef CATCHCHALLENGER_CLASS_LOGIN
        PreparedDBQueryCommon::db_query_get_character_count_by_account=std::stringLiteral("SELECT COUNT(*) FROM character WHERE account=%1");
        #endif
        PreparedDBQueryCommon::db_query_insert_monster=std::stringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,%3,%4,%5,0,0,%6,%7,0,%3,%8,0)");
        PreparedDBQueryCommon::db_query_insert_monster_full=std::stringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,0)");
        PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=std::stringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,1)");

        PreparedDBQueryCommon::db_query_update_monster_move_to_player=std::stringLiteral("UPDATE monster SET place=0,position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player=std::stringLiteral("UPDATE monster SET place=0,character=%1,position=%2 WHERE id=%3");
        PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse=std::stringLiteral("UPDATE monster SET place=1,position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_market=std::stringLiteral("UPDATE monster SET place=2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_insert_monster_skill=std::stringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryCommon::db_query_insert_item=std::stringLiteral("INSERT INTO item(item,character,quantity) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_insert_item_warehouse=std::stringLiteral("INSERT INTO item_warehouse(item,character,quantity) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id=std::stringLiteral("SELECT account,time_to_delete FROM character WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id=std::stringLiteral("UPDATE character SET time_to_delete=%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_select_reputation_by_id=std::stringLiteral("SELECT type,point,level FROM reputation WHERE character=%1 ORDER BY type");
        PreparedDBQueryCommon::db_query_select_recipes_by_player_id=std::stringLiteral("SELECT recipe FROM recipe WHERE character=%1 ORDER BY recipe");
        PreparedDBQueryCommon::db_query_select_items_by_player_id=std::stringLiteral("SELECT item,quantity FROM item WHERE character=%1 ORDER BY item");
        PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id=std::stringLiteral("SELECT item,quantity FROM item_warehouse WHERE character=%1 ORDER BY item");
        PreparedDBQueryCommon::db_query_select_monstersSkill_by_id=std::stringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1 ORDER BY skill");
        PreparedDBQueryCommon::db_query_select_monstersBuff_by_id=std::stringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1 ORDER BY buff");
        PreparedDBQueryCommon::db_query_change_right=std::stringLiteral("UPDATE \"character\" SET \"type\"=%2 WHERE \"id\"=%1");
        PreparedDBQueryCommon::db_query_update_item=std::stringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryCommon::db_query_update_item_warehouse=std::stringLiteral("UPDATE item_warehouse SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryCommon::db_query_delete_item=std::stringLiteral("DELETE FROM item WHERE item=%1 AND character=%2");
        PreparedDBQueryCommon::db_query_delete_item_warehouse=std::stringLiteral("DELETE FROM item_warehouse WHERE item=%1 AND character=%2");
        PreparedDBQueryCommon::db_query_update_cash=std::stringLiteral("UPDATE character SET cash=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_warehouse_cash=std::stringLiteral("UPDATE character SET warehouse_cash=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_insert_recipe=std::stringLiteral("INSERT INTO recipe(character,recipe) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_insert_character_allow=std::stringLiteral("INSERT INTO character_allow(character,allow) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_delete_character_allow=std::stringLiteral("DELETE FROM character_allow WHERE character=%1 AND allow=%2");
        PreparedDBQueryCommon::db_query_insert_reputation=std::stringLiteral("INSERT INTO reputation(character,type,point,level) VALUES(%1,'%2',%3,%4)");
        PreparedDBQueryCommon::db_query_update_reputation=std::stringLiteral("UPDATE reputation SET point=%3,level=%4 WHERE character=%1 AND type='%2'");
        PreparedDBQueryCommon::db_query_update_character_clan=std::stringLiteral("UPDATE character SET clan=0 WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader=std::stringLiteral("UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;");
        PreparedDBQueryCommon::db_query_delete_clan=std::stringLiteral("DELETE FROM clan WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_by_pseudo=std::stringLiteral("UPDATE character SET clan=0 WHERE pseudo=%1 AND clan=%2");
        PreparedDBQueryCommon::db_query_update_monster_hp_only=std::stringLiteral("UPDATE monster SET hp=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_sp_only=std::stringLiteral("UPDATE monster SET sp=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_skill_level=std::stringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3");
        PreparedDBQueryCommon::db_query_insert_monster_buff=std::stringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_update_monster_level=std::stringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_update_monster_position=std::stringLiteral("UPDATE monster SET position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_and_hp=std::stringLiteral("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3");
        PreparedDBQueryCommon::db_query_delete_monster_specific_buff=std::stringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_update_monster_level_only=std::stringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_delete_monster_specific_skill=std::stringLiteral("DELETE FROM monster_skill WHERE monster=%1 AND skill=%2");
        PreparedDBQueryCommon::db_query_insert_clan=std::stringLiteral("INSERT INTO clan(id,name,date) VALUES(%1,'%2',%3);");
        PreparedDBQueryCommon::db_query_update_monster_owner=std::stringLiteral("UPDATE monster SET character=%2 WHERE id=%1;");
        PreparedDBQueryCommon::db_query_select_server_time=std::stringLiteral("SELECT server,played_time,last_connect FROM server_time WHERE account=%1");//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommon::db_query_insert_server_time=std::stringLiteral("INSERT INTO server_time(server,account,played_time,last_connect) VALUES(%1,%2,0,%3);");
        PreparedDBQueryCommon::db_query_update_server_time_played_time=std::stringLiteral("UPDATE server_time SET played_time=played_time+%1 WHERE server=%2 AND account=%3;");
        PreparedDBQueryCommon::db_query_update_server_time_last_connect=std::stringLiteral("UPDATE server_time SET last_connect=%1 WHERE server=%2 AND account=%3;");
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        PreparedDBQueryCommon::db_query_select_allow=std::stringLiteral("SELECT allow FROM character_allow WHERE character=%1");
        PreparedDBQueryCommon::db_query_characters=std::stringLiteral("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect FROM character WHERE account=%1 ORDER BY played_time LIMIT %2");
        PreparedDBQueryCommon::db_query_played_time=std::stringLiteral("UPDATE character SET played_time=played_time+%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_monster_skill=std::stringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3");
        PreparedDBQueryCommon::db_query_character_by_id=std::stringLiteral("SELECT account,pseudo,skin,type,clan,cash,warehouse_cash,clan_leader,time_to_delete,starter FROM character WHERE id=%1");

        PreparedDBQueryCommon::db_query_update_character_time_to_delete=std::stringLiteral("UPDATE character SET time_to_delete=0 WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_last_connect=std::stringLiteral("UPDATE character SET last_connect=%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_clan=std::stringLiteral("SELECT name,cash FROM clan WHERE id=%1");

        PreparedDBQueryCommon::db_query_monster_by_character_id=std::stringLiteral("SELECT id FROM monster WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_buff=std::stringLiteral("DELETE FROM monster_buff WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_monster_skill=std::stringLiteral("DELETE FROM monster_skill WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_character=std::stringLiteral("DELETE FROM character WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_all_item=std::stringLiteral("DELETE FROM item WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_all_item_warehouse=std::stringLiteral("DELETE FROM item_warehouse WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_character=std::stringLiteral("DELETE FROM monster WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_id=std::stringLiteral("DELETE FROM monster WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_recipes=std::stringLiteral("DELETE FROM recipe WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_reputation=std::stringLiteral("DELETE FROM reputation WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_allow=std::stringLiteral("DELETE FROM character_allow WHERE character=%1");

        PreparedDBQueryCommon::db_query_select_clan_by_name=std::stringLiteral("SELECT id FROM clan WHERE name='%1'");
        PreparedDBQueryCommon::db_query_select_character_by_pseudo=std::stringLiteral("SELECT id FROM character WHERE pseudo='%1'");
        #ifdef CATCHCHALLENGER_CLASS_LOGIN
        PreparedDBQueryCommon::db_query_get_character_count_by_account=std::stringLiteral("SELECT COUNT(*) FROM character WHERE account=%1");
        #endif
        PreparedDBQueryCommon::db_query_insert_monster=std::stringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,%3,%4,%5,0,0,%6,%7,0,%3,%8,0)");
        PreparedDBQueryCommon::db_query_insert_monster_full=std::stringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,0)");
        PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=std::stringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,1)");

        PreparedDBQueryCommon::db_query_update_monster_move_to_player=std::stringLiteral("UPDATE monster SET place=0,position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player=std::stringLiteral("UPDATE monster SET place=0,character=%1,position=%2 WHERE id=%3");
        PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse=std::stringLiteral("UPDATE monster SET place=1,position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_market=std::stringLiteral("UPDATE monster SET place=2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_insert_monster_skill=std::stringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryCommon::db_query_insert_item=std::stringLiteral("INSERT INTO item(item,character,quantity) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_insert_item_warehouse=std::stringLiteral("INSERT INTO item_warehouse(item,character,quantity) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id=std::stringLiteral("SELECT account,time_to_delete FROM character WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id=std::stringLiteral("UPDATE character SET time_to_delete=%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_select_reputation_by_id=std::stringLiteral("SELECT type,point,level FROM reputation WHERE character=%1 ORDER BY type");
        PreparedDBQueryCommon::db_query_select_recipes_by_player_id=std::stringLiteral("SELECT recipe FROM recipe WHERE character=%1 ORDER BY recipe");
        PreparedDBQueryCommon::db_query_select_items_by_player_id=std::stringLiteral("SELECT item,quantity FROM item WHERE character=%1 ORDER BY item");
        PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id=std::stringLiteral("SELECT item,quantity FROM item_warehouse WHERE character=%1 ORDER BY item");
        PreparedDBQueryCommon::db_query_select_monstersSkill_by_id=std::stringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1 ORDER BY skill");
        PreparedDBQueryCommon::db_query_select_monstersBuff_by_id=std::stringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1 ORDER BY buff");
        PreparedDBQueryCommon::db_query_change_right=std::stringLiteral("UPDATE \"character\" SET \"type\"=%2 WHERE \"id\"=%1");
        PreparedDBQueryCommon::db_query_update_item=std::stringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryCommon::db_query_update_item_warehouse=std::stringLiteral("UPDATE item_warehouse SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryCommon::db_query_delete_item=std::stringLiteral("DELETE FROM item WHERE item=%1 AND character=%2");
        PreparedDBQueryCommon::db_query_delete_item_warehouse=std::stringLiteral("DELETE FROM item_warehouse WHERE item=%1 AND character=%2");
        PreparedDBQueryCommon::db_query_update_cash=std::stringLiteral("UPDATE character SET cash=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_warehouse_cash=std::stringLiteral("UPDATE character SET warehouse_cash=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_insert_recipe=std::stringLiteral("INSERT INTO recipe(character,recipe) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_insert_character_allow=std::stringLiteral("INSERT INTO character_allow(character,allow) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_delete_character_allow=std::stringLiteral("DELETE FROM character_allow WHERE character=%1 AND allow=%2");
        PreparedDBQueryCommon::db_query_insert_reputation=std::stringLiteral("INSERT INTO reputation(character,type,point,level) VALUES(%1,'%2',%3,%4)");
        PreparedDBQueryCommon::db_query_update_reputation=std::stringLiteral("UPDATE reputation SET point=%3,level=%4 WHERE character=%1 AND type='%2'");
        PreparedDBQueryCommon::db_query_update_character_clan=std::stringLiteral("UPDATE character SET clan=0 WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader=std::stringLiteral("UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;");
        PreparedDBQueryCommon::db_query_delete_clan=std::stringLiteral("DELETE FROM clan WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_by_pseudo=std::stringLiteral("UPDATE character SET clan=0 WHERE pseudo=%1 AND clan=%2");
        PreparedDBQueryCommon::db_query_update_monster_hp_only=std::stringLiteral("UPDATE monster SET hp=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_sp_only=std::stringLiteral("UPDATE monster SET sp=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_skill_level=std::stringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3");
        PreparedDBQueryCommon::db_query_insert_monster_buff=std::stringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_update_monster_level=std::stringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_update_monster_position=std::stringLiteral("UPDATE monster SET position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_and_hp=std::stringLiteral("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3");
        PreparedDBQueryCommon::db_query_delete_monster_specific_buff=std::stringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_update_monster_level_only=std::stringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_delete_monster_specific_skill=std::stringLiteral("DELETE FROM monster_skill WHERE monster=%1 AND skill=%2");
        PreparedDBQueryCommon::db_query_insert_clan=std::stringLiteral("INSERT INTO clan(id,name,date) VALUES(%1,'%2',%3);");
        PreparedDBQueryCommon::db_query_update_monster_owner=std::stringLiteral("UPDATE monster SET character=%2 WHERE id=%1;");
        PreparedDBQueryCommon::db_query_select_server_time=std::stringLiteral("SELECT server,played_time,last_connect FROM server_time WHERE account=%1");//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommon::db_query_insert_server_time=std::stringLiteral("INSERT INTO server_time(server,account,played_time,last_connect) VALUES(%1,%2,0,%3);");
        PreparedDBQueryCommon::db_query_update_server_time_played_time=std::stringLiteral("UPDATE server_time SET played_time=played_time+%1 WHERE server=%2 AND account=%3;");
        PreparedDBQueryCommon::db_query_update_server_time_last_connect=std::stringLiteral("UPDATE server_time SET last_connect=%1 WHERE server=%2 AND account=%3;");
        break;
    }
}

void PreparedDBQueryCommon::initDatabaseQueryCommonWithSP(const DatabaseBase::DatabaseType &type,const bool &useSP)
{
    switch(type)
    {
        default:
        return;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_monster=std::stringLiteral("UPDATE `monster` SET `hp`=%3,`xp`=%4,`level`=%5,`sp`=%6,`position`=%7 WHERE `id`=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=std::stringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=0 ORDER BY `position` ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=std::stringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=1 ORDER BY `position` ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=std::stringLiteral("UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4,`sp`=%5 WHERE `id`=%1");
            PreparedDBQueryCommon::db_query_update_monster_xp=std::stringLiteral("UPDATE `monster` SET `xp`=%2,`sp`=%3 WHERE `id`=%1");
        }
        else
        {
            PreparedDBQueryCommon::db_query_monster=std::stringLiteral("UPDATE `monster` SET `hp`=%3,`xp`=%4,`level`=%5,`position`=%6 WHERE `id`=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=std::stringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=0 ORDER BY `position` ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=std::stringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=1 ORDER BY `position` ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=std::stringLiteral("UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4 WHERE `id`=%1");
            PreparedDBQueryCommon::db_query_update_monster_xp=std::stringLiteral("UPDATE `monster` SET `xp`=%2 WHERE `id`=%1");
        }
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_monster=std::stringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6,position=%7 WHERE id=%1");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=std::stringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=std::stringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=std::stringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp=std::stringLiteral("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1");
        }
        else
        {
            PreparedDBQueryCommon::db_query_monster=std::stringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,position=%6 WHERE id=%1");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=std::stringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=std::stringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=std::stringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp=std::stringLiteral("UPDATE monster SET xp=%2 WHERE id=%1");
        }
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_monster=std::stringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6,position=%7 WHERE id=%1");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=std::stringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=std::stringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=std::stringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp=std::stringLiteral("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1");
        }
        else
        {
            PreparedDBQueryCommon::db_query_monster=std::stringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,position=%6 WHERE id=%1");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=std::stringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=std::stringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=std::stringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp=std::stringLiteral("UPDATE monster SET xp=%2 WHERE id=%1");
        }
        break;
    }
}

void PreparedDBQueryServer::initDatabaseQueryServer(const DatabaseBase::DatabaseType &type)
{
    switch(type)
    {
        default:
        return;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        PreparedDBQueryServer::db_query_character_server_by_id=std::stringLiteral("SELECT `map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`market_cash` FROM `character_forserver` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_delete_all_item_market=std::stringLiteral("DELETE FROM `item_market` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_insert_item_market=std::stringLiteral("INSERT INTO `item_market`(`item`,`character`,`quantity`,`market_price`) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryServer::db_query_delete_item_market=std::stringLiteral("DELETE FROM `item_market` WHERE `item`=%1 AND `character`=%2");
        PreparedDBQueryServer::db_query_update_item_market=std::stringLiteral("UPDATE `item_market` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3");
        PreparedDBQueryServer::db_query_update_item_market_and_price=std::stringLiteral("UPDATE `item_market` SET `quantity`=%1,`market_price`=%2 WHERE `item`=%3 AND `character`=%4;");
        PreparedDBQueryServer::db_query_update_charaters_market_cash=std::stringLiteral("UPDATE `character_forserver` SET `market_cash`=`market_cash`+%1 WHERE `character`=%2");
        PreparedDBQueryServer::db_query_get_market_cash=std::stringLiteral("UPDATE `character` SET `cash`=%1,`market_cash`=0 WHERE `id`=%2;");
        PreparedDBQueryServer::db_query_insert_monster_market_price=std::stringLiteral("INSERT INTO `monster_market_price`(`id`,`market_price`) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_delete_monster_market_price=std::stringLiteral("DELETE FROM `monster_market_price` WHERE `id`=%1");
        PreparedDBQueryServer::db_query_delete_bot_already_beaten=std::stringLiteral("DELETE FROM `bot_already_beaten` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_select_plant=std::stringLiteral("SELECT `pointOnMap`,`plant`,`plant_timestamps` FROM `plant` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_delete_plant=std::stringLiteral("DELETE FROM `plant` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_delete_plant_by_index=std::stringLiteral("DELETE FROM `plant` WHERE `character`=%1 AND `pointOnMap`=%2");
        PreparedDBQueryServer::db_query_delete_quest=std::stringLiteral("DELETE FROM `quest` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_select_quest_by_id=std::stringLiteral("SELECT `quest`,`finish_one_time`,`step` FROM `quest` WHERE `character`=%1 ORDER BY `quest`");
        PreparedDBQueryServer::db_query_select_bot_beaten=std::stringLiteral("SELECT `botfight_id` FROM `bot_already_beaten` WHERE `character`=%1 ORDER BY `botfight_id`");
        PreparedDBQueryServer::db_query_select_itemOnMap=std::stringLiteral("SELECT `pointOnMap` FROM `character_itemonmap` WHERE `character`=%1 ORDER BY `pointOnMap`");
        PreparedDBQueryServer::db_query_insert_itemonmap=std::stringLiteral("INSERT INTO `character_itemonmap`(`character`,`pointOnMap`) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_factory=std::stringLiteral("INSERT INTO `factory`(`id`,`resources`,`products`,`last_update`) VALUES(%1,'%2','%3',%4)");
        PreparedDBQueryServer::db_query_update_factory=std::stringLiteral("UPDATE `factory` SET `resources`='%2',`products`='%3',`last_update`=%4 WHERE `id`=%1");
        PreparedDBQueryServer::db_query_delete_city=std::stringLiteral("DELETE FROM `city` WHERE `city`='%1'");
        PreparedDBQueryServer::db_query_insert_bot_already_beaten=std::stringLiteral("INSERT INTO `bot_already_beaten`(`character`,`botfight_id`) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_plant=std::stringLiteral("INSERT INTO `plant`(`character`,`pointOnMap`,`plant`,`plant_timestamps`) VALUES(%1,%2,%3,%4);");
        PreparedDBQueryServer::db_query_update_quest_finish=std::stringLiteral("UPDATE `quest` SET `step`=0,`finish_one_time`=1 WHERE `character`=%1 AND `quest`=%2;");
        PreparedDBQueryServer::db_query_update_quest_step=std::stringLiteral("UPDATE `quest` SET `step`=%3 WHERE `character`=%1 AND `quest`=%2;");
        PreparedDBQueryServer::db_query_update_quest_restart=std::stringLiteral("UPDATE `quest` SET `step`=1 WHERE `character`=%1 AND `quest`=%2;");
        PreparedDBQueryServer::db_query_insert_quest=std::stringLiteral("INSERT INTO `quest`(`character`,`quest`,`finish_one_time`,`step`) VALUES(%1,%2,0,%3);");
        PreparedDBQueryServer::db_query_update_city_clan=std::stringLiteral("UPDATE `city` SET `clan`=%1 WHERE `city`='%2';");
        PreparedDBQueryServer::db_query_insert_city=std::stringLiteral("INSERT INTO `city`(`clan`,`city`) VALUES(%1,'%2');");
        PreparedDBQueryServer::db_query_update_character_forserver_map_part1=std::stringLiteral("UPDATE `character_forserver` SET `map`=%1,`x`=%2,`y`=%3,`orientation`=%4,%5 WHERE `character`=%6");
        PreparedDBQueryServer::db_query_update_character_forserver_map_part2=std::stringLiteral("`rescue_map`=%1,`rescue_x`=%2,`rescue_y`=%3,`rescue_orientation`=%4,`unvalidated_rescue_map`=%5,`unvalidated_rescue_x`=%6,`unvalidated_rescue_y`=%7,`unvalidated_rescue_orientation`=%8");
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        PreparedDBQueryServer::db_query_character_server_by_id=std::stringLiteral("SELECT map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,market_cash FROM character_forserver WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_all_item_market=std::stringLiteral("DELETE FROM item_market WHERE character=%1");
        PreparedDBQueryServer::db_query_insert_item_market=std::stringLiteral("INSERT INTO item_market(item,character,quantity,market_price) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryServer::db_query_delete_item_market=std::stringLiteral("DELETE FROM item_market WHERE item=%1 AND character=%2");
        PreparedDBQueryServer::db_query_update_item_market=std::stringLiteral("UPDATE item_market SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryServer::db_query_update_item_market_and_price=std::stringLiteral("UPDATE item_market SET quantity=%1,market_price=%2 WHERE item=%3 AND character=%4;");
        PreparedDBQueryServer::db_query_update_charaters_market_cash=std::stringLiteral("UPDATE character_forserver SET market_cash=market_cash+%1 WHERE character=%2");
        PreparedDBQueryServer::db_query_get_market_cash=std::stringLiteral("UPDATE character SET cash=%1,market_cash=0 WHERE id=%2;");
        PreparedDBQueryServer::db_query_insert_monster_market_price=std::stringLiteral("INSERT INTO monster_market_price(id,market_price) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_delete_monster_market_price=std::stringLiteral("DELETE FROM monster_market_price WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_bot_already_beaten=std::stringLiteral("DELETE FROM bot_already_beaten WHERE character=%1");
        PreparedDBQueryServer::db_query_select_plant=std::stringLiteral("SELECT \"pointOnMap\",plant,plant_timestamps FROM plant WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_plant=std::stringLiteral("DELETE FROM plant WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_plant_by_index=std::stringLiteral("DELETE FROM plant WHERE character=%1 AND \"pointOnMap\"=%2");
        PreparedDBQueryServer::db_query_delete_quest=std::stringLiteral("DELETE FROM quest WHERE character=%1");
        PreparedDBQueryServer::db_query_select_quest_by_id=std::stringLiteral("SELECT quest,finish_one_time,step FROM quest WHERE character=%1 ORDER BY quest");
        PreparedDBQueryServer::db_query_select_bot_beaten=std::stringLiteral("SELECT botfight_id FROM bot_already_beaten WHERE character=%1 ORDER BY bot_already_beaten");
        PreparedDBQueryServer::db_query_select_itemOnMap=std::stringLiteral("SELECT \"pointOnMap\" FROM \"character_itemonmap\" WHERE character=%1 ORDER BY \"pointOnMap\"");
        PreparedDBQueryServer::db_query_insert_itemonmap=std::stringLiteral("INSERT INTO \"character_itemonmap\"(character,\"pointOnMap\") VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_factory=std::stringLiteral("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)");
        PreparedDBQueryServer::db_query_update_factory=std::stringLiteral("UPDATE factory SET resources='%2',products='%3',last_update=%4 WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_city=std::stringLiteral("DELETE FROM city WHERE city='%1'");
        PreparedDBQueryServer::db_query_insert_bot_already_beaten=std::stringLiteral("INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_plant=std::stringLiteral("INSERT INTO plant(character,\"pointOnMap\",plant,plant_timestamps) VALUES(%1,%2,%3,%4);");
        PreparedDBQueryServer::db_query_update_quest_finish=std::stringLiteral("UPDATE quest SET step=0,finish_one_time=1 WHERE character=%1 AND quest=%2;");
        PreparedDBQueryServer::db_query_update_quest_step=std::stringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;");
        PreparedDBQueryServer::db_query_update_quest_restart=std::stringLiteral("UPDATE quest SET step=1 WHERE character=%1 AND quest=%2;");
        PreparedDBQueryServer::db_query_insert_quest=std::stringLiteral("INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,0,%3);");
        PreparedDBQueryServer::db_query_update_city_clan=std::stringLiteral("UPDATE city SET clan=%1 WHERE city='%2';");
        PreparedDBQueryServer::db_query_insert_city=std::stringLiteral("INSERT INTO city(clan,city) VALUES(%1,'%2');");
        PreparedDBQueryServer::db_query_update_character_forserver_map_part1=std::stringLiteral("UPDATE character_forserver SET map=%1,x=%2,y=%3,orientation=%4,%5 WHERE character=%6");
        PreparedDBQueryServer::db_query_update_character_forserver_map_part2=std::stringLiteral("rescue_map=%1,rescue_x=%2,rescue_y=%3,rescue_orientation=%4,unvalidated_rescue_map=%5,unvalidated_rescue_x=%6,unvalidated_rescue_y=%7,unvalidated_rescue_orientation=%8");
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        PreparedDBQueryServer::db_query_character_server_by_id=std::stringLiteral("SELECT map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,market_cash FROM character_forserver WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_all_item_market=std::stringLiteral("DELETE FROM item_market WHERE character=%1");
        PreparedDBQueryServer::db_query_insert_item_market=std::stringLiteral("INSERT INTO item_market(item,character,quantity,market_price) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryServer::db_query_delete_item_market=std::stringLiteral("DELETE FROM item_market WHERE item=%1 AND character=%2");
        PreparedDBQueryServer::db_query_update_item_market=std::stringLiteral("UPDATE item_market SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryServer::db_query_update_item_market_and_price=std::stringLiteral("UPDATE item_market SET quantity=%1,market_price=%2 WHERE item=%3 AND character=%4;");
        PreparedDBQueryServer::db_query_update_charaters_market_cash=std::stringLiteral("UPDATE character_forserver SET market_cash=market_cash+%1 WHERE character=%2");
        PreparedDBQueryServer::db_query_get_market_cash=std::stringLiteral("UPDATE character SET cash=%1,market_cash=0 WHERE id=%2;");
        PreparedDBQueryServer::db_query_insert_monster_market_price=std::stringLiteral("INSERT INTO monster_market_price(id,market_price) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_delete_monster_market_price=std::stringLiteral("DELETE FROM monster_market_price WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_bot_already_beaten=std::stringLiteral("DELETE FROM bot_already_beaten WHERE character=%1");
        PreparedDBQueryServer::db_query_select_plant=std::stringLiteral("SELECT \"pointOnMap\",plant,plant_timestamps FROM plant WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_plant=std::stringLiteral("DELETE FROM plant WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_plant_by_index=std::stringLiteral("DELETE FROM plant WHERE character=%1 AND \"pointOnMap\"=%2");
        PreparedDBQueryServer::db_query_delete_quest=std::stringLiteral("DELETE FROM quest WHERE character=%1");
        PreparedDBQueryServer::db_query_select_quest_by_id=std::stringLiteral("SELECT quest,finish_one_time,step FROM quest WHERE character=%1 ORDER BY quest");
        PreparedDBQueryServer::db_query_select_bot_beaten=std::stringLiteral("SELECT botfight_id FROM bot_already_beaten WHERE character=%1 ORDER BY bot_already_beaten");
        PreparedDBQueryServer::db_query_select_itemOnMap=std::stringLiteral("SELECT \"pointOnMap\" FROM \"character_itemonmap\" WHERE character=%1 ORDER BY \"pointOnMap\"");
        PreparedDBQueryServer::db_query_insert_itemonmap=std::stringLiteral("INSERT INTO \"character_itemonmap\"(character,\"pointOnMap\") VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_factory=std::stringLiteral("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)");
        PreparedDBQueryServer::db_query_update_factory=std::stringLiteral("UPDATE factory SET resources='%2',products='%3',last_update=%4 WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_city=std::stringLiteral("DELETE FROM city WHERE city='%1'");
        PreparedDBQueryServer::db_query_insert_bot_already_beaten=std::stringLiteral("INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_plant=std::stringLiteral("INSERT INTO plant(character,\"pointOnMap\",plant,plant_timestamps) VALUES(%1,%2,%3,%4);");
        PreparedDBQueryServer::db_query_update_quest_finish=std::stringLiteral("UPDATE quest SET step=0,finish_one_time=1 WHERE character=%1 AND quest=%2;");
        PreparedDBQueryServer::db_query_update_quest_step=std::stringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;");
        PreparedDBQueryServer::db_query_update_quest_restart=std::stringLiteral("UPDATE quest SET step=1 WHERE character=%1 AND quest=%2;");
        PreparedDBQueryServer::db_query_insert_quest=std::stringLiteral("INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,0,%3);");
        PreparedDBQueryServer::db_query_update_city_clan=std::stringLiteral("UPDATE city SET clan=%1 WHERE city='%2';");
        PreparedDBQueryServer::db_query_insert_city=std::stringLiteral("INSERT INTO city(clan,city) VALUES(%1,'%2');");
        PreparedDBQueryServer::db_query_update_character_forserver_map_part1=std::stringLiteral("UPDATE character_forserver SET map=%1,x=%2,y=%3,orientation=%4,%5 WHERE character=%6");
        PreparedDBQueryServer::db_query_update_character_forserver_map_part2=std::stringLiteral("rescue_map=%1,rescue_x=%2,rescue_y=%3,rescue_orientation=%4,unvalidated_rescue_map=%5,unvalidated_rescue_x=%6,unvalidated_rescue_y=%7,unvalidated_rescue_orientation=%8");
        break;
    }
}

