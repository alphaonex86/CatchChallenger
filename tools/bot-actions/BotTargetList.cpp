#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "MapBrowse.h"

#include <chrono>

BotTargetList::BotTargetList(QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient,
                             QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient,
                             QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient,
                             ActionsAction *actionsAction) :
    ui(new Ui::BotTargetList),
    apiToCatchChallengerClient(apiToCatchChallengerClient),
    connectedSocketToCatchChallengerClient(connectedSocketToCatchChallengerClient),
    sslSocketToCatchChallengerClient(sslSocketToCatchChallengerClient),
    actionsAction(actionsAction),
    botsInformationLoaded(false)
{
    ui->setupUi(this);
    ui->comboBoxStep->setCurrentIndex(1);
    ui->graphvizText->setVisible(false);
    mapId=0;

    QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *>::const_iterator i = apiToCatchChallengerClient.constBegin();
    while (i != apiToCatchChallengerClient.constEnd()) {
        MultipleBotConnection::CatchChallengerClient * const client=i.value();
        if(client->have_informations)
        {
            const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=client->api->get_player_informations();
            const QString &qtpseudo=QString::fromStdString(player_private_and_public_informations.public_informations.pseudo);
            ui->bots->addItem(qtpseudo);
            pseudoToBot[qtpseudo]=client;
        }
        ++i;
    }

    MapServerMini::colorsList << QColor(255, 255, 0, 255);
    MapServerMini::colorsList << QColor(70, 187, 70, 255);
    MapServerMini::colorsList << QColor(100, 100, 200, 255);
    MapServerMini::colorsList << QColor(255, 128, 128, 255);
    MapServerMini::colorsList << QColor(180, 70, 180, 255);
    MapServerMini::colorsList << QColor(255, 200, 110, 255);
    MapServerMini::colorsList << QColor(115, 255, 240, 255);
    MapServerMini::colorsList << QColor(115, 255, 120, 255);
    MapServerMini::colorsList << QColor(200, 70, 70, 255);

    connect(&actionsAction->moveTimer,&QTimer::timeout,this,&BotTargetList::updatePlayerStep);
    connect(&actionsAction->moveTimer,&QTimer::timeout,this,&BotTargetList::updatePlayerMap);
}

BotTargetList::~BotTargetList()
{
    delete ui;
}

void BotTargetList::loadAllBotsInformation()
{
    if(botsInformationLoaded)
        return;
    botsInformationLoaded=true;

    /*if(!connect(actionsAction,&ActionsAction::preload_the_map_finished,this,&BotTargetList::loadAllBotsInformation2))
        abort();
    if(!connect(this,&BotTargetList::start_preload_the_map,actionsAction,&ActionsAction::preload_the_map))
        abort();
    emit start_preload_the_map();
    waitScreen.show();
    waitScreen.updateWaitScreen();*/
    actionsAction->preload_the_map();
    loadAllBotsInformation2();
}

void BotTargetList::loadAllBotsInformation2()
{
    if(!actionsAction->preload_the_map_step2())
        abort();
    show();
    //waitScreen.hide();
}

void BotTargetList::on_bots_itemSelectionChanged()
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

    ui->label_local_target->setEnabled(true);
    ui->comboBoxStep->setEnabled(true);
    ui->globalTargets->setEnabled(true);
    ui->browseMap->setEnabled(true);
    ui->groupBoxPlayer->setEnabled(true);

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);
    mapId=player.mapId;

    updateMapInformation();
    updatePlayerInformation();
}

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
                    const std::pair<uint8_t,uint8_t> &point=getNextPosition(layer.blockObject,player.target);
                    const uint8_t x=point.first,y=point.second;
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
        for(const auto& n:bot.items) {
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
        const std::vector<CatchChallenger::PlayerMonster> &playerMonsters=player_private_and_public_informations.playerMonster;
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

void BotTargetList::updatePlayerMap()
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

    if(player.mapId==mapId)
        updateMapContent();
}

void BotTargetList::updatePlayerStep()
{
    QHashIterator<CatchChallenger::Api_protocol *,ActionsAction::Player> i(actionsAction->clientList);
    while (i.hasNext()) {
        i.next();
        CatchChallenger::Api_protocol *api=i.key();
        ActionsAction::Player &player=actionsAction->clientList[i.key()];
        if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
            abort();
        if(api->getCaracterSelected())
        {
            if(player.target.localStep.empty())
            {
                if(!player.target.bestPath.empty())
                {
                    switch(player.target.localType)
                    {
                        case MapServerMini::BlockObject::LinkType::SourceNone:
                        break;
                        default:
                        break;
                    }
                    player.target.bestPath.erase(player.target.bestPath.cbegin());
                    uint8_t o=player.direction;
                    while(o>4)
                        o-=4;

                    if(!player.target.bestPath.empty())
                    {
                        const MapServerMini::BlockObject * const blockObject=player.target.bestPath.at(0);
                        const std::pair<uint8_t,uint8_t> &point=getNextPosition(blockObject,player.target/*hop list, first is the next hop*/);

                        const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                                    blockObject,
                                    static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                                    CatchChallenger::Orientation::Orientation_none,point.first,point.second
                                    );
                        player.target.localStep=returnPath;
                        const MapServerMini::BlockObject * const nextBlock=player.target.bestPath.at(0);
                        //search the next position
                        for(const auto& n:blockObject->links) {
                            const MapServerMini::BlockObject * const tempNextBlock=n.first;
                            const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
                            if(tempNextBlock==nextBlock)
                            {
                                const MapServerMini::BlockObject::LinkPoint &firstPoint=linkInformation.points.at(0);
                                player.target.localType=firstPoint.type;
                                break;
                            }
                        }
                    }
                    else
                    {
                        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
                        const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
                        if(playerMap->step.size()<2)
                            abort();
                        const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
                        const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
                        const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);
                        const MapServerMini::BlockObject * const blockObject=layer.blockObject;
                        switch(player.target.type)
                        {
                            case ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal:
                                for(const auto& n:blockObject->heal) {
                                    const std::pair<uint8_t,uint8_t> &point=n;
                                    const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                                                blockObject,
                                                static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                                                CatchChallenger::Orientation::Orientation_none,point.first,point.second
                                                );
                                    player.target.localStep=returnPath;
                                    player.target.localType=MapServerMini::BlockObject::LinkType::SourceNone;
                                }
                            break;
                            default:
                            break;
                        }
                    }
                }
                else
                {
                    //finish correctly the step
                    switch(player.target.type)
                    {
                        case ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal:
                            api->heal();
                        break;
                        default:
                        break;
                    }
                }
            }
        }
    }
}

void BotTargetList::startPlayerMove()
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

    ActionsBotInterface::Player &player=actionsAction->clientList[client->api];

    if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
        return;
    const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
    const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
    QString mapString=QString::fromStdString(playerMap->map_file)+QString(" (%1,%2)").arg(player.x).arg(player.y);
    ui->label_local_target->setTitle("Target on the map: "+mapString);
    if(playerMap->step.size()<2)
        abort();
    const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
    const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
    if(playerCodeZone<=0 || (uint32_t)(playerCodeZone-1)>=(uint32_t)stepPlayer.layers.size())
        return;
    const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);
    std::unordered_map<const MapServerMini::BlockObject *,MapServerMini::BlockObjectPathFinding> resolvedBlock;
    playerMap->targetBlockList(layer.blockObject,resolvedBlock,ui->searchDeep->value());
    const MapServerMini::MapParsedForBot &step=playerMap->step.at(ui->comboBoxStep->currentIndex());
    if(step.map==NULL)
        return;

    const std::pair<uint8_t,uint8_t> &point=getNextPosition(layer.blockObject,player.target/*hop list, first is the next hop*/);
    uint8_t o=player.direction;
    while(o>4)
        o-=4;
    const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                layer.blockObject,
                static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                CatchChallenger::Orientation::Orientation_none,point.first,point.second
                );
    player.target.localStep=returnPath;
    player.target.localType=MapServerMini::BlockObject::LinkType::SourceNone;
    if(!player.target.bestPath.empty())
    {
        const MapServerMini::BlockObject * const nextBlock=player.target.bestPath.at(0);
        //search the next position
        for(const auto& n:layer.blockObject->links) {
            const MapServerMini::BlockObject * const tempNextBlock=n.first;
            const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
            if(tempNextBlock==nextBlock)
            {
                const MapServerMini::BlockObject::LinkPoint &firstPoint=linkInformation.points.at(0);
                player.target.localType=firstPoint.type;
                break;
            }
        }
    }

    ui->label_action->setText("Start this: "+QString::fromStdString(BotTargetList::stepToString(returnPath)));
    updateMapInformation();
}

std::string BotTargetList::stepToString(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath)
{
    std::string stepToDo;
    unsigned int index=0;
    while(index<returnPath.size())
    {
        const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &entry=returnPath.at(index);
        if(!stepToDo.empty())
            stepToDo+=", ";
        stepToDo+=std::to_string(entry.second);
        switch(entry.first)
        {
            case CatchChallenger::Orientation::Orientation_bottom:
                stepToDo+=" bottom";
            break;
            case CatchChallenger::Orientation::Orientation_top:
                stepToDo+=" top";
            break;
            case CatchChallenger::Orientation::Orientation_left:
                stepToDo+=" left";
            break;
            case CatchChallenger::Orientation::Orientation_right:
                stepToDo+=" right";
            break;
            default:
                abort();
            break;
        }

        index++;
    }
    return stepToDo;
}

void BotTargetList::updateMapInformation()
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

    if(actionsAction->id_map_to_map.find(mapId)!=actionsAction->id_map_to_map.cend())
    {
        const std::string &mapStdString=actionsAction->id_map_to_map.at(mapId);
        CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
        MapServerMini *mapServer=static_cast<MapServerMini *>(map);
        if((uint32_t)ui->comboBoxStep->currentIndex()>=mapServer->step.size())
            return;
        MapServerMini::MapParsedForBot &step=mapServer->step.at(ui->comboBoxStep->currentIndex());
        if(step.map==NULL)
            return;
        QString QtGraphvizText=QString::fromStdString(graphLocalMap());

        ui->mapPreview->setColumnCount(0);
        ui->mapPreview->setRowCount(0);
        /*ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);*/
        ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);

        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        if(playerMap->step.size()<2)
            abort();

        ui->comboBox_Layer->clear();
        unsigned int index=0;
        while(index<step.layers.size())
        {
            const MapServerMini::MapParsedForBot::Layer &layer=step.layers.at(index);

            //if(layer.name!="Lost layer" || !layer.contentList.empty())
                ui->comboBox_Layer->addItem(QString::fromStdString(layer.name),index);

            index++;
        }
        if(QtGraphvizText.isEmpty())
            ui->graphvizText->setVisible(false);
        else
        {
            ui->graphvizText->setVisible(true);
            ui->graphvizText->setPlainText(QtGraphvizText);
        }
    }
    updateMapContent();
    updateLayerElements();
}

void BotTargetList::updateMapContent()
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

    if(actionsAction->id_map_to_map.find(mapId)!=actionsAction->id_map_to_map.cend())
    {
        const std::string &mapStdString=actionsAction->id_map_to_map.at(mapId);
        CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
        MapServerMini *mapServer=static_cast<MapServerMini *>(map);
        if((uint32_t)ui->comboBoxStep->currentIndex()>=mapServer->step.size())
            return;
        MapServerMini::MapParsedForBot &step=mapServer->step.at(ui->comboBoxStep->currentIndex());
        if(step.map==NULL)
            return;

        ui->mapPreview->setColumnCount(0);
        ui->mapPreview->setRowCount(0);
        /*ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);*/
        ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);

        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        if(playerMap->step.size()<2)
            abort();
        const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
        const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
        const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);

        {
            int y=mapServer->min_y;
            while(y<mapServer->max_y)
            {
                int x=mapServer->min_x;
                while(x<mapServer->max_x)
                {
                    QTableWidgetItem *tablewidgetitem = new QTableWidgetItem();
                    //color
                    int codeZone=step.map[x+y*mapServer->width];
                    if(codeZone>0)
                    {
                        const MapServerMini::MapParsedForBot::Layer &layer=step.layers.at(codeZone-1);
                        QColor color=layer.blockObject->color;
                        QBrush brush1(color);
                        brush1.setStyle(Qt::SolidPattern);
                        tablewidgetitem->setBackground(brush1);
                        tablewidgetitem->setText(QString::number(codeZone));
                    }
                    //icon
                    if(x==player.x && y==player.y && player.mapId==mapId)
                    {
                        QIcon icon;
                        icon.addFile(QStringLiteral(":/playerloc.png"), QSize(), QIcon::Normal, QIcon::Off);
                        tablewidgetitem->setText("");
                        tablewidgetitem->setIcon(icon);
                    }
                    else if(player.target.blockObject!=NULL)
                    {
                        const std::pair<uint8_t,uint8_t> &point=getNextPosition(layer.blockObject,player.target);
                        if(x==point.first && y==point.second && player.mapId==mapId)
                        {
                            QIcon icon;
                            icon.addFile(QStringLiteral(":/6.png"), QSize(), QIcon::Normal, QIcon::Off);
                            tablewidgetitem->setText("");
                            tablewidgetitem->setIcon(icon);
                        }
                    }
                    ui->mapPreview->setItem(y-mapServer->min_y,x-mapServer->min_x,tablewidgetitem);

                    x++;
                }
                y++;
            }
        }
    }
    else
        ui->label_local_target->setTitle("Unknown map ("+QString::number(mapId)+")");
}

void BotTargetList::updateLayerElements()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    QListWidgetItem* selectedItem=selectedItems.at(0);
    const QString &pseudo=selectedItem->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;
    if(ui->comboBox_Layer->count()==0)
        return;

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);
    QString mapString="Unknown map ("+QString::number(mapId)+")";
    if(actionsAction->id_map_to_map.find(mapId)==actionsAction->id_map_to_map.cend())
        return;
    const std::string &mapStdString=actionsAction->id_map_to_map.at(mapId);
    mapString=QString::fromStdString(mapStdString)+QString(" (%1,%2)").arg(player.x).arg(player.y);
    CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
    MapServerMini *mapServer=static_cast<MapServerMini *>(map);
    if((uint32_t)ui->comboBoxStep->currentIndex()>=mapServer->step.size())
        return;
    MapServerMini::MapParsedForBot &step=mapServer->step.at(ui->comboBoxStep->currentIndex());
    if(step.map==NULL)
        return;
    ui->label_select_map->setText("Displayed map: "+QString::fromStdString(mapStdString));

    const MapServerMini::MapParsedForBot::Layer &layer=step.layers.at(ui->comboBox_Layer->currentIndex());
    alternateColor=false;
    ui->localTargets->clear();
    contentToGUI(layer.blockObject,client,ui->localTargets);

    ui->label_zone->setText(QString::fromStdString(layer.text));
}

void BotTargetList::on_comboBox_Layer_activated(int index)
{
    (void)index;
    updateLayerElements();
}

void BotTargetList::on_localTargets_itemActivated(QListWidgetItem *item)
{
    (void)item;

    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    QListWidgetItem* selectedItem=selectedItems.at(0);
    const QString &pseudo=selectedItem->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;
    if(ui->comboBox_Layer->count()==0)
        return;

    if(ui->localTargets->currentRow()<0)
        return;
    if(ui->localTargets->currentRow()>=(int32_t)mapIdListLocalTarget.size())
        return;
    const uint32_t &newMapId=mapIdListLocalTarget.at(ui->localTargets->currentRow());
    if(mapId!=newMapId)
    {
        mapId=newMapId;
        updateMapInformation();
    }
}

void BotTargetList::on_comboBoxStep_currentIndexChanged(int index)
{
    (void)index;
    updateMapInformation();
}

void BotTargetList::on_browseMap_clicked()
{
    if(!ui->browseMap->isEnabled())
        return;
    std::vector<std::string> mapList;
    for(const auto& n:actionsAction->map_list)
        mapList.push_back(n.first);
    MapBrowse mapBrowse(mapList,this);
    mapBrowse.exec();
    const std::string &selectedMapString=mapBrowse.mapSelected();
    if(selectedMapString.empty())
        return;
    if(actionsAction->map_list.find(selectedMapString)==actionsAction->map_list.cend())
        return;
    const MapServerMini * const mapServer=static_cast<MapServerMini *>(actionsAction->map_list.at(selectedMapString));
    mapId=mapServer->id;
    updateMapInformation();
}

void BotTargetList::on_searchDeep_editingFinished()
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
        if(playerMap->step.size()<2)
            abort();
        const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
        const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
        const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);
        QString overall_graphvizText=QString::fromStdString(graphStepNearMap(client,layer.blockObject,ui->searchDeep->value()));
        if(overall_graphvizText.isEmpty())
            ui->overall_graphvizText->setVisible(false);
        else
        {
            ui->overall_graphvizText->setVisible(true);
            ui->overall_graphvizText->setPlainText(overall_graphvizText);
        }
    }
}

void BotTargetList::on_globalTargets_itemActivated(QListWidgetItem *item)
{
    (void)item;
    const int &currentRow=ui->globalTargets->currentRow();
    if(currentRow==-1)
        return;
    const ActionsBotInterface::GlobalTarget &globalTarget=targetListGlobalTarget.at(currentRow);

    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;
    ActionsBotInterface::Player &player=actionsAction->clientList[client->api];

    if(player.target.blockObject!=NULL || player.target.type!=ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
        return;
    player.target=globalTarget;
    startPlayerMove();
    updatePlayerInformation();
}

void BotTargetList::on_tooHard_clicked()
{
    updatePlayerInformation();
}

std::pair<uint8_t, uint8_t> BotTargetList::getNextPosition(const MapServerMini::BlockObject * const blockObject, const ActionsBotInterface::GlobalTarget &target)
{
    std::pair<uint8_t,uint8_t> point(0,0);
    if(target.blockObject==NULL)
    {
        std::cerr << "error: target.blockObject==NULL into getNextPosition()" << std::endl;
        return point;
    }
    if(target.bestPath.empty())
    {
        switch(target.type)
        {
            case ActionsBotInterface::GlobalTarget::ItemOnMap:
            for(auto it = blockObject->pointOnMap_Item.begin();it!=blockObject->pointOnMap_Item.cend();++it)
            {
                const MapServerMini::ItemOnMap &itemOnMap=it->second;
                if(itemOnMap.item==target.extra)
                    return it->first;
            }
            break;
            case ActionsBotInterface::GlobalTarget::Fight:
                for(auto it = blockObject->botsFight.begin();it!=blockObject->botsFight.cend();++it)
                {
                    const std::vector<uint32_t> &botsFightList=it->second;
                    if(vectorcontainsAtLeastOne(botsFightList,target.extra))
                        return it->first;
                }
            break;
            case ActionsBotInterface::GlobalTarget::Shop:
                for(auto it = blockObject->shops.begin();it!=blockObject->shops.cend();++it)
                {
                    const std::vector<uint32_t> &shops=it->second;
                    if(vectorcontainsAtLeastOne(shops,target.extra))
                        return it->first;
                }
            break;
            case ActionsBotInterface::GlobalTarget::Heal:
                for(auto it = blockObject->heal.begin();it!=blockObject->heal.cend();++it)
                    return *it;
            break;
            case ActionsBotInterface::GlobalTarget::WildMonster:
            abort();//not coded
            break;
            default:
            abort();
            break;
        }
    }
    else
    {
        const MapServerMini::BlockObject * const nextBlock=target.bestPath.at(0);
        if(nextBlock==blockObject)
            abort();
        //search the next position
        for(const auto& n:blockObject->links) {
            const MapServerMini::BlockObject * const tempNextBlock=n.first;
            const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
            if(tempNextBlock==nextBlock)
            {
                const MapServerMini::BlockObject::LinkPoint &firstPoint=linkInformation.points.at(0);
                point.first=firstPoint.x;
                point.second=firstPoint.y;
                return point;
            }
        }
        abort();//path def but next hop not found
    }
    return point;
}

std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > BotTargetList::pathFinding(
        const MapServerMini::BlockObject * const blockObject,
        const CatchChallenger::Orientation &source_orientation,const uint8_t &source_x,const uint8_t &source_y,
        const CatchChallenger::Orientation &destination_orientation,const uint8_t &destination_x,const uint8_t &destination_y)
{
    /// \todo ignore and not optimised:
    (void)source_orientation;
    (void)destination_orientation;
    auto start = std::chrono::high_resolution_clock::now();

    //resolv the path
    std::vector<std::pair<uint8_t,uint8_t> > mapPointToParseList;
    SimplifiedMapForPathFinding simplifiedMap;
    const uint8_t &currentCodeZone=blockObject->id+1;
    if(blockObject->map==NULL)
        abort();
    if(blockObject->map->step.size()<2)
        abort();
    const MapServerMini::MapParsedForBot &step=blockObject->map->step.at(1);
    if(step.map==NULL)
        abort();

    //init the first case
    {
        std::pair<uint8_t,uint8_t> tempPoint;
        tempPoint.first=source_x;
        tempPoint.second=source_y;
        mapPointToParseList.push_back(tempPoint);

        std::pair<uint8_t,uint8_t> coord(source_x,source_y);
        SimplifiedMapForPathFinding::PathToGo &pathToGo=simplifiedMap.pathToGo[coord];
        pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1));
        pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1));
        pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1));
        pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1));
    }

    std::pair<uint8_t,uint8_t> coord;
    while(!mapPointToParseList.empty())
    {
        const std::pair<uint8_t,uint8_t> tempPoint=mapPointToParseList.at(0);
        const uint8_t x=tempPoint.first,y=tempPoint.second;
        mapPointToParseList.erase(mapPointToParseList.begin());
        SimplifiedMapForPathFinding::PathToGo pathToGo;
        if(x==destination_x && y==destination_y)
            std::cout << "final dest" << std::endl;
        //resolv the own point
        int index=0;
        while(index<1)/*2*/
        {
            {
                //if the right case have been parsed
                coord=std::pair<uint8_t,uint8_t>(x+1,y);
                if(simplifiedMap.pathToGo.find(coord)!=simplifiedMap.pathToGo.cend())
                {
                    const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMap.pathToGo.at(coord);
                    if(pathToGo.left.empty() || pathToGo.left.size()>nearPathToGo.left.size())
                    {
                        pathToGo.left=nearPathToGo.left;
                        pathToGo.left.back().second++;
                    }
                    if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.left.size()+1))
                    {
                        pathToGo.top=nearPathToGo.left;
                        pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1));
                    }
                    if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.left.size()+1))
                    {
                        pathToGo.bottom=nearPathToGo.left;
                        pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1));
                    }
                }
                //if the left case have been parsed
                coord=std::pair<uint8_t,uint8_t>(x-1,y);
                if(simplifiedMap.pathToGo.find(coord)!=simplifiedMap.pathToGo.cend())
                {
                    const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMap.pathToGo.at(coord);
                    if(pathToGo.right.empty() || pathToGo.right.size()>nearPathToGo.right.size())
                    {
                        pathToGo.right=nearPathToGo.right;
                        pathToGo.right.back().second++;
                    }
                    if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.right.size()+1))
                    {
                        pathToGo.top=nearPathToGo.right;
                        pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1));
                    }
                    if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.right.size()+1))
                    {
                        pathToGo.bottom=nearPathToGo.right;
                        pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1));
                    }
                }
                //if the top case have been parsed
                coord=std::pair<uint8_t,uint8_t>(x,y+1);
                if(simplifiedMap.pathToGo.find(coord)!=simplifiedMap.pathToGo.cend())
                {
                    const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMap.pathToGo.at(coord);
                    if(pathToGo.top.empty() || pathToGo.top.size()>nearPathToGo.top.size())
                    {
                        pathToGo.top=nearPathToGo.top;
                        pathToGo.top.back().second++;
                    }
                    if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.top.size()+1))
                    {
                        pathToGo.left=nearPathToGo.top;
                        pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1));
                    }
                    if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.top.size()+1))
                    {
                        pathToGo.right=nearPathToGo.top;
                        pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1));
                    }
                }
                //if the bottom case have been parsed
                coord=std::pair<uint8_t,uint8_t>(x,y-1);
                if(simplifiedMap.pathToGo.find(coord)!=simplifiedMap.pathToGo.cend())
                {
                    const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMap.pathToGo.at(coord);
                    if(pathToGo.bottom.empty() || pathToGo.bottom.size()>nearPathToGo.bottom.size())
                    {
                        pathToGo.bottom=nearPathToGo.bottom;
                        pathToGo.bottom.back().second++;
                    }
                    if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.bottom.size()+1))
                    {
                        pathToGo.left=nearPathToGo.bottom;
                        pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1));
                    }
                    if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.bottom.size()+1))
                    {
                        pathToGo.right=nearPathToGo.bottom;
                        pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1));
                    }
                }
            }
            index++;
        }
        coord=std::pair<uint8_t,uint8_t>(x,y);
        if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
        {
            simplifiedMap.pathToGo[coord]=pathToGo;
        }
        if(x==destination_x && y==destination_y)
        {
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > returnedVar;
            if(returnedVar.empty() || pathToGo.bottom.size()<returnedVar.size())
                if(!pathToGo.bottom.empty())
                    returnedVar=pathToGo.bottom;
            if(returnedVar.empty() || pathToGo.top.size()<returnedVar.size())
                if(!pathToGo.top.empty())
                    returnedVar=pathToGo.top;
            if(returnedVar.empty() || pathToGo.right.size()<returnedVar.size())
                if(!pathToGo.right.empty())
                    returnedVar=pathToGo.right;
            if(returnedVar.empty() || pathToGo.left.size()<returnedVar.size())
                if(!pathToGo.left.empty())
                    returnedVar=pathToGo.left;
            if(!returnedVar.empty())
            {
                if(returnedVar.back().second<=1)
                {
                    std::cout << "Bug due for last step" << std::endl;
                    return returnedVar;
                }
                else
                {
                    auto end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> elapsed = end-start;

                    std::cout << "Path result into " <<  (uint32_t)elapsed.count() << "ms" << std::endl;
                    returnedVar.back().second--;
                    return returnedVar;
                }
            }
            else
            {
                returnedVar.clear();
                qDebug() << "Bug due to resolved path is empty";
                return returnedVar;
            }
        }
        //revers resolv
        //add to point to parse
        {
            //if the right case have been parsed
            coord=std::pair<uint8_t,uint8_t>(x+1,y);
            if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
            {
                std::pair<uint8_t,uint8_t> newPoint=tempPoint;
                newPoint.first++;
                if(newPoint.first<blockObject->map->width)
                {
                    const uint8_t &newCodeZone=step.map[newPoint.first+newPoint.second*blockObject->map->width];
                    if(currentCodeZone==newCodeZone || (newPoint.first==destination_x && newPoint.second==destination_y))
                    {
                        if(simplifiedMap.pointQueued.find(newPoint)==simplifiedMap.pointQueued.cend())
                        {
                            simplifiedMap.pointQueued.insert(newPoint);
                            mapPointToParseList.push_back(newPoint);
                        }
                    }
                }
            }
            //if the left case have been parsed
            coord=std::pair<uint8_t,uint8_t>(x-1,y);
            if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
            {
                std::pair<uint8_t,uint8_t> newPoint=tempPoint;
                if(newPoint.first>0)
                {
                    newPoint.first--;
                    const uint8_t &newCodeZone=step.map[newPoint.first+newPoint.second*blockObject->map->width];
                    if(currentCodeZone==newCodeZone || (newPoint.first==destination_x && newPoint.second==destination_y))
                    {
                        if(simplifiedMap.pointQueued.find(newPoint)==simplifiedMap.pointQueued.cend())
                        {
                            simplifiedMap.pointQueued.insert(newPoint);
                            mapPointToParseList.push_back(newPoint);
                        }
                    }
                }
            }
            //if the bottom case have been parsed
            coord=std::pair<uint8_t,uint8_t>(x,y+1);
            if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
            {
                std::pair<uint8_t,uint8_t> newPoint=tempPoint;
                newPoint.second++;
                if(newPoint.second<blockObject->map->height)
                {
                    const uint8_t &newCodeZone=step.map[newPoint.first+newPoint.second*blockObject->map->width];
                    if(currentCodeZone==newCodeZone || (newPoint.first==destination_x && newPoint.second==destination_y))
                    {
                        if(simplifiedMap.pointQueued.find(newPoint)==simplifiedMap.pointQueued.cend())
                        {
                            simplifiedMap.pointQueued.insert(newPoint);
                            mapPointToParseList.push_back(newPoint);
                        }
                    }
                }
            }
            //if the top case have been parsed
            coord=std::pair<uint8_t,uint8_t>(x,y-1);
            if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
            {
                std::pair<uint8_t,uint8_t> newPoint=tempPoint;
                if(newPoint.second>0)
                {
                    newPoint.second--;
                    const uint8_t &newCodeZone=step.map[newPoint.first+newPoint.second*blockObject->map->width];
                    if(currentCodeZone==newCodeZone || (newPoint.first==destination_x && newPoint.second==destination_y))
                    {
                        if(simplifiedMap.pointQueued.find(newPoint)==simplifiedMap.pointQueued.cend())
                        {
                            simplifiedMap.pointQueued.insert(newPoint);
                            mapPointToParseList.push_back(newPoint);
                        }
                    }
                }
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end-start;
    std::cout << "Path not found into " << (uint32_t)elapsed.count() << "ms" << std::endl;
    return std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >();
}
