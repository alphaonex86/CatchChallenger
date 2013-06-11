#include "MapVisualiserPlayerWithFight.h"

#include "../../fight/interface/FightEngine.h"

MapVisualiserPlayerWithFight::MapVisualiserPlayerWithFight(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,OpenGL)
{
}

void MapVisualiserPlayerWithFight::addBotAlreadyBeaten(const QList<quint32> &botAlreadyBeaten)
{
    this->botAlreadyBeaten << botAlreadyBeaten;
    resetAll();
}

void MapVisualiserPlayerWithFight::resetAll()
{
    botAlreadyBeaten.clear();
    MapVisualiserPlayer::resetAll();
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
    QList<CatchChallenger::BotFightOnMap> botFightList=static_cast<CatchChallenger::Map_client *>(new_map)->botsFight.values(QPair<quint8,quint8>(x,y));
    int index=0;
    while(index<botFightList)
    {
        const CatchChallenger::BotFightOnMap &botFightOnMap=botFightList.at(index);
        if(static_cast<CatchChallenger::Map_client *>(new_map)->bots.contains(QPair<quint8,quint8>(botFightOnMap.position.first,botFightOnMap.position.second)))
        {
            const CatchChallenger::Bot &bot=static_cast<CatchChallenger::Map_client *>(new_map)->bots[QPair<quint8,quint8>(botFightOnMap.position.first,botFightOnMap.position.second)];
            //bot.step.contains(botFightOnMap.botId)
            if(bot.botId==botFightOnMap.botId)
            {
                if(bot.step.contains(botFightOnMap.step))
                {
                    const QDomElement &step=bot.step[botFightOnMap.step];
                    if(step.hasAttribute("type") && step.hasAttribute("fightid") && step.attribute("type")=="fight")
                    {
                        bool ok;
                        quint32 fightid=step.attribute("fightid").toUInt(&ok);
                        if(ok)
                        {
                            if(!botAlreadyBeaten.contains(fightid))
                            {
                                inMove=false;
                                emit send_player_direction(direction);
                                parseStop();
                                emit fightCollision(static_cast<CatchChallenger::Map_client *>(&current_map->logicalMap),x,y);
                                keyPressed.clear();
                                return true;
                            }
                            else
                                qDebug() << "Internal error: bot already beaten";
                        }
                        else
                            qDebug() << "Internal error: fightid is not a number";
                    }
                }
                else
                    qDebug() << "Internal error: bot have not this step to fight";
            }
            else
                qDebug() << "Internal error: bot id don't match";
        }
        else
            qDebug() << "Internal error: have bot point listed but not linked with a bot";
        index++;
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
    QList<CatchChallenger::BotFightOnMap> botFightList=static_cast<CatchChallenger::Map_client *>(new_map)->botsFight.values(QPair<quint8,quint8>(x,y));
    int index=0;
    while(index<botFightList)
    {
        const CatchChallenger::BotFightOnMap &botFightOnMap=botFightList.at(index);
        if(static_cast<CatchChallenger::Map_client *>(new_map)->bots.contains(QPair<quint8,quint8>(botFightOnMap.position.first,botFightOnMap.position.second)))
        {
            const CatchChallenger::Bot &bot=static_cast<CatchChallenger::Map_client *>(new_map)->bots[QPair<quint8,quint8>(botFightOnMap.position.first,botFightOnMap.position.second)];
            //bot.step.contains(botFightOnMap.botId)
            if(bot.botId==botFightOnMap.botId)
            {
                if(bot.step.contains(botFightOnMap.step))
                {
                    const QDomElement &step=bot.step[botFightOnMap.step];
                    if(step.hasAttribute("type") && step.hasAttribute("fightid") && step.attribute("type")=="fight")
                    {
                        bool ok;
                        quint32 fightid=step.attribute("fightid").toUInt(&ok);
                        if(ok)
                        {
                            if(!botAlreadyBeaten.contains(fightid))
                            {
                                if(!CatchChallenger::FightEngine::fightEngine.canDoFight())
                                {
                                    emit blockedOn(MapVisualiserPlayer::BlockedOn_Fight);
                                    return false;
                                }
                            }
                            else
                                qDebug() << "Internal error: bot already beaten";
                        }
                        else
                            qDebug() << "Internal error: fightid is not a number";
                    }
                }
                else
                    qDebug() << "Internal error: bot have not this step to fight";
            }
            else
                qDebug() << "Internal error: bot id don't match";
        }
        else
            qDebug() << "Internal error: have bot point listed but not linked with a bot";
        index++;
    }
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
