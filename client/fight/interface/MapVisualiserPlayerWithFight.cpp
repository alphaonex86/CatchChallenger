#include "MapVisualiserPlayerWithFight.h"

#include "../../fight/interface/ClientFightEngine.h"

MapVisualiserPlayerWithFight::MapVisualiserPlayerWithFight(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,OpenGL)
{
    repel_step=0;
    items=NULL;
    quests=NULL;
}

void MapVisualiserPlayerWithFight::setBotsAlreadyBeaten(const QSet<quint32> &botAlreadyBeaten)
{
    this->botAlreadyBeaten=botAlreadyBeaten;
}

void MapVisualiserPlayerWithFight::addBeatenBotFight(const quint32 &botFightId)
{
    botAlreadyBeaten << botFightId;
}

bool MapVisualiserPlayerWithFight::haveBeatBot(const quint32 &botFightId) const
{
    return botAlreadyBeaten.contains(botFightId);
}

void MapVisualiserPlayerWithFight::addRepelStep(const quint32 &repel_step)
{
    this->repel_step+=repel_step;
}

void MapVisualiserPlayerWithFight::setInformations(QHash<quint32,quint32> *items,QHash<quint32, CatchChallenger::PlayerQuest> *quests)
{
    this->items=items;
    this->quests=quests;
}

void MapVisualiserPlayerWithFight::resetAll()
{
    botAlreadyBeaten.clear();
    MapVisualiserPlayer::resetAll();
}

void MapVisualiserPlayerWithFight::keyPressParse()
{
    if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        qDebug() << "Strange, try move when is in fight at keyPressParse()";
        return;
    }

    MapVisualiserPlayer::keyPressParse();
}

bool MapVisualiserPlayerWithFight::haveStopTileAction()
{
    if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        qDebug() << "Strange, try move when is in fight at moveStepSlot()";
        return true;
    }
    CatchChallenger::PlayerMonster *fightMonster;
    if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
        fightMonster=NULL;
    else
        fightMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(fightMonster!=NULL)
    {
        QList<quint32> botFightList=all_map.value(current_map)->logicalMap.botsFightTrigger.values(QPair<quint8,quint8>(x,y));
        QList<QPair<quint8,quint8> > botFightRemotePointList=all_map.value(current_map)->logicalMap.botsFightTriggerExtra.values(QPair<quint8,quint8>(x,y));
        int index=0;
        while(index<botFightList.size())
        {
            if(!botAlreadyBeaten.contains(botFightList.at(index)))
            {
                if(inMove)
                {
                    inMove=false;
                    emit send_player_direction(direction);
                    keyPressed.clear();
                }
                parseStop();
                emit botFightCollision(static_cast<CatchChallenger::Map_client *>(&all_map.value(current_map)->logicalMap),botFightRemotePointList.at(index).first,botFightRemotePointList.at(index).second);
                return true;
            }
            index++;
        }
        //check if is in fight collision, but only if is move
        if(repel_step<=0)
        {
            if(inMove)
            {
                if(CatchChallenger::ClientFightEngine::fightEngine.generateWildFightIfCollision(&all_map.value(current_map)->logicalMap,x,y))
                {
                    inMove=false;
                    emit send_player_direction(direction);
                    keyPressed.clear();
                    parseStop();
                    emit wildFightCollision(static_cast<CatchChallenger::Map_client *>(&all_map.value(current_map)->logicalMap),x,y);
                    return true;
                }
            }
        }
        else
        {
            repel_step--;
            if(repel_step==0)
                emit repelEffectIsOver();
        }
    }
    else
        qDebug() << "Strange, not monster, skip all the fight type";
    return false;
}

bool MapVisualiserPlayerWithFight::canGoTo(const CatchChallenger::Direction &direction, CatchChallenger::Map map, quint8 x, quint8 y, const bool &checkCollision)
{
    if(!MapVisualiserPlayer::canGoTo(direction,map,x,y,checkCollision))
        return false;
    if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        qDebug() << "Strange, try move when is in fight";
        return false;
    }
    CatchChallenger::Map *new_map=&map;
    if(CatchChallenger::MoveOnTheMap::moveWithoutTeleport(direction,&new_map,&x,&y,false))
    {
        qDebug() << "Strange, can go but move failed";
        return false;
    }
    if(!all_map.contains(new_map->map_file))
        return false;
    const CatchChallenger::Map_client &map_client=all_map.value(new_map->map_file)->logicalMap;

    {
        int list_size=map_client.teleport_semi.size();
        int index=0;
        while(index<list_size)
        {
            const CatchChallenger::Map_semi_teleport &teleporter=map_client.teleport_semi.at(index);
            if(teleporter.source_x==x && teleporter.source_y==y)
            {
                switch(teleporter.condition.type)
                {
                    case CatchChallenger::MapConditionType_None:
                    case CatchChallenger::MapConditionType_Clan://not do for now
                    break;
                    case CatchChallenger::MapConditionType_FightBot:
                        if(!haveBeatBot(teleporter.condition.value))
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                    break;
                    case CatchChallenger::MapConditionType_Item:
                        if(items==NULL)
                            break;
                        if(!items->contains(teleporter.condition.value))
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                    break;
                    case CatchChallenger::MapConditionType_Quest:
                        if(quests==NULL)
                            break;
                        if(!quests->contains(teleporter.condition.value))
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                        if(!quests->value(teleporter.condition.value).finish_one_time)
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                    break;
                    default:
                    break;
                }
            }
            index++;
        }
    }

    QList<quint32> botFightList=map_client.botsFightTrigger.values(QPair<quint8,quint8>(x,y));
    int index=0;
    while(index<botFightList.size())
    {
        if(!botAlreadyBeaten.contains(botFightList.at(index)))
        {
            if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
            {
                emit blockedOn(MapVisualiserPlayer::BlockedOn_Fight);
                return false;
            }
        }
        index++;
    }
    if(CatchChallenger::MoveOnTheMap::isGrass(*new_map,x,y) && !new_map->grassMonster.empty())
    {
        if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_Grass);
            return false;
        }
        if(!CatchChallenger::ClientFightEngine::fightEngine.canDoRandomFight(*new_map,x,y))
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
            return false;
        }
    }
    if(CatchChallenger::MoveOnTheMap::isWater(*new_map,x,y) && !new_map->waterMonster.empty())
    {
        if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_Wather);
            return false;
        }
        if(!CatchChallenger::ClientFightEngine::fightEngine.canDoRandomFight(*new_map,x,y))
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
            return false;
        }
    }
    if(!new_map->caveMonster.empty())
    {
        if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_Cave);
            return false;
        }
        if(!CatchChallenger::ClientFightEngine::fightEngine.canDoRandomFight(*new_map,x,y))
        {
            emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
            return false;
        }
    }
    return true;
}
