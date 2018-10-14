#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "../../general/base/CommonSettingsServer.h"
#include "MapBrowse.h"

#include <chrono>
#include <QMessageBox>
#include <iostream>

void BotTargetList::updatePlayerInformation()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(actionsAction->clientList.find(client->api)==actionsAction->clientList.cend())
        return;

    ActionsBotInterface::Player &player=actionsAction->clientList[client->api];

    if(actionsAction->id_map_to_map.find(player.mapId)!=actionsAction->id_map_to_map.cend())
    {
        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        QString mapString=QString::fromStdString(playerMap->map_file)+QString(" (%1,%2)").arg(player.x).arg(player.y);
        ui->label_local_target->setTitle("Target on the map: "+mapString);
        if(playerMap->step.size()<2)
            return;
        const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
        const uint16_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
        if(playerCodeZone>0 && (uint32_t)(playerCodeZone-1)<(uint32_t)stepPlayer.layers.size())
        {
            const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);
            QString overall_graphvizText=QString::fromStdString(graphStepNearMap(client,layer.blockObject,ui->searchDeep->value()));
            if(overall_graphvizText.isEmpty())
                ui->overall_graphvizText->setVisible(false);
            else
            {
                ui->overall_graphvizText->setVisible(true);
                ui->overall_graphvizText->setPlainText(overall_graphvizText);
            }

            {
                std::unordered_map<const MapServerMini::BlockObject *,MapServerMini::BlockObjectPathFinding> resolvedBlock;

                if(ui->hideTooHard->isChecked())
                    playerMap->targetBlockList(layer.blockObject,resolvedBlock,ui->searchDeep->value(),client->api);
                else
                    playerMap->targetBlockList(layer.blockObject,resolvedBlock,ui->searchDeep->value());

                if(player.target.blockObject!=NULL)
                    std::cerr << player.api->getPseudo() << " set blockObject " << player.target.blockObject->map->map_file
                              << " block " << (player.target.blockObject->id+1) << std::endl;

                const MapServerMini::MapParsedForBot &step=playerMap->step.at(ui->comboBoxStep->currentIndex());
                if(step.map==NULL)
                    return;

                ui->globalTargets->clear();
                player.targetListGlobalTarget.clear();
                alternateColor=false;
                ActionsBotInterface::GlobalTarget bestTarget;
                contentToGUI(client->api,ui->globalTargets,resolvedBlock,!ui->hideTooHard->isChecked(),dirt,itemOnMap,fight,shop,heal,wildMonster,bestTarget,playerMap,player.x,player.y);
                //show the best target
                switch(bestTarget.type)
                {
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::ItemOnMap:
                        ui->bestTarget->setText(QString::fromStdString("Best target: ItemOnMap on "+bestTarget.blockObject->map->map_file+" (Block "+std::to_string(bestTarget.blockObject->id+1)+"), extra: "+std::to_string(bestTarget.extra)));
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Fight:
                        ui->bestTarget->setText(QString::fromStdString("Best target: Fight on "+bestTarget.blockObject->map->map_file+" (Block "+std::to_string(bestTarget.blockObject->id+1)+"), extra: "+std::to_string(bestTarget.extra)));
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Shop:
                        ui->bestTarget->setText(QString::fromStdString("Best target: Shop on "+bestTarget.blockObject->map->map_file+" (Block "+std::to_string(bestTarget.blockObject->id+1)+"), extra: "+std::to_string(bestTarget.extra)));
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal:
                        ui->bestTarget->setText(QString::fromStdString("Best target: Heal on "+bestTarget.blockObject->map->map_file+" (Block "+std::to_string(bestTarget.blockObject->id+1)+"), extra: "+std::to_string(bestTarget.extra)));
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::WildMonster:
                        ui->bestTarget->setText(QString::fromStdString("Best target: WildMonster on "+bestTarget.blockObject->map->map_file+" (Block "+std::to_string(bestTarget.blockObject->id+1)+"), extra: "+std::to_string(bestTarget.extra)));
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Dirt:
                        ui->bestTarget->setText(QString::fromStdString("Best target: Dirt on "+bestTarget.blockObject->map->map_file+" (Block "+std::to_string(bestTarget.blockObject->id+1)+"), extra: "+std::to_string(bestTarget.extra)));
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Plant:
                        ui->bestTarget->setText(QString::fromStdString("Best target: Plant on "+bestTarget.blockObject->map->map_file+" (Block "+std::to_string(bestTarget.blockObject->id+1)+"), extra: "+std::to_string(bestTarget.extra)));
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::None:
                        ui->bestTarget->setText("Best target: None");
                    break;
                    default:
                        ui->bestTarget->setText("Best target: Unknown");
                    break;
                }
            }
            //the next target
            {
                if(player.target.blockObject==NULL)
                    ui->label_next_local_target->setText("Next local target: None");
                else
                {
                    const uint8_t x=player.target.linkPoint.x,y=player.target.linkPoint.y;
                    std::string stepToDo=BotTargetList::stepToString(player.target.localStep);
                    if(!stepToDo.empty())
                        stepToDo=", steps: "+stepToDo;
                    if(player.target.bestPath.empty())
                        ui->label_next_local_target->setText("Next global target: "+QString::fromStdString(player.target.blockObject->map->map_file)+" at "+QString::number(x)+","+QString::number(y)+QString::fromStdString(stepToDo));
                    else
                    {
                        const MapServerMini::BlockObject * const nextBlock=player.target.bestPath.at(0);
                        const MapServerMini::MapParsedForBot::Layer * const nextLlayer=static_cast<const MapServerMini::MapParsedForBot::Layer *>(nextBlock->layer);
                        ui->label_next_local_target->setText("Next local target: "+QString::fromStdString(nextLlayer->name)+" on "+QString::fromStdString(nextBlock->map->map_file)+QString::fromStdString(stepToDo));
                        //search the next position
                        for(const auto& n:layer.blockObject->links) {
                            const MapServerMini::BlockObject * const tempNextBlock=n.first;
                            //const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
                            if(tempNextBlock==nextBlock)
                            {
                                ui->label_next_local_target->setText("Next local target: "+QString::fromStdString(nextLlayer->name)+" on "+QString::fromStdString(nextBlock->map->map_file)+/*", go to "+
                                                                     QString::number(player.target.x)+","+QString::number(player.target.y)+*/
                                                                     QString::fromStdString(stepToDo)
                                                                     );
                                break;
                            }
                        }
                    }
                }
            }
        }
        else
            ui->label_local_target->setTitle(ui->label_local_target->title()+" (Out of the map)");
    }
    else
        ui->label_local_target->setTitle("Unknown player map ("+QString::number(player.mapId)+")");

    const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=client->api->get_player_informations();
    ui->label_player_cash->setText(QString("Cash: %1$").arg(player_private_and_public_informations.cash));
    {
        ui->inventory->clear();
        for(const auto& n:player_private_and_public_informations.items) {
            const uint32_t &itemId=n.first;
            const uint32_t &quantity=n.second;
            QListWidgetItem *item=new QListWidgetItem();
            if(DatapackClientLoader::datapackLoader.itemsExtra.find(itemId)!=DatapackClientLoader::datapackLoader.itemsExtra.cend())
            {
                item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.at(itemId).image);
                if(quantity>1)
                    item->setText(QString::number(quantity));
                item->setText(item->text()+" "+QString::fromStdString(DatapackClientLoader::datapackLoader.itemsExtra.at(itemId).name));
                item->setToolTip(QString::fromStdString(DatapackClientLoader::datapackLoader.itemsExtra.at(itemId).name));
            }
            else
            {
                item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
                if(quantity>1)
                    item->setText(QStringLiteral("id: %1 (x%2)").arg(itemId).arg(quantity));
                else
                    item->setText(QStringLiteral("id: %1").arg(itemId));
            }
            ui->inventory->addItem(item);
        }
    }

    updateFightStats();
}

void BotTargetList::updateFightStats()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(actionsAction->clientList.find(client->api)==actionsAction->clientList.cend())
        return;

    const ActionsBotInterface::Player &player=actionsAction->clientList.at(client->api);

    {
        const std::vector<CatchChallenger::PlayerMonster> &playerMonsters=player.fightEngine->getPlayerMonster();
        ui->monsterList->clear();
        if(playerMonsters.empty())
            return;
        unsigned int index=0;
        const uint8_t &selectedMonster=player.fightEngine->getCurrentSelectedMonsterNumber();
        while(index<playerMonsters.size())
        {
            const CatchChallenger::PlayerMonster &monster=playerMonsters.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
            {
                CatchChallenger::Monster::Stat stat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster),monster.level);

                QListWidgetItem *item=new QListWidgetItem();
                item->setToolTip(QString::fromStdString(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).description));
                if(!DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).thumb.isNull())
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).thumb);
                else
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).front);

                QString lastSkill;
                QHash<uint32_t,uint8_t> skillToDisplay;
                unsigned int sub_index=0;
                while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.size())
                {
                    CatchChallenger::Monster::AttackToLearn learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.at(sub_index);
                    if(learn.learnAtLevel<=monster.level)
                    {
                        unsigned int sub_index2=0;
                        while(sub_index2<monster.skills.size())
                        {
                            const CatchChallenger::PlayerMonster::PlayerSkill &skill=monster.skills.at(sub_index2);
                            if(skill.skill==learn.learnSkill)
                                break;
                            sub_index2++;
                        }
                        if(
                                //if skill not found
                                (sub_index2==monster.skills.size() && learn.learnSkillLevel==1)
                                ||
                                //if skill already found and need level up
                                (sub_index2<monster.skills.size() && (monster.skills.at(sub_index2).level+1)==learn.learnSkillLevel)
                        )
                        {
                            if(skillToDisplay.contains(learn.learnSkill))
                            {
                                if(skillToDisplay.value(learn.learnSkill)>learn.learnSkillLevel)
                                    skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                            }
                            else
                                skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                        }
                    }
                    sub_index++;

                    if(skillToDisplay.isEmpty())
                        lastSkill=tr("No skill to learn");
                    else
                        lastSkill=tr("%n skill(s) to learn","",skillToDisplay.size());

                }
                const CatchChallenger::Monster &monsterGeneralInfo=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster);
                if(monster.level>=monsterGeneralInfo.level_to_xp.size())
                {
                    std::cerr << "monster.level>=monsterGeneralInfo.level_to_xp.size()" << std::endl;
                    abort();
                }
                const uint32_t &maxXp=monsterGeneralInfo.level_to_xp.at(monster.level-1);
                item->setText(tr("%1, level: %2\nHP: %3/%4, SP: %5, XP: %6/%7\n%8")
                        .arg(QString::fromStdString(DatapackClientLoader::datapackLoader.monsterExtra.at(monster.monster).name))
                        .arg(monster.level)
                        .arg(monster.hp)
                        .arg(stat.hp)
                        .arg(monster.sp)
                        .arg(monster.remaining_xp)
                        .arg(maxXp)
                        .arg(lastSkill)
                        );
                if(monster.hp==0)
                    item->setBackgroundColor(QColor(255,220,220,255));
                else if(index==selectedMonster)
                {
                    if(player.fightEngine->isInFight())
                        item->setBackgroundColor(QColor(200,255,200,255));
                    else
                        item->setBackgroundColor(QColor(200,255,255,255));
                }
                ui->monsterList->addItem(item);
            }
            index++;
        }
    }

    if(player.fightEngine->isInFight())
    {
        CatchChallenger::PublicPlayerMonster *othermonster=player.fightEngine->getOtherMonster();
        if(othermonster==NULL)
            ui->groupBoxFight->setVisible(false);
        else
        {
            ui->groupBoxFight->setVisible(true);
            const CatchChallenger::Monster::Stat &wildMonsterStat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(othermonster->monster),othermonster->level);
            ui->fightOtherHp->setMaximum(wildMonsterStat.hp);
            ui->fightOtherHp->setValue(othermonster->hp);
            ui->fightOtherHp->setFormat(QString("%1/%2").arg(othermonster->hp).arg(wildMonsterStat.hp));
            const DatapackClientLoader::MonsterExtra &monsterExtra=DatapackClientLoader::datapackLoader.monsterExtra.at(othermonster->monster);
            ui->monsterFightName->setText(QString("%1 level %2").arg(QString::fromStdString(monsterExtra.name)).arg(othermonster->level));
            ui->monsterFightImage->setPixmap(monsterExtra.front);
        }
    }
    else
        ui->groupBoxFight->setVisible(false);
}

void BotTargetList::teleportTo()
{
    CatchChallenger::Api_protocol * apiSelectedClient=NULL;
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()==1)
    {
        const QString &pseudo=selectedItems.at(0)->text();
        if(!pseudoToBot.contains(pseudo))
            return;
        MultipleBotConnection::CatchChallengerClient * currentSelectedclient=pseudoToBot.value(pseudo);
        apiSelectedClient=currentSelectedclient->api;
    }
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());

    ActionsBotInterface::Player &player=actionsAction->clientList[api];
    if(api==NULL)
        return;
    std::cout << player.api->getPseudo() << ": localStep: " << BotTargetList::stepToString(player.target.localStep)
              << " from " << actionsAction->id_map_to_map.at(player.mapId) << " " << std::to_string(player.x) << "," << std::to_string(player.y)
              << ", " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;

    //player reset local step
    player.target.bestPath.clear();
    player.target.localStep.clear();
    player.canMoveOnMap=true;
    player.target.extra=0;
    player.target.linkPoint.x=0;
    player.target.linkPoint.y=0;
    player.target.linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
    player.target.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::None;
    player.target.wildCycle=0;

    if(api==apiSelectedClient)
    {
        updatePlayerInformation();
        updatePlayerMap(true);
    }
}

void BotTargetList::monsterCatch(const bool &success)
{
    Q_UNUSED(success);
    CatchChallenger::Api_protocol * apiSelectedClient=NULL;
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()==1)
    {
        const QString &pseudo=selectedItems.at(0)->text();
        if(!pseudoToBot.contains(pseudo))
            return;
        MultipleBotConnection::CatchChallengerClient * currentSelectedclient=pseudoToBot.value(pseudo);
        apiSelectedClient=currentSelectedclient->api;
    }
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(api==NULL)
        return;
    if(api==apiSelectedClient)
    {
        updatePlayerInformation();
    }
}
