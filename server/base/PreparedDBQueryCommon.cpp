#include "PreparedDBQuery.hpp"
#include <iostream>

using namespace CatchChallenger;

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
void PreparedDBQueryCommon::initDatabaseQueryCommonWithoutSP(const DatabaseBase::DatabaseType &type, CatchChallenger::DatabaseBase * const database)
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
        //login and gameserver alone
        PreparedDBQueryCommon::db_query_delete_monster_by_id=PreparedStatementUnit("DELETE FROM `monster` WHERE `id`=%1",database);
        PreparedDBQueryCommon::db_query_insert_monster="INSERT INTO `monster`(`id`,`character`,`place`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`buffs`,`skills`,`skills_endurance`) "
                                                       "VALUES(%1,%2,%3,%4,%5,%6,0,0,%7,%8,0,%9,%10,'',UNHEX('%11'),UNHEX('%12'))";
        /*wild catch*/
        PreparedDBQueryCommon::db_query_insert_monster_full=PreparedStatementUnit("INSERT INTO `monster`(`id`,`character`,`place`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`,`buffs`,`skills`,`skills_endurance`) "
                                                       "VALUES(%1,%2,%3,%4,%5,%6,0,0,%7,%8,0,%9,%10,UNHEX('%11'),UNHEX('%12'),UNHEX('%13')",database);

        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
        PreparedDBQueryCommon::db_query_update_character_item=PreparedStatementUnit("UPDATE `character` SET `item`=UNHEX('%1') WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_character_item_and_encyclopedia=PreparedStatementUnit("UPDATE `character` SET `item`=UNHEX('%1'),`encyclopedia_item`=UNHEX('%2') WHERE `id`=%3",database);
        PreparedDBQueryCommon::db_query_update_character_item_warehouse=PreparedStatementUnit("UPDATE `character` SET `item_warehouse`=UNHEX('%1') WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_character_monster_encyclopedia=PreparedStatementUnit("UPDATE `character` SET `encyclopedia_monster`=UNHEX('%1') WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_cash=PreparedStatementUnit("UPDATE `character` SET `cash`=%1 WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_warehouse_cash=PreparedStatementUnit("UPDATE `character` SET `warehouse_cash`=%1 WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_character_recipe=PreparedStatementUnit("UPDATE `character` SET `recipes`=UNHEX('%1') WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_character_allow=PreparedStatementUnit("UPDATE `character` SET `allow`=UNHEX('%1') WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_character_reputations=PreparedStatementUnit("UPDATE `character` SET `reputations`=UNHEX('%1') WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_select_clan_by_name=PreparedStatementUnit("SELECT `id` FROM `clan` WHERE `name`='%1'",database);

        PreparedDBQueryCommon::db_query_insert_clan=PreparedStatementUnit("INSERT INTO `clan`(`id`,`name`,`cash`,`date`) VALUES(%1,'%2',0,%3);",database);
        PreparedDBQueryCommon::db_query_update_character_clan_to_reset=PreparedStatementUnit("UPDATE `character` SET `clan`=0 WHERE `id`=%1",database);
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader=PreparedStatementUnit("UPDATE `character` SET `clan`=%1,`clan_leader`=%2 WHERE `id`=%3;",database);
        PreparedDBQueryCommon::db_query_delete_clan=PreparedStatementUnit("DELETE FROM `clan` WHERE `id`=%1",database);
        PreparedDBQueryCommon::db_query_update_monster_hp_only=PreparedStatementUnit("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_sp_only=PreparedStatementUnit("UPDATE `monster` SET `sp`=%1 WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_played_time=PreparedStatementUnit("UPDATE `character` SET `played_time`=`played_time`+%1 WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_monster_update_skill_and_endurance=PreparedStatementUnit("UPDATE `monster` SET `skills`=UNHEX('%1'),`skills_endurance`=UNHEX('%2') WHERE `id`=%3",database);
        PreparedDBQueryCommon::db_query_monster_update_endurance=PreparedStatementUnit("UPDATE `monster` SET `skills_endurance`=UNHEX('%1') WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_buff=PreparedStatementUnit("UPDATE `monster` SET `buffs`=UNHEX('%1') WHERE `id`=%2",database);

        PreparedDBQueryCommon::db_query_character_by_id=PreparedStatementUnit("SELECT `account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`warehouse_cash`,`clan_leader`,`time_to_delete`,`starter`,LOWER(HEX(`allow`)),LOWER(HEX(`item`)),LOWER(HEX(`item_warehouse`)),LOWER(HEX(`recipes`)),LOWER(HEX(`reputations`)),LOWER(HEX(`encyclopedia_monster`)),LOWER(HEX(`encyclopedia_item`)),LOWER(HEX(`achievements`)),`blob_version`,`date`,`lastdaillygift` FROM `character` WHERE `id`=%1",database);
        PreparedDBQueryCommon::db_query_set_character_time_to_delete_to_zero=PreparedStatementUnit("UPDATE `character` SET `time_to_delete`=0 WHERE `id`=%1",database);
        PreparedDBQueryCommon::db_query_update_character_last_connect=PreparedStatementUnit("UPDATE `character` SET `last_connect`=%1 WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_clan=PreparedStatementUnit("SELECT `name`,`cash` FROM `clan` WHERE `id`=%1",database);
        PreparedDBQueryCommon::db_query_change_right=PreparedStatementUnit("UPDATE `character` SET `type`=%1 WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_delete_monster_buff=PreparedStatementUnit("UPDATE `monster` SET `buffs`='' WHERE `id`=%1",database);
        PreparedDBQueryCommon::db_query_insert_warehouse_monster=PreparedStatementUnit("INSERT INTO `monster`(`id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`skills`,`skills_endurance`) "
                                                       "VALUES(%1,%2,%3,%4,0,0,%5,%6,0,%7,UNHEX('%8'),UNHEX('%9')",database);
        PreparedDBQueryCommon::db_query_update_monster_position=PreparedStatementUnit("UPDATE `monster` SET `position`=%1 WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_position_and_place=PreparedStatementUnit("UPDATE `monster` SET `position`=%1,`place`=%2 WHERE `id`=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_place=PreparedStatementUnit("UPDATE `monster` SET `place`=%1 WHERE `id`=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_and_hp=PreparedStatementUnit("UPDATE `monster` SET `hp`=%1,`monster`=%2 WHERE `id`=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_hp_and_level=PreparedStatementUnit("UPDATE `monster` SET `hp`=%1,`level`=%2 WHERE `id`=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player=PreparedStatementUnit("UPDATE `monster` SET `place`=1,`character`=%1,`position`=%2 WHERE `id`=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_owner=PreparedStatementUnit("UPDATE `monster` SET `character`=%1,`position`=%2 WHERE `id`=%3;",database);
        PreparedDBQueryCommon::db_query_update_gift=PreparedStatementUnit("UPDATE `character` SET `lastdaillygift`=%1 WHERE `id`=%2;",database);
        #endif
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        //login and gameserver alone
        PreparedDBQueryCommon::db_query_delete_monster_by_id=PreparedStatementUnit("DELETE FROM monster WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_insert_monster="INSERT INTO monster(id,character,place,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,buffs,skills,skills_endurance) "
                                                       "VALUES(%1,%2,%3,%4,%5,%6,0,0,%7,%8,0,%9,%10,'','%11','%12')";
        /*wild catch*/
        PreparedDBQueryCommon::db_query_insert_monster_full=PreparedStatementUnit("INSERT INTO monster(id,character,place,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,buffs,skills,skills_endurance) "
                                                       "VALUES(%1,%2,%3,%4,%5,%6,0,0,%7,%8,0,%9,%10,'%11','%12','%13')",database);

        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
        PreparedDBQueryCommon::db_query_update_character_item=PreparedStatementUnit("UPDATE character SET item='%1' WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_item_and_encyclopedia=PreparedStatementUnit("UPDATE character SET item='%1',encyclopedia_item='%2' WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_character_item_warehouse=PreparedStatementUnit("UPDATE character SET item_warehouse='%1' WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_monster_encyclopedia=PreparedStatementUnit("UPDATE character SET encyclopedia_monster='%1' WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_cash=PreparedStatementUnit("UPDATE character SET cash=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_warehouse_cash=PreparedStatementUnit("UPDATE character SET warehouse_cash=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_recipe=PreparedStatementUnit("UPDATE character SET recipes='%1' WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_allow=PreparedStatementUnit("UPDATE character SET allow='%1' WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_reputations=PreparedStatementUnit("UPDATE character SET reputations='%1' WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_select_clan_by_name=PreparedStatementUnit("SELECT id FROM clan WHERE name='%1'",database);

        PreparedDBQueryCommon::db_query_insert_clan=PreparedStatementUnit("INSERT INTO clan(id,name,cash,date) VALUES(%1,'%2',0,%3);",database);
        PreparedDBQueryCommon::db_query_update_character_clan_to_reset=PreparedStatementUnit("UPDATE character SET clan=0 WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader=PreparedStatementUnit("UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;",database);
        PreparedDBQueryCommon::db_query_delete_clan=PreparedStatementUnit("DELETE FROM clan WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_update_monster_hp_only=PreparedStatementUnit("UPDATE monster SET hp=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_sp_only=PreparedStatementUnit("UPDATE monster SET sp=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_played_time=PreparedStatementUnit("UPDATE character SET played_time=played_time+%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_monster_update_skill_and_endurance=PreparedStatementUnit("UPDATE monster SET skills='%1',skills_endurance='%2' WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_monster_update_endurance=PreparedStatementUnit("UPDATE monster SET skills_endurance='%1' WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_buff=PreparedStatementUnit("UPDATE monster SET buffs='%1' WHERE id=%2",database);

        PreparedDBQueryCommon::db_query_character_by_id=PreparedStatementUnit("SELECT account,pseudo,skin,type,clan,cash,warehouse_cash,clan_leader,time_to_delete,starter,allow,item,item_warehouse,recipes,reputations,encyclopedia_monster,encyclopedia_item,achievements,blob_version,date,lastdaillygift FROM character WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_set_character_time_to_delete_to_zero=PreparedStatementUnit("UPDATE character SET time_to_delete=0 WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_update_character_last_connect=PreparedStatementUnit("UPDATE character SET last_connect=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_clan=PreparedStatementUnit("SELECT name,cash FROM clan WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_change_right=PreparedStatementUnit("UPDATE \"character\" SET \"type\"=%1 WHERE \"id\"=%2",database);
        PreparedDBQueryCommon::db_query_delete_monster_buff=PreparedStatementUnit("UPDATE monster SET buffs='' WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_insert_warehouse_monster=PreparedStatementUnit("INSERT INTO monster(id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,skills,skills_endurance) "
                                                       "VALUES(%1,%2,%3,%4,0,0,%5,%6,0,%7,'%8','%9')",database);
        PreparedDBQueryCommon::db_query_update_monster_position=PreparedStatementUnit("UPDATE monster SET position=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_position_and_place=PreparedStatementUnit("UPDATE monster SET position=%1,place=%2 WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_place=PreparedStatementUnit("UPDATE monster SET place=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_and_hp=PreparedStatementUnit("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_hp_and_level=PreparedStatementUnit("UPDATE monster SET hp=%1,level=%2 WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player=PreparedStatementUnit("UPDATE monster SET place=1,character=%1,position=%2 WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_owner=PreparedStatementUnit("UPDATE monster SET character=%1,position=%2 WHERE id=%3;",database);
        PreparedDBQueryCommon::db_query_update_gift=PreparedStatementUnit("UPDATE character SET lastdaillygift=%1 WHERE id=%2;",database);
        #endif
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
        //login and gameserver alone
        PreparedDBQueryCommon::db_query_delete_monster_by_id=PreparedStatementUnit("DELETE FROM monster WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_insert_monster="INSERT INTO monster(id,character,place,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,buffs,skills,skills_endurance) "
                                                       "VALUES(%1,%2,%3,%4,%5,%6,0,0,%7,%8,0,%9,%10,'',decode('%11','hex'),decode('%12','hex'))";
        /*wild catch*/
        PreparedDBQueryCommon::db_query_insert_monster_full=PreparedStatementUnit("INSERT INTO monster(id,character,place,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,buffs,skills,skills_endurance) "
                                                       "VALUES(%1,%2,%3,%4,%5,%6,0,0,%7,%8,0,%9,%10,decode('%11','hex'),decode('%12','hex'),decode('%13','hex'))",database);

        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
        PreparedDBQueryCommon::db_query_update_character_item=PreparedStatementUnit("UPDATE character SET item=decode('%1','hex') WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_item_and_encyclopedia=PreparedStatementUnit("UPDATE character SET item=decode('%1','hex'),encyclopedia_item=decode('%2','hex') WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_character_item_warehouse=PreparedStatementUnit("UPDATE character SET item_warehouse=decode('%1','hex') WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_monster_encyclopedia=PreparedStatementUnit("UPDATE character SET encyclopedia_monster=decode('%1','hex') WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_cash=PreparedStatementUnit("UPDATE character SET cash=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_warehouse_cash=PreparedStatementUnit("UPDATE character SET warehouse_cash=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_recipe=PreparedStatementUnit("UPDATE character SET recipes=decode('%1','hex') WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_allow=PreparedStatementUnit("UPDATE character SET allow=decode('%1','hex') WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_character_reputations=PreparedStatementUnit("UPDATE character SET reputations=decode('%1','hex') WHERE id=%2",database);
        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        PreparedDBQueryCommon::db_query_insert_clan=PreparedStatementUnit("INSERT INTO clan(id,name,cash,date) VALUES(%1,%2,0,%3);",database);
        PreparedDBQueryCommon::db_query_select_clan_by_name=PreparedStatementUnit("SELECT id FROM clan WHERE name=%1",database);
        #else
        PreparedDBQueryCommon::db_query_insert_clan=PreparedStatementUnit("INSERT INTO clan(id,name,cash,date) VALUES(%1,'%2',0,%3);",database);
        PreparedDBQueryCommon::db_query_select_clan_by_name=PreparedStatementUnit("SELECT id FROM clan WHERE name='%1'",database);
        #endif
        PreparedDBQueryCommon::db_query_update_character_clan_to_reset=PreparedStatementUnit("UPDATE character SET clan=0 WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_update_character_clan_and_leader=PreparedStatementUnit("UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;",database);
        PreparedDBQueryCommon::db_query_delete_clan=PreparedStatementUnit("DELETE FROM clan WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_update_monster_hp_only=PreparedStatementUnit("UPDATE monster SET hp=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_sp_only=PreparedStatementUnit("UPDATE monster SET sp=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_played_time=PreparedStatementUnit("UPDATE character SET played_time=played_time+%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_monster_update_skill_and_endurance=PreparedStatementUnit("UPDATE monster SET skills=decode('%1','hex'),skills_endurance=decode('%2','hex') WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_monster_update_endurance=PreparedStatementUnit("UPDATE monster SET skills_endurance=decode('%1','hex') WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_buff=PreparedStatementUnit("UPDATE monster SET buffs=decode('%1','hex') WHERE id=%2",database);

        PreparedDBQueryCommon::db_query_character_by_id=PreparedStatementUnit("SELECT account,pseudo,skin,type,clan,cash,warehouse_cash,clan_leader,time_to_delete,starter,encode(allow,'hex'),encode(item,'hex'),encode(item_warehouse,'hex'),encode(recipes,'hex'),encode(reputations,'hex'),encode(encyclopedia_monster,'hex'),encode(encyclopedia_item,'hex'),encode(achievements,'hex'),blob_version,date,lastdaillygift FROM character WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_set_character_time_to_delete_to_zero=PreparedStatementUnit("UPDATE character SET time_to_delete=0 WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_update_character_last_connect=PreparedStatementUnit("UPDATE character SET last_connect=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_clan=PreparedStatementUnit("SELECT name,cash FROM clan WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_change_right=PreparedStatementUnit("UPDATE \"character\" SET \"type\"=%1 WHERE \"id\"=%2",database);
        PreparedDBQueryCommon::db_query_delete_monster_buff=PreparedStatementUnit("UPDATE monster SET buffs='' WHERE id=%1",database);
        PreparedDBQueryCommon::db_query_insert_warehouse_monster=PreparedStatementUnit("INSERT INTO monster(id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,skills,skills_endurance) "
                                                       "VALUES(%1,%2,%3,%4,0,0,%5,%6,0,%7,decode('%8','hex'),decode('%9','hex'))",database);
        PreparedDBQueryCommon::db_query_update_monster_position=PreparedStatementUnit("UPDATE monster SET position=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_position_and_place=PreparedStatementUnit("UPDATE monster SET position=%1,place=%2 WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_place=PreparedStatementUnit("UPDATE monster SET place=%1 WHERE id=%2",database);
        PreparedDBQueryCommon::db_query_update_monster_and_hp=PreparedStatementUnit("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_hp_and_level=PreparedStatementUnit("UPDATE monster SET hp=%1,level=%2 WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_move_to_new_player=PreparedStatementUnit("UPDATE monster SET place=1,character=%1,position=%2 WHERE id=%3",database);
        PreparedDBQueryCommon::db_query_update_monster_owner=PreparedStatementUnit("UPDATE monster SET character=%1,position=%2 WHERE id=%3;",database);
        PreparedDBQueryCommon::db_query_update_gift=PreparedStatementUnit("UPDATE character SET lastdaillygift=%1 WHERE id=%2;",database);
        #endif
        #if defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
        PreparedDBQueryCommon::db_query_insert_server_time=PreparedStatementUnit("INSERT INTO server_time(server,account,played_time,last_connect) VALUES(%1,%2,0,%3);",database);
        PreparedDBQueryCommon::db_query_update_server_time_played_time=PreparedStatementUnit("UPDATE server_time SET played_time=played_time+%1 WHERE server=%2 AND account=%3;",database);
        PreparedDBQueryCommon::db_query_update_server_time_last_connect=PreparedStatementUnit("UPDATE server_time SET last_connect=%1 WHERE server=%2 AND account=%3;",database);
        #endif
        break;
        #endif
    }
}

void PreparedDBQueryCommon::initDatabaseQueryCommonWithSP(const DatabaseBase::DatabaseType &type, const bool &useSP, CatchChallenger::DatabaseBase * const database)
{
    (void)useSP;
    (void)database;
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
        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=PreparedStatementUnit("SELECT `id`,`character`,`place`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,LOWER(HEX(`buffs`)),LOWER(HEX(`skills`)),LOWER(HEX(`skills_endurance`)),`sp` FROM `monster` WHERE `character`=%1 ORDER BY `position` ASC",database);
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=PreparedStatementUnit("UPDATE `monster` SET `hp`=%1,`xp`=%2,`level`=%3,`sp`=%4 WHERE `id`=%5",database);
            PreparedDBQueryCommon::db_query_update_monster_xp=PreparedStatementUnit("UPDATE `monster` SET `xp`=%1,`sp`=%2 WHERE `id`=%3",database);
        }
        else
        {
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=PreparedStatementUnit("SELECT `id`,`character`,`place`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,LOWER(HEX(`buffs`)),LOWER(HEX(`skills`)),LOWER(HEX(`skills_endurance`)) FROM `monster` WHERE `character`=%1 ORDER BY `position` ASC",database);
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=PreparedStatementUnit("UPDATE `monster` SET `hp`=%1,`xp`=%2,`level`=%3 WHERE `id`=%4",database);
            PreparedDBQueryCommon::db_query_update_monster_xp=PreparedStatementUnit("UPDATE `monster` SET `xp`=%1 WHERE `id`=%2",database);
        }
        #endif
        break;
        #endif

        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=PreparedStatementUnit("UPDATE monster SET hp=%1,xp=%2,level=%3,sp=%4 WHERE id=%5",database);
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=PreparedStatementUnit("SELECT id,character,place,hp,monster,level,xp,captured_with,gender,egg_step,character_origin,buffs,skills,skills_endurance,sp FROM monster WHERE character=%1 ORDER BY position ASC",database);
            PreparedDBQueryCommon::db_query_update_monster_xp=PreparedStatementUnit("UPDATE monster SET xp=%1,sp=%2 WHERE id=%3",database);
        }
        else
        {
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=PreparedStatementUnit("UPDATE monster SET hp=%1,xp=%2,level=%3 WHERE id=%4",database);
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=PreparedStatementUnit("SELECT id,character,place,hp,monster,level,xp,captured_with,gender,egg_step,character_origin,buffs,skills,skills_endurance FROM monster WHERE character=%1 ORDER BY position ASC",database);
            PreparedDBQueryCommon::db_query_update_monster_xp=PreparedStatementUnit("UPDATE monster SET xp=%1 WHERE id=%2",database);
        }
        #endif
        break;
        #endif

        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
        #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
        if(useSP)
        {
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=PreparedStatementUnit("UPDATE monster SET hp=%1,xp=%2,level=%3,sp=%4 WHERE id=%5",database);
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=PreparedStatementUnit("SELECT id,character,place,hp,monster,level,xp,captured_with,gender,egg_step,character_origin,encode(buffs,'hex'),encode(skills,'hex'),encode(skills_endurance,'hex'),sp FROM monster WHERE character=%1 ORDER BY position ASC",database);
            PreparedDBQueryCommon::db_query_update_monster_xp=PreparedStatementUnit("UPDATE monster SET xp=%1,sp=%2 WHERE id=%3",database);
        }
        else
        {
            PreparedDBQueryCommon::db_query_update_monster_xp_hp_level=PreparedStatementUnit("UPDATE monster SET hp=%1,xp=%2,level=%3 WHERE id=%4",database);
            PreparedDBQueryCommon::db_query_select_monsters_by_player_id=PreparedStatementUnit("SELECT id,character,place,hp,monster,level,xp,captured_with,gender,egg_step,character_origin,encode(buffs,'hex'),encode(skills,'hex'),encode(skills_endurance,'hex') FROM monster WHERE character=%1 ORDER BY position ASC",database);
            PreparedDBQueryCommon::db_query_update_monster_xp=PreparedStatementUnit("UPDATE monster SET xp=%1 WHERE id=%2",database);
        }
        #endif
        break;
        #endif
    }
}
#endif
