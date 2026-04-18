#include "MapVisualiserPlayerWithFight.hpp"

#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/MoveOnTheMap.hpp"
#include "../../libcatchchallenger/Api_protocol.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "QMap_client.hpp"
#include "TemporaryTile.hpp"

#include <iostream>
#include <QDebug>

MapVisualiserPlayerWithFight::MapVisualiserPlayerWithFight(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &openGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,openGL)
{
    repel_step=0;
    fightCollisionBot=NULL;
    botAlreadyBeaten=NULL;
}

MapVisualiserPlayerWithFight::~MapVisualiserPlayerWithFight()
{
    if(fightCollisionBot!=NULL)
    {
        //delete[] fightCollisionBot;
        fightCollisionBot=NULL;
    }
    if(botAlreadyBeaten!=NULL)
    {
        delete[] botAlreadyBeaten;
        botAlreadyBeaten=NULL;
    }
}

void MapVisualiserPlayerWithFight::addBeatenBotFight(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botFightId)
{
    (void)mapId;
    if(botAlreadyBeaten==NULL)
        abort();
    botAlreadyBeaten[botFightId/8]|=(1<<(7-botFightId%8));
}

bool MapVisualiserPlayerWithFight::haveBeatBot(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botFightId) const
{
    (void)mapId;
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
    if(client->isInFight())
    {
        qDebug() << "Strange, try move when is in fight at keyPressParse()";
        return;
    }

    MapVisualiserPlayer::keyPressParse();
}

bool MapVisualiserPlayerWithFight::haveStopTileAction()
{
    //--dropsenddataafteronmap: skip all fight-triggering collisions on the
    //stopped tile so the player can freely explore the datapack map.
    if(CatchChallenger::Api_protocol::dropOutputAfterOnMap)
        return false;
    if(client->isInFight())
    {
        qDebug() << "Strange, try move when is in fight at moveStepSlot()";
        return true;
    }
    CatchChallenger::PlayerMonster *fightMonster;
    if(!client->getAbleToFight())
        fightMonster=NULL;
    else
        fightMonster=client->getCurrentMonster();
    if(fightMonster!=NULL)
    {
        std::pair<uint8_t,uint8_t> pos(getPos());
        const QMap_client * current_map_pointer=CatchChallenger::QMap_client::all_map.at(current_map);
        const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(current_map);

        if(logicalMap.botsFightTrigger.find(pos)!=logicalMap.botsFightTrigger.cend())
        {
            const uint8_t fightId=logicalMap.botsFightTrigger.at(pos);
            if(!haveBeatBot(current_map,fightId))
            {
                qDebug() <<  "is now in fight with: " << fightId;
                if(isInMove())
                {
                    stopMove();
                    emit send_player_direction(getDirection());
                    keyPressed.clear();
                }
                parseStop();
                std::pair<uint8_t,uint8_t> remotePoint(0,0);
                if(current_map_pointer->botsFightTriggerExtra.find(pos)!=current_map_pointer->botsFightTriggerExtra.cend())
                    remotePoint=current_map_pointer->botsFightTriggerExtra.at(pos);
                emit botFightCollision(const_cast<CatchChallenger::Map_client *>(static_cast<const CatchChallenger::Map_client *>(&logicalMap)),
                                       remotePoint.first,remotePoint.second);
                if(current_map_pointer->botsDisplay.find(remotePoint)!=
                        current_map_pointer->botsDisplay.cend())
                {
                    TemporaryTile *temporaryTile=current_map_pointer->botsDisplay.at(remotePoint).temporaryTile;
                    //show a temporary flags
                    {
                        if(fightCollisionBot==NULL)
                        {
                            fightCollisionBot=Tiled::Tileset::create(QStringLiteral("fightCollisionBot"),16,16);
                            fightCollisionBot->loadFromImage(QImage(QStringLiteral(":/CC/images/fightCollisionBot.png")),QStringLiteral(":/CC/images/fightCollisionBot.png"));
                        }
                    }
                    temporaryTile->startAnimation(fightCollisionBot->tileAt(0),150,static_cast<uint8_t>(fightCollisionBot->tileCount()));
                }
                else
                    qDebug() <<  "temporaryTile not found";
                blocked=true;
                return true;
            }
        }
        //check if is in fight collision, but only if is move
        if(repel_step<=0)
        {
            if(inMove)
            {
                if(client->generateWildFightIfCollision(logicalMap,x,y,client->get_player_informations().items,client->getEvents()))
                {
                    inMove=false;
                    emit send_player_direction(direction);
                    keyPressed.clear();
                    parseStop();
                    emit wildFightCollision(const_cast<CatchChallenger::Map_client *>(static_cast<const CatchChallenger::Map_client *>(&logicalMap)),x,y);
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

bool MapVisualiserPlayerWithFight::canGoTo(const CatchChallenger::Direction &direction, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y, const bool &checkCollision)
{
    if(!MapVisualiserPlayer::canGoTo(direction,mapIndex,x,y,checkCollision))
        return false;
    if(!CatchChallenger::Api_protocol::dropOutputAfterOnMap && client->isInFight())
    {
        qDebug() << "Strange, try move when is in fight";
        return false;
    }
    const std::vector<CatchChallenger::CommonMap> &mapList=QtDatapackClientLoader::datapackLoader->get_mapList();
    CATCHCHALLENGER_TYPE_MAPID newMapIndex=mapIndex;
    COORD_TYPE lx=x,ly=y;
    if(!CatchChallenger::MoveOnTheMap::moveWithoutTeleport(mapList,direction,newMapIndex,lx,ly,false))
    {
        qDebug() << "Strange, can go but move failed";
        return false;
    }
    if(CatchChallenger::QMap_client::all_map.find(newMapIndex)==CatchChallenger::QMap_client::all_map.cend())
        return false;
    const CatchChallenger::CommonMap &map_client=QtDatapackClientLoader::datapackLoader->getMap(newMapIndex);
    const QMap_client *map_full=CatchChallenger::QMap_client::all_map.at(newMapIndex);
    const std::map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &playerItems=client->get_player_informations().items;
    const std::map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &playerQuests=client->get_player_informations().quests;

    if(!CatchChallenger::Api_protocol::dropOutputAfterOnMap)
    {
        {
            int list_size=map_client.teleporters.size();
            int index=0;
            while(index<list_size)
            {
                const CatchChallenger::Teleporter &teleporter=map_client.teleporters.at(index);
                if(teleporter.source_x==lx && teleporter.source_y==ly)
                {
                    switch(teleporter.condition.type)
                    {
                        case CatchChallenger::MapConditionType_None:
                        case CatchChallenger::MapConditionType_Clan://not do for now
                        break;
                        case CatchChallenger::MapConditionType_FightBot:
                            if(!haveBeatBot(newMapIndex,teleporter.condition.data.fightBot))
                                return false;
                        break;
                       case CatchChallenger::MapConditionType_Item:
                            if(playerItems.find(teleporter.condition.data.item)==playerItems.cend())
                            {
                                if(index<(int)map_full->teleport_condition_texts.size() && !map_full->teleport_condition_texts.at(index).empty())
                                    emit teleportConditionNotRespected(map_full->teleport_condition_texts.at(index));
                                return false;
                            }
                        break;
                        case CatchChallenger::MapConditionType_Quest:
                            if(playerQuests.find(teleporter.condition.data.quest)==playerQuests.cend())
                            {
                                if(index<(int)map_full->teleport_condition_texts.size() && !map_full->teleport_condition_texts.at(index).empty())
                                    emit teleportConditionNotRespected(map_full->teleport_condition_texts.at(index));
                                return false;
                            }
                            if(!playerQuests.at(teleporter.condition.data.quest).finish_one_time)
                            {
                                if(index<(int)map_full->teleport_condition_texts.size() && !map_full->teleport_condition_texts.at(index).empty())
                                    emit teleportConditionNotRespected(map_full->teleport_condition_texts.at(index));
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
            std::pair<uint8_t,uint8_t> pos(lx,ly);
            if(map_client.botsFightTrigger.find(pos)!=map_client.botsFightTrigger.cend())
            {
                const uint8_t fightId=map_client.botsFightTrigger.at(pos);
                if(!haveBeatBot(newMapIndex,fightId))
                {
                    if(!client->getAbleToFight())
                    {
                        emit blockedOn(MapVisualiserPlayer::BlockedOn_Fight);
                        return false;
                    }
                }
            }
        }
    }
    const CatchChallenger::CommonMap &commonMap=map_client;
    const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(commonMap,lx,ly);
    if(!monstersCollisionValue.walkOn.empty())
    {
        unsigned int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().at(monstersCollisionValue.walkOn.at(index));
            if(monstersCollision.item==0 || playerItems.find(monstersCollision.item)!=playerItems.cend())
            {
                if(!CatchChallenger::Api_protocol::dropOutputAfterOnMap)
                {
                    if(!monstersCollisionValue.walkOnMonsters.at(index).defaultMonsters.empty())
                    {
                        if(!client->getAbleToFight())
                        {
                            emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneFight);
                            return false;
                        }
                        if(!client->canDoRandomFight(commonMap,lx,ly))
                        {
                            emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
                            return false;
                        }
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
