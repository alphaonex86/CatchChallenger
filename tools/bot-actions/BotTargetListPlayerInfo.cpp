#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "../../general/base/CommonSettingsServer.h"
#include "MapBrowse.h"

#include <chrono>
#include <QMessageBox>

void BotTargetList::updatePlayerInformation()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);

    if(actionsAction->id_map_to_map.find(player.mapId)!=actionsAction->id_map_to_map.cend())
    {
        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        QString mapString=QString::fromStdString(playerMap->map_file)+QString(" (%1,%2)").arg(player.x).arg(player.y);
        ui->label_local_target->setTitle("Target on the map: "+mapString);
        if(playerMap->step.size()<2)
            abort();
        const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
        const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
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
                playerMap->targetBlockList(layer.blockObject,resolvedBlock,ui->searchDeep->value());
                const MapServerMini::MapParsedForBot &step=playerMap->step.at(ui->comboBoxStep->currentIndex());
                if(step.map==NULL)
                    return;

                ui->globalTargets->clear();
                targetListGlobalTarget.clear();
                alternateColor=false;
                contentToGUI(client,ui->globalTargets,resolvedBlock,!ui->tooHard->isChecked());
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
                            const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
                            if(tempNextBlock==nextBlock)
                            {
                                const MapServerMini::BlockObject::LinkPoint &firstPoint=linkInformation.points.at(0);
                                ui->label_next_local_target->setText("Next local target: "+QString::fromStdString(nextLlayer->name)+" on "+QString::fromStdString(nextBlock->map->map_file)+", go to "+
                                                                     QString::number(firstPoint.x)+","+QString::number(firstPoint.y)+
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
        const ActionsBotInterface::Player &bot=actionsAction->clientList.value(client->api);
        for(const auto& n:player_private_and_public_informations.items) {
            const uint32_t &itemId=n.first;
            const uint32_t &quantity=n.second;
            QListWidgetItem *item=new QListWidgetItem();
            if(DatapackClientLoader::datapackLoader.itemsExtra.contains(itemId))
            {
                item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.value(itemId).image);
                if(quantity>1)
                    item->setText(QString::number(quantity));
                item->setText(item->text()+" "+DatapackClientLoader::datapackLoader.itemsExtra.value(itemId).name);
                item->setToolTip(DatapackClientLoader::datapackLoader.itemsExtra.value(itemId).name);
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

    {
        const std::vector<CatchChallenger::PlayerMonster> &playerMonsters=player.fightEngine->getPlayerMonster();
        ui->monsterList->clear();
        if(playerMonsters.empty())
            return;
        unsigned int index=0;
        while(index<playerMonsters.size())
        {
            const CatchChallenger::PlayerMonster &monster=playerMonsters.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
            {
                CatchChallenger::Monster::Stat stat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster),monster.level);

                QListWidgetItem *item=new QListWidgetItem();
                item->setToolTip(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).description);
                if(!DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).thumb.isNull())
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).thumb);
                else
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).front);

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
                        item->setText(tr("%1, level: %2\nHP: %3/%4\n%5")
                                .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).name)
                                .arg(monster.level)
                                .arg(monster.hp)
                                .arg(stat.hp)
                                .arg(tr("No skill to learn"))
                                );
                    else
                        item->setText(tr("%1, level: %2\nHP: %3/%4\n%5")
                                .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).name)
                                .arg(monster.level)
                                .arg(monster.hp)
                                .arg(stat.hp)
                                .arg(tr("%n skill(s) to learn","",skillToDisplay.size()))
                                );
                }
                ui->monsterList->addItem(item);
            }
            index++;
        }
    }
}
