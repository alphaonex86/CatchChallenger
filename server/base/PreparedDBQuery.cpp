#include "PreparedDBQuery.h"
#include <iostream>

using namespace CatchChallenger;

//query
#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
StringWithReplacement PreparedDBQueryLogin::db_query_login;
StringWithReplacement PreparedDBQueryLogin::db_query_insert_login;
#endif

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
StringWithReplacement PreparedDBQueryCommonForLogin::db_query_characters;
StringWithReplacement PreparedDBQueryCommonForLogin::db_query_characters_with_monsters;
StringWithReplacement PreparedDBQueryCommonForLogin::db_query_select_server_time;
StringWithReplacement PreparedDBQueryCommonForLogin::db_query_delete_character;
StringWithReplacement PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo;
StringWithReplacement PreparedDBQueryCommonForLogin::db_query_get_character_count_by_account;
StringWithReplacement PreparedDBQueryCommonForLogin::db_query_account_time_to_delete_character_by_id;
StringWithReplacement PreparedDBQueryCommonForLogin::db_query_update_character_time_to_delete_by_id;
#endif

//login and gameserver alone
StringWithReplacement PreparedDBQueryCommon::db_query_delete_monster_by_id;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_monster;

#if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
StringWithReplacement PreparedDBQueryServer::db_query_update_character_forserver_map;

/*
StringWithReplacement PreparedDBQueryCommon::db_query_insert_item;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_item_warehouse;
StringWithReplacement PreparedDBQueryCommon::db_query_update_item;
StringWithReplacement PreparedDBQueryCommon::db_query_update_item_warehouse;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_item;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_item_warehouse;
StringWithReplacement PreparedDBQueryCommon::db_query_update_cash;
StringWithReplacement PreparedDBQueryCommon::db_query_update_warehouse_cash;
StringWithReplacement PreparedDBQueryCommon::db_query_select_monstersBuff_by_id;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_move_to_player;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_move_to_new_player;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_move_to_market;
StringWithReplacement PreparedDBQueryCommon::db_query_select_allow;
StringWithReplacement PreparedDBQueryCommon::db_query_played_time;
StringWithReplacement PreparedDBQueryCommon::db_query_monster_skill;
StringWithReplacement PreparedDBQueryCommon::db_query_monster;
StringWithReplacement PreparedDBQueryCommon::db_query_character_by_id;
StringWithReplacement PreparedDBQueryCommon::db_query_update_character_time_to_delete;
StringWithReplacement PreparedDBQueryCommon::db_query_update_character_last_connect;
StringWithReplacement PreparedDBQueryCommon::db_query_clan;

StringWithReplacement PreparedDBQueryCommon::db_query_monster_by_character_id;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_monster_buff;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_monster_specific_buff;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_monster_skill;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_all_item;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_all_item_warehouse;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_monster_by_character;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_monster_warehouse_by_id;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_recipes;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_reputation;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_allow;

StringWithReplacement PreparedDBQueryCommon::db_query_select_clan_by_name;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_monster_full;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_warehouse_monster_full;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_monster_skill;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_reputation;
StringWithReplacement PreparedDBQueryCommon::db_query_select_reputation_by_id;
StringWithReplacement PreparedDBQueryCommon::db_query_select_recipes_by_player_id;
StringWithReplacement PreparedDBQueryCommon::db_query_select_items_by_player_id;
StringWithReplacement PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id;
StringWithReplacement PreparedDBQueryCommon::db_query_select_monsters_by_player_id;
StringWithReplacement PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id;
StringWithReplacement PreparedDBQueryCommon::db_query_select_monstersSkill_by_id;
StringWithReplacement PreparedDBQueryCommon::db_query_change_right;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_recipe;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_character_allow;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_character_allow;
StringWithReplacement PreparedDBQueryCommon::db_query_update_reputation;
StringWithReplacement PreparedDBQueryCommon::db_query_update_character_clan;
StringWithReplacement PreparedDBQueryCommon::db_query_update_character_clan_and_leader;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_clan;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_xp_hp_level;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_hp_only;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_sp_only;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_skill_level;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_xp;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_monster_buff;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_level;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_position;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_and_hp;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_level_only;
StringWithReplacement PreparedDBQueryCommon::db_query_delete_monster_specific_skill;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_clan;
StringWithReplacement PreparedDBQueryCommon::db_query_update_monster_owner;
StringWithReplacement PreparedDBQueryCommon::db_query_insert_server_time;
StringWithReplacement PreparedDBQueryCommon::db_query_update_server_time_played_time;
StringWithReplacement PreparedDBQueryCommon::db_query_update_server_time_last_connect;
#endif

#if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
StringWithReplacement PreparedDBQueryServer::db_query_character_server_by_id;
StringWithReplacement PreparedDBQueryServer::db_query_delete_all_item_market;
StringWithReplacement PreparedDBQueryServer::db_query_insert_item_market;
StringWithReplacement PreparedDBQueryServer::db_query_delete_item_market;
StringWithReplacement PreparedDBQueryServer::db_query_update_item_market;
StringWithReplacement PreparedDBQueryServer::db_query_update_item_market_and_price;
StringWithReplacement PreparedDBQueryServer::db_query_update_charaters_market_cash;
StringWithReplacement PreparedDBQueryServer::db_query_get_market_cash;
StringWithReplacement PreparedDBQueryServer::db_query_insert_monster_market_price;
StringWithReplacement PreparedDBQueryServer::db_query_delete_monster_market_price;

StringWithReplacement PreparedDBQueryServer::db_query_insert_plant;
StringWithReplacement PreparedDBQueryServer::db_query_update_quest_finish;
StringWithReplacement PreparedDBQueryServer::db_query_update_quest_step;
StringWithReplacement PreparedDBQueryServer::db_query_update_quest_restart;
StringWithReplacement PreparedDBQueryServer::db_query_insert_quest;
StringWithReplacement PreparedDBQueryServer::db_query_update_city_clan;
StringWithReplacement PreparedDBQueryServer::db_query_insert_city;
StringWithReplacement PreparedDBQueryServer::db_query_select_plant;
StringWithReplacement PreparedDBQueryServer::db_query_delete_plant;
StringWithReplacement PreparedDBQueryServer::db_query_delete_plant_by_index;
StringWithReplacement PreparedDBQueryServer::db_query_delete_quest;
StringWithReplacement PreparedDBQueryServer::db_query_select_quest_by_id;
StringWithReplacement PreparedDBQueryServer::db_query_select_bot_beaten;
StringWithReplacement PreparedDBQueryServer::db_query_select_itemOnMap;
StringWithReplacement PreparedDBQueryServer::db_query_insert_itemonmap;
StringWithReplacement PreparedDBQueryServer::db_query_insert_factory;
StringWithReplacement PreparedDBQueryServer::db_query_update_factory;
StringWithReplacement PreparedDBQueryServer::db_query_delete_city;
StringWithReplacement PreparedDBQueryServer::db_query_insert_bot_already_beaten;
StringWithReplacement PreparedDBQueryServer::db_query_delete_bot_already_beaten;
*/
#endif

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
void PreparedDBQueryLogin::initDatabaseQueryLogin(const DatabaseBase::DatabaseType &type)
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
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_MYSQL)
        case DatabaseBase::DatabaseType::Mysql:
        PreparedDBQueryLogin::db_query_login="SELECT `id`,LOWER(HEX(`password`)),blob_version FROM `account` WHERE `login`=UNHEX('%1')";
        PreparedDBQueryLogin::db_query_insert_login="INSERT INTO account(id,login,password,date,blob_version) VALUES(%1,UNHEX('%2'),UNHEX('%3'),%4,%5)";
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        PreparedDBQueryLogin::db_query_login="SELECT id,password,blob_version FROM account WHERE login='%1'";
        PreparedDBQueryLogin::db_query_insert_login="INSERT INTO account(id,login,password,date,blob_version) VALUES(%1,'%2','%3',%4,%5)";
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
        PreparedDBQueryLogin::db_query_login="SELECT id,encode(password,'hex'),blob_version FROM account WHERE login='\\x%1'";
        PreparedDBQueryLogin::db_query_insert_login="INSERT INTO account(id,login,password,date,blob_version) VALUES(%1,'\\x%2','\\x%3',%4,%5)";
        break;
        #endif
    }
}
#endif

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
void PreparedDBQueryCommonForLogin::initDatabaseQueryCommonForLogin(const DatabaseBase::DatabaseType &type)
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
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_MYSQL)
        case DatabaseBase::DatabaseType::Mysql:
        PreparedDBQueryCommonForLogin::db_query_characters="SELECT `id`,`pseudo`,`skin`,`time_to_delete`,`played_time`,`last_connect` FROM `character` WHERE `account`=%1 LIMIT 0,%2";
        PreparedDBQueryCommonForLogin::db_query_characters_with_monsters="SELECT LOWER(HEX(`monster`)),LOWER(HEX(`monster_warehouse`)),LOWER(HEX(`monster_market`)) FROM `character` WHERE `id`=%1";
        PreparedDBQueryCommonForLogin::db_query_select_server_time="SELECT `server`,`played_time`,`last_connect` FROM `server_time` WHERE `account`=%1";//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommonForLogin::db_query_delete_character="DELETE FROM `character` WHERE `id`=%1";
        PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo="SELECT `id` FROM `character` WHERE `pseudo`='%1'";
        PreparedDBQueryCommonForLogin::db_query_get_character_count_by_account="SELECT COUNT(*) FROM `character` WHERE `account`=%1";
        PreparedDBQueryCommonForLogin::db_query_account_time_to_delete_character_by_id="SELECT `account`,`time_to_delete` FROM `character` WHERE `id`=%1";
        PreparedDBQueryCommonForLogin::db_query_update_character_time_to_delete_by_id="UPDATE `character` SET `time_to_delete`=%2 WHERE `id`=%1";
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        PreparedDBQueryCommonForLogin::db_query_characters="SELECT id,pseudo,skin,time_to_delete,played_time,last_connect FROM character WHERE account=%1 LIMIT 0,%2";
        PreparedDBQueryCommonForLogin::db_query_characters_with_monsters="SELECT monster,monster_warehouse,monster_market FROM character WHERE id=%1";
        PreparedDBQueryCommonForLogin::db_query_select_server_time="SELECT server,played_time,last_connect FROM server_time WHERE account=%1";//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommonForLogin::db_query_delete_character="DELETE FROM character WHERE id=%1";
        PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo="SELECT id FROM character WHERE pseudo='%1'";
        PreparedDBQueryCommonForLogin::db_query_get_character_count_by_account="SELECT COUNT(*) FROM character WHERE account=%1";
        PreparedDBQueryCommonForLogin::db_query_account_time_to_delete_character_by_id="SELECT account,time_to_delete FROM character WHERE id=%1";
        PreparedDBQueryCommonForLogin::db_query_update_character_time_to_delete_by_id="UPDATE character SET time_to_delete=%2 WHERE id=%1";
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
        PreparedDBQueryCommonForLogin::db_query_characters="SELECT id,pseudo,skin,time_to_delete,played_time,last_connect FROM character WHERE account=%1 LIMIT %2";
        PreparedDBQueryCommonForLogin::db_query_characters_with_monsters="SELECT encode(monster,'hex'),encode(monster_warehouse,'hex'),encode(monster_market,'hex') FROM character WHERE id=%1";
        PreparedDBQueryCommonForLogin::db_query_select_server_time="SELECT server,played_time,last_connect FROM server_time WHERE account=%1";//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommonForLogin::db_query_delete_character="DELETE FROM character WHERE id=%1";
        PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo="SELECT id FROM character WHERE pseudo='%1'";
        PreparedDBQueryCommonForLogin::db_query_get_character_count_by_account="SELECT COUNT(*) FROM character WHERE account=%1";
        PreparedDBQueryCommonForLogin::db_query_account_time_to_delete_character_by_id="SELECT account,time_to_delete FROM character WHERE id=%1";
        PreparedDBQueryCommonForLogin::db_query_update_character_time_to_delete_by_id="UPDATE character SET time_to_delete=%2 WHERE id=%1";
        break;
        #endif
    }
}
#endif

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
void PreparedDBQueryCommon::initDatabaseQueryCommonWithoutSP(const DatabaseBase::DatabaseType &type)
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
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_MYSQL)
        case DatabaseBase::DatabaseType::Mysql:
        //login and gameserver alone
        PreparedDBQueryCommon::db_query_delete_monster_by_id="DELETE FROM `monster` WHERE `id`=%1";
        PreparedDBQueryCommon::db_query_insert_monster="INSERT INTO `monster`(`id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`skills`,`skills_endurance`) "
                                                       "VALUES(%1,%2,%3,%4,0,0,%5,%6,0,%7,UNHEX('%8'),UNHEX('%9'))";

        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
        PreparedDBQueryCommon::db_query_select_allow="SELECT `allow` FROM `character_allow` WHERE `character`=%1";
        PreparedDBQueryCommon::db_query_played_time="UPDATE `character` SET `played_time`=`played_time`+%2 WHERE `id`=%1";
        PreparedDBQueryCommon::db_query_monster_skill="UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3";
        PreparedDBQueryCommon::db_query_character_by_id="SELECT `account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`warehouse_cash`,`clan_leader`,`time_to_delete`,`starter` FROM `character` WHERE `id`=%1";

        PreparedDBQueryCommon::db_query_update_character_time_to_delete="UPDATE `character` SET `time_to_delete`=0 WHERE `id`=%1";
        PreparedDBQueryCommon::db_query_update_character_last_connect="UPDATE `character` SET `last_connect`=%2 WHERE `id`=%1";
        PreparedDBQueryCommon::db_query_clan="SELECT `name`,`cash` FROM `clan` WHERE `id`=%1";

        PreparedDBQueryCommon::db_query_monster_by_character_id="SELECT `id` FROM `monster` WHERE `character`=%1";
        PreparedDBQueryCommon::db_query_delete_monster_buff="DELETE FROM `monster_buff` WHERE monster=%1";
        PreparedDBQueryCommon::db_query_delete_monster_skill="DELETE FROM `monster_skill` WHERE monster=%1";
        PreparedDBQueryCommon::db_query_delete_all_item="DELETE FROM `item` WHERE `character`=%1";
        PreparedDBQueryCommon::db_query_delete_all_item_warehouse="DELETE FROM `item_warehouse` WHERE `character`=%1";
        PreparedDBQueryCommon::db_query_delete_monster_by_character="DELETE FROM `monster` WHERE `character`=%1";
        PreparedDBQueryCommon::db_query_delete_recipes="DELETE FROM `recipe` WHERE `character`=%1";
        PreparedDBQueryCommon::db_query_delete_reputation="DELETE FROM `reputation` WHERE `character`=%1";
        PreparedDBQueryCommon::db_query_delete_allow="DELETE FROM `character_allow` WHERE `character`=%1";

        PreparedDBQueryCommon::db_query_select_clan_by_name="SELECT `id` FROM `clan` WHERE `name`='%1'";
        PreparedDBQueryCommon::db_query_insert_monster_full="INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`) VALUES(%1,%2,0)";
        PreparedDBQueryCommon::db_query_insert_warehouse_monster_full="INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,1)";

        PreparedDBQueryCommon::db_query_update_monster_move_to_player="UPDATE `monster` SET `place`=0,`position`=%1 WHERE `id`=%2";
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player="UPDATE `monster` SET `place`=0,`character`=%1,`position`=%2 WHERE `id`=%3";
        PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse="UPDATE `monster` SET `place`=1,`position`=%1 WHERE `id`=%2";
        PreparedDBQueryCommon::db_query_update_monster_move_to_market="UPDATE `monster` SET `place`=2 WHERE `id`=%1";
        PreparedDBQueryCommon::db_query_insert_monster_skill="INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4)";
        PreparedDBQueryCommon::db_query_insert_item="INSERT INTO `item`(`item`,`character`,`quantity`) VALUES(%1,%2,%3)";
        PreparedDBQueryCommon::db_query_insert_item_warehouse="INSERT INTO `item_warehouse`(`item`,`character`,`quantity`) VALUES(%1,%2,%3)";
        PreparedDBQueryCommon::db_query_select_reputation_by_id="SELECT `type`,`point`,`level` FROM `reputation` WHERE `character`=%1 ORDER BY `type`";
        PreparedDBQueryCommon::db_query_select_recipes_by_player_id="SELECT `recipe` FROM `recipe` WHERE `character`=%1 ORDER BY `recipe`";
        PreparedDBQueryCommon::db_query_select_items_by_player_id="SELECT `item`,`quantity` FROM `item` WHERE `character`=%1 ORDER BY `item`";
        PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id="SELECT `item`,`quantity` FROM `item_warehouse` WHERE `character`=%1 ORDER BY `item`";
        PreparedDBQueryCommon::db_query_select_monstersSkill_by_id="SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`=%1 ORDER BY `skill`";
        PreparedDBQueryCommon::db_query_select_monstersBuff_by_id="SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`=%1 ORDER BY `buff`";
        PreparedDBQueryCommon::db_query_change_right="UPDATE `character` SET `type`=%2 WHERE `id`=%1";
        PreparedDBQueryCommon::db_query_update_item="UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3";
        PreparedDBQueryCommon::db_query_update_item_warehouse="UPDATE `item_warehouse` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3";
        PreparedDBQueryCommon::db_query_delete_item="DELETE FROM `item` WHERE `item`=%1 AND `character`=%2";
        PreparedDBQueryCommon::db_query_delete_item_warehouse="DELETE FROM `item_warehouse` WHERE `item`=%1 AND `character`=%2";
        PreparedDBQueryCommon::db_query_update_cash="UPDATE `character` SET `cash`=%1 WHERE `id`=%2";
        PreparedDBQueryCommon::db_query_update_warehouse_cash="UPDATE `character` SET `warehouse_cash`=%1 WHERE `id`=%2";
        PreparedDBQueryCommon::db_query_insert_recipe="INSERT INTO `recipe`(`character`,`recipe`) VALUES(%1,%2)";
        PreparedDBQueryCommon::db_query_insert_character_allow="INSERT INTO `character_allow`(`character`,`allow`) VALUES(%1,%2)";
        PreparedDBQueryCommon::db_query_delete_character_allow="DELETE FROM `character_allow` WHERE `character`=%1 AND `allow`=%2";
        PreparedDBQueryCommon::db_query_insert_reputation="INSERT INTO `reputation`(`character`,`type`,`point`,`level`) VALUES(%1,'%2',%3,%4)";
        PreparedDBQueryCommon::db_query_update_reputation="UPDATE `reputation` SET `point`=%3,`level`=%4 WHERE `character`=%1 AND `type`='%2'";
        PreparedDBQueryCommon::db_query_update_character_clan="UPDATE `character` SET `clan`=0 WHERE `id`=%1";
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader="UPDATE `character` SET `clan`=%1,`clan_leader`=%2 WHERE `id`=%3;";
        PreparedDBQueryCommon::db_query_delete_clan="DELETE FROM `clan` WHERE `id`=%1";
        PreparedDBQueryCommon::db_query_update_monster_hp_only="UPDATE `monster` SET `hp`=%1 WHERE `id`=%2";
        PreparedDBQueryCommon::db_query_update_monster_sp_only="UPDATE `monster` SET `sp`=%1 WHERE `id`=%2";
        PreparedDBQueryCommon::db_query_update_monster_skill_level="UPDATE `monster_skill` SET `level`=%1 WHERE `monster`=%2 AND `skill`=%3";
        PreparedDBQueryCommon::db_query_insert_monster_buff="INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3)";
        PreparedDBQueryCommon::db_query_update_monster_level="UPDATE `monster` SET `level`=%3 WHERE `monster`=%1 AND `buff`=%2";
        PreparedDBQueryCommon::db_query_update_monster_position="UPDATE `monster` SET `position`=%1 WHERE `id`=%2";
        PreparedDBQueryCommon::db_query_update_monster_and_hp="UPDATE `monster` SET `hp`=%1,`monster`=%2 WHERE `id`=%3";
        PreparedDBQueryCommon::db_query_delete_monster_specific_buff="DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2";
        PreparedDBQueryCommon::db_query_update_monster_level_only="DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2";
        PreparedDBQueryCommon::db_query_delete_monster_specific_skill="DELETE FROM `monster_skill` WHERE `monster`=%1 AND `skill`=%2";
        PreparedDBQueryCommon::db_query_insert_clan="INSERT INTO `clan`(`id`,`name`,`date`) VALUES(%1,'%2',%3);";
        PreparedDBQueryCommon::db_query_update_monster_owner="UPDATE `monster` SET `character`=%2 WHERE `id`=%1;";
        PreparedDBQueryCommon::db_query_insert_server_time="INSERT INTO `server_time`(`server`,`account`,`played_time`,`last_connect`) VALUES(%1,%2,0,%3);";
        PreparedDBQueryCommon::db_query_update_server_time_played_time="UPDATE `server_time` SET `played_time`=`played_time`+%1 WHERE `server`=%2 AND `account`=%3;";
        PreparedDBQueryCommon::db_query_update_server_time_last_connect="UPDATE `server_time` SET `last_connect`=%1 WHERE `server`=%2 AND `account`=%3;";
        #endif
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        //login and gameserver alone
        PreparedDBQueryCommon::db_query_delete_monster_by_id="DELETE FROM monster WHERE id=%1";
        PreparedDBQueryCommon::db_query_insert_monster="INSERT INTO monster(id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,skills,skills_endurance) "
                                                       "VALUES(%1,%2,%3,%4,0,0,%5,%6,0,%7,%8,%9)";

        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
        PreparedDBQueryCommon::db_query_select_allow="SELECT allow FROM character_allow WHERE character=%1";
        PreparedDBQueryCommon::db_query_played_time="UPDATE character SET played_time=played_time+%2 WHERE id=%1";
        PreparedDBQueryCommon::db_query_monster_skill="UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3";
        PreparedDBQueryCommon::db_query_character_by_id="SELECT account,pseudo,skin,type,clan,cash,warehouse_cash,clan_leader,time_to_delete,starter FROM character WHERE id=%1";

        PreparedDBQueryCommon::db_query_update_character_time_to_delete="UPDATE character SET time_to_delete=0 WHERE id=%1";
        PreparedDBQueryCommon::db_query_update_character_last_connect="UPDATE character SET last_connect=%2 WHERE id=%1";
        PreparedDBQueryCommon::db_query_clan="SELECT name,cash FROM clan WHERE id=%1";

        PreparedDBQueryCommon::db_query_monster_by_character_id="SELECT id FROM monster WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_monster_buff="DELETE FROM monster_buff WHERE monster=%1";
        PreparedDBQueryCommon::db_query_delete_monster_skill="DELETE FROM monster_skill WHERE monster=%1";
        PreparedDBQueryCommon::db_query_delete_all_item="DELETE FROM item WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_all_item_warehouse="DELETE FROM item_warehouse WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_monster_by_character="DELETE FROM monster WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_recipes="DELETE FROM recipe WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_reputation="DELETE FROM reputation WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_allow="DELETE FROM character_allow WHERE character=%1";

        PreparedDBQueryCommon::db_query_select_clan_by_name="SELECT id FROM clan WHERE name='%1'";
        PreparedDBQueryCommon::db_query_insert_monster_full="INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,0)";
        PreparedDBQueryCommon::db_query_insert_warehouse_monster_full="INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,1)";

        PreparedDBQueryCommon::db_query_update_monster_move_to_player="UPDATE monster SET place=0,position=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player="UPDATE monster SET place=0,character=%1,position=%2 WHERE id=%3";
        PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse="UPDATE monster SET place=1,position=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_move_to_market="UPDATE monster SET place=2 WHERE id=%1";
        PreparedDBQueryCommon::db_query_insert_monster_skill="INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4)";
        PreparedDBQueryCommon::db_query_insert_item="INSERT INTO item(item,character,quantity) VALUES(%1,%2,%3)";
        PreparedDBQueryCommon::db_query_insert_item_warehouse="INSERT INTO item_warehouse(item,character,quantity) VALUES(%1,%2,%3)";
        PreparedDBQueryCommon::db_query_select_reputation_by_id="SELECT type,point,level FROM reputation WHERE character=%1 ORDER BY type";
        PreparedDBQueryCommon::db_query_select_recipes_by_player_id="SELECT recipe FROM recipe WHERE character=%1 ORDER BY recipe";
        PreparedDBQueryCommon::db_query_select_items_by_player_id="SELECT item,quantity FROM item WHERE character=%1 ORDER BY item";
        PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id="SELECT item,quantity FROM item_warehouse WHERE character=%1 ORDER BY item";
        PreparedDBQueryCommon::db_query_select_monstersSkill_by_id="SELECT skill,level,endurance FROM monster_skill WHERE monster=%1 ORDER BY skill";
        PreparedDBQueryCommon::db_query_select_monstersBuff_by_id="SELECT buff,level FROM monster_buff WHERE monster=%1 ORDER BY buff";
        PreparedDBQueryCommon::db_query_change_right="UPDATE \"character\" SET \"type\"=%2 WHERE \"id\"=%1";
        PreparedDBQueryCommon::db_query_update_item="UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3";
        PreparedDBQueryCommon::db_query_update_item_warehouse="UPDATE item_warehouse SET quantity=%1 WHERE item=%2 AND character=%3";
        PreparedDBQueryCommon::db_query_delete_item="DELETE FROM item WHERE item=%1 AND character=%2";
        PreparedDBQueryCommon::db_query_delete_item_warehouse="DELETE FROM item_warehouse WHERE item=%1 AND character=%2";
        PreparedDBQueryCommon::db_query_update_cash="UPDATE character SET cash=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_warehouse_cash="UPDATE character SET warehouse_cash=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_insert_recipe="INSERT INTO recipe(character,recipe) VALUES(%1,%2)";
        PreparedDBQueryCommon::db_query_insert_character_allow="INSERT INTO character_allow(character,allow) VALUES(%1,%2)";
        PreparedDBQueryCommon::db_query_delete_character_allow="DELETE FROM character_allow WHERE character=%1 AND allow=%2";
        PreparedDBQueryCommon::db_query_insert_reputation="INSERT INTO reputation(character,type,point,level) VALUES(%1,'%2',%3,%4)";
        PreparedDBQueryCommon::db_query_update_reputation="UPDATE reputation SET point=%3,level=%4 WHERE character=%1 AND type='%2'";
        PreparedDBQueryCommon::db_query_update_character_clan="UPDATE character SET clan=0 WHERE id=%1";
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader="UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;";
        PreparedDBQueryCommon::db_query_delete_clan="DELETE FROM clan WHERE id=%1";
        PreparedDBQueryCommon::db_query_update_monster_hp_only="UPDATE monster SET hp=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_sp_only="UPDATE monster SET sp=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_skill_level="UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3";
        PreparedDBQueryCommon::db_query_insert_monster_buff="INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3)";
        PreparedDBQueryCommon::db_query_update_monster_level="UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2";
        PreparedDBQueryCommon::db_query_update_monster_position="UPDATE monster SET position=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_and_hp="UPDATE monster SET hp=%1,monster=%2 WHERE id=%3";
        PreparedDBQueryCommon::db_query_delete_monster_specific_buff="DELETE FROM monster_buff WHERE monster=%1 AND buff=%2";
        PreparedDBQueryCommon::db_query_update_monster_level_only="DELETE FROM monster_buff WHERE monster=%1 AND buff=%2";
        PreparedDBQueryCommon::db_query_delete_monster_specific_skill="DELETE FROM monster_skill WHERE monster=%1 AND skill=%2";
        PreparedDBQueryCommon::db_query_insert_clan="INSERT INTO clan(id,name,date) VALUES(%1,'%2',%3);";
        PreparedDBQueryCommon::db_query_update_monster_owner="UPDATE monster SET character=%2 WHERE id=%1;";
        PreparedDBQueryCommon::db_query_insert_server_time="INSERT INTO server_time(server,account,played_time,last_connect) VALUES(%1,%2,0,%3);";
        PreparedDBQueryCommon::db_query_update_server_time_played_time="UPDATE server_time SET played_time=played_time+%1 WHERE server=%2 AND account=%3;";
        PreparedDBQueryCommon::db_query_update_server_time_last_connect="UPDATE server_time SET last_connect=%1 WHERE server=%2 AND account=%3;";
        #endif
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
        //login and gameserver alone
        PreparedDBQueryCommon::db_query_delete_monster_by_id="DELETE FROM monster WHERE id=%1";
        PreparedDBQueryCommon::db_query_insert_monster="INSERT INTO monster(id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,skills,skills_endurance) "
                                                       "VALUES(%1,%2,%3,%4,0,0,%5,%6,0,%7,'\\x%8','\\x%9')";

        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
        PreparedDBQueryCommon::db_query_select_allow="SELECT allow FROM character_allow WHERE character=%1";
        PreparedDBQueryCommon::db_query_played_time="UPDATE character SET played_time=played_time+%2 WHERE id=%1";
        PreparedDBQueryCommon::db_query_monster_skill="UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3";
        PreparedDBQueryCommon::db_query_character_by_id="SELECT account,pseudo,skin,type,clan,cash,warehouse_cash,clan_leader,time_to_delete,starter FROM character WHERE id=%1";

        PreparedDBQueryCommon::db_query_update_character_time_to_delete="UPDATE character SET time_to_delete=0 WHERE id=%1";
        PreparedDBQueryCommon::db_query_update_character_last_connect="UPDATE character SET last_connect=%2 WHERE id=%1";
        PreparedDBQueryCommon::db_query_clan="SELECT name,cash FROM clan WHERE id=%1";

        PreparedDBQueryCommon::db_query_monster_by_character_id="SELECT id FROM monster WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_monster_buff="DELETE FROM monster_buff WHERE monster=%1";
        PreparedDBQueryCommon::db_query_delete_monster_skill="DELETE FROM monster_skill WHERE monster=%1";
        PreparedDBQueryCommon::db_query_delete_all_item="DELETE FROM item WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_all_item_warehouse="DELETE FROM item_warehouse WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_monster_by_character="DELETE FROM monster WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_recipes="DELETE FROM recipe WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_reputation="DELETE FROM reputation WHERE character=%1";
        PreparedDBQueryCommon::db_query_delete_allow="DELETE FROM character_allow WHERE character=%1";

        PreparedDBQueryCommon::db_query_select_clan_by_name="SELECT id FROM clan WHERE name='%1'";
        PreparedDBQueryCommon::db_query_insert_monster_full="INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,0)";
        PreparedDBQueryCommon::db_query_insert_warehouse_monster_full="INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,1)";

        PreparedDBQueryCommon::db_query_update_monster_move_to_player="UPDATE monster SET place=0,position=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player="UPDATE monster SET place=0,character=%1,position=%2 WHERE id=%3";
        PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse="UPDATE monster SET place=1,position=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_move_to_market="UPDATE monster SET place=2 WHERE id=%1";
        PreparedDBQueryCommon::db_query_insert_monster_skill="INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4)";
        PreparedDBQueryCommon::db_query_insert_item="INSERT INTO item(item,character,quantity) VALUES(%1,%2,%3)";
        PreparedDBQueryCommon::db_query_insert_item_warehouse="INSERT INTO item_warehouse(item,character,quantity) VALUES(%1,%2,%3)";
        PreparedDBQueryCommon::db_query_select_reputation_by_id="SELECT type,point,level FROM reputation WHERE character=%1 ORDER BY type";
        PreparedDBQueryCommon::db_query_select_recipes_by_player_id="SELECT recipe FROM recipe WHERE character=%1 ORDER BY recipe";
        PreparedDBQueryCommon::db_query_select_items_by_player_id="SELECT item,quantity FROM item WHERE character=%1 ORDER BY item";
        PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id="SELECT item,quantity FROM item_warehouse WHERE character=%1 ORDER BY item";
        PreparedDBQueryCommon::db_query_select_monstersSkill_by_id="SELECT skill,level,endurance FROM monster_skill WHERE monster=%1 ORDER BY skill";
        PreparedDBQueryCommon::db_query_select_monstersBuff_by_id="SELECT buff,level FROM monster_buff WHERE monster=%1 ORDER BY buff";
        PreparedDBQueryCommon::db_query_change_right="UPDATE \"character\" SET \"type\"=%2 WHERE \"id\"=%1";
        PreparedDBQueryCommon::db_query_update_item="UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3";
        PreparedDBQueryCommon::db_query_update_item_warehouse="UPDATE item_warehouse SET quantity=%1 WHERE item=%2 AND character=%3";
        PreparedDBQueryCommon::db_query_delete_item="DELETE FROM item WHERE item=%1 AND character=%2";
        PreparedDBQueryCommon::db_query_delete_item_warehouse="DELETE FROM item_warehouse WHERE item=%1 AND character=%2";
        PreparedDBQueryCommon::db_query_update_cash="UPDATE character SET cash=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_warehouse_cash="UPDATE character SET warehouse_cash=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_insert_recipe="INSERT INTO recipe(character,recipe) VALUES(%1,%2)";
        PreparedDBQueryCommon::db_query_insert_character_allow="INSERT INTO character_allow(character,allow) VALUES(%1,%2)";
        PreparedDBQueryCommon::db_query_delete_character_allow="DELETE FROM character_allow WHERE character=%1 AND allow=%2";
        PreparedDBQueryCommon::db_query_insert_reputation="INSERT INTO reputation(character,type,point,level) VALUES(%1,'%2',%3,%4)";
        PreparedDBQueryCommon::db_query_update_reputation="UPDATE reputation SET point=%3,level=%4 WHERE character=%1 AND type='%2'";
        PreparedDBQueryCommon::db_query_update_character_clan="UPDATE character SET clan=0 WHERE id=%1";
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader="UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;";
        PreparedDBQueryCommon::db_query_delete_clan="DELETE FROM clan WHERE id=%1";
        PreparedDBQueryCommon::db_query_update_monster_hp_only="UPDATE monster SET hp=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_sp_only="UPDATE monster SET sp=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_skill_level="UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3";
        PreparedDBQueryCommon::db_query_insert_monster_buff="INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3)";
        PreparedDBQueryCommon::db_query_update_monster_level="UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2";
        PreparedDBQueryCommon::db_query_update_monster_position="UPDATE monster SET position=%1 WHERE id=%2";
        PreparedDBQueryCommon::db_query_update_monster_and_hp="UPDATE monster SET hp=%1,monster=%2 WHERE id=%3";
        PreparedDBQueryCommon::db_query_delete_monster_specific_buff="DELETE FROM monster_buff WHERE monster=%1 AND buff=%2";
        PreparedDBQueryCommon::db_query_update_monster_level_only="DELETE FROM monster_buff WHERE monster=%1 AND buff=%2";
        PreparedDBQueryCommon::db_query_delete_monster_specific_skill="DELETE FROM monster_skill WHERE monster=%1 AND skill=%2";
        PreparedDBQueryCommon::db_query_insert_clan="INSERT INTO clan(id,name,date) VALUES(%1,'%2',%3);";
        PreparedDBQueryCommon::db_query_update_monster_owner="UPDATE monster SET character=%2 WHERE id=%1;";
        PreparedDBQueryCommon::db_query_insert_server_time="INSERT INTO server_time(server,account,played_time,last_connect) VALUES(%1,%2,0,%3);";
        PreparedDBQueryCommon::db_query_update_server_time_played_time="UPDATE server_time SET played_time=played_time+%1 WHERE server=%2 AND account=%3;";
        PreparedDBQueryCommon::db_query_update_server_time_last_connect="UPDATE server_time SET last_connect=%1 WHERE server=%2 AND account=%3;";
        #endif
        break;
        #endif
    }
}

void PreparedDBQueryCommon::initDatabaseQueryCommonWithSP(const DatabaseBase::DatabaseType &type,const bool &useSP)
{
    (void)useSP;
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
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_MYSQL)
        case DatabaseBase::DatabaseType::Mysql:
        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_monster="UPDATE `monster` SET `hp`=%3,`xp`=%4,`level`=%5,`sp`=%6,`position`=%7 WHERE `id`=%1";
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id="SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=0 ORDER BY `position` ASC";
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id="SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=1 ORDER BY `position` ASC";
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level="UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4,`sp`=%5 WHERE `id`=%1";
            PreparedDBQueryCommon::db_query_update_monster_xp="UPDATE `monster` SET `xp`=%2,`sp`=%3 WHERE `id`=%1";
        }
        else
        {
            PreparedDBQueryCommon::db_query_monster="UPDATE `monster` SET `hp`=%3,`xp`=%4,`level`=%5,`position`=%6 WHERE `id`=%1";
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id="SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=0 ORDER BY `position` ASC";
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id="SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=1 ORDER BY `position` ASC";
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level="UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4 WHERE `id`=%1";
            PreparedDBQueryCommon::db_query_update_monster_xp="UPDATE `monster` SET `xp`=%2 WHERE `id`=%1";
        }
        #endif
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_monster="UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6,position=%7 WHERE id=%1";
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level="UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1";
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id="SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC";
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id="SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC";
            PreparedDBQueryCommon::db_query_update_monster_xp="UPDATE monster SET xp=%2,sp=%3 WHERE id=%1";
        }
        else
        {
            PreparedDBQueryCommon::db_query_monster="UPDATE monster SET hp=%3,xp=%4,level=%5,position=%6 WHERE id=%1";
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level="UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1";
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id="SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC";
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id="SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC";
            PreparedDBQueryCommon::db_query_update_monster_xp="UPDATE monster SET xp=%2 WHERE id=%1";
        }
        #endif
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_monster="UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6,position=%7 WHERE id=%1";
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level="UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1";
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id="SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC";
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id="SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC";
            PreparedDBQueryCommon::db_query_update_monster_xp="UPDATE monster SET xp=%2,sp=%3 WHERE id=%1";
        }
        else
        {
            PreparedDBQueryCommon::db_query_monster="UPDATE monster SET hp=%3,xp=%4,level=%5,position=%6 WHERE id=%1";
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level="UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1";
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id="SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC";
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id="SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC";
            PreparedDBQueryCommon::db_query_update_monster_xp="UPDATE monster SET xp=%2 WHERE id=%1";
        }
        #endif
        break;
        #endif
    }
}
#endif

#if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER)
void PreparedDBQueryServer::initDatabaseQueryServer(const DatabaseBase::DatabaseType &type)
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
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_MYSQL)
        case DatabaseBase::DatabaseType::Mysql:
        PreparedDBQueryServer::db_query_update_character_forserver_map="UPDATE `character_forserver` SET `map`=%1,`x`=%2,`y`=%3,`orientation`=%4,`rescue_map`=%5,`rescue_x`=%6,`rescue_y`=%7,`rescue_orientation`=%8,`unvalidated_rescue_map`=%9,`unvalidated_rescue_x`=%10,`unvalidated_rescue_y`=%11,`unvalidated_rescue_orientation`=%12 WHERE `character`=%13";

        PreparedDBQueryServer::db_query_character_server_by_id="SELECT `map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`market_cash` FROM `character_forserver` WHERE `character`=%1";
        PreparedDBQueryServer::db_query_delete_all_item_market="DELETE FROM `item_market` WHERE `character`=%1";
        PreparedDBQueryServer::db_query_insert_item_market="INSERT INTO `item_market`(`item`,`character`,`quantity`,`market_price`) VALUES(%1,%2,%3,%4)";
        PreparedDBQueryServer::db_query_delete_item_market="DELETE FROM `item_market` WHERE `item`=%1 AND `character`=%2";
        PreparedDBQueryServer::db_query_update_item_market="UPDATE `item_market` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3";
        PreparedDBQueryServer::db_query_update_item_market_and_price="UPDATE `item_market` SET `quantity`=%1,`market_price`=%2 WHERE `item`=%3 AND `character`=%4;";
        PreparedDBQueryServer::db_query_update_charaters_market_cash="UPDATE `character_forserver` SET `market_cash`=`market_cash`+%1 WHERE `character`=%2";
        PreparedDBQueryServer::db_query_get_market_cash="UPDATE `character` SET `cash`=%1,`market_cash`=0 WHERE `id`=%2;";
        PreparedDBQueryServer::db_query_insert_monster_market_price="INSERT INTO `monster_market_price`(`id`,`market_price`) VALUES(%1,%2)";
        PreparedDBQueryServer::db_query_delete_monster_market_price="DELETE FROM `monster_market_price` WHERE `id`=%1";
        PreparedDBQueryServer::db_query_delete_bot_already_beaten="DELETE FROM `bot_already_beaten` WHERE `character`=%1";
        PreparedDBQueryServer::db_query_select_plant="SELECT `pointOnMap`,`plant`,`plant_timestamps` FROM `plant` WHERE `character`=%1";
        PreparedDBQueryServer::db_query_delete_plant="DELETE FROM `plant` WHERE `character`=%1";
        PreparedDBQueryServer::db_query_delete_plant_by_index="DELETE FROM `plant` WHERE `character`=%1 AND `pointOnMap`=%2";
        PreparedDBQueryServer::db_query_delete_quest="DELETE FROM `quest` WHERE `character`=%1";
        PreparedDBQueryServer::db_query_select_quest_by_id="SELECT `quest`,`finish_one_time`,`step` FROM `quest` WHERE `character`=%1 ORDER BY `quest`";
        PreparedDBQueryServer::db_query_select_bot_beaten="SELECT `botfight_id` FROM `bot_already_beaten` WHERE `character`=%1 ORDER BY `botfight_id`";
        PreparedDBQueryServer::db_query_select_itemOnMap="SELECT `pointOnMap` FROM `character_itemonmap` WHERE `character`=%1 ORDER BY `pointOnMap`";
        PreparedDBQueryServer::db_query_insert_itemonmap="INSERT INTO `character_itemonmap`(`character`,`pointOnMap`) VALUES(%1,%2)";
        PreparedDBQueryServer::db_query_insert_factory="INSERT INTO `factory`(`id`,`resources`,`products`,`last_update`) VALUES(%1,'%2','%3',%4)";
        PreparedDBQueryServer::db_query_update_factory="UPDATE `factory` SET `resources`='%2',`products`='%3',`last_update`=%4 WHERE `id`=%1";
        PreparedDBQueryServer::db_query_delete_city="DELETE FROM `city` WHERE `city`='%1'";
        PreparedDBQueryServer::db_query_insert_bot_already_beaten="INSERT INTO `bot_already_beaten`(`character`,`botfight_id`) VALUES(%1,%2)";
        PreparedDBQueryServer::db_query_insert_plant="INSERT INTO `plant`(`character`,`pointOnMap`,`plant`,`plant_timestamps`) VALUES(%1,%2,%3,%4);";
        PreparedDBQueryServer::db_query_update_quest_finish="UPDATE `quest` SET `step`=0,`finish_one_time`=1 WHERE `character`=%1 AND `quest`=%2;";
        PreparedDBQueryServer::db_query_update_quest_step="UPDATE `quest` SET `step`=%3 WHERE `character`=%1 AND `quest`=%2;";
        PreparedDBQueryServer::db_query_update_quest_restart="UPDATE `quest` SET `step`=1 WHERE `character`=%1 AND `quest`=%2;";
        PreparedDBQueryServer::db_query_insert_quest="INSERT INTO `quest`(`character`,`quest`,`finish_one_time`,`step`) VALUES(%1,%2,0,%3);";
        PreparedDBQueryServer::db_query_update_city_clan="UPDATE `city` SET `clan`=%1 WHERE `city`='%2';";
        PreparedDBQueryServer::db_query_insert_city="INSERT INTO `city`(`clan`,`city`) VALUES(%1,'%2');";
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        PreparedDBQueryServer::db_query_update_character_forserver_map="UPDATE character_forserver SET map=%1,x=%2,y=%3,orientation=%4,rescue_map=%5,rescue_x=%6,rescue_y=%7,rescue_orientation=%8,unvalidated_rescue_map=%9,unvalidated_rescue_x=%10,unvalidated_rescue_y=%11,unvalidated_rescue_orientation=%12 WHERE character=%13";

        PreparedDBQueryServer::db_query_character_server_by_id="SELECT map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,market_cash FROM character_forserver WHERE character=%1";
        PreparedDBQueryServer::db_query_delete_all_item_market="DELETE FROM item_market WHERE character=%1";
        PreparedDBQueryServer::db_query_insert_item_market="INSERT INTO item_market(item,character,quantity,market_price) VALUES(%1,%2,%3,%4)";
        PreparedDBQueryServer::db_query_delete_item_market="DELETE FROM item_market WHERE item=%1 AND character=%2";
        PreparedDBQueryServer::db_query_update_item_market="UPDATE item_market SET quantity=%1 WHERE item=%2 AND character=%3";
        PreparedDBQueryServer::db_query_update_item_market_and_price="UPDATE item_market SET quantity=%1,market_price=%2 WHERE item=%3 AND character=%4;";
        PreparedDBQueryServer::db_query_update_charaters_market_cash="UPDATE character_forserver SET market_cash=market_cash+%1 WHERE character=%2";
        PreparedDBQueryServer::db_query_get_market_cash="UPDATE character SET cash=%1,market_cash=0 WHERE id=%2;";
        PreparedDBQueryServer::db_query_insert_monster_market_price="INSERT INTO monster_market_price(id,market_price) VALUES(%1,%2)";
        PreparedDBQueryServer::db_query_delete_monster_market_price="DELETE FROM monster_market_price WHERE id=%1";
        PreparedDBQueryServer::db_query_delete_bot_already_beaten="DELETE FROM bot_already_beaten WHERE character=%1";
        PreparedDBQueryServer::db_query_select_plant="SELECT \"pointOnMap\",plant,plant_timestamps FROM plant WHERE character=%1";
        PreparedDBQueryServer::db_query_delete_plant="DELETE FROM plant WHERE character=%1";
        PreparedDBQueryServer::db_query_delete_plant_by_index="DELETE FROM plant WHERE character=%1 AND \"pointOnMap\"=%2";
        PreparedDBQueryServer::db_query_delete_quest="DELETE FROM quest WHERE character=%1";
        PreparedDBQueryServer::db_query_select_quest_by_id="SELECT quest,finish_one_time,step FROM quest WHERE character=%1 ORDER BY quest";
        PreparedDBQueryServer::db_query_select_bot_beaten="SELECT botfight_id FROM bot_already_beaten WHERE character=%1 ORDER BY bot_already_beaten";
        PreparedDBQueryServer::db_query_select_itemOnMap="SELECT \"pointOnMap\" FROM \"character_itemonmap\" WHERE character=%1 ORDER BY \"pointOnMap\"";
        PreparedDBQueryServer::db_query_insert_itemonmap="INSERT INTO \"character_itemonmap\"(character,\"pointOnMap\") VALUES(%1,%2)";
        PreparedDBQueryServer::db_query_insert_factory="INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)";
        PreparedDBQueryServer::db_query_update_factory="UPDATE factory SET resources='%2',products='%3',last_update=%4 WHERE id=%1";
        PreparedDBQueryServer::db_query_delete_city="DELETE FROM city WHERE city='%1'";
        PreparedDBQueryServer::db_query_insert_bot_already_beaten="INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2)";
        PreparedDBQueryServer::db_query_insert_plant="INSERT INTO plant(character,\"pointOnMap\",plant,plant_timestamps) VALUES(%1,%2,%3,%4);";
        PreparedDBQueryServer::db_query_update_quest_finish="UPDATE quest SET step=0,finish_one_time=1 WHERE character=%1 AND quest=%2;";
        PreparedDBQueryServer::db_query_update_quest_step="UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;";
        PreparedDBQueryServer::db_query_update_quest_restart="UPDATE quest SET step=1 WHERE character=%1 AND quest=%2;";
        PreparedDBQueryServer::db_query_insert_quest="INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,0,%3);";
        PreparedDBQueryServer::db_query_update_city_clan="UPDATE city SET clan=%1 WHERE city='%2';";
        PreparedDBQueryServer::db_query_insert_city="INSERT INTO city(clan,city) VALUES(%1,'%2');";
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
        PreparedDBQueryServer::db_query_update_character_forserver_map="UPDATE character_forserver SET map=%1,x=%2,y=%3,orientation=%4,rescue_map=%5,rescue_x=%6,rescue_y=%7,rescue_orientation=%8,unvalidated_rescue_map=%9,unvalidated_rescue_x=%10,unvalidated_rescue_y=%11,unvalidated_rescue_orientation=%12 WHERE character=%13";

        /*PreparedDBQueryServer::db_query_character_server_by_id="SELECT map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,market_cash FROM character_forserver WHERE character=%1";
        PreparedDBQueryServer::db_query_delete_all_item_market="DELETE FROM item_market WHERE character=%1";
        PreparedDBQueryServer::db_query_insert_item_market="INSERT INTO item_market(item,character,quantity,market_price) VALUES(%1,%2,%3,%4)";
        PreparedDBQueryServer::db_query_delete_item_market="DELETE FROM item_market WHERE item=%1 AND character=%2";
        PreparedDBQueryServer::db_query_update_item_market="UPDATE item_market SET quantity=%1 WHERE item=%2 AND character=%3";
        PreparedDBQueryServer::db_query_update_item_market_and_price="UPDATE item_market SET quantity=%1,market_price=%2 WHERE item=%3 AND character=%4;";
        PreparedDBQueryServer::db_query_update_charaters_market_cash="UPDATE character_forserver SET market_cash=market_cash+%1 WHERE character=%2";
        PreparedDBQueryServer::db_query_get_market_cash="UPDATE character SET cash=%1,market_cash=0 WHERE id=%2;";
        PreparedDBQueryServer::db_query_insert_monster_market_price="INSERT INTO monster_market_price(id,market_price) VALUES(%1,%2)";
        PreparedDBQueryServer::db_query_delete_monster_market_price="DELETE FROM monster_market_price WHERE id=%1";
        PreparedDBQueryServer::db_query_delete_bot_already_beaten="DELETE FROM bot_already_beaten WHERE character=%1";
        PreparedDBQueryServer::db_query_select_plant="SELECT \"pointOnMap\",plant,plant_timestamps FROM plant WHERE character=%1";
        PreparedDBQueryServer::db_query_delete_plant="DELETE FROM plant WHERE character=%1";
        PreparedDBQueryServer::db_query_delete_plant_by_index="DELETE FROM plant WHERE character=%1 AND \"pointOnMap\"=%2";
        PreparedDBQueryServer::db_query_delete_quest="DELETE FROM quest WHERE character=%1";
        PreparedDBQueryServer::db_query_select_quest_by_id="SELECT quest,finish_one_time,step FROM quest WHERE character=%1 ORDER BY quest";
        PreparedDBQueryServer::db_query_select_bot_beaten="SELECT botfight_id FROM bot_already_beaten WHERE character=%1 ORDER BY bot_already_beaten";
        PreparedDBQueryServer::db_query_select_itemOnMap="SELECT \"pointOnMap\" FROM \"character_itemonmap\" WHERE character=%1 ORDER BY \"pointOnMap\"";
        PreparedDBQueryServer::db_query_insert_itemonmap="INSERT INTO \"character_itemonmap\"(character,\"pointOnMap\") VALUES(%1,%2)";
        PreparedDBQueryServer::db_query_insert_factory="INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)";
        PreparedDBQueryServer::db_query_update_factory="UPDATE factory SET resources='%2',products='%3',last_update=%4 WHERE id=%1";
        PreparedDBQueryServer::db_query_delete_city="DELETE FROM city WHERE city='%1'";
        PreparedDBQueryServer::db_query_insert_bot_already_beaten="INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2)";
        PreparedDBQueryServer::db_query_insert_plant="INSERT INTO plant(character,\"pointOnMap\",plant,plant_timestamps) VALUES(%1,%2,%3,%4);";
        PreparedDBQueryServer::db_query_update_quest_finish="UPDATE quest SET step=0,finish_one_time=1 WHERE character=%1 AND quest=%2;";
        PreparedDBQueryServer::db_query_update_quest_step="UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;";
        PreparedDBQueryServer::db_query_update_quest_restart="UPDATE quest SET step=1 WHERE character=%1 AND quest=%2;";
        PreparedDBQueryServer::db_query_insert_quest="INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,0,%3);";
        PreparedDBQueryServer::db_query_update_city_clan="UPDATE city SET clan=%1 WHERE city='%2';";
        PreparedDBQueryServer::db_query_insert_city="INSERT INTO city(clan,city) VALUES(%1,'%2');";*/
        break;
        #endif
    }
}
#endif
