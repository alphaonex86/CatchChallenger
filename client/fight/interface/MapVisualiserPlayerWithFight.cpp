#include "MapVisualiserPlayerWithFight.h"

#include "../../fight/interface/FightEngine.h"

MapVisualiserPlayerWithFight::MapVisualiserPlayerWithFight(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,OpenGL)
{
}

void MapVisualiserPlayerWithFight::keyPressParse()
{
    if(CatchChallenger::FightEngine::fightEngine.isInFight())
    {
        qDebug() << "Strange, try move when is in fight at keyPressParse()";
        return;
    }

    MapVisualiserPlayer::keyPressParse();
}

bool MapVisualiserPlayerWithFight::haveStopTileAction()
{
    if(CatchChallenger::FightEngine::fightEngine.isInFight())
    {
        qDebug() << "Strange, try move when is in fight at moveStepSlot()";
        return true;
    }
    //check if is in fight collision
    if(CatchChallenger::FightEngine::fightEngine.haveRandomFight(current_map->logicalMap,x,y))
    {
        inMove=false;
        emit send_player_direction(direction);
        parseStop();
        emit fightCollision(static_cast<CatchChallenger::Map_client *>(&current_map->logicalMap),x,y);
        keyPressed.clear();
        return true;
    }
    return false;
}

bool MapVisualiserPlayerWithFight::canGoTo(const CatchChallenger::Direction &direction, CatchChallenger::Map map, quint8 x, quint8 y, const bool &checkCollision)
{
    if(!MapVisualiserPlayer::canGoTo(direction,map,x,y,checkCollision))
        return false;
    if(CatchChallenger::FightEngine::fightEngine.isInFight())
    {
        qDebug() << "Strange, try move when is in fight";
        return false;
    }
    CatchChallenger::Map *new_map=&map;
    CatchChallenger::MoveOnTheMap::move(direction,&new_map,&x,&y,false);
    if(CatchChallenger::MoveOnTheMap::isGrass(*new_map,x,y) && !new_map->grassMonster.empty())
    {
        if(!CatchChallenger::FightEngine::fightEngine.canDoFight())
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_Grass);
            return false;
        }
        if(!CatchChallenger::FightEngine::fightEngine.canDoRandomFight(*new_map,x,y))
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
            return false;
        }
    }
    if(CatchChallenger::MoveOnTheMap::isWater(*new_map,x,y) && !new_map->waterMonster.empty())
    {
        if(!CatchChallenger::FightEngine::fightEngine.canDoFight())
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_Wather);
            return false;
        }
        if(!CatchChallenger::FightEngine::fightEngine.canDoRandomFight(*new_map,x,y))
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
            return false;
        }
    }
    if(!new_map->caveMonster.empty())
    {
        if(!CatchChallenger::FightEngine::fightEngine.canDoFight())
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_Cave);
            return false;
        }
        if(!CatchChallenger::FightEngine::fightEngine.canDoRandomFight(*new_map,x,y))
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
            return false;
        }
    }
    return true;
}
