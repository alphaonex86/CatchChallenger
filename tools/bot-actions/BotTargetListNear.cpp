#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "MapBrowse.h"

std::string BotTargetList::graphStepNearMap(const MultipleBotConnection::CatchChallengerClient * const client,const MapServerMini::BlockObject * const currentNearBlock,const unsigned int &depth)
{
    const std::unordered_set<const MapServerMini *> &validMaps=currentNearBlock->map->getValidMaps(depth);
    const std::unordered_set<const MapServerMini::BlockObject *> &accessibleBlock=currentNearBlock->map->getAccessibleBlock(validMaps,currentNearBlock);
    std::unordered_map<const MapServerMini::BlockObject *,MapServerMini::BlockObjectPathFinding> resolvedBlock;
    currentNearBlock->map->resolvBlockPath(currentNearBlock,resolvedBlock,accessibleBlock);

    // do the GraphViz content
    {
        std::string overall_graphvizText="";
        overall_graphvizText+="digraph G {\n";
        overall_graphvizText+="rankdir=LR\n";
        overall_graphvizText+="node [shape=record]\n";

        std::unordered_set<const MapServerMini::BlockObject *> destinationMaps;
        for(const auto& n : ActionsAction::actionsAction->map_list) {
            const MapServerMini * const mapServer=static_cast<MapServerMini *>(n.second);
            if(mapServer->step.size()>=2)
            {
                const MapServerMini::MapParsedForBot &step2=mapServer->step[1];
                if(step2.map!=NULL)
                {
                    unsigned int indexLayer=0;
                    while(indexLayer<step2.layers.size())
                    {
                        const MapServerMini::MapParsedForBot::Layer &layer=step2.layers.at(indexLayer);
                        const MapServerMini::BlockObject &block=*layer.blockObject;
                        for(const auto& n:block.links) {
                            const MapServerMini::BlockObject * const nextBlock=n.first;
                            if(validMaps.find(mapServer)!=validMaps.cend() && validMaps.find(nextBlock->map)!=validMaps.cend())
                                destinationMaps.insert(nextBlock);
                        }
                        indexLayer++;
                    }
                }
            }
        }

        std::string stringLinks;
        for(const auto& n : ActionsAction::actionsAction->map_list) {
            const MapServerMini * const mapServer=static_cast<MapServerMini *>(n.second);
            if(mapServer->step.size()>=2 && validMaps.find(mapServer)!=validMaps.cend())
            {
                const MapServerMini::MapParsedForBot &step2=mapServer->step[1];
                if(step2.map!=NULL)
                {
                    unsigned int contentDisplayed=0;
                    std::string subgraph;
                    subgraph+="subgraph cluster_"+std::to_string((uint64_t)mapServer)+" {\n";
                    subgraph+="label=\""+mapServer->map_file+"\";\n";
                    unsigned int indexLayer=0;
                    while(indexLayer<step2.layers.size())
                    {
                        const MapServerMini::MapParsedForBot::Layer &layer=step2.layers.at(indexLayer);
                        const MapServerMini::BlockObject &block=*layer.blockObject;

                        /*unsigned int index=0;
                        bool contentEmpty=true;
                        while(index<layer.contentList.size())
                        {
                            const MapServerMini::MapParsedForBot::Layer::Content &itemEntry=layer.contentList.at(index);
                            if(itemEntry.destinationDisplay==MapParsedForBot::Layer::DestinationDisplay::All)
                            {
                                contentEmpty=false;
                                break;
                            }
                            index++;
                        }
                        (void)contentEmpty;*/

                        bool haveValidDestination=false;
                        for(const auto& n:block.links) {
                            const MapServerMini::BlockObject * const nextBlock=n.first;
                            if(destinationMaps.find(nextBlock)!=destinationMaps.cend())
                            {
                                haveValidDestination=true;
                                break;
                            }
                        }

                        //layer.name!="Lost layer" || !contentEmpty ||
                        if(haveValidDestination || destinationMaps.find(&block)!=destinationMaps.cend())
                        {
                            if(accessibleBlock.find(&block)!=accessibleBlock.cend())
                            {
                                const bool &zoneIsAccessible=nextZoneIsAccessible(client->api,&block);
                                subgraph+="struct"+std::to_string((uint64_t)&block)+" [label=\"";
                                //subgraph+="<f0> "+mapServer->map_file+" | "+layer.name+" |";
                                subgraph+="<f0> "+layer.name;
                                if(!zoneIsAccessible)
                                    subgraph+=" (Not accessible)";
                                /*unsigned int index=0;
                                while(index<layer.contentList.size())
                                {
                                    const MapServerMini::MapParsedForBot::Layer::Content &itemEntry=layer.contentList.at(index);
                                    if(itemEntry.destinationDisplay==MapParsedForBot::Layer::DestinationDisplay::All)
                                        subgraph+=itemEntry.text.toStdString()+"\\n";
                                    index++;
                                }*/
                                std::vector<std::string> stringList=contentToGUI(&block,client->api);
                                const std::string &content=stringimplode(stringList,"\\n");
                                if(!content.empty())
                                    subgraph+="|<f1> "+content;
                                {
                                    const MapServerMini::BlockObjectPathFinding &blockObjectPathFinding=resolvedBlock.at(&block);
                                    if(!blockObjectPathFinding.bestPath.empty())
                                    {
                                        subgraph+="|";
                                        unsigned int index=0;
                                        while(index<blockObjectPathFinding.bestPath.size())
                                        {
                                            const MapServerMini::BlockObject * const block=blockObjectPathFinding.bestPath.at(index);
                                            subgraph+="Block "+std::to_string(block->id+1)+"("+block->map->map_file+")\\n";
                                            index++;
                                        }
                                    }
                                }
                                subgraph+="\" style=filled fillcolor=\"";
                                if(zoneIsAccessible)
                                    subgraph+=block.color.name(QColor::HexRgb).toStdString();
                                else
                                    subgraph+="red";
                                subgraph+="\"";
                                if(!zoneIsAccessible)
                                    subgraph+=" fontcolor=\"#999999\"";
                                subgraph+="]\n";
                                contentDisplayed++;
                            }
                        }

                        for(const auto& n:block.links) {
                            const MapServerMini::BlockObject * const nextBlock=n.first;
                            const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
                            std::vector<CatchChallenger::MapCondition> uniqueCondition;
                            const std::vector<MapServerMini::BlockObject::LinkCondition> &linkConditions=linkInformation.linkConditions;
                            unsigned int indexCondition=0;
                            while(indexCondition<linkConditions.size())
                            {
                                const MapServerMini::BlockObject::LinkCondition &condition=linkConditions.at(indexCondition);
                                unsigned int index=0;
                                while(index<condition.points.size())
                                {
                                    //const MapServerMini::BlockObject::LinkPoint &point=condition.points.at(index);
                                    unsigned int search=0;
                                    while(search<uniqueCondition.size())
                                    {
                                        if(uniqueCondition.at(search)==condition.condition)
                                            break;
                                        search++;
                                    }
                                    if(search>=uniqueCondition.size())
                                        uniqueCondition.push_back(condition.condition);
                                    index++;
                                }
                                indexCondition++;
                            }
                            unsigned int conditionIndex=0;
                            while(conditionIndex<uniqueCondition.size())
                            {
                                bool directionBoth=false;
                                const CatchChallenger::MapCondition &condition=uniqueCondition.at(conditionIndex);
                                if(condition.type==CatchChallenger::MapConditionType_None)
                                    directionBoth=MapServerMini::haveDirectReturnWithoutCondition(block,*nextBlock);
                                if(!directionBoth || &block<=nextBlock)
                                {
                                    if(validMaps.find(block.map)!=validMaps.cend() && validMaps.find(nextBlock->map)!=validMaps.cend())
                                        if(accessibleBlock.find(&block)!=accessibleBlock.cend() && accessibleBlock.find(nextBlock)!=accessibleBlock.cend())
                                        {
                                            contentDisplayed++;
                                            stringLinks+="struct"+std::to_string((uint64_t)&block)+" -> struct"+std::to_string((uint64_t)nextBlock);
                                            std::vector<std::string> attrs;
                                            if(directionBoth)
                                                attrs.push_back("dir=both");
                                            if(condition.type!=CatchChallenger::MapConditionType_None)
                                            {
                                                attrs.push_back("style=\"dashed\"");
                                                if(ActionsAction::mapConditionIsRepected(client->api,condition))
                                                     attrs.push_back("color=\"green\"");
                                                else
                                                    attrs.push_back("color=\"red\"");
                                                switch(condition.type)
                                                {
                                                    case CatchChallenger::MapConditionType_Clan:
                                                        attrs.push_back("label=\"Need be owner clan\"");
                                                    break;
                                                    case CatchChallenger::MapConditionType_Quest:
                                                        attrs.push_back("label=\"Quest "+std::to_string(condition.data.quest)+"\"");
                                                    break;
                                                    case CatchChallenger::MapConditionType_Item:
                                                        attrs.push_back("label=\"Item "+std::to_string(condition.data.item)+"\"");
                                                    break;
                                                    case CatchChallenger::MapConditionType_FightBot:
                                                        attrs.push_back("label=\"Fight "+std::to_string(condition.data.fightBot)+"\"");
                                                    break;
                                                    default:
                                                    break;
                                                }
                                            }
                                            if(!attrs.empty())
                                            {
                                                stringLinks+=" [";
                                                stringLinks+=stringimplode(attrs," ");
                                                stringLinks+="]";
                                            }
                                            stringLinks+=";\n";
                                        }
                                }
                                conditionIndex++;
                            }
                        }

                        indexLayer++;
                    }
                    subgraph+="}\n";
                    if(contentDisplayed>0)
                        overall_graphvizText+=subgraph;
                }
            }
        }

        overall_graphvizText+=stringLinks;
        overall_graphvizText+="}";
        return overall_graphvizText;
    }
    return "";
}

std::string BotTargetList::graphLocalMap()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return "";
    QListWidgetItem* selectedItem=selectedItems.at(0);
    const QString &pseudo=selectedItem->text();
    if(!pseudoToBot.contains(pseudo))
        return "";
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return "";
    if(ui->comboBox_Layer->count()==0)
        return "";

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);
    QString mapString="Unknown map ("+QString::number(mapId)+")";
    if(actionsAction->id_map_to_map.find(mapId)==actionsAction->id_map_to_map.cend())
        return "";
    const std::string &mapStdString=actionsAction->id_map_to_map.at(mapId);
    mapString=QString::fromStdString(mapStdString)+QString(" (%1,%2)").arg(player.x).arg(player.y);
    CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
    MapServerMini *mapServer=static_cast<MapServerMini *>(map);
    if((uint32_t)ui->comboBoxStep->currentIndex()>=mapServer->step.size())
        return "";
    MapServerMini::MapParsedForBot &step2=mapServer->step.at(ui->comboBoxStep->currentIndex());
    if(step2.map==NULL)
        return "";

    std::string graphvizText;
    // do the GraphViz content
    {
        graphvizText+="digraph G {\n";
        graphvizText+="rankdir=LR\n";
        graphvizText+="node [shape=record]\n";

        // the block
        std::unordered_map<const MapServerMini::BlockObject *,unsigned int> pointerToIndex;
        {
            unsigned int blockIndex=0;
            while(blockIndex<step2.layers.size())
            {
                const MapServerMini::MapParsedForBot::Layer &layer=step2.layers.at(blockIndex);
                const MapServerMini::BlockObject &block=*layer.blockObject;

                std::vector<std::string> stringList=contentToGUI(&block,client->api);
                const std::string &content=stringimplode(stringList,"\\n");

                /*unsigned int index=0;
                bool contentEmpty=true;
                while(index<layer.contentList.size())
                {
                    const MapServerMini::MapParsedForBot::Layer::Content &itemEntry=layer.contentList.at(index);
                    if(itemEntry.destinationDisplay==MapServerMini::MapParsedForBot::Layer::DestinationDisplay::All)
                    {
                        contentEmpty=false;
                        break;
                    }
                    index++;
                }*/

                if(layer.name!="Lost layer" || !content.empty())
                {
                    graphvizText+="struct"+std::to_string(blockIndex+1)+" [label=\"";
                    graphvizText+="<f0> "+layer.name;
                    //graphvizText+="<f1> "+layer.text;

                    if(!content.empty())
                    {
                        graphvizText+="|<f1> ";
                        graphvizText+=content;
                    }
                    graphvizText+="\" style=filled fillcolor=\""+block.color.name(QColor::HexRgb).toStdString()+"\"]\n";
                }
                pointerToIndex[&block]=blockIndex;
                blockIndex++;
            }
        }
        //the link
        {
            unsigned int blockIndex=0;
            while(blockIndex<step2.layers.size())
            {
                MapServerMini::BlockObject &block=*step2.layers[blockIndex].blockObject;

                for(const auto& n:block.links) {
                    const MapServerMini::BlockObject * const nextBlock=n.first;
                    const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
                    std::vector<CatchChallenger::MapCondition> uniqueCondition;
                    const std::vector<MapServerMini::BlockObject::LinkCondition> &linkConditions=linkInformation.linkConditions;
                    unsigned int indexCondition=0;
                    while(indexCondition<linkConditions.size())
                    {
                        const MapServerMini::BlockObject::LinkCondition &condition=linkConditions.at(indexCondition);
                        unsigned int index=0;
                        while(index<condition.points.size())
                        {
                            //const MapServerMini::BlockObject::LinkPoint &point=condition.points.at(index);
                            unsigned int search=0;
                            while(search<uniqueCondition.size())
                            {
                                if(uniqueCondition.at(search)==condition.condition)
                                    break;
                                search++;
                            }
                            if(search>=uniqueCondition.size())
                                uniqueCondition.push_back(condition.condition);
                            index++;
                        }
                        indexCondition++;
                    }
                    unsigned int conditionIndex=0;
                    while(conditionIndex<uniqueCondition.size())
                    {
                        bool directionBoth=false;
                        const CatchChallenger::MapCondition &condition=uniqueCondition.at(conditionIndex);
                        if(condition.type==CatchChallenger::MapConditionType_None)
                            directionBoth=MapServerMini::haveDirectReturnWithoutCondition(block,*nextBlock);
                        if(!directionBoth || &block<=nextBlock || block.map!=nextBlock->map)
                        {
                            if(pointerToIndex.find(nextBlock)!=pointerToIndex.cend())
                                graphvizText+="struct"+std::to_string(blockIndex+1)+" -> struct"+std::to_string(pointerToIndex.at(nextBlock)+1);
                            else
                                graphvizText+="struct"+std::to_string(blockIndex+1)+" -> \""+nextBlock->map->map_file+", block "+std::to_string(nextBlock->id+1)+"\"";
                            std::vector<std::string> attrs;
                            if(directionBoth)
                                attrs.push_back("dir=both");
                            if(condition.type!=CatchChallenger::MapConditionType_None)
                            {
                                attrs.push_back("style=\"dashed\"");
                                if(ActionsAction::mapConditionIsRepected(client->api,condition))
                                     attrs.push_back("color=\"green\"");
                                else
                                    attrs.push_back("color=\"red\"");
                                switch(condition.type)
                                {
                                    case CatchChallenger::MapConditionType_Clan:
                                        attrs.push_back("label=\"Need be owner clan\"");
                                    break;
                                    case CatchChallenger::MapConditionType_Quest:
                                        attrs.push_back("label=\"Quest "+std::to_string(condition.data.quest)+"\"");
                                    break;
                                    case CatchChallenger::MapConditionType_Item:
                                        attrs.push_back("label=\"Item "+std::to_string(condition.data.item)+"\"");
                                    break;
                                    case CatchChallenger::MapConditionType_FightBot:
                                        attrs.push_back("label=\"Fight "+std::to_string(condition.data.fightBot)+"\"");
                                    break;
                                    default:
                                    break;
                                }
                            }
                            if(!attrs.empty())
                            {
                                graphvizText+=" [";
                                graphvizText+=stringimplode(attrs," ");
                                graphvizText+="]";
                            }
                            graphvizText+=";\n";
                        }
                        conditionIndex++;
                    }
                }
//                    const std::vector<BlockObject::LinkSource> &sources=linkInformation.sources;
                    /*if(linkInformation.direction!=MapServerMini::BlockObject::LinkDirection::BothDirection || &block<=nextBlock || block.map!=nextBlock->map)
                    {

                        switch(linkInformation.direction)
                        {
                            case MapServerMini::BlockObject::LinkDirection::BothDirection:
                                graphvizText+=" [dir=both];\n";
                            break;
                            default:
                                graphvizText+=";\n";
                            break;
                        }
                    }
                }*/

                blockIndex++;
            }
        }

        //graphvizText+="struct2 -> struct3 [dir=both];\n";

        graphvizText+="}";
    }
    return graphvizText;
}

bool operator==(const CatchChallenger::MapCondition& lhs, const CatchChallenger::MapCondition& rhs)
{
    return std::memcmp(&lhs,&rhs,sizeof(CatchChallenger::MapCondition))==0;
}

bool BotTargetList::nextZoneIsAccessible(const CatchChallenger::Api_protocol *api,const MapServerMini::BlockObject * const blockObject)
{
    const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations_ro();
    uint32_t maxMonsterLevel=0;
    {
        unsigned int index=0;
        while(index<player_private_and_public_informations.playerMonster.size())
        {
            const CatchChallenger::PlayerMonster &playerMonster=player_private_and_public_informations.playerMonster.at(index);
            if(playerMonster.level>maxMonsterLevel)
                maxMonsterLevel=playerMonster.level;
            index++;
        }
    }

    for(auto it = blockObject->botsFight.begin();it!=blockObject->botsFight.cend();++it)
    {
        const std::vector<uint16_t> &botsFightList=it->second;
        unsigned int index=0;
        while(index<botsFightList.size())
        {
            const uint32_t &fightId=botsFightList.at(index);
            const CatchChallenger::BotFight &fight=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(fightId);
            uint8_t maxFightLevel=0;
            {
                unsigned int sub_index=0;
                while(sub_index<fight.monsters.size())
                {
                    const CatchChallenger::BotFight::BotFightMonster &monster=fight.monsters.at(sub_index);
                    if(monster.level>maxFightLevel)
                        maxFightLevel=monster.level;
                    sub_index++;
                }
            }
            const bool tooHard=maxFightLevel>(maxMonsterLevel+2);
            if(tooHard)
                return false;
            index++;
        }
    }

    if(blockObject->monstersCollisionValue!=NULL)
    {
        const CatchChallenger::MonstersCollisionValue &monsterCollisionValue=*blockObject->monstersCollisionValue;
        if(!monsterCollisionValue.walkOnMonsters.empty())
        {
            /// \todo do the real code
            const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monsterCollisionContent=monsterCollisionValue.walkOnMonsters.at(0);
            if(!monsterCollisionContent.defaultMonsters.empty())
            {
                uint8_t maxFightLevel=0;
                unsigned int sub_index=0;
                while(sub_index<monsterCollisionContent.defaultMonsters.size())
                {
                    const CatchChallenger::MapMonster &monster=monsterCollisionContent.defaultMonsters.at(sub_index);
                    if(monster.maxLevel>maxFightLevel)
                        maxFightLevel=monster.maxLevel;
                    sub_index++;
                }

                const bool tooHard=maxFightLevel>(maxMonsterLevel+2);
                if(tooHard)
                    return false;
            }
        }
    }

    return true;
}
