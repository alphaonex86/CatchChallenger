#include "PreparedDBQuery.h"

using namespace CatchChallenger;

//query
QString PreparedDBQueryLogin::db_query_login=NULL;
QString PreparedDBQueryLogin::db_query_insert_login=NULL;

QString PreparedDBQueryServer::db_query_delete_all_item_market=NULL;
QString PreparedDBQueryServer::db_query_delete_monster_market_by_character=NULL;
QString PreparedDBQueryServer::db_query_delete_monster_market_by_id=NULL;
QString PreparedDBQueryServer::db_query_insert_item_market=NULL;
QString PreparedDBQueryServer::db_query_delete_item_market=NULL;
QString PreparedDBQueryServer::db_query_update_item_market=NULL;
QString PreparedDBQueryServer::db_query_update_item_market_and_price=NULL;
QString PreparedDBQueryServer::db_query_update_charaters_market_cash=NULL;
QString PreparedDBQueryServer::db_query_get_market_cash=NULL;
QString PreparedDBQueryServer::db_query_insert_monster_market_price=NULL;
QString PreparedDBQueryServer::db_query_delete_monster_market_price=NULL;

QString PreparedDBQueryServer::db_query_insert_plant=NULL;
QString PreparedDBQueryServer::db_query_update_quest_finish=NULL;
QString PreparedDBQueryServer::db_query_update_quest_step=NULL;
QString PreparedDBQueryServer::db_query_update_quest_restart=NULL;
QString PreparedDBQueryServer::db_query_insert_quest=NULL;
QString PreparedDBQueryServer::db_query_update_city_clan=NULL;
QString PreparedDBQueryServer::db_query_insert_city=NULL;
QString PreparedDBQueryServer::db_query_delete_plant=NULL;
QString PreparedDBQueryServer::db_query_delete_plant_by_id=NULL;
QString PreparedDBQueryServer::db_query_delete_quest=NULL;
QString PreparedDBQueryServer::db_query_select_quest_by_id=NULL;
QString PreparedDBQueryServer::db_query_select_monstersBuff_by_id=NULL;
QString PreparedDBQueryServer::db_query_select_bot_beaten=NULL;
QString PreparedDBQueryServer::db_query_select_itemOnMap=NULL;
QString PreparedDBQueryServer::db_query_insert_itemonmap=NULL;
QString PreparedDBQueryServer::db_query_insert_factory=NULL;
QString PreparedDBQueryServer::db_query_update_factory=NULL;
QString PreparedDBQueryServer::db_query_delete_city=NULL;

QString PreparedDBQueryCommon::db_query_update_monster_move_to_player;
QString PreparedDBQueryCommon::db_query_update_monster_move_to_new_player;
QString PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse;
QString PreparedDBQueryCommon::db_query_update_monster_move_to_market;
QString PreparedDBQueryCommon::db_query_select_allow=NULL;
QString PreparedDBQueryCommon::db_query_characters=NULL;
QString PreparedDBQueryCommon::db_query_played_time=NULL;
QString PreparedDBQueryCommon::db_query_monster_skill=NULL;
QString PreparedDBQueryCommon::db_query_monster=NULL;
QString PreparedDBQueryCommon::db_query_character_by_id=NULL;
QString PreparedDBQueryCommon::db_query_update_character_time_to_delete=NULL;
QString PreparedDBQueryCommon::db_query_update_character_last_connect=NULL;
QString PreparedDBQueryCommon::db_query_clan=NULL;

QString PreparedDBQueryCommon::db_query_monster_by_character_id=NULL;
QString PreparedDBQueryCommon::db_query_delete_monster_buff=NULL;
QString PreparedDBQueryCommon::db_query_delete_monster_specific_buff=NULL;
QString PreparedDBQueryCommon::db_query_delete_monster_skill=NULL;
QString PreparedDBQueryCommon::db_query_delete_bot_already_beaten=NULL;
QString PreparedDBQueryCommon::db_query_delete_character=NULL;
QString PreparedDBQueryCommon::db_query_delete_all_item=NULL;
QString PreparedDBQueryCommon::db_query_delete_all_item_warehouse=NULL;
QString PreparedDBQueryCommon::db_query_delete_monster_by_character=NULL;
QString PreparedDBQueryCommon::db_query_delete_monster_warehouse_by_character=NULL;
QString PreparedDBQueryCommon::db_query_delete_monster_by_id=NULL;
QString PreparedDBQueryCommon::db_query_delete_monster_warehouse_by_id=NULL;
QString PreparedDBQueryCommon::db_query_delete_recipes=NULL;
QString PreparedDBQueryCommon::db_query_delete_reputation=NULL;
QString PreparedDBQueryCommon::db_query_delete_allow=NULL;

QString PreparedDBQueryCommon::db_query_select_clan_by_name=NULL;
QString PreparedDBQueryCommon::db_query_select_character_by_pseudo=NULL;
QString PreparedDBQueryCommon::db_query_insert_monster=NULL;
QString PreparedDBQueryCommon::db_query_insert_monster_full=NULL;
QString PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=NULL;
QString PreparedDBQueryCommon::db_query_insert_monster_skill=NULL;
QString PreparedDBQueryCommon::db_query_insert_reputation=NULL;
QString PreparedDBQueryCommon::db_query_insert_item=NULL;
QString PreparedDBQueryCommon::db_query_insert_item_warehouse=NULL;
QString PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id=NULL;
QString PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id=NULL;
QString PreparedDBQueryCommon::db_query_select_reputation_by_id=NULL;
QString PreparedDBQueryCommon::db_query_select_recipes_by_player_id=NULL;
QString PreparedDBQueryCommon::db_query_select_items_by_player_id=NULL;
QString PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id=NULL;
QString PreparedDBQueryCommon::db_query_select_monsters_by_player_id=NULL;
QString PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=NULL;
QString PreparedDBQueryCommon::db_query_select_monstersSkill_by_id=NULL;
QString PreparedDBQueryCommon::db_query_change_right=NULL;
QString PreparedDBQueryCommon::db_query_update_item=NULL;
QString PreparedDBQueryCommon::db_query_update_item_warehouse=NULL;
QString PreparedDBQueryCommon::db_query_delete_item=NULL;
QString PreparedDBQueryCommon::db_query_delete_item_warehouse=NULL;
QString PreparedDBQueryCommon::db_query_update_cash=NULL;
QString PreparedDBQueryCommon::db_query_update_warehouse_cash=NULL;
QString PreparedDBQueryCommon::db_query_insert_recipe=NULL;
QString PreparedDBQueryCommon::db_query_insert_character_allow=NULL;
QString PreparedDBQueryCommon::db_query_delete_character_allow=NULL;
QString PreparedDBQueryCommon::db_query_update_reputation=NULL;
QString PreparedDBQueryCommon::db_query_update_character_clan=NULL;
QString PreparedDBQueryCommon::db_query_update_character_clan_and_leader=NULL;
QString PreparedDBQueryCommon::db_query_delete_clan=NULL;
QString PreparedDBQueryCommon::db_query_update_character_clan_by_pseudo=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_hp_only=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_sp_only=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_skill_level=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_xp=NULL;
QString PreparedDBQueryCommon::db_query_insert_bot_already_beaten=NULL;
QString PreparedDBQueryCommon::db_query_insert_monster_buff=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_level=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_position=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_and_hp=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_level_only=NULL;
QString PreparedDBQueryCommon::db_query_delete_monster_specific_skill=NULL;
QString PreparedDBQueryCommon::db_query_insert_clan=NULL;
QString PreparedDBQueryCommon::db_query_update_monster_owner=NULL;
QString PreparedDBQueryCommon::db_query_select_server_time=NULL;
QString PreparedDBQueryCommon::db_query_insert_server_time=NULL;

QString PreparedDBQueryCommon::db_query_select_server=NULL;

void PreparedDBQueryLogin::initDatabaseQueryLogin(const DatabaseBase::Type &type)
{
    switch(type)
    {
        default:
        return;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::Type::Mysql:
        PreparedDBQueryLogin::db_query_login=QStringLiteral("SELECT `id`,LOWER(HEX(`password`)) FROM `account` WHERE `login`=UNHEX('%1')");
        PreparedDBQueryLogin::db_query_insert_login=QStringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,UNHEX('%2'),UNHEX('%3'),%4)");
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::Type::SQLite:
        PreparedDBQueryLogin::db_query_login=QStringLiteral("SELECT id,password FROM account WHERE login='%1'");
        PreparedDBQueryLogin::db_query_insert_login=QStringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4)");
        #endif

        case DatabaseBase::Type::PostgreSQL:
        PreparedDBQueryLogin::db_query_login=QStringLiteral("SELECT id,password FROM account WHERE login='%1'");
        PreparedDBQueryLogin::db_query_insert_login=QStringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4)");
        break;
    }
}

void PreparedDBQueryCommon::initDatabaseQueryCommon(const DatabaseBase::Type &type,const bool &useSP)
{
    switch(type)
    {
        default:
        return;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::Type::Mysql:
        PreparedDBQueryCommon::db_query_select_allow=QStringLiteral("SELECT `allow` FROM `character_allow` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_characters=QStringLiteral("SELECT `id`,`pseudo`,`skin`,`time_to_delete`,`played_time`,`last_connect`,`map` FROM `character` WHERE `account`=%1 ORDER BY `played_time` LIMIT 0,%2");
        PreparedDBQueryCommon::db_query_played_time=QStringLiteral("UPDATE `character` SET `played_time`=`played_time`+%2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_monster_skill=QStringLiteral("UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3");
        PreparedDBQueryCommon::db_query_character_by_id=QStringLiteral("SELECT `account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`warehouse_cash`,`clan_leader`,`time_to_delete`,`starter` FROM `character` WHERE `id`=%1");

        PreparedDBQueryCommon::db_query_update_character_time_to_delete=QStringLiteral("UPDATE `character` SET `time_to_delete`=0 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_character_last_connect=QStringLiteral("UPDATE `character` SET `last_connect`=%2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_clan=QStringLiteral("SELECT `name`,`cash` FROM `clan` WHERE `id`=%1");

        PreparedDBQueryCommon::db_query_monster_by_character_id=QStringLiteral("SELECT `id` FROM `monster` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_monster_buff=QStringLiteral("DELETE FROM `monster_buff` WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_monster_skill=QStringLiteral("DELETE FROM `monster_skill` WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_character=QStringLiteral("DELETE FROM `character` WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_delete_all_item=QStringLiteral("DELETE FROM `item` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_all_item_warehouse=QStringLiteral("DELETE FROM `item_warehouse` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_character=QStringLiteral("DELETE FROM `monster` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_id=QStringLiteral("DELETE FROM `monster` WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_delete_recipes=QStringLiteral("DELETE FROM `recipe` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_reputation=QStringLiteral("DELETE FROM `reputation` WHERE `character`=%1");
        PreparedDBQueryCommon::db_query_delete_allow=QStringLiteral("DELETE FROM `character_allow` WHERE `character`=%1");

        PreparedDBQueryCommon::db_query_select_character_by_pseudo=QStringLiteral("SELECT `id` FROM `character` WHERE `pseudo`='%1'");
        PreparedDBQueryCommon::db_query_select_clan_by_name=QStringLiteral("SELECT `id` FROM `clan` WHERE `name`='%1'");
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_monster=QStringLiteral("UPDATE `monster` SET `hp`=%3,`xp`=%4,`level`=%5,`sp`=%6,`position`=%7 WHERE `id`=%1");
            PreparedDBQueryCommon::db_query_insert_monster=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,%3,%4,%5,0,0,%6,%7,0,%3,%8,0)");
            PreparedDBQueryCommon::db_query_insert_monster_full=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,0)");
            PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,1)");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=QStringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=0 ORDER BY `position` ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=1 ORDER BY `position` ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4,`sp`=%5 WHERE `id`=%1");
            PreparedDBQueryCommon::db_query_update_monster_xp=QStringLiteral("UPDATE `monster` SET `xp`=%2,`sp`=%3 WHERE `id`=%1");
        }
        else
        {
            PreparedDBQueryCommon::db_query_monster=QStringLiteral("UPDATE `monster` SET `hp`=%3,`xp`=%4,`level`=%5,`position`=%6 WHERE `id`=%1");
            PreparedDBQueryCommon::db_query_insert_monster=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,%3,%4,%5,0,%6,%7,0,%3,%8,0)");
            PreparedDBQueryCommon::db_query_insert_monster_full=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,0)");
            PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`place`) VALUES(%1,%2,1)");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=QStringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=0 ORDER BY `position` ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 AND `place`=1 ORDER BY `position` ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4 WHERE `id`=%1");
            PreparedDBQueryCommon::db_query_update_monster_xp=QStringLiteral("UPDATE `monster` SET `xp`=%2 WHERE `id`=%1");
        }
        PreparedDBQueryCommon::db_query_update_monster_move_to_player=QStringLiteral("UPDATE `monster` SET `place`=0,`position`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player=QStringLiteral("UPDATE `monster` SET `place`=0,`character`=%1,`position`=%2 WHERE `id`=%3");
        PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse=QStringLiteral("UPDATE `monster` SET `place`=1,`position`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_market=QStringLiteral("UPDATE `monster` SET `place`=2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_insert_monster_skill=QStringLiteral("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryCommon::db_query_insert_item=QStringLiteral("INSERT INTO `item`(`item`,`character`,`quantity`) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_insert_item_warehouse=QStringLiteral("INSERT INTO `item_warehouse`(`item`,`character`,`quantity`) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id=QStringLiteral("SELECT `account`,`time_to_delete` FROM `character` WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id=QStringLiteral("UPDATE `character` SET `time_to_delete`=%2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_select_reputation_by_id=QStringLiteral("SELECT `type`,`point`,`level` FROM `reputation` WHERE `character`=%1 ORDER BY `type`");
        PreparedDBQueryCommon::db_query_select_recipes_by_player_id=QStringLiteral("SELECT `recipe` FROM `recipe` WHERE `character`=%1 ORDER BY `recipe`");
        PreparedDBQueryCommon::db_query_select_items_by_player_id=QStringLiteral("SELECT `item`,`quantity` FROM `item` WHERE `character`=%1 ORDER BY `item`");
        PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id=QStringLiteral("SELECT `item`,`quantity` FROM `item_warehouse` WHERE `character`=%1 ORDER BY `item`");
        PreparedDBQueryCommon::db_query_select_monstersSkill_by_id=QStringLiteral("SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`=%1 ORDER BY `skill`");
        PreparedDBQueryCommon::db_query_select_monstersBuff_by_id=QStringLiteral("SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`=%1 ORDER BY `buff`");
        PreparedDBQueryCommon::db_query_change_right=QStringLiteral("UPDATE `character` SET `type`=%2 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_item=QStringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3");
        PreparedDBQueryCommon::db_query_update_item_warehouse=QStringLiteral("UPDATE `item_warehouse` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3");
        PreparedDBQueryCommon::db_query_delete_item=QStringLiteral("DELETE FROM `item` WHERE `item`=%1 AND `character`=%2");
        PreparedDBQueryCommon::db_query_delete_item_warehouse=QStringLiteral("DELETE FROM `item_warehouse` WHERE `item`=%1 AND `character`=%2");
        PreparedDBQueryCommon::PreparedDBQueryCommon::db_query_update_cash=QStringLiteral("UPDATE `character` SET `cash`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_warehouse_cash=QStringLiteral("UPDATE `character` SET `warehouse_cash`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_insert_recipe=QStringLiteral("INSERT INTO `recipe`(`character`,`recipe`) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_insert_character_allow=QStringLiteral("INSERT INTO `character_allow`(`character`,`allow`) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_delete_character_allow=QStringLiteral("DELETE FROM `character_allow` WHERE `character`=%1 AND `allow`=%2");
        PreparedDBQueryCommon::db_query_insert_reputation=QStringLiteral("INSERT INTO `reputation`(`character`,`type`,`point`,`level`) VALUES(%1,'%2',%3,%4)");
        PreparedDBQueryCommon::db_query_update_reputation=QStringLiteral("UPDATE `reputation` SET `point`=%3,`level`=%4 WHERE `character`=%1 AND `type`='%2'");
        PreparedDBQueryCommon::db_query_update_character_clan=QStringLiteral("UPDATE `character` SET `clan`=0 WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader=QStringLiteral("UPDATE `character` SET `clan`=%1,`clan_leader`=%2 WHERE `id`=%3;");
        PreparedDBQueryCommon::db_query_delete_clan=QStringLiteral("DELETE FROM `clan` WHERE `id`=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_by_pseudo=QStringLiteral("UPDATE `character` SET `clan`=0 WHERE `pseudo`=%1 AND `clan`=%2");
        PreparedDBQueryCommon::db_query_update_monster_hp_only=QStringLiteral("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_sp_only=QStringLiteral("UPDATE `monster` SET `sp`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_skill_level=QStringLiteral("UPDATE `monster_skill` SET `level`=%1 WHERE `monster`=%2 AND `skill`=%3");
        PreparedDBQueryCommon::db_query_insert_monster_buff=QStringLiteral("INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_update_monster_level=QStringLiteral("UPDATE `monster` SET `level`=%3 WHERE `monster`=%1 AND `buff`=%2");
        PreparedDBQueryCommon::db_query_update_monster_position=QStringLiteral("UPDATE `monster` SET `position`=%1 WHERE `id`=%2");
        PreparedDBQueryCommon::db_query_update_monster_and_hp=QStringLiteral("UPDATE `monster` SET `hp`=%1,`monster`=%2 WHERE `id`=%3");
        PreparedDBQueryCommon::db_query_delete_monster_specific_buff=QStringLiteral("DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2");
        PreparedDBQueryCommon::db_query_update_monster_level_only=QStringLiteral("DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2");
        PreparedDBQueryCommon::db_query_delete_monster_specific_skill=QStringLiteral("DELETE FROM `monster_skill` WHERE `monster`=%1 AND `skill`=%2");
        PreparedDBQueryCommon::db_query_insert_clan=QStringLiteral("INSERT INTO `clan`(`id`,`name`,`date`) VALUES(%1,'%2',%3);");
        PreparedDBQueryCommon::db_query_update_monster_owner=QStringLiteral("UPDATE `monster` SET `character`=%2 WHERE `id`=%1;");
        PreparedDBQueryCommon::db_query_update_quest_finish=QStringLiteral("UPDATE `quest` SET `step`=0,`finish_one_time`=1 WHERE `character`=%1 AND `quest`=%2;");
        PreparedDBQueryCommon::db_query_select_server_time=QStringLiteral("SELECT `server`,`played_time`,`last_connect` FROM `server_time` WHERE `account`=%1");//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommon::db_query_insert_server_time=QStringLiteral("INSERT INTO `server_time`(`server`,`account`,`played_time`,`last_connect`) VALUES(0,%1,0,%2);");
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::Type::SQLite:
        PreparedDBQueryCommon::db_query_select_allow=QStringLiteral("SELECT allow FROM character_allow WHERE character=%1");
        PreparedDBQueryCommon::db_query_characters=QStringLiteral("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect,map FROM character WHERE account=%1 ORDER BY played_time LIMIT 0,%2");
        PreparedDBQueryCommon::db_query_played_time=QStringLiteral("UPDATE character SET played_time=played_time+%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_monster_skill=QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3");
        PreparedDBQueryCommon::db_query_character_by_id=QStringLiteral("SELECT account,pseudo,skin,type,clan,cash,warehouse_cash,clan_leader,time_to_delete,starter FROM character WHERE id=%1");

        PreparedDBQueryCommon::db_query_update_character_time_to_delete=QStringLiteral("UPDATE character SET time_to_delete=0 WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_last_connect=QStringLiteral("UPDATE character SET last_connect=%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_clan=QStringLiteral("SELECT name,cash FROM clan WHERE id=%1");

        PreparedDBQueryCommon::db_query_monster_by_character_id=QStringLiteral("SELECT id FROM monster WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_buff=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_monster_skill=QStringLiteral("DELETE FROM monster_skill WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_character=QStringLiteral("DELETE FROM character WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_all_item=QStringLiteral("DELETE FROM item WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_all_item_warehouse=QStringLiteral("DELETE FROM item_warehouse WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_character=QStringLiteral("DELETE FROM monster WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_id=QStringLiteral("DELETE FROM monster WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_recipes=QStringLiteral("DELETE FROM recipe WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_reputation=QStringLiteral("DELETE FROM reputation WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_allow=QStringLiteral("DELETE FROM character_allow WHERE character=%1");

        PreparedDBQueryCommon::db_query_select_clan_by_name=QStringLiteral("SELECT id FROM clan WHERE name='%1'");
        PreparedDBQueryCommon::db_query_select_character_by_pseudo=QStringLiteral("SELECT id FROM character WHERE pseudo='%1'");
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_monster=QStringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6,position=%7 WHERE id=%1");
            PreparedDBQueryCommon::db_query_insert_monster=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,%3,%4,%5,0,0,%6,%7,0,%3,%8,0)");
            PreparedDBQueryCommon::db_query_insert_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,0)");
            PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,1)");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp=QStringLiteral("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1");
        }
        else
        {
            PreparedDBQueryCommon::db_query_monster=QStringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,position=%6 WHERE id=%1");
            PreparedDBQueryCommon::db_query_insert_monster=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,%3,%4,%5,0,%6,%7,0,%3,%8,0)");
            PreparedDBQueryCommon::db_query_insert_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,0)");
            PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,1)");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp=QStringLiteral("UPDATE monster SET xp=%2 WHERE id=%1");
        }
        PreparedDBQueryCommon::db_query_update_monster_move_to_player=QStringLiteral("UPDATE monster SET place=0,position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player=QStringLiteral("UPDATE monster SET place=0,character=%1,position=%2 WHERE id=%3");
        PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse=QStringLiteral("UPDATE monster SET place=1,position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_market=QStringLiteral("UPDATE monster SET place=2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_insert_monster_skill=QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryCommon::db_query_insert_item=QStringLiteral("INSERT INTO item(item,character,quantity) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_insert_item_warehouse=QStringLiteral("INSERT INTO item_warehouse(item,character,quantity) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id=QStringLiteral("SELECT account,time_to_delete FROM character WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id=QStringLiteral("UPDATE character SET time_to_delete=%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_select_reputation_by_id=QStringLiteral("SELECT type,point,level FROM reputation WHERE character=%1 ORDER BY type");
        PreparedDBQueryCommon::db_query_select_recipes_by_player_id=QStringLiteral("SELECT recipe FROM recipe WHERE character=%1 ORDER BY recipe");
        PreparedDBQueryCommon::db_query_select_items_by_player_id=QStringLiteral("SELECT item,quantity FROM item WHERE character=%1 ORDER BY item");
        PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id=QStringLiteral("SELECT item,quantity FROM item_warehouse WHERE character=%1 ORDER BY item");
        PreparedDBQueryCommon::db_query_select_monstersSkill_by_id=QStringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1 ORDER BY skill");
        PreparedDBQueryCommon::db_query_select_monstersBuff_by_id=QStringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1 ORDER BY buff");
        PreparedDBQueryCommon::db_query_change_right=QStringLiteral("UPDATE \"character\" SET \"type\"=%2 WHERE \"id\"=%1");
        PreparedDBQueryCommon::db_query_update_item=QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryCommon::db_query_update_item_warehouse=QStringLiteral("UPDATE item_warehouse SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryCommon::db_query_delete_item=QStringLiteral("DELETE FROM item WHERE item=%1 AND character=%2");
        PreparedDBQueryCommon::db_query_delete_item_warehouse=QStringLiteral("DELETE FROM item_warehouse WHERE item=%1 AND character=%2");
        PreparedDBQueryCommon::db_query_update_cash=QStringLiteral("UPDATE character SET cash=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_warehouse_cash=QStringLiteral("UPDATE character SET warehouse_cash=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_insert_recipe=QStringLiteral("INSERT INTO recipe(character,recipe) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_insert_character_allow=QStringLiteral("INSERT INTO character_allow(character,allow) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_delete_character_allow=QStringLiteral("DELETE FROM character_allow WHERE character=%1 AND allow=%2");
        PreparedDBQueryCommon::db_query_insert_reputation=QStringLiteral("INSERT INTO reputation(character,type,point,level) VALUES(%1,'%2',%3,%4)");
        PreparedDBQueryCommon::db_query_update_reputation=QStringLiteral("UPDATE reputation SET point=%3,level=%4 WHERE character=%1 AND type='%2'");
        PreparedDBQueryCommon::db_query_update_character_clan=QStringLiteral("UPDATE character SET clan=0 WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader=QStringLiteral("UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;");
        PreparedDBQueryCommon::db_query_delete_clan=QStringLiteral("DELETE FROM clan WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_city=QStringLiteral("DELETE FROM city WHERE city='%1'");
        PreparedDBQueryCommon::db_query_update_character_clan_by_pseudo=QStringLiteral("UPDATE character SET clan=0 WHERE pseudo=%1 AND clan=%2");
        PreparedDBQueryCommon::db_query_update_monster_hp_only=QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_sp_only=QStringLiteral("UPDATE monster SET sp=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_skill_level=QStringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3");
        PreparedDBQueryCommon::db_query_insert_monster_buff=QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_update_monster_level=QStringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_update_monster_position=QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_and_hp=QStringLiteral("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3");
        PreparedDBQueryCommon::db_query_delete_monster_specific_buff=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_update_monster_level_only=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_delete_monster_specific_skill=QStringLiteral("DELETE FROM monster_skill WHERE monster=%1 AND skill=%2");
        PreparedDBQueryCommon::db_query_insert_clan=QStringLiteral("INSERT INTO clan(id,name,date) VALUES(%1,'%2',%3);");
        PreparedDBQueryCommon::db_query_update_monster_owner=QStringLiteral("UPDATE monster SET character=%2 WHERE id=%1;");
        PreparedDBQueryCommon::db_query_select_server_time=QStringLiteral("SELECT server,played_time,last_connect FROM server_time WHERE account=%1");//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommon::db_query_insert_server_time=QStringLiteral("INSERT INTO server_time(server,account,played_time,last_connect) VALUES(0,%1,0,%2);");
        break;
        #endif

        case DatabaseBase::Type::PostgreSQL:
        PreparedDBQueryCommon::db_query_select_allow=QStringLiteral("SELECT allow FROM character_allow WHERE character=%1");
        PreparedDBQueryCommon::db_query_characters=QStringLiteral("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect,map FROM character WHERE account=%1 ORDER BY played_time LIMIT %2");
        PreparedDBQueryCommon::db_query_played_time=QStringLiteral("UPDATE character SET played_time=played_time+%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_monster_skill=QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3");
        PreparedDBQueryCommon::db_query_character_by_id=QStringLiteral("SELECT account,pseudo,skin,type,clan,cash,warehouse_cash,clan_leader,time_to_delete,starter FROM character WHERE id=%1");

        PreparedDBQueryCommon::db_query_update_character_time_to_delete=QStringLiteral("UPDATE character SET time_to_delete=0 WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_last_connect=QStringLiteral("UPDATE character SET last_connect=%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_clan=QStringLiteral("SELECT name,cash FROM clan WHERE id=%1");

        PreparedDBQueryCommon::db_query_monster_by_character_id=QStringLiteral("SELECT id FROM monster WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_buff=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_monster_skill=QStringLiteral("DELETE FROM monster_skill WHERE monster=%1");
        PreparedDBQueryCommon::db_query_delete_character=QStringLiteral("DELETE FROM character WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_all_item=QStringLiteral("DELETE FROM item WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_all_item_warehouse=QStringLiteral("DELETE FROM item_warehouse WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_character=QStringLiteral("DELETE FROM monster WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_monster_by_id=QStringLiteral("DELETE FROM monster WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_recipes=QStringLiteral("DELETE FROM recipe WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_reputation=QStringLiteral("DELETE FROM reputation WHERE character=%1");
        PreparedDBQueryCommon::db_query_delete_allow=QStringLiteral("DELETE FROM character_allow WHERE character=%1");

        PreparedDBQueryCommon::db_query_select_clan_by_name=QStringLiteral("SELECT id FROM clan WHERE name='%1'");
        PreparedDBQueryCommon::db_query_select_character_by_pseudo=QStringLiteral("SELECT id FROM character WHERE pseudo='%1'");
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_monster=QStringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6,position=%7 WHERE id=%1");
            PreparedDBQueryCommon::db_query_insert_monster=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,%3,%4,%5,0,0,%6,%7,0,%3,%8,0)");
            PreparedDBQueryCommon::db_query_insert_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,0)");
            PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,1)");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp=QStringLiteral("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1");
        }
        else
        {
            PreparedDBQueryCommon::db_query_monster=QStringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,position=%6 WHERE id=%1");
            PreparedDBQueryCommon::db_query_insert_monster=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,%3,%4,%5,0,%6,%7,0,%3,%8,0)");
            PreparedDBQueryCommon::db_query_insert_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,0)");
            PreparedDBQueryCommon::db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position,place) VALUES(%1,%2,1)");
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1");
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=0 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 AND place=1 ORDER BY position ASC");
            PreparedDBQueryCommon::db_query_update_monster_xp=QStringLiteral("UPDATE monster SET xp=%2 WHERE id=%1");
        }
        PreparedDBQueryCommon::db_query_update_monster_move_to_player=QStringLiteral("UPDATE monster SET place=0,position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player=QStringLiteral("UPDATE monster SET place=0,character=%1,position=%2 WHERE id=%3");
        PreparedDBQueryCommon::db_query_update_monster_move_to_warehouse=QStringLiteral("UPDATE monster SET place=1,position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_move_to_market=QStringLiteral("UPDATE monster SET place=2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_insert_monster_skill=QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryCommon::db_query_insert_item=QStringLiteral("INSERT INTO item(item,character,quantity) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_insert_item_warehouse=QStringLiteral("INSERT INTO item_warehouse(item,character,quantity) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id=QStringLiteral("SELECT account,time_to_delete FROM character WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id=QStringLiteral("UPDATE character SET time_to_delete=%2 WHERE id=%1");
        PreparedDBQueryCommon::db_query_select_reputation_by_id=QStringLiteral("SELECT type,point,level FROM reputation WHERE character=%1 ORDER BY type");
        PreparedDBQueryCommon::db_query_select_recipes_by_player_id=QStringLiteral("SELECT recipe FROM recipe WHERE character=%1 ORDER BY recipe");
        PreparedDBQueryCommon::db_query_select_items_by_player_id=QStringLiteral("SELECT item,quantity FROM item WHERE character=%1 ORDER BY item");
        PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id=QStringLiteral("SELECT item,quantity FROM item_warehouse WHERE character=%1 ORDER BY item");
        PreparedDBQueryCommon::db_query_select_monstersSkill_by_id=QStringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1 ORDER BY skill");
        PreparedDBQueryCommon::db_query_select_monstersBuff_by_id=QStringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1 ORDER BY buff");
        PreparedDBQueryCommon::db_query_change_right=QStringLiteral("UPDATE \"character\" SET \"type\"=%2 WHERE \"id\"=%1");
        PreparedDBQueryCommon::db_query_update_item=QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryCommon::db_query_update_item_warehouse=QStringLiteral("UPDATE item_warehouse SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryCommon::db_query_delete_item=QStringLiteral("DELETE FROM item WHERE item=%1 AND character=%2");
        PreparedDBQueryCommon::db_query_delete_item_warehouse=QStringLiteral("DELETE FROM item_warehouse WHERE item=%1 AND character=%2");
        PreparedDBQueryCommon::db_query_update_cash=QStringLiteral("UPDATE character SET cash=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_warehouse_cash=QStringLiteral("UPDATE character SET warehouse_cash=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_insert_recipe=QStringLiteral("INSERT INTO recipe(character,recipe) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_insert_character_allow=QStringLiteral("INSERT INTO character_allow(character,allow) VALUES(%1,%2)");
        PreparedDBQueryCommon::db_query_delete_character_allow=QStringLiteral("DELETE FROM character_allow WHERE character=%1 AND allow=%2");
        PreparedDBQueryCommon::db_query_insert_reputation=QStringLiteral("INSERT INTO reputation(character,type,point,level) VALUES(%1,'%2',%3,%4)");
        PreparedDBQueryCommon::db_query_update_reputation=QStringLiteral("UPDATE reputation SET point=%3,level=%4 WHERE character=%1 AND type='%2'");
        PreparedDBQueryCommon::db_query_update_character_clan=QStringLiteral("UPDATE character SET clan=0 WHERE id=%1");
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader=QStringLiteral("UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;");
        PreparedDBQueryCommon::db_query_delete_clan=QStringLiteral("DELETE FROM clan WHERE id=%1");
        PreparedDBQueryCommon::db_query_delete_city=QStringLiteral("DELETE FROM city WHERE city='%1'");
        PreparedDBQueryCommon::db_query_update_character_clan_by_pseudo=QStringLiteral("UPDATE character SET clan=0 WHERE pseudo=%1 AND clan=%2");
        PreparedDBQueryCommon::db_query_update_monster_hp_only=QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_sp_only=QStringLiteral("UPDATE monster SET sp=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_skill_level=QStringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3");
        PreparedDBQueryCommon::db_query_insert_monster_buff=QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3)");
        PreparedDBQueryCommon::db_query_update_monster_level=QStringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_update_monster_position=QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2");
        PreparedDBQueryCommon::db_query_update_monster_and_hp=QStringLiteral("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3");
        PreparedDBQueryCommon::db_query_delete_monster_specific_buff=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_update_monster_level_only=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        PreparedDBQueryCommon::db_query_delete_monster_specific_skill=QStringLiteral("DELETE FROM monster_skill WHERE monster=%1 AND skill=%2");
        PreparedDBQueryCommon::db_query_insert_clan=QStringLiteral("INSERT INTO clan(id,name,date) VALUES(%1,'%2',%3);");
        PreparedDBQueryCommon::db_query_update_monster_owner=QStringLiteral("UPDATE monster SET character=%2 WHERE id=%1;");
        PreparedDBQueryCommon::db_query_select_server_time=QStringLiteral("SELECT server,played_time,last_connect FROM server_time WHERE account=%1");//not by characters to prevent too hurge datas to store
        PreparedDBQueryCommon::db_query_insert_server_time=QStringLiteral("INSERT INTO server_time(server,account,played_time,last_connect) VALUES(0,%1,0,%2);");
        break;
    }
}

void PreparedDBQueryServer::initDatabaseQueryServer(const DatabaseBase::Type &type)
{
    switch(type)
    {
        default:
        return;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::Type::Mysql:
        PreparedDBQueryServer::db_query_character_server_by_id=QStringLiteral("SELECT `x`,`y`,`orientation`,`map`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`market_cash` FROM `character_forserver` WHERE `id`=%1");
        PreparedDBQueryServer::db_query_delete_all_item_market=QStringLiteral("DELETE FROM `item_market` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_delete_monster_market_by_character=QStringLiteral("DELETE FROM `monster_market` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_delete_monster_market_by_id=QStringLiteral("DELETE FROM `monster_market` WHERE `id`=%1");
        PreparedDBQueryServer::db_query_insert_item_market=QStringLiteral("INSERT INTO `item_market`(`item`,`character`,`quantity`,`market_price`) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryServer::db_query_delete_item_market=QStringLiteral("DELETE FROM `item_market` WHERE `item`=%1 AND `character`=%2");
        PreparedDBQueryServer::db_query_update_item_market=QStringLiteral("UPDATE `item_market` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3");
        PreparedDBQueryServer::db_query_update_item_market_and_price=QStringLiteral("UPDATE `item_market` SET `quantity`=%1,`market_price`=%2 WHERE `item`=%3 AND `character`=%4;");
        PreparedDBQueryServer::db_query_update_charaters_market_cash=QStringLiteral("UPDATE `character_forserver` SET `market_cash`=`market_cash`+%1 WHERE `id`=%2");
        PreparedDBQueryServer::db_query_get_market_cash=QStringLiteral("UPDATE `character` SET `cash`=%1,`market_cash`=0 WHERE `id`=%2;");
        PreparedDBQueryServer::db_query_insert_monster_market_price=QStringLiteral("INSERT INTO `monster_market_price`(`id`,`market_price`) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_delete_monster_market_price=QStringLiteral("DELETE FROM `monster_market_price` WHERE `id`=%1");

        PreparedDBQueryServer::db_query_delete_bot_already_beaten=QStringLiteral("DELETE FROM `bot_already_beaten` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_delete_plant=QStringLiteral("DELETE FROM `plant` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_delete_plant_by_id=QStringLiteral("DELETE FROM `plant` WHERE `id`=%1");
        PreparedDBQueryServer::db_query_delete_quest=QStringLiteral("DELETE FROM `quest` WHERE `character`=%1");
        PreparedDBQueryServer::db_query_select_quest_by_id=QStringLiteral("SELECT `quest`,`finish_one_time`,`step` FROM `quest` WHERE `character`=%1 ORDER BY `quest`");
        PreparedDBQueryServer::db_query_select_bot_beaten=QStringLiteral("SELECT `botfight_id` FROM `bot_already_beaten` WHERE `character`=%1 ORDER BY `botfight_id`");
        PreparedDBQueryServer::db_query_select_itemOnMap=QStringLiteral("SELECT `itemOnMap` FROM `character_itemonmap` WHERE `character`=%1 ORDER BY `itemOnMap`");
        PreparedDBQueryServer::db_query_insert_itemonmap=QStringLiteral("INSERT INTO `character_itemonmap`(`character`,`itemOnMap`) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_factory=QStringLiteral("INSERT INTO `factory`(`id`,`resources`,`products`,`last_update`) VALUES(%1,'%2','%3',%4)");
        PreparedDBQueryServer::db_query_update_factory=QStringLiteral("UPDATE `factory` SET `resources`='%2',`products`='%3',`last_update`=%4 WHERE `id`=%1");
        PreparedDBQueryServer::db_query_delete_city=QStringLiteral("DELETE FROM `city` WHERE `city`='%1'");
        PreparedDBQueryServer::db_query_update_character_clan_by_pseudo=QStringLiteral("UPDATE `character` SET `clan`=0 WHERE `pseudo`=%1 AND `clan`=%2");
        PreparedDBQueryServer::db_query_insert_bot_already_beaten=QStringLiteral("INSERT INTO `bot_already_beaten`(`character`,`botfight_id`) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_plant=QStringLiteral("INSERT INTO `plant`(`id`,`map`,`x`,`y`,`plant`,`character`,`plant_timestamps`) VALUES(%1,%2,%3,%4,%5,%6,%7);");
        PreparedDBQueryServer::db_query_update_quest_finish=QStringLiteral("UPDATE `quest` SET `step`=0,`finish_one_time`=1 WHERE `character`=%1 AND `quest`=%2;");
        PreparedDBQueryServer::db_query_update_quest_step=QStringLiteral("UPDATE `quest` SET `step`=%3 WHERE `character`=%1 AND `quest`=%2;");
        PreparedDBQueryServer::db_query_update_quest_restart=QStringLiteral("UPDATE `quest` SET `step`=1 WHERE `character`=%1 AND `quest`=%2;");
        PreparedDBQueryServer::db_query_insert_quest=QStringLiteral("INSERT INTO `quest`(`character`,`quest`,`finish_one_time`,`step`) VALUES(%1,%2,0,%3);");
        PreparedDBQueryServer::db_query_update_city_clan=QStringLiteral("UPDATE `city` SET `clan`=%1 WHERE `city`='%2';");
        PreparedDBQueryServer::db_query_insert_city=QStringLiteral("INSERT INTO `city`(`clan`,`city`) VALUES(%1,'%2');");
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::Type::SQLite:
        PreparedDBQueryServer::db_query_character_server_by_id=QStringLiteral("SELECT x,y,orientation,map,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,market_cash FROM character_forserver WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_all_item_market=QStringLiteral("DELETE FROM item_market WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_monster_market_by_character=QStringLiteral("DELETE FROM monster_market WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_monster_market_by_id=QStringLiteral("DELETE FROM monster_market WHERE id=%1");
        PreparedDBQueryServer::db_query_insert_item_market=QStringLiteral("INSERT INTO item_market(item,character,quantity,market_price) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryServer::db_query_delete_item_market=QStringLiteral("DELETE FROM item_market WHERE item=%1 AND character=%2");
        PreparedDBQueryServer::db_query_update_item_market=QStringLiteral("UPDATE item_market SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryServer::db_query_update_item_market_and_price=QStringLiteral("UPDATE item_market SET quantity=%1,market_price=%2 WHERE item=%3 AND character=%4;");
        PreparedDBQueryServer::db_query_update_charaters_market_cash=QStringLiteral("UPDATE character_forserver SET market_cash=market_cash+%1 WHERE id=%2");
        PreparedDBQueryServer::db_query_get_market_cash=QStringLiteral("UPDATE character SET cash=%1,market_cash=0 WHERE id=%2;");
        PreparedDBQueryServer::db_query_insert_monster_market_price=QStringLiteral("INSERT INTO monster_market_price(id,market_price) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_delete_monster_market_price=QStringLiteral("DELETE FROM monster_market_price WHERE id=%1");

        PreparedDBQueryServer::db_query_delete_bot_already_beaten=QStringLiteral("DELETE FROM bot_already_beaten WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_plant=QStringLiteral("DELETE FROM plant WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_plant_by_id=QStringLiteral("DELETE FROM plant WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_quest=QStringLiteral("DELETE FROM quest WHERE character=%1");

        PreparedDBQueryServer::db_query_select_quest_by_id=QStringLiteral("SELECT quest,finish_one_time,step FROM quest WHERE character=%1 ORDER BY quest");
        PreparedDBQueryServer::db_query_select_bot_beaten=QStringLiteral("SELECT botfight_id FROM bot_already_beaten WHERE character=%1 ORDER BY bot_already_beaten");
        PreparedDBQueryServer::db_query_select_itemOnMap=QStringLiteral("SELECT \"itemOnMap\" FROM \"character_itemonmap\" WHERE character=%1 ORDER BY \"itemOnMap\"");
        PreparedDBQueryServer::db_query_insert_itemonmap=QStringLiteral("INSERT INTO \"character_itemonmap\"(character,\"itemOnMap\") VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_factory=QStringLiteral("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)");
        PreparedDBQueryServer::db_query_update_factory=QStringLiteral("UPDATE factory SET resources='%2',products='%3',last_update=%4 WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_city=QStringLiteral("DELETE FROM city WHERE city='%1'");
        PreparedDBQueryServer::db_query_insert_bot_already_beaten=QStringLiteral("INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_plant=QStringLiteral("INSERT INTO plant(id,map,x,y,plant,character,plant_timestamps) VALUES(%1,%2,%3,%4,%5,%6,%7);");
        PreparedDBQueryServer::db_query_update_city_clan=QStringLiteral("UPDATE city SET clan=%1 WHERE city='%2';");
        PreparedDBQueryServer::db_query_insert_city=QStringLiteral("INSERT INTO city(clan,city) VALUES(%1,'%2');");
        break;
        #endif

        case DatabaseBase::Type::PostgreSQL:
        PreparedDBQueryServer::db_query_character_server_by_id=QStringLiteral("SELECT x,y,orientation,map,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,market_cash FROM character_forserver WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_all_item_market=QStringLiteral("DELETE FROM item_market WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_monster_market_by_character=QStringLiteral("DELETE FROM monster_market WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_monster_market_by_id=QStringLiteral("DELETE FROM monster_market WHERE id=%1");
        PreparedDBQueryServer::db_query_insert_item_market=QStringLiteral("INSERT INTO item_market(item,character,quantity,market_price) VALUES(%1,%2,%3,%4)");
        PreparedDBQueryServer::db_query_delete_item_market=QStringLiteral("DELETE FROM item_market WHERE item=%1 AND character=%2");
        PreparedDBQueryServer::db_query_update_item_market=QStringLiteral("UPDATE item_market SET quantity=%1 WHERE item=%2 AND character=%3");
        PreparedDBQueryServer::db_query_update_item_market_and_price=QStringLiteral("UPDATE item_market SET quantity=%1,market_price=%2 WHERE item=%3 AND character=%4;");
        PreparedDBQueryServer::db_query_update_charaters_market_cash=QStringLiteral("UPDATE character_forserver SET market_cash=market_cash+%1 WHERE id=%2");
        PreparedDBQueryServer::db_query_get_market_cash=QStringLiteral("UPDATE character SET cash=%1,market_cash=0 WHERE id=%2;");
        PreparedDBQueryServer::db_query_insert_monster_market_price=QStringLiteral("INSERT INTO monster_market_price(id,market_price) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_delete_monster_market_price=QStringLiteral("DELETE FROM monster_market_price WHERE id=%1");

        PreparedDBQueryServer::db_query_delete_bot_already_beaten=QStringLiteral("DELETE FROM bot_already_beaten WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_plant=QStringLiteral("DELETE FROM plant WHERE character=%1");
        PreparedDBQueryServer::db_query_delete_plant_by_id=QStringLiteral("DELETE FROM plant WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_quest=QStringLiteral("DELETE FROM quest WHERE character=%1");

        PreparedDBQueryServer::db_query_select_quest_by_id=QStringLiteral("SELECT quest,finish_one_time,step FROM quest WHERE character=%1 ORDER BY quest");
        PreparedDBQueryServer::db_query_select_bot_beaten=QStringLiteral("SELECT botfight_id FROM bot_already_beaten WHERE character=%1 ORDER BY bot_already_beaten");
        PreparedDBQueryServer::db_query_select_itemOnMap=QStringLiteral("SELECT \"itemOnMap\" FROM \"character_itemonmap\" WHERE character=%1 ORDER BY \"itemOnMap\"");
        PreparedDBQueryServer::db_query_insert_itemonmap=QStringLiteral("INSERT INTO \"character_itemonmap\"(character,\"itemOnMap\") VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_factory=QStringLiteral("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)");
        PreparedDBQueryServer::db_query_update_factory=QStringLiteral("UPDATE factory SET resources='%2',products='%3',last_update=%4 WHERE id=%1");
        PreparedDBQueryServer::db_query_delete_city=QStringLiteral("DELETE FROM city WHERE city='%1'");
        PreparedDBQueryServer::db_query_insert_bot_already_beaten=QStringLiteral("INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2)");
        PreparedDBQueryServer::db_query_insert_plant=QStringLiteral("INSERT INTO plant(id,map,x,y,plant,character,plant_timestamps) VALUES(%1,%2,%3,%4,%5,%6,%7);");
        PreparedDBQueryServer::db_query_update_city_clan=QStringLiteral("UPDATE city SET clan=%1 WHERE city='%2';");
        PreparedDBQueryServer::db_query_insert_city=QStringLiteral("INSERT INTO city(clan,city) VALUES(%1,'%2');");
        break;
    }
}

