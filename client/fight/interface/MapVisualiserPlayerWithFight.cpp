  #include "MapVisualiserPlayerWithFight.h"

#include "../../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"

MapVisualiserPlayerWithFight::MapVisualiserPlayerWithFight(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,OpenGL)
{
    this->events=events;
    repel_step=0;
    items=NULL;
    quests=NULL;
    fightCollisionBot=NULL;
    botAlreadyBeaten=NULL;
}

MapVisualiserPlayerWithFight::~MapVisualiserPlayerWithFight()
{
    if(fightCollisionBot!=NULL)
    {
        delete fightCollisionBot;
        fightCollisionBot=NULL;
    }
    if(botAlreadyBeaten!=NULL)
    {
        delete botAlreadyBeaten;
        botAlreadyBeaten=NULL;
    }
}

void MapVisualiserPlayerWithFight::setBotsAlreadyBeaten(const char * const botAlreadyBeaten)
{
    if(this->botAlreadyBeaten!=NULL)
    {
        delete this->botAlreadyBeaten;
        this->botAlreadyBeaten=NULL;
    }
    this->botAlreadyBeaten=(char *)malloc(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
    memset(this->botAlreadyBeaten,0,CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
    if(botAlreadyBeaten!=NULL)
        memcpy(this->botAlreadyBeaten,botAlreadyBeaten,CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
}

void MapVisualiserPlayerWithFight::addBeatenBotFight(const uint16_t &botFightId)
{
    if(botAlreadyBeaten==NULL)
        abort();
    botAlreadyBeaten[botFightId/8]|=(1<<(7-botFightId%8));
}

bool MapVisualiserPlayerWithFight::haveBeatBot(const uint16_t &botFightId) const
{
    if(botAlreadyBeaten==NULL)
        abort();
    return botAlreadyBeaten[botFightId/8] & (1<<(7-botFightId%8));
}

void MapVisualiserPlayerWithFight::addRepelStep(const uint32_t &repel_step)
{
    this->repel_step+=repel_step;
}

void MapVisualiserPlayerWithFight::resetAll()
{
    if(botAlreadyBeaten!=NULL)
    {
        delete botAlreadyBeaten;
        botAlreadyBeaten=NULL;
    }
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
    if(items==NULL)
    {
        qDebug() << "items is null, can't move";
        return true;
    }
    if(events==NULL)
    {
        qDebug() << "events is null, can't move";
        return true;
    }
    CatchChallenger::PlayerMonster *fightMonster;
    if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
        fightMonster=NULL;
    else
        fightMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(fightMonster!=NULL)
    {
        std::pair<uint8_t,uint8_t> pos(x,y);
        if(all_map.value(current_map)->logicalMap.botsFightTrigger.find(pos)!=all_map.value(current_map)->logicalMap.botsFightTrigger.cend())
        {
            std::vector<uint32_t> botFightList=all_map.value(current_map)->logicalMap.botsFightTrigger.at(pos);
            QList<QPair<uint8_t,uint8_t> > botFightRemotePointList=all_map.value(current_map)->logicalMap.botsFightTriggerExtra.values(QPair<uint8_t,uint8_t>(x,y));
            unsigned int index=0;
            while(index<botFightList.size())
            {
                const uint32_t &fightId=botFightList.at(index);
                if(!haveBeatBot(fightId))
                {
                    qDebug() <<  "is now in fight with: " << fightId;
                    if(inMove)
                    {
                        inMove=false;
                        emit send_player_direction(direction);
                        keyPressed.clear();
                    }
                    parseStop();
                    emit botFightCollision(static_cast<CatchChallenger::Map_client *>(&all_map.value(current_map)->logicalMap),botFightRemotePointList.at(index).first,botFightRemotePointList.at(index).second);
                    if(all_map.value(current_map)->logicalMap.botsDisplay.contains(botFightRemotePointList.at(index)))
                    {
                        TemporaryTile *temporaryTile=all_map.value(current_map)->logicalMap.botsDisplay.value(botFightRemotePointList.at(index)).temporaryTile;
                        //show a temporary flags
                        {
                            if(fightCollisionBot==NULL)
                            {
                                fightCollisionBot=new Tiled::Tileset(QLatin1Literal("fightCollisionBot"),16,16);
                                fightCollisionBot->loadFromImage(QImage(QStringLiteral(":/images/fightCollisionBot.png")),QStringLiteral(":/images/fightCollisionBot.png"));
                            }
                        }
                        temporaryTile->startAnimation(fightCollisionBot->tileAt(0),150,fightCollisionBot->tileCount());
                    }
                    else
                        qDebug() <<  "temporaryTile not found";
                    blocked=true;
                    return true;
                }
                index++;
            }
        }
        //check if is in fight collision, but only if is move
        if(repel_step<=0)
        {
            if(inMove)
            {
                if(CatchChallenger::ClientFightEngine::fightEngine.generateWildFightIfCollision(&all_map.value(current_map)->logicalMap,x,y,*items,*events))
                {
                    inMove=false;
                    emit send_player_direction(direction);
                    keyPressed.clear();
                    parseStop();
                    emit wildFightCollision(static_cast<CatchChallenger::Map_client *>(&all_map.value(current_map)->logicalMap),x,y);
                    blocked=true;
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

bool MapVisualiserPlayerWithFight::canGoTo(const CatchChallenger::Direction &direction, CatchChallenger::CommonMap map, uint8_t x, uint8_t y, const bool &checkCollision)
{
    if(!MapVisualiserPlayer::canGoTo(direction,map,x,y,checkCollision))
        return false;
    if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        qDebug() << "Strange, try move when is in fight";
        return false;
    }
    CatchChallenger::CommonMap *new_map=&map;
    if(!CatchChallenger::MoveOnTheMap::moveWithoutTeleport(direction,&new_map,&x,&y,false))
    {
        qDebug() << "Strange, can go but move failed";
        return false;
    }
    if(!all_map.contains(QString::fromStdString(new_map->map_file)))
        return false;
    const CatchChallenger::Map_client &map_client=all_map.value(QString::fromStdString(new_map->map_file))->logicalMap;

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
                        if(items->find(teleporter.condition.value)==items->cend())
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                    break;
                    case CatchChallenger::MapConditionType_Quest:
                        if(quests==NULL)
                            break;
                        if(quests->find(teleporter.condition.value)==quests->cend())
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                        if(!quests->at(teleporter.condition.value).finish_one_time)
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

    {
        std::pair<uint8_t,uint8_t> pos(x,y);
        if(map_client.botsFightTrigger.find(pos)!=map_client.botsFightTrigger.cend())
        {
            std::vector<uint32_t> botFightList=map_client.botsFightTrigger.at(pos);
            unsigned int index=0;
            while(index<botFightList.size())
            {
                if(!haveBeatBot(botFightList.at(index)))
                {
                    if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
                    {
                        emit blockedOn(MapVisualiserPlayer::BlockedOn_Fight);
                        return false;
                    }
                }
                index++;
            }
        }
    }
    const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(*new_map,x,y);
    if(!monstersCollisionValue.walkOn.empty())
    {
        unsigned int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(monstersCollisionValue.walkOn.at(index));
            if(monstersCollision.item==0 || items->find(monstersCollision.item)!=items->cend())
            {
                if(!monstersCollisionValue.walkOnMonsters.at(index).defaultMonsters.empty())
                {
                    if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
                    {
                        emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneFight);
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
            index++;
        }
        emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneItem);
        return false;
    }
    return true;
}
