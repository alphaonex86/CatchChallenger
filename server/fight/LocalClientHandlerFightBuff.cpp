#include "../base/VariableServer.hpp"
#include "../base/GlobalServerData.hpp"
#include "../base/MapServer.hpp"
#include "../base/Client.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../base/Client.hpp"
#include "../base/PreparedDBQuery.hpp"

using namespace CatchChallenger;

bool Client::removeBuffOnMonster(PlayerMonster * currentMonster, const uint32_t &buffId)
{
    if(currentMonster==NULL)
    {
        errorOutput("removeBuffOnMonster(): PlayerMonster * currentMonster==NULL");
        return false;
    }
    const bool returnVal=CommonFightEngine::removeBuffOnMonster(currentMonster,buffId);
    if(returnVal)
    {
        /*cosnt std::string &queryText=PreparedDBQueryCommon::db_query_delete_monster_specific_buff;
        stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
        stringreplaceOne(queryText,"%2",std::to_string(buffId));
        dbQueryWriteCommon(queryText);*/
        syncMonsterBuff(*currentMonster);
    }
    return returnVal;
}

bool Client::removeAllBuffOnMonster(PlayerMonster * currentMonster)
{
    const bool &returnVal=CommonFightEngine::removeAllBuffOnMonster(currentMonster);
    if(returnVal)
    {
        /*std::string queryText=PreparedDBQueryCommon::db_query_delete_monster_buff;
        stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
        dbQueryWriteCommon(queryText);*/
        syncMonsterBuff(*currentMonster);
    }
    return returnVal;
}

int Client::addCurrentBuffEffect(const Skill::BuffEffect &effect)
{
    const int &returnCode=CommonFightEngine::addCurrentBuffEffect(effect);
    if(returnCode==-2)
        return returnCode;
    if(CommonDatapack::commonDatapack.get_monsterBuffs().at(effect.buff).level.at(effect.level-1).duration==Buff::Duration_Always)
    {
        //if(returnCode==-1)
            switch(effect.on)
            {
                case ApplyOn_AloneEnemy:
                case ApplyOn_AllEnemy:
                if(isInBattle())
                {
                    syncMonsterBuff(*otherPlayerBattle->getCurrentMonster());
                    /*std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_buff;
                    stringreplaceOne(queryText,"%1",std::to_string(->id));
                    stringreplaceOne(queryText,"%2",std::to_string(effect.buff));
                    stringreplaceOne(queryText,"%3",std::to_string(effect.level));
                    dbQueryWriteCommon(queryText);*/
                }
                break;
                case ApplyOn_Themself:
                case ApplyOn_AllAlly:
                {
                    syncMonsterBuff(*getCurrentMonster());
                    /*std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_buff;
                    stringreplaceOne(queryText,"%1",std::to_string(getCurrentMonster()->id));
                    stringreplaceOne(queryText,"%2",std::to_string(effect.buff));
                    stringreplaceOne(queryText,"%3",std::to_string(effect.level));
                    dbQueryWriteCommon(queryText);*/
                }
                break;
                default:
                    errorOutput("Not apply match, can't apply the buff");
                break;
            }
        /*else
            switch(effect.on)
            {
                case ApplyOn_AloneEnemy:
                case ApplyOn_AllEnemy:
                if(isInBattle())
                {
                    std::string queryText=PreparedDBQueryCommon::db_query_update_monster_level;
                    stringreplaceOne(queryText,"%1",std::to_string(otherPlayerBattle->getCurrentMonster()->id));
                    stringreplaceOne(queryText,"%2",std::to_string(effect.buff));
                    stringreplaceOne(queryText,"%3",std::to_string(effect.level));
                    dbQueryWriteCommon(queryText);
                }
                break;
                case ApplyOn_Themself:
                case ApplyOn_AllAlly:
                {
                    std::string queryText=PreparedDBQueryCommon::db_query_update_monster_level;
                    stringreplaceOne(queryText,"%1",std::to_string(getCurrentMonster()->id));
                    stringreplaceOne(queryText,"%2",std::to_string(effect.buff));
                    stringreplaceOne(queryText,"%3",std::to_string(effect.level));
                    dbQueryWriteCommon(queryText);
                }
                break;
                default:
                    errorOutput("Not apply match, can't apply the buff");
                break;
            }*/
    }
    return returnCode;
}
