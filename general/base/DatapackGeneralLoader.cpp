#include "DatapackGeneralLoader.h"
#include "GeneralVariable.h"
#include "CommonDatapack.h"
#include "FacilityLib.h"
#include "FacilityLibGeneral.h"
#include "tinyXML/tinyxml.h"

#include <QFile>
#include <vector>
#include <QFileInfoList>
#include <QDir>
#include <iostream>

using namespace CatchChallenger;

std::vector<Reputation> DatapackGeneralLoader::loadReputation(const std::string &file)
{
    std::regex excludeFilterRegex("[\"']");
    std::regex typeRegex("^[a-z]{1,32}$");
    TiXmlDocument domDocument(file.c_str());
    std::vector<Reputation> reputation;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return reputation;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="reputations")
    {
        std::cerr << "Unable to open the file: " << file << ", \"reputations\" root balise not found for reputation of the xml file" << std::endl;
        return reputation;
    }

    //load the content
    bool ok;
    const TiXmlElement * item = root->FirstChildElement("reputation");
    while(item!=NULL)
    {
        if(item->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            if(item.hasAttribute("type"))
            {
                std::vector<int32_t> point_list_positive,point_list_negative;
                std::vector<std::string> text_positive,text_negative;
                const TiXmlElement * level = item->FirstChildElement("level");
                ok=true;
                while(level!=NULL && ok)
                {
                    if(level->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                    {
                        if(level.hasAttribute("point"))
                        {
                            const int32_t &point=level->Attribute("point").toInt(&ok);
                            std::string text_val;
                            if(ok)
                            {
                                ok=true;
                                bool found=false;
                                unsigned int index=0;
                                if(point>=0)
                                {
                                    while(index<point_list_positive.size())
                                    {
                                        if(point_list_positive.at(index)==point)
                                        {
                                            std::cerr << "Unable to open the file: " << file << ", reputation level with same number of point found!: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                                            found=true;
                                            ok=false;
                                            break;
                                        }
                                        if(point_list_positive.at(index)>point)
                                        {
                                            point_list_positive.insert(point_list_positive.begin()+index,point);
                                            text_positive.insert(text_positive.begin()+index,text_val);
                                            found=true;
                                            break;
                                        }
                                        index++;
                                    }
                                    if(!found)
                                    {
                                        point_list_positive.push_back(point);
                                        text_positive.push_back(text_val);
                                    }
                                }
                                else
                                {
                                    while(index<point_list_negative.size())
                                    {
                                        if(point_list_negative.at(index)==point)
                                        {
                                            std::cerr << "Unable to open the file: " << file << ", reputation level with same number of point found!: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                                            found=true;
                                            ok=false;
                                            break;
                                        }
                                        if(point_list_negative.at(index)<point)
                                        {
                                            point_list_negative.insert(point_list_negative.begin()+index,point);
                                            text_negative.insert(text_negative.begin()+index,text_val);
                                            found=true;
                                            break;
                                        }
                                        index++;
                                    }
                                    if(!found)
                                    {
                                        point_list_negative.push_back(point);
                                        text_negative.push_back(text_val);
                                    }
                                }
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", point is not number: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                        }
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", point attribute not found: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                    level = level->NextSiblingElement("level");
                }
                qSort(point_list_positive);
                qSort(point_list_negative.end(),point_list_negative.begin());
                if(ok)
                    if(point_list_positive.size()<2)
                    {
                        std::cerr << "Unable to open the file: " << file << ", reputation have to few level: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(!vectorcontainsAtLeastOne(point_list_positive,0))
                    {
                        std::cerr << "Unable to open the file: " << file << ", no starting level for the positive: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(!point_list_negative.empty() && !vectorcontainsAtLeastOne(point_list_negative,-1))
                    {
                        //std::cerr << "Unable to open the file: " << file << ", no starting level for the negative, first level need start with -1, fix by change range: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                        std::vector<int32_t> point_list_negative_new;
                        int lastValue=-1;
                        unsigned int index=0;
                        while(index<point_list_negative.size())
                        {
                            point_list_negative_new.push_back(lastValue);
                            lastValue=point_list_negative.at(index);//(1 less to negative value)
                            index++;
                        }
                        point_list_negative=point_list_negative_new;
                    }
                if(ok)
                    if(!std::regex_match(item->Attribute("type"),typeRegex))
                    {
                        std::cerr << "Unable to open the file: " << file << ", the type " << item->Attribute("type") << " don't match wiuth the regex: ^[a-z]{1,32}$: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                        ok=false;
                    }
                if(ok)
                {
                    const std::string &type=item->Attribute("type");
                    if(!std::regex_match(type,excludeFilterRegex))
                    {
                        Reputation reputationTemp;
                        reputationTemp.name=type;
                        reputationTemp.reputation_positive=point_list_positive;
                        reputationTemp.reputation_negative=point_list_negative;
                        reputation.push_back(reputationTemp);
                    }
                }
            }
            else
                std::cerr << "Unable to open the file: " << file << ", have not the item id: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
        item = item->NextSiblingElement("reputation");
    }

    return reputation;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::unordered_map<uint16_t, Quest> DatapackGeneralLoader::loadQuests(const std::string &folder)
{
    bool ok;
    std::unordered_map<uint16_t, Quest> quests;
    //open and quick check the file
    QFileInfoList entryList=QDir(QString::fromStdString(folder)).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    while(index<entryList.size())
    {
        if(!entryList.at(index).isDir())
        {
            index++;
            continue;
        }
        if(!QFile(entryList.at(index).absoluteFilePath()+"/definition.xml").exists())
        {
            index++;
            continue;
        }
        const uint32_t &questId=stringtouint32(entryList.at(index).fileName(),&ok);
        if(ok)
        {
            //add it, all seam ok
            std::pair<bool,Quest> returnedQuest=loadSingleQuest(entryList.at(index).absoluteFilePath()+"/definition.xml");
            if(returnedQuest.first==true)
            {
                returnedQuest.second.id=questId;
                if(quests.find(returnedQuest.second.id)!=quests.cend())
                    std::cerr << "The quest with id: " << returnedQuest.second.id << " is already found, disable: " << entryList.at(index).absoluteFilePath() << "/definition.xml" << std::endl;
                else
                    quests[returnedQuest.second.id]=returnedQuest.second;
            }
        }
        else
            std::cerr << "Unable to open the folder: " << entryList.at(index).fileName() << ", because is folder name is not a number" << std::endl;
        index++;
    }
    return quests;
}

std::pair<bool,Quest> DatapackGeneralLoader::loadSingleQuest(const std::string &file)
{
    std::unordered_map<std::string,int> reputationNameToId;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    CatchChallenger::Quest quest;
    quest.id=0;
    TiXmlDocument domDocument(file.c_str());
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return std::pair<bool,Quest>(false,quest);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="quest")
    {
        std::cerr << "Unable to open the file: " << file << ", \"quest\" root balise not found for reputation of the xml file" << std::endl;
        return std::pair<bool,Quest>(false,quest);
    }

    //load the content
    bool ok;
    std::vector<uint16_t> defaultBots;
    quest.id=0;
    quest.repeatable=false;
    if(root->hasAttribute("repeatable"))
        if(root->Attribute("repeatable")=="yes" || root->Attribute("repeatable")=="true")
            quest.repeatable=true;
    if(root->hasAttribute("bot"))
    {
        const std::vector<std::string> &tempStringList=stringsplit(root->Attribute("bot"),';');
        unsigned int index=0;
        while(index<tempStringList.size())
        {
            uint16_t tempInt=stringtouint16(tempStringList.at(index),&ok);
            if(ok)
                defaultBots.push_back(tempInt);
            index++;
        }
    }

    //load requirements
    const TiXmlElement * requirements = root->FirstChildElement("requirements");
    while(requirements!=NULL)
    {
        if(requirements->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            //load requirements reputation
            {
                const TiXmlElement * requirementsItem = requirements->FirstChildElement("reputation");
                while(requirementsItem!=NULL)
                {
                    if(requirementsItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                    {
                        if(requirementsItem.hasAttribute("type") && requirementsItem.hasAttribute("level"))
                        {
                            if(reputationNameToId.find(requirementsItem->Attribute("type"))!=reputationNameToId.cend())
                            {
                                std::string stringLevel=requirementsItem->Attribute("level");
                                bool positif=!stringStartWith(stringLevel,"-");
                                if(!positif)
                                    stringLevel.erase(0,1);
                                uint8_t level=stringtouint8(stringLevel,&ok);
                                if(ok)
                                {
                                    CatchChallenger::ReputationRequirements reputation;
                                    reputation.level=level;
                                    reputation.positif=positif;
                                    reputation.reputationId=reputationNameToId.at(requirementsItem->Attribute("type"));
                                    quest.requirements.reputation.push_back(reputation);
                                }
                                else
                                    std::cerr << "Unable to open the file: " << file << ", reputation is not a number " << stringLevel << ": child->ValueStr(): " << requirementsItem->ValueStr() << " (at line: " << requirementsItem->Row() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Has not the attribute: " << requirementsItem->Attribute("type") << ", reputation not found: child->ValueStr(): " << requirementsItem->ValueStr() << " (at line: " << requirementsItem->Row() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has not the attribute: type level, have not attribute type or level: child->ValueStr(): " << requirementsItem->ValueStr() << " (at line: " << requirementsItem->Row() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << requirementsItem->ValueStr() << " (at line: " << requirementsItem->Row() << ")" << std::endl;
                    requirementsItem = requirementsItem->NextSiblingElement("reputation");
                }
            }
            //load requirements quest
            {
                const TiXmlElement * requirementsItem = requirements->FirstChildElement("quest");
                while(requirementsItem!=NULL)
                {
                    if(requirementsItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                    {
                        if(requirementsItem.hasAttribute("id"))
                        {
                            const uint32_t &questId=stringtouint32(requirementsItem->Attribute("id"),&ok);
                            if(ok)
                            {
                                QuestRequirements questNewEntry;
                                questNewEntry.quest=questId;
                                questNewEntry.inverse=false;
                                if(requirementsItem.hasAttribute("inverse"))
                                    if(requirementsItem->Attribute("inverse")=="true")
                                        questNewEntry.inverse=true;
                                quest.requirements.quests.push_back(questNewEntry);
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", requirement quest item id is not a number " << requirementsItem->Attribute("id") << ": child->ValueStr(): " << requirementsItem->ValueStr() << " (at line: " << requirementsItem->Row() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has attribute: %1, requirement quest item have not id attribute: child->ValueStr(): " << requirementsItem->ValueStr() << " (at line: " << requirementsItem->Row() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << requirementsItem->ValueStr() << " (at line: " << requirementsItem->Row() << ")" << std::endl;
                    requirementsItem = requirementsItem->NextSiblingElement("quest");
                }
            }
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << requirements->ValueStr() << " (at line: " << requirements->Row() << ")" << std::endl;
        requirements = requirements->NextSiblingElement("requirements");
    }

    //load rewards
    const TiXmlElement * rewards = root->FirstChildElement("rewards");
    while(rewards!=NULL)
    {
        if(rewards->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            //load rewards reputation
            {
                const TiXmlElement * reputationItem = rewards->FirstChildElement("reputation");
                while(reputationItem!=NULL)
                {
                    if(reputationItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                    {
                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("point"))
                        {
                            if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                            {
                                const int32_t &point=reputationItem->Attribute("point").toInt(&ok);
                                if(ok)
                                {
                                    CatchChallenger::ReputationRewards reputation;
                                    reputation.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                    reputation.point=point;
                                    quest.rewards.reputation.push_back(reputation);
                                }
                                else
                                    std::cerr << "Unable to open the file: " << file << ", quest rewards point is not a number: child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", quest rewards point is not a number: child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has not the attribute: type/point, quest rewards point have not type or point attribute: child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                    reputationItem = reputationItem->NextSiblingElement("reputation");
                }
            }
            //load rewards item
            {
                const TiXmlElement * rewardsItem = rewards->FirstChildElement("item");
                while(rewardsItem!=NULL)
                {
                    if(rewardsItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                    {
                        if(rewardsItem.hasAttribute("id"))
                        {
                            CatchChallenger::Quest::Item item;
                            item.item=stringtouint32(rewardsItem->Attribute("id"),&ok);
                            item.quantity=1;
                            if(ok)
                            {
                                if(CommonDatapack::commonDatapack.items.item.find(item.item)==CommonDatapack::commonDatapack.items.item.cend())
                                {
                                    std::cerr << "Unable to open the file: " << file << ", rewards item id is not into the item list: " << rewardsItem->Attribute("id") << ": child->ValueStr(): " << rewardsItem->ValueStr() << " (at line: " << rewardsItem->Row() << ")" << std::endl;
                                    return std::pair<bool,Quest>(false,quest);
                                }
                                if(rewardsItem.hasAttribute("quantity"))
                                {
                                    item.quantity=stringtouint32(rewardsItem->Attribute("quantity"),&ok);
                                    if(!ok)
                                        item.quantity=1;
                                }
                                quest.rewards.items.push_back(item);
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", rewards item id is not a number: " << rewardsItem->Attribute("id") << ": child->ValueStr(): " << rewardsItem->ValueStr() << " (at line: " << rewardsItem->Row() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has not the attribute: id, rewards item have not attribute id: child->ValueStr(): " << rewardsItem->ValueStr() << " (at line: " << rewardsItem->Row() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << rewardsItem->ValueStr() << " (at line: " << rewardsItem->Row() << ")" << std::endl;
                    rewardsItem = rewardsItem->NextSiblingElement("item");
                }
            }
            //load rewards allow
            {
                const TiXmlElement * allowItem = rewards->FirstChildElement("allow");
                while(allowItem!=NULL)
                {
                    if(allowItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                    {
                        if(allowItem.hasAttribute("type"))
                        {
                            if(allowItem->Attribute("type")=="clan")
                                quest.rewards.allow.push_back(CatchChallenger::ActionAllow_Clan);
                            else
                                std::cerr << "Unable to open the file: " << file << ", allow type not understand " << allowItem->Attribute("id") << ": child->ValueStr(): " << allowItem->ValueStr() << " (at line: " << allowItem->Row() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has attribute: %1, rewards item have not attribute id: child->ValueStr(): " << allowItem->ValueStr() << " (at line: " << allowItem->Row() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << allowItem->ValueStr() << " (at line: " << allowItem->Row() << ")" << std::endl;
                    allowItem = allowItem->NextSiblingElement("allow");
                }
            }
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << rewards->ValueStr() << " (at line: " << rewards->Row() << ")" << std::endl;
        rewards = rewards->NextSiblingElement("rewards");
    }

    std::unordered_map<uint8_t,CatchChallenger::Quest::Step> steps;
    //load step
    const TiXmlElement * step = root->FirstChildElement("step");
    while(step!=NULL)
    {
        if(step->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            if(step.hasAttribute("id"))
            {
                const uint32_t &id=stringtouint32(step->Attribute("id"),&ok);
                if(ok)
                {
                    CatchChallenger::Quest::Step stepObject;
                    if(step.hasAttribute("bot"))
                    {
                        const std::vector<std::string> &tempStringList=stringsplit(step->Attribute("bot"),';');
                        unsigned int index=0;
                        while(index<tempStringList.size())
                        {
                            const uint32_t &tempInt=stringtouint32(tempStringList.at(index),&ok);
                            if(ok)
                                stepObject.bots.push_back(tempInt);
                            index++;
                        }
                    }
                    else
                        stepObject.bots=defaultBots;
                    //do the item
                    {
                        const TiXmlElement * stepItem = step->FirstChildElement("item");
                        while(stepItem!=NULL)
                        {
                            if(stepItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                if(stepItem.hasAttribute("id"))
                                {
                                    CatchChallenger::Quest::Item item;
                                    item.item=stringtouint32(stepItem->Attribute("id"),&ok);
                                    item.quantity=1;
                                    if(ok)
                                    {
                                        if(CommonDatapack::commonDatapack.items.item.find(item.item)==CommonDatapack::commonDatapack.items.item.cend())
                                        {
                                            std::cerr << "Unable to open the file: " << file << ", rewards item id is not into the item list: " << stepItem->Attribute("id") << ": child->ValueStr(): " << stepItem->ValueStr() << " (at line: " << stepItem->Row() << ")" << std::endl;
                                            return std::pair<bool,Quest>(false,quest);
                                        }
                                        if(stepItem.hasAttribute("quantity"))
                                        {
                                            item.quantity=stringtouint32(stepItem->Attribute("quantity"),&ok);
                                            if(!ok)
                                                item.quantity=1;
                                        }
                                        stepObject.requirements.items.push_back(item);
                                        if(stepItem.hasAttribute("monster") && stepItem.hasAttribute("rate"))
                                        {
                                            CatchChallenger::Quest::ItemMonster itemMonster;
                                            itemMonster.item=item.item;

                                            const std::vector<std::string> &tempStringList=stringsplit(stepItem->Attribute("monster"),';');
                                            unsigned int index=0;
                                            while(index<tempStringList.size())
                                            {
                                                const uint32_t &tempInt=stringtouint32(tempStringList.at(index),&ok);
                                                if(ok)
                                                    itemMonster.monsters.push_back(tempInt);
                                                index++;
                                            }

                                            std::string rateString=stepItem->Attribute("rate");
                                            stringreplaceOne(rateString,"%","");
                                            itemMonster.rate=stringtouint8(rateString,&ok);
                                            if(ok)
                                                stepObject.itemsMonster.push_back(itemMonster);
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the file: " << file << ", step id is not a number " << stepItem->Attribute("id") << ": child->ValueStr(): " << step->ValueStr() << " (at line: " << step->Row() << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Has not the attribute: id, step have not id attribute: child->ValueStr(): " << step->ValueStr() << " (at line: " << step->Row() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << step->ValueStr() << " (at line: " << step->Row() << ")" << std::endl;
                            stepItem = stepItem->NextSiblingElement("item");
                        }
                    }
                    //do the fight
                    {
                        const TiXmlElement * fightItem = step->FirstChildElement("fight");
                        while(fightItem!=NULL)
                        {
                            if(fightItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                if(fightItem.hasAttribute("id"))
                                {
                                    const uint32_t &fightId=stringtouint32(fightItem->Attribute("id"),&ok);
                                    if(ok)
                                        stepObject.requirements.fightId.push_back(fightId);
                                    else
                                        std::cerr << "Unable to open the file: " << file << ", step id is not a number " << fightItem->Attribute("id") << ": child->ValueStr(): " << fightItem->ValueStr() << " (at line: " << fightItem->Row() << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Has attribute: %1, step have not id attribute: child->ValueStr(): " << fightItem->ValueStr() << " (at line: " << fightItem->Row() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << fightItem->ValueStr() << " (at line: " << fightItem->Row() << ")" << std::endl;
                            fightItem = fightItem->NextSiblingElement("fight");
                        }
                    }
                    steps[id]=stepObject;
                }
                else
                    std::cerr << "Unable to open the file: " << file << ", step id is not a number: child->ValueStr(): " << step->ValueStr() << " (at line: " << step->Row() << ")" << std::endl;
            }
            else
                std::cerr << "Has attribute: %1, step have not id attribute: child->ValueStr(): " << step->ValueStr() << " (at line: " << step->Row() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child->ValueStr(): " << step->ValueStr() << " (at line: " << step->Row() << ")" << std::endl;
        step = step->NextSiblingElement("step");
    }

    //sort the step
    unsigned int indexLoop=1;
    while(indexLoop<(steps.size()+1))
    {
        if(steps.find(indexLoop)==steps.cend())
            break;
        quest.steps.push_back(steps.at(indexLoop));
        indexLoop++;
    }
    if(indexLoop<(steps.size()+1))
        return std::pair<bool,Quest>(false,quest);
    return std::pair<bool,Quest>(true,quest);
}

std::unordered_map<uint8_t, Plant> DatapackGeneralLoader::loadPlants(const std::string &file)
{
    std::unordered_map<std::string,int> reputationNameToId;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    std::unordered_map<uint8_t, Plant> plants;
    TiXmlDocument domDocument(file.c_str());
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return plants;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="plants")
    {
        std::cerr << "Unable to open the file: " << file << ", \"plants\" root balise not found for reputation of the xml file" << std::endl;
        return plants;
    }

    //load the content
    bool ok,ok2;
    const TiXmlElement * plantItem = root->FirstChildElement("plant");
    while(plantItem!=NULL)
    {
        if(plantItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            if(plantItem.hasAttribute("id") && plantItem.hasAttribute("itemUsed"))
            {
                const uint8_t &id=plantItem->Attribute("id").toUShort(&ok);
                const uint32_t &itemUsed=stringtouint32(plantItem->Attribute("itemUsed"),&ok2);
                if(ok && ok2)
                {
                    if(plants.find(id)==plants.cend())
                    {
                        Plant plant;
                        plant.fruits_seconds=0;
                        plant.sprouted_seconds=0;
                        plant.taller_seconds=0;
                        plant.flowering_seconds=0;
                        plant.itemUsed=itemUsed;
                        {
                            const TiXmlElement * requirementsItem = plantItem->FirstChildElement("requirements");
                            if(requirementsItem!=NULL && requirementsItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                const TiXmlElement * reputationItem = requirementsItem->FirstChildElement("reputation");
                                while(reputationItem!=NULL)
                                {
                                    if(reputationItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("level"))
                                        {
                                            if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                                            {
                                                ReputationRequirements reputationRequirements;
                                                std::string stringLevel=reputationItem->Attribute("level");
                                                reputationRequirements.positif=!stringStartWith(stringLevel,"-");
                                                if(!reputationRequirements.positif)
                                                    stringLevel.erase(0,1);
                                                reputationRequirements.level=stringtouint8(stringLevel,&ok);
                                                if(ok)
                                                {
                                                    reputationRequirements.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                                    plant.requirements.reputation.push_back(reputationRequirements);
                                                }
                                            }
                                            else
                                                std::cerr << "Reputation type not found: " << reputationItem->Attribute("type") << ", have not the id, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                    reputationItem = reputationItem->NextSiblingElement("reputation");
                                }
                            }
                        }
                        {
                            const TiXmlElement * rewardsItem = plantItem->FirstChildElement("rewards");
                            if(rewardsItem!=NULL && rewardsItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                const TiXmlElement * reputationItem = rewardsItem->FirstChildElement("reputation");
                                while(reputationItem!=NULL)
                                {
                                    if(reputationItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("point"))
                                        {
                                            if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                                            {
                                                ReputationRewards reputationRewards;
                                                reputationRewards.point=reputationItem->Attribute("point").toInt(&ok);
                                                if(ok)
                                                {
                                                    reputationRewards.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                                    plant.rewards.reputation.push_back(reputationRewards);
                                                }
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                    reputationItem = reputationItem->NextSiblingElement("reputation");
                                }
                            }
                        }
                        ok=false;
                        const TiXmlElement * quantity = plantItem->FirstChildElement("quantity");
                        if(quantity!=NULL)
                        {
                            if(quantity->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                const float &float_quantity=quantity.text().toFloat(&ok2);
                                const int &integer_part=float_quantity;
                                float random_part=float_quantity-integer_part;
                                random_part*=RANDOM_FLOAT_PART_DIVIDER;
                                plant.fix_quantity=integer_part;
                                plant.random_quantity=random_part;
                                ok=ok2;
                            }
                        }
                        int intermediateTimeCount=0;
                        const TiXmlElement * grow = plantItem->FirstChildElement("grow");
                        if(grow!=NULL)
                        {
                            if(grow->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                const TiXmlElement * fruits = grow->FirstChildElement("fruits");
                                if(fruits!=NULL)
                                {
                                    if(fruits->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        plant.fruits_seconds=stringtouint32(fruits.text(),&ok2)*60;
                                        plant.sprouted_seconds=plant.fruits_seconds;
                                        plant.taller_seconds=plant.fruits_seconds;
                                        plant.flowering_seconds=plant.fruits_seconds;
                                        if(!ok2)
                                        {
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << fruits.text() << " child->ValueStr(): " << fruits->ValueStr() << " (at line: " << fruits->Row() << ")" << std::endl;
                                            ok=false;
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "Unable to parse the plants file: " << file << ", fruits is not an element: child->ValueStr(): " << fruits->ValueStr() << " (at line: " << fruits->Row() << ")" << std::endl;
                                    }
                                }
                                else
                                {
                                    ok=false;
                                    std::cerr << "Unable to parse the plants file: " << file << ", fruits is null: child->ValueStr(): " << fruits->ValueStr() << " (at line: " << fruits->Row() << ")" << std::endl;
                                }
                                const TiXmlElement * sprouted = grow->FirstChildElement("sprouted");
                                if(sprouted!=NULL)
                                {
                                    if(sprouted->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        plant.sprouted_seconds=stringtouint32(sprouted.text(),&ok2)*60;
                                        if(!ok2)
                                        {
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << sprouted.text() << " child->ValueStr(): " << sprouted->ValueStr() << " (at line: " << sprouted->Row() << ")" << std::endl;
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not an element: child->ValueStr(): " << sprouted->ValueStr() << " (at line: " << sprouted->Row() << ")" << std::endl;
                                }
                                const TiXmlElement * taller = grow->FirstChildElement("taller");
                                if(taller!=NULL)
                                {
                                    if(taller->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        plant.taller_seconds=stringtouint32(taller.text(),&ok2)*60;
                                        if(!ok2)
                                        {
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << taller.text() << " child->ValueStr(): " << taller->ValueStr() << " (at line: " << taller->Row() << ")" << std::endl;
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        std::cerr << "Unable to parse the plants file: " << file << ", taller is not an element: child->ValueStr(): " << taller->ValueStr() << " (at line: " << taller->Row() << ")" << std::endl;
                                }
                                const TiXmlElement * flowering = grow->FirstChildElement("flowering");
                                if(flowering!=NULL)
                                {
                                    if(flowering->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        plant.flowering_seconds=stringtouint32(flowering.text(),&ok2)*60;
                                        if(!ok2)
                                        {
                                            ok=false;
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << flowering.text() << " child->ValueStr(): " << flowering->ValueStr() << " (at line: " << flowering->Row() << ")" << std::endl;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        std::cerr << "Unable to parse the plants file: " << file << ", flowering is not an element: child->ValueStr(): " << flowering->ValueStr() << " (at line: " << flowering->Row() << ")" << std::endl;
                                }
                            }
                            else
                                std::cerr << "Unable to parse the plants file: " << file << ", grow is not an element: child->ValueStr(): child->ValueStr(): " << grow->ValueStr() << " (at line: " << grow->Row() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Unable to parse the plants file: " << file << ", grow is null: child->ValueStr(): child->ValueStr(): " << grow->ValueStr() << " (at line: " << grow->Row() << ")" << std::endl;
                        if(ok)
                        {
                            bool needIntermediateTimeFix=false;
                            if(plant.flowering_seconds>=plant.fruits_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", flowering_seconds>=fruits_seconds: child->ValueStr(): child->ValueStr(): " << grow->ValueStr() << " (at line: " << grow->Row() << ")" << std::endl;
                            }
                            if(plant.taller_seconds>=plant.flowering_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", taller_seconds>=flowering_seconds: child->ValueStr(): " << grow->ValueStr() << " (at line: " << grow->Row() << ")" << std::endl;
                            }
                            if(plant.sprouted_seconds>=plant.taller_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", sprouted_seconds>=taller_seconds: child->ValueStr(): " << grow->ValueStr() << " (at line: " << grow->Row() << ")" << std::endl;
                            }
                            if(plant.sprouted_seconds<=0)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", sprouted_seconds<=0: child->ValueStr(): " << grow->ValueStr() << " (at line: " << grow->Row() << ")" << std::endl;
                            }
                            if(needIntermediateTimeFix)
                            {
                                plant.flowering_seconds=plant.fruits_seconds*3/4;
                                plant.taller_seconds=plant.fruits_seconds*2/4;
                                plant.sprouted_seconds=plant.fruits_seconds*1/4;
                            }
                            plants[id]=plant;
                        }
                    }
                    else
                        std::cerr << "Unable to open the plants file: " << file << ", id number already set: child->ValueStr(): " << plantItem->ValueStr() << " (at line: " << plantItem->Row() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the plants file: " << file << ", id is not a number: child->ValueStr(): " << plantItem->ValueStr() << " (at line: " << plantItem->Row() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the plants file: " << file << ", have not the plant id: child->ValueStr(): " << plantItem->ValueStr() << " (at line: " << plantItem->Row() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the plants file: " << file << ", is not an element: child->ValueStr(): " << plantItem->ValueStr() << " (at line: " << plantItem->Row() << ")" << std::endl;
        plantItem = plantItem->NextSiblingElement("plant");
    }
    return plants;
}

std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> > DatapackGeneralLoader::loadCraftingRecipes(const std::string &file,const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<std::string,int> reputationNameToId;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    std::unordered_map<uint16_t,CrafingRecipe> crafingRecipes;
    std::unordered_map<uint16_t,uint16_t> itemToCrafingRecipes;
    TiXmlDocument domDocument(file.c_str());
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="recipes")
    {
        std::cerr << "Unable to open the file: " << file << ", \"recipes\" root balise not found for reputation of the xml file" << std::endl;
        return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
    }

    //load the content
    bool ok,ok2,ok3;
    const TiXmlElement * recipeItem = root->FirstChildElement("recipe");
    while(recipeItem!=NULL)
    {
        if(recipeItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            if(recipeItem.hasAttribute("id") && recipeItem.hasAttribute("itemToLearn") && recipeItem.hasAttribute("doItemId"))
            {
                uint8_t success=100;
                if(recipeItem.hasAttribute("success"))
                {
                    const uint8_t &tempShort=recipeItem->Attribute("success").toUShort(&ok);
                    if(ok)
                    {
                        if(tempShort>100)
                            std::cerr << "preload_crafting_recipes() success can't be greater than 100 for crafting recipe file: " << file << ", child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                        else
                            success=tempShort;
                    }
                    else
                        std::cerr << "preload_crafting_recipes() success in not an number for crafting recipe file: " << file << ", child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                }
                uint16_t quantity=1;
                if(recipeItem.hasAttribute("quantity"))
                {
                    const uint32_t &tempShort=stringtouint32(recipeItem->Attribute("quantity"),&ok);
                    if(ok)
                    {
                        if(tempShort>65535)
                            std::cerr << "preload_crafting_recipes() quantity can't be greater than 65535 for crafting recipe file: " << file << ", child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                        else
                            quantity=tempShort;
                    }
                    else
                        std::cerr << "preload_crafting_recipes() quantity in not an number for crafting recipe file: " << file << ", child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                }

                const uint32_t &id=stringtouint32(recipeItem->Attribute("id"),&ok);
                const uint32_t &itemToLearn=stringtouint32(recipeItem->Attribute("itemToLearn"),&ok2);
                const uint32_t &doItemId=stringtouint32(recipeItem->Attribute("doItemId"),&ok3);
                if(ok && ok2 && ok3)
                {
                    if(crafingRecipes.find(id)==crafingRecipes.cend())
                    {
                        ok=true;
                        CatchChallenger::CrafingRecipe recipe;
                        recipe.doItemId=doItemId;
                        recipe.itemToLearn=itemToLearn;
                        recipe.quantity=quantity;
                        recipe.success=success;
                        {
                            const TiXmlElement * requirementsItem = recipeItem->FirstChildElement("requirements");
                            if(requirementsItem!=NULL && requirementsItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                const TiXmlElement * reputationItem = requirementsItem->FirstChildElement("reputation");
                                while(reputationItem!=NULL)
                                {
                                    if(reputationItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("level"))
                                        {
                                            if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                                            {
                                                ReputationRequirements reputationRequirements;
                                                std::string stringLevel=reputationItem->Attribute("level");
                                                reputationRequirements.positif=!stringStartWith(stringLevel,"-");
                                                if(!reputationRequirements.positif)
                                                    stringLevel.erase(0,1);
                                                reputationRequirements.level=stringtouint8(stringLevel,&ok);
                                                if(ok)
                                                {
                                                    reputationRequirements.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                                    recipe.requirements.reputation.push_back(reputationRequirements);
                                                }
                                            }
                                            else
                                                std::cerr << "Reputation type not found: " << reputationItem->Attribute("type") << ", have not the id, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                    reputationItem = reputationItem->NextSiblingElement("reputation");
                                }
                            }
                        }
                        {
                            const TiXmlElement * rewardsItem = recipeItem->FirstChildElement("rewards");
                            if(rewardsItem!=NULL && rewardsItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                const TiXmlElement * reputationItem = rewardsItem->FirstChildElement("reputation");
                                while(reputationItem!=NULL)
                                {
                                    if(reputationItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("point"))
                                        {
                                            if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                                            {
                                                ReputationRewards reputationRewards;
                                                reputationRewards.point=reputationItem->Attribute("point").toInt(&ok);
                                                if(ok)
                                                {
                                                    reputationRewards.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                                    recipe.rewards.reputation.push_back(reputationRewards);
                                                }
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                    reputationItem = reputationItem->NextSiblingElement("reputation");
                                }
                            }
                        }
                        const TiXmlElement * material = recipeItem->FirstChildElement("material");
                        while(material!=NULL && ok)
                        {
                            if(material->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                if(material.hasAttribute("itemId"))
                                {
                                    const uint32_t &itemId=stringtouint32(material->Attribute("itemId"),&ok2);
                                    if(!ok2)
                                    {
                                        ok=false;
                                        std::cerr << "preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: " << file << ": child->ValueStr(): " << material->ValueStr() << " (at line: " << material->Row() << ")" << std::endl;
                                        break;
                                    }
                                    uint16_t quantity=1;
                                    if(material.hasAttribute("quantity"))
                                    {
                                        const uint32_t &tempShort=stringtouint32(material->Attribute("quantity"),&ok2);
                                        if(ok2)
                                        {
                                            if(tempShort>65535)
                                            {
                                                ok=false;
                                                std::cerr << "preload_crafting_recipes() material quantity can't be greater than 65535 for crafting recipe file: " << file << ": child->ValueStr(): " << material->ValueStr() << " (at line: " << material->Row() << ")" << std::endl;
                                                break;
                                            }
                                            else
                                                quantity=tempShort;
                                        }
                                        else
                                        {
                                            ok=false;
                                            std::cerr << "preload_crafting_recipes() material quantity in not an number for crafting recipe file: " << file << ": child->ValueStr(): " << material->ValueStr() << " (at line: " << material->Row() << ")" << std::endl;
                                            break;
                                        }
                                    }
                                    if(items.find(itemId)==items.cend())
                                    {
                                        ok=false;
                                        std::cerr << "preload_crafting_recipes() material itemId in not into items list for crafting recipe file: " << file << ": child->ValueStr(): " << material->ValueStr() << " (at line: " << material->Row() << ")" << std::endl;
                                        break;
                                    }
                                    CatchChallenger::CrafingRecipe::Material newMaterial;
                                    newMaterial.item=itemId;
                                    newMaterial.quantity=quantity;
                                    unsigned int index=0;
                                    while(index<recipe.materials.size())
                                    {
                                        if(recipe.materials.at(index).item==newMaterial.item)
                                            break;
                                        index++;
                                    }
                                    if(index<recipe.materials.size())
                                    {
                                        ok=false;
                                        std::cerr << "id of item already into resource or product list: %1: child->ValueStr(): " << material->ValueStr() << " (at line: " << material->Row() << ")" << std::endl;
                                    }
                                    else
                                    {
                                        if(recipe.doItemId==newMaterial.item)
                                        {
                                            std::cerr << "id of item already into resource or product list: %1: child->ValueStr(): " << material->ValueStr() << " (at line: " << material->Row() << ")" << std::endl;
                                            ok=false;
                                        }
                                        else
                                            recipe.materials.push_back(newMaterial);
                                    }
                                }
                                else
                                    std::cerr << "preload_crafting_recipes() material have not attribute itemId for crafting recipe file: " << file << ": child->ValueStr(): " << material->ValueStr() << " (at line: " << material->Row() << ")" << std::endl;
                            }
                            else
                                std::cerr << "preload_crafting_recipes() material is not an element for crafting recipe file: " << file << ": child->ValueStr(): " << material->ValueStr() << " (at line: " << material->Row() << ")" << std::endl;
                            material = material->NextSiblingElement("material");
                        }
                        if(ok)
                        {
                            if(items.find(recipe.itemToLearn)==items.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() itemToLearn is not into items list for crafting recipe file: " << file << ": child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(items.find(recipe.doItemId)==items.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() doItemId is not into items list for crafting recipe file: " << file << ": child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.find(recipe.itemToLearn)!=itemToCrafingRecipes.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() itemToLearn already used to learn another recipe: " << itemToCrafingRecipes.at(recipe.doItemId) << ": file: " << file << " child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(recipe.itemToLearn==recipe.doItemId)
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() the product of the recipe can't be them self: " << id << ": file: " << file << ": child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.find(recipe.doItemId)!=itemToCrafingRecipes.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() the product of the recipe can't be a recipe: " << itemToCrafingRecipes.at(recipe.doItemId) << ": file: " << file << ": child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            crafingRecipes[id]=recipe;
                            itemToCrafingRecipes[recipe.itemToLearn]=id;
                        }
                    }
                    else
                        std::cerr << "Unable to open the crafting recipe file: " << file << ", id number already set: child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the crafting recipe file: " << file << ", id is not a number: child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the crafting recipe file: " << file << ", have not the crafting recipe id: child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the crafting recipe file: " << file << ", is not an element: child->ValueStr(): " << recipeItem->ValueStr() << " (at line: " << recipeItem->Row() << ")" << std::endl;
        recipeItem = recipeItem->NextSiblingElement("recipe");
    }
    return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
}

std::unordered_map<uint16_t,Industry> DatapackGeneralLoader::loadIndustries(const std::string &folder,const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<uint16_t,Industry> industries;
    QDir dir(QString::fromStdString(folder));
    const QFileInfoList &fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        TiXmlDocument domDocument(file.c_str());
        const std::string &file=fileList.at(file_index).absoluteFilePath();
        //open and quick check the file
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            #endif
            const bool loadOkay=domDocument.LoadFile();
            if(!loadOkay)
            {
                std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        const TiXmlElement * root = domDocument.RootElement();
        if(root->ValueStr()!="industries")
        {
            std::cerr << "Unable to open the file: " << file << ", \"industries\" root balise not found for reputation of the xml file" << std::endl;
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2,ok3;
        const TiXmlElement * industryItem = root->FirstChildElement("industrialrecipe");
        while(industryItem!=NULL)
        {
            if(industryItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
            {
                if(industryItem.hasAttribute("id") && industryItem.hasAttribute("time") && industryItem.hasAttribute("cycletobefull"))
                {
                    Industry industry;
                    const uint32_t &id=stringtouint32(industryItem->Attribute("id"),&ok);
                    industry.time=stringtouint32(industryItem->Attribute("time"),&ok2);
                    industry.cycletobefull=stringtouint32(industryItem->Attribute("cycletobefull"),&ok3);
                    if(ok && ok2 && ok3)
                    {
                        if(industries.find(id)==industries.cend())
                        {
                            if(industry.time<60*5)
                            {
                                std::cerr << "the time need be greater than 5*60 seconds to not slow down the server: " << industry.time << ", " << file << ": child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                industry.time=60*5;
                            }
                            if(industry.cycletobefull<1)
                            {
                                std::cerr << "cycletobefull need be greater than 0: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                industry.cycletobefull=1;
                            }
                            else if(industry.cycletobefull>65535)
                            {
                                std::cerr << "cycletobefull need be lower to 10 to not slow down the server, use the quantity: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                industry.cycletobefull=10;
                            }
                            //resource
                            {
                                const TiXmlElement * resourceItem = industryItem->FirstChildElement("resource");
                                ok=true;
                                while(resourceItem!=NULL && ok)
                                {
                                    if(resourceItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        Industry::Resource resource;
                                        resource.quantity=1;
                                        if(resourceItem.hasAttribute("quantity"))
                                        {
                                            resource.quantity=stringtouint32(resourceItem->Attribute("quantity"),&ok);
                                            if(!ok)
                                                std::cerr << "quantity is not a number: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                        }
                                        if(ok)
                                        {
                                            if(resourceItem.hasAttribute("id"))
                                            {
                                                resource.item=stringtouint32(resourceItem->Attribute("id"),&ok);
                                                if(!ok)
                                                    std::cerr << "id is not a number: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                                else if(items.find(resource.item)==items.cend())
                                                {
                                                    ok=false;
                                                    std::cerr << "id is not into the item list: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                                }
                                                else
                                                {
                                                    unsigned int index=0;
                                                    while(index<industry.resources.size())
                                                    {
                                                        if(industry.resources.at(index).item==resource.item)
                                                            break;
                                                        index++;
                                                    }
                                                    if(index<industry.resources.size())
                                                    {
                                                        ok=false;
                                                        std::cerr << "id of item already into resource or product list: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                                    }
                                                    else
                                                    {
                                                        index=0;
                                                        while(index<industry.products.size())
                                                        {
                                                            if(industry.products.at(index).item==resource.item)
                                                                break;
                                                            index++;
                                                        }
                                                        if(index<industry.products.size())
                                                        {
                                                            ok=false;
                                                            std::cerr << "id of item already into resource or product list: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                                        }
                                                        else
                                                            industry.resources.push_back(resource);
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                std::cerr << "have not the id attribute: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "is not a elements: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                    }
                                    resourceItem = resourceItem->NextSiblingElement("resource");
                                }
                            }

                            //product
                            if(ok)
                            {
                                const TiXmlElement * productItem = industryItem->FirstChildElement("product");
                                ok=true;
                                while(productItem!=NULL && ok)
                                {
                                    if(productItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        Industry::Product product;
                                        product.quantity=1;
                                        if(productItem.hasAttribute("quantity"))
                                        {
                                            product.quantity=stringtouint32(productItem->Attribute("quantity"),&ok);
                                            if(!ok)
                                                std::cerr << "quantity is not a number: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                        }
                                        if(ok)
                                        {
                                            if(productItem.hasAttribute("id"))
                                            {
                                                product.item=stringtouint32(productItem->Attribute("id"),&ok);
                                                if(!ok)
                                                    std::cerr << "id is not a number: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                                else if(items.find(product.item)==items.cend())
                                                {
                                                    ok=false;
                                                    std::cerr << "id is not into the item list: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                                }
                                                else
                                                {
                                                    unsigned int index=0;
                                                    while(index<industry.resources.size())
                                                    {
                                                        if(industry.resources.at(index).item==product.item)
                                                            break;
                                                        index++;
                                                    }
                                                    if(index<industry.resources.size())
                                                    {
                                                        ok=false;
                                                        std::cerr << "id of item already into resource or product list: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                                    }
                                                    else
                                                    {
                                                        index=0;
                                                        while(index<industry.products.size())
                                                        {
                                                            if(industry.products.at(index).item==product.item)
                                                                break;
                                                            index++;
                                                        }
                                                        if(index<industry.products.size())
                                                        {
                                                            ok=false;
                                                            std::cerr << "id of item already into resource or product list: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                                        }
                                                        else
                                                            industry.products.push_back(product);
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                std::cerr << "have not the id attribute: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "is not a elements: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                    }
                                    productItem = productItem->NextSiblingElement("product");
                                }
                            }

                            //add
                            if(ok)
                            {
                                if(industry.products.empty() || industry.resources.empty())
                                    std::cerr << "product or resources is empty: child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                                else
                                    industries[id]=industry;
                            }
                        }
                        else
                            std::cerr << "Unable to open the industries id number already set: file: " << file << ", child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the industries id is not a number: file: " << file << ", child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the industries have not the id: file: " << file << ", child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the industries is not an element: file: " << file << ", child->ValueStr(): " << industryItem->ValueStr() << " (at line: " << industryItem->Row() << ")" << std::endl;
            industryItem = industryItem->NextSiblingElement("industrialrecipe");
        }
        file_index++;
    }
    return industries;
}

std::unordered_map<uint16_t,IndustryLink> DatapackGeneralLoader::loadIndustriesLink(const std::string &file,const std::unordered_map<uint16_t,Industry> &industries)
{
    std::unordered_map<std::string,int> reputationNameToId;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    std::unordered_map<uint16_t,IndustryLink> industriesLink;
    TiXmlDocument domDocument(file.c_str());
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return industriesLink;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="industries")
    {
        std::cerr << "Unable to open the file: " << file << ", \"industries\" root balise not found for reputation of the xml file" << std::endl;
        return industriesLink;
    }

    //load the content
    bool ok,ok2;
    const TiXmlElement * linkItem = root->FirstChildElement("link");
    while(linkItem!=NULL)
    {
        if(linkItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            if(linkItem.hasAttribute("industrialrecipe") && linkItem.hasAttribute("industry"))
            {
                const uint32_t &industry_id=stringtouint32(linkItem->Attribute("industrialrecipe"),&ok);
                const uint32_t &factory_id=stringtouint32(linkItem->Attribute("industry"),&ok2);
                if(ok && ok2)
                {
                    if(industriesLink.find(factory_id)==industriesLink.cend())
                    {
                        if(industries.find(industry_id)!=industries.cend())
                        {
                            industriesLink[factory_id].industry=industry_id;
                            IndustryLink *industryLink=&industriesLink[factory_id];
                            {
                                {
                                    const TiXmlElement * requirementsItem = linkItem->FirstChildElement("requirements");
                                    if(requirementsItem!=NULL && requirementsItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        const TiXmlElement * reputationItem = requirementsItem->FirstChildElement("reputation");
                                        while(reputationItem!=NULL)
                                        {
                                            if(reputationItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                            {
                                                if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("level"))
                                                {
                                                    if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                                                    {
                                                        ReputationRequirements reputationRequirements;
                                                        std::string stringLevel=reputationItem->Attribute("level");
                                                        reputationRequirements.positif=!stringStartWith(stringLevel,"-");
                                                        if(!reputationRequirements.positif)
                                                            stringLevel.erase(0,1);
                                                        reputationRequirements.level=stringtouint8(stringLevel,&ok);
                                                        if(ok)
                                                        {
                                                            reputationRequirements.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                                            industryLink->requirements.reputation.push_back(reputationRequirements);
                                                        }
                                                    }
                                                    else
                                                        std::cerr << "Reputation type not found: have not the id, child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                                }
                                                else
                                                    std::cerr << "Unable to open the industries link have not the id, file: " << file << ", child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the industries link is not a element, file: " << file << ", child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                            reputationItem = reputationItem->NextSiblingElement("reputation");
                                        }
                                    }
                                }
                                {
                                    const TiXmlElement * rewardsItem = linkItem->FirstChildElement("rewards");
                                    if(rewardsItem!=NULL && rewardsItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        const TiXmlElement * reputationItem = rewardsItem->FirstChildElement("reputation");
                                        while(reputationItem!=NULL)
                                        {
                                            if(reputationItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                            {
                                                if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("point"))
                                                {
                                                    if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                                                    {
                                                        ReputationRewards reputationRewards;
                                                        reputationRewards.point=reputationItem->Attribute("point").toInt(&ok);
                                                        if(ok)
                                                        {
                                                            reputationRewards.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                                            industryLink->rewards.reputation.push_back(reputationRewards);
                                                        }
                                                    }
                                                }
                                                else
                                                    std::cerr << "Unable to open the industries link have not the id, file: " << file << ", child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the industries link is not a element, file: " << file << ", child->ValueStr(): " << reputationItem->ValueStr() << " (at line: " << reputationItem->Row() << ")" << std::endl;
                                            reputationItem = reputationItem->NextSiblingElement("reputation");
                                        }
                                    }
                                }
                            }
                        }
                        else
                            std::cerr << "Industry id for factory is not found: " << industry_id << ", file: " << file << ", child->ValueStr(): " << linkItem->ValueStr() << " (at line: " << linkItem->Row() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Factory already found: " << factory_id << ", file: " << file << ", child->ValueStr(): " << linkItem->ValueStr() << " (at line: " << linkItem->Row() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the industries link the attribute is not a number, file: " << file << ", child->ValueStr(): " << linkItem->ValueStr() << " (at line: " << linkItem->Row() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the industries link have not the id, file: " << file << ", child->ValueStr(): " << linkItem->ValueStr() << " (at line: " << linkItem->Row() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the industries link is not a element, file: " << file << ", child->ValueStr(): " << linkItem->ValueStr() << " (at line: " << linkItem->Row() << ")" << std::endl;
        linkItem = linkItem->NextSiblingElement("link");
    }
    return industriesLink;
}

ItemFull DatapackGeneralLoader::loadItems(const std::string &folder,const std::unordered_map<uint8_t,Buff> &monsterBuffs)
{
    #ifdef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    (void)monsterBuffs;
    #endif
    ItemFull items;
    QDir dir(QString::fromStdString(folder));
    const std::vector<std::string> &fileList=FacilityLibGeneral::listFolder(dir.absolutePath()+"/");
    unsigned int file_index=0;
    while(file_index<fileList.size())
    {
        if(!stringEndsWith(fileList.at(file_index),".xml"))
        {
            file_index++;
            continue;
        }
        const std::string &file=folder+fileList.at(file_index);
        if(!QFileInfo(QString::fromStdString(file)).isFile())
        {
            file_index++;
            continue;
        }
        TiXmlDocument domDocument(file.c_str());
        //open and quick check the file
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            #endif
            const bool loadOkay=domDocument.LoadFile();
            if(!loadOkay)
            {
                std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        const TiXmlElement * root = domDocument.RootElement();
        if(root->ValueStr()!="items")
        {
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        const TiXmlElement * item = root->FirstChildElement("item");
        while(item!=NULL)
        {
            if(item->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
            {
                if(item.hasAttribute("id"))
                {
                    const uint32_t &id=stringtouint32(item->Attribute("id"),&ok);
                    if(ok)
                    {
                        if(items.item.find(id)==items.item.cend())
                        {
                            //load the price
                            {
                                if(item.hasAttribute("price"))
                                {
                                    bool ok;
                                    items.item[id].price=stringtouint32(item->Attribute("price"),&ok);
                                    if(!ok)
                                    {
                                        std::cerr << "price is not a number, file: " << file << ", child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                                        items.item[id].price=0;
                                    }
                                }
                                else
                                {
                                    /*if(!item.hasAttribute("quest") || item->Attribute("quest")!="yes")
                                        std::cerr << "For parse item: Price not found, default to 0 (not sellable): child->ValueStr(): %1 (%2 at line: %3)").arg(item->ValueStr()).arg(file).arg(item->Row());*/
                                    items.item[id].price=0;
                                }
                            }
                            //load the consumeAtUse
                            {
                                if(item.hasAttribute("consumeAtUse"))
                                {
                                    if(item->Attribute("consumeAtUse")=="false")
                                        items.item[id].consumeAtUse=false;
                                    else
                                        items.item[id].consumeAtUse=true;
                                }
                                else
                                    items.item[id].consumeAtUse=true;
                            }
                            bool haveAnEffect=false;
                            //load the trap
                            if(!haveAnEffect)
                            {
                                const TiXmlElement * trapItem = item->FirstChildElement("trap");
                                if(trapItem!=NULL)
                                {
                                    if(trapItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        Trap trap;
                                        trap.bonus_rate=1.0;
                                        if(trapItem.hasAttribute("bonus_rate"))
                                        {
                                            float bonus_rate=trapItem->Attribute("bonus_rate").toFloat(&ok);
                                            if(ok)
                                                trap.bonus_rate=bonus_rate;
                                            else
                                                std::cerr << "Unable to open the file: bonus_rate is not a number, file: " << file << ", child->ValueStr(): " << trapItem->ValueStr() << " (at line: " << trapItem->Row() << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the file: trap have not the attribute bonus_rate, file: " << file << ", child->ValueStr(): " << trapItem->ValueStr() << " (at line: " << trapItem->Row() << ")" << std::endl;
                                        items.trap[id]=trap;
                                        haveAnEffect=true;
                                    }
                                }
                            }
                            //load the repel
                            if(!haveAnEffect)
                            {
                                const TiXmlElement * repelItem = item->FirstChildElement("repel");
                                if(repelItem!=NULL)
                                {
                                    if(repelItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        if(repelItem.hasAttribute("step"))
                                        {
                                            const uint32_t &step=stringtouint32(repelItem->Attribute("step"),&ok);
                                            if(ok)
                                            {
                                                if(step>0)
                                                {
                                                    items.repel[id]=step;
                                                    haveAnEffect=true;
                                                }
                                                else
                                                    std::cerr << "Unable to open the file: step is not greater than 0, file: " << file << ", child->ValueStr(): " << repelItem->ValueStr() << " (at line: " << repelItem->Row() << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the file: step is not a number, file: " << file << ", child->ValueStr(): " << repelItem->ValueStr() << " (at line: " << repelItem->Row() << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the file: repel have not the attribute step, file: " << file << ", child->ValueStr(): " << repelItem->ValueStr() << " (at line: " << repelItem->Row() << ")" << std::endl;
                                    }
                                }
                            }
                            //load the monster effect
                            if(!haveAnEffect)
                            {
                                {
                                    const TiXmlElement * hpItem = item->FirstChildElement("hp");
                                    while(hpItem!=NULL)
                                    {
                                        if(hpItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                        {
                                            if(hpItem.hasAttribute("add"))
                                            {
                                                if(hpItem->Attribute("add")=="all")
                                                {
                                                    MonsterItemEffect monsterItemEffect;
                                                    monsterItemEffect.type=MonsterItemEffectType_AddHp;
                                                    monsterItemEffect.value=-1;
                                                    items.monsterItemEffect[id].push_back(monsterItemEffect);
                                                }
                                                else
                                                {
                                                    if(!hpItem->Attribute("add").contains("%"))//todo this part
                                                    {
                                                        const int32_t &add=stringtouint32(hpItem->Attribute("add"),&ok);
                                                        if(ok)
                                                        {
                                                            if(add>0)
                                                            {
                                                                MonsterItemEffect monsterItemEffect;
                                                                monsterItemEffect.type=MonsterItemEffectType_AddHp;
                                                                monsterItemEffect.value=add;
                                                                items.monsterItemEffect[id].push_back(monsterItemEffect);
                                                            }
                                                            else
                                                                std::cerr << "Unable to open the file, add is not greater than 0, file: " << file << ", child->ValueStr(): " << hpItem->ValueStr() << " (at line: " << hpItem->Row() << ")" << std::endl;
                                                        }
                                                        else
                                                            std::cerr << "Unable to open the file, add is not a number, file: " << file << ", child->ValueStr(): " << hpItem->ValueStr() << " (at line: " << hpItem->Row() << ")" << std::endl;
                                                    }
                                                }
                                            }
                                            else
                                                std::cerr << "Unable to open the file, hp have not the attribute add, file: " << file << ", child->ValueStr(): " << hpItem->ValueStr() << " (at line: " << hpItem->Row() << ")" << std::endl;
                                        }
                                        hpItem = hpItem->NextSiblingElement("hp");
                                    }
                                }
                                #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                                {
                                    const TiXmlElement * buffItem = item->FirstChildElement("buff");
                                    while(buffItem!=NULL)
                                    {
                                        if(buffItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                        {
                                            if(buffItem.hasAttribute("remove"))
                                            {
                                                if(buffItem->Attribute("remove")=="all")
                                                {
                                                    MonsterItemEffect monsterItemEffect;
                                                    monsterItemEffect.type=MonsterItemEffectType_RemoveBuff;
                                                    monsterItemEffect.value=-1;
                                                    items.monsterItemEffect[id].push_back(monsterItemEffect);
                                                }
                                                else
                                                {
                                                    const int32_t &remove=stringtouint32(buffItem->Attribute("remove"),&ok);
                                                    if(ok)
                                                    {
                                                        if(remove>0)
                                                        {
                                                            if(monsterBuffs.find(remove)!=monsterBuffs.cend())
                                                            {
                                                                MonsterItemEffect monsterItemEffect;
                                                                monsterItemEffect.type=MonsterItemEffectType_RemoveBuff;
                                                                monsterItemEffect.value=remove;
                                                                items.monsterItemEffect[id].push_back(monsterItemEffect);
                                                            }
                                                            else
                                                                std::cerr << "Unable to open the file, buff item to remove is not found, file: " << file << ", child->ValueStr(): " << buffItem->ValueStr() << " (at line: " << buffItem->Row() << ")" << std::endl;
                                                        }
                                                        else
                                                            std::cerr << "Unable to open the file, step is not greater than 0, file: " << file << ", child->ValueStr(): " << buffItem->ValueStr() << " (at line: " << buffItem->Row() << ")" << std::endl;
                                                    }
                                                    else
                                                        std::cerr << "Unable to open the file, step is not a number, file: " << file << ", child->ValueStr(): " << buffItem->ValueStr() << " (at line: " << buffItem->Row() << ")" << std::endl;
                                                }
                                            }
                                            /// \todo
                                             /* else
                                                std::cerr << "Unable to open the file: " << file << ", buff have not the attribute know attribute like remove: child->ValueStr(): %2 (at line: %3)").arg(file).arg(buffItem->ValueStr()).arg(buffItem->Row());*/
                                        }
                                        buffItem = buffItem->NextSiblingElement("buff");
                                    }
                                }
                                #endif
                                if(items.monsterItemEffect.find(id)!=items.monsterItemEffect.cend())
                                    haveAnEffect=true;
                            }
                            //load the monster offline effect
                            if(!haveAnEffect)
                            {
                                const TiXmlElement * levelItem = item->FirstChildElement("level");
                                while(levelItem!=NULL)
                                {
                                    if(levelItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                    {
                                        if(levelItem.hasAttribute("up"))
                                        {
                                            const uint32_t &levelUp=stringtouint32(levelItem->Attribute("up"),&ok);
                                            if(!ok)
                                                std::cerr << "Unable to open the file, level up is not possitive number, file: " << file << ", child->ValueStr(): " << levelItem->ValueStr() << " (at line: " << levelItem->Row() << ")" << std::endl;
                                            else if(levelUp<=0)
                                                std::cerr << "Unable to open the file, level up is greater than 0, file: " << file << ", child->ValueStr(): " << levelItem->ValueStr() << " (at line: " << levelItem->Row() << ")" << std::endl;
                                            else
                                            {
                                                MonsterItemEffectOutOfFight monsterItemEffectOutOfFight;
                                                monsterItemEffectOutOfFight.type=MonsterItemEffectTypeOutOfFight_AddLevel;
                                                monsterItemEffectOutOfFight.value=levelUp;
                                                items.monsterItemEffectOutOfFight[id].push_back(monsterItemEffectOutOfFight);
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the file, level have not the attribute up, file: " << file << ", child->ValueStr(): " << levelItem->ValueStr() << " (at line: " << levelItem->Row() << ")" << std::endl;
                                    }
                                    levelItem = levelItem->NextSiblingElement("level");
                                }
                            }
                        }
                        else
                            std::cerr << "Unable to open the file, id number already set, file: " << file << ", child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file, id is not a number, file: " << file << ", child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the file, have not the item id, file: " << file << ", child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the file, is not an element, file: " << file << ", child->ValueStr(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
            item = item->NextSiblingElement("item");
        }
        file_index++;
    }
    return items;
}
#endif

std::vector<std::string> DatapackGeneralLoader::loadSkins(const std::string &folder)
{
    return FacilityLibGeneral::skinIdList(folder);
}

std::pair<std::vector<const TiXmlElement *>, std::vector<Profile> > DatapackGeneralLoader::loadProfileList(const std::string &datapackPath, const std::string &file,
                                                                                  #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                                                  const std::unordered_map<uint16_t, Item> &items,
                                                                                  #endif // CATCHCHALLENGER_CLASS_MASTER
                                                                                  const std::unordered_map<uint16_t,Monster> &monsters,const std::vector<Reputation> &reputations)
{
    std::unordered_set<std::string> idDuplicate;
    std::unordered_map<std::string,int> reputationNameToId;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    std::vector<std::string> defaultforcedskinList;
    std::unordered_map<std::string,uint8_t> skinNameToId;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.skins.size())
        {
            skinNameToId[CommonDatapack::commonDatapack.skins.at(index)]=index;
            defaultforcedskinList.push_back(CommonDatapack::commonDatapack.skins.at(index));
            index++;
        }
    }
    std::pair<std::vector<const TiXmlElement *>, std::vector<Profile> > returnVar;
    TiXmlDocument domDocument(file.c_str());
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="profile")
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    bool ok;
    const TiXmlElement * startItem = root->FirstChildElement("start");
    while(startItem!=NULL)
    {
        if(startItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            Profile profile;

            if(startItem.hasAttribute("id"))
                profile.id=startItem->Attribute("id");

            if(idDuplicate.find(profile.id)!=idDuplicate.cend())
            {
                std::cerr << "Unable to open the xml file: " << file << ", child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                startItem = startItem->NextSiblingElement("start");
                continue;
            }

            if(!profile.id.empty() && idDuplicate.find(profile.id)==idDuplicate.cend())
            {
                const TiXmlElement * forcedskin = startItem->FirstChildElement("forcedskin");

                std::vector<std::string> forcedskinList;
                if(forcedskin!=NULL && forcedskin->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT && forcedskin.hasAttribute("value"))
                    forcedskinList=stringsplit(forcedskin->Attribute("value"),';');
                else
                    forcedskinList=defaultforcedskinList;
                {
                    unsigned int index=0;
                    while(index<forcedskinList.size())
                    {
                        if(skinNameToId.find(forcedskinList.at(index))!=skinNameToId.cend())
                            profile.forcedskin.push_back(skinNameToId.at(forcedskinList.at(index)));
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", skin " << forcedskinList.at(index) << " don't exists: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        index++;
                    }
                }
                unsigned int index=0;
                while(index<profile.forcedskin.size())
                {
                    if(!QFile::exists(QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_SKIN+QString::fromStdString(CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index)))))
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", skin skin " << forcedskinList.at(index) << " don't exists into: into " << datapackPath << DATAPACK_BASE_PATH_SKIN << CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index)) << ": child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        profile.forcedskin.erase(profile.forcedskin.begin()+index);
                    }
                    else
                        index++;
                }

                profile.cash=0;
                const TiXmlElement * cash = startItem->FirstChildElement("cash");
                if(cash!=NULL && cash->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT && cash.hasAttribute("value"))
                {
                    profile.cash=cash->Attribute("value").toULongLong(&ok);
                    if(!ok)
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", cash is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        profile.cash=0;
                    }
                }
                const TiXmlElement * monstersElement = startItem->FirstChildElement("monster");
                while(monstersElement!=NULL)
                {
                    Profile::Monster monster;
                    if(monstersElement->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT && monstersElement.hasAttribute("id") && monstersElement.hasAttribute("level") && monstersElement.hasAttribute("captured_with"))
                    {
                        monster.id=stringtouint32(monstersElement->Attribute("id"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", monster id is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        if(ok)
                        {
                            monster.level=monstersElement->Attribute("level").toUShort(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", monster level is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        }
                        if(ok)
                        {
                            if(monster.level==0 || monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                                std::cerr << "Unable to open the xml file: " << file << ", monster level is not into the range: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        }
                        if(ok)
                        {
                            monster.captured_with=stringtouint32(monstersElement->Attribute("captured_with"),&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", captured_with is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        }
                        if(ok)
                        {
                            if(monsters.find(monster.id)==monsters.cend())
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", starter don't found the monster " << monster.id << ": child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                ok=false;
                            }
                        }
                        #ifndef CATCHCHALLENGER_CLASS_MASTER
                        if(ok)
                        {
                            if(items.find(monster.captured_with)==items.cend())
                                std::cerr << "Unable to open the xml file: " << file << ", starter don't found the monster capture item " << monster.id << ": child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        }
                        #endif // CATCHCHALLENGER_CLASS_MASTER
                        if(ok)
                            profile.monsters.push_back(monster);
                    }
                    monstersElement = monstersElement->NextSiblingElement("monster");
                }
                if(profile.monsters.empty())
                {
                    std::cerr << "Unable to open the xml file: " << file << ", not monster to load: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                    startItem = startItem->NextSiblingElement("start");
                    continue;
                }
                const TiXmlElement * reputationElement = startItem->FirstChildElement("reputation");
                while(reputationElement!=NULL)
                {
                    Profile::Reputation reputationTemp;
                    if(reputationElement->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT && reputationElement.hasAttribute("type") && reputationElement.hasAttribute("level"))
                    {
                        reputationTemp.level=reputationElement->Attribute("level").toShort(&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", reputation level is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        if(ok)
                        {
                            if(reputationNameToId.find(reputationElement->Attribute("type"))==reputationNameToId.cend())
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", reputation type not found " << reputationElement->Attribute("type") << ": child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                ok=false;
                            }
                            if(ok)
                            {
                                reputationTemp.reputationId=reputationNameToId.at(reputationElement->Attribute("type"));
                                if(reputationTemp.level==0)
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", reputation level is useless if level 0: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                    ok=false;
                                }
                                else if(reputationTemp.level<0)
                                {
                                    if((-reputationTemp.level)>(int32_t)reputations.at(reputationTemp.reputationId).reputation_negative.size())
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", reputation level is lower than minimal level for " << reputationElement->Attribute("type") << ": child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                        ok=false;
                                    }
                                }
                                else// if(reputationTemp.level>0)
                                {
                                    if((reputationTemp.level)>=(int32_t)reputations.at(reputationTemp.reputationId).reputation_positive.size())
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", reputation level is higther than maximal level for " << reputationElement->Attribute("type") << ": child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                        ok=false;
                                    }
                                }
                            }
                            if(ok)
                            {
                                reputationTemp.point=0;
                                if(reputationElement.hasAttribute("point"))
                                {
                                    reputationTemp.point=reputationElement->Attribute("point").toInt(&ok);
                                    std::cerr << "Unable to open the xml file: " << file << ", reputation point is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                    if(ok)
                                    {
                                        if((reputationTemp.point>0 && reputationTemp.level<0) || (reputationTemp.point<0 && reputationTemp.level>=0))
                                            std::cerr << "Unable to open the xml file: " << file << ", reputation point is not negative/positive like the level: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                    }
                                }
                            }
                        }
                        if(ok)
                            profile.reputation.push_back(reputationTemp);
                    }
                    reputationElement = reputationElement->NextSiblingElement("reputation");
                }
                const TiXmlElement * itemElement = startItem->FirstChildElement("item");
                while(itemElement!=NULL)
                {
                    Profile::Item itemTemp;
                    if(itemElement->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT && itemElement.hasAttribute("id"))
                    {
                        itemTemp.id=stringtouint32(itemElement->Attribute("id"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", item id is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                        if(ok)
                        {
                            itemTemp.quantity=0;
                            if(itemElement.hasAttribute("quantity"))
                            {
                                itemTemp.quantity=stringtouint32(itemElement->Attribute("quantity"),&ok);
                                if(!ok)
                                    std::cerr << "Unable to open the xml file: " << file << ", item quantity is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                if(ok)
                                {
                                    if(itemTemp.quantity==0)
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", item quantity is null: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                        ok=false;
                                    }
                                }
                                #ifndef CATCHCHALLENGER_CLASS_MASTER
                                if(ok)
                                {
                                    if(items.find(itemTemp.id)==items.cend())
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", item not found as know item " << itemTemp.id << ": child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                                        ok=false;
                                    }
                                }
                                #endif // CATCHCHALLENGER_CLASS_MASTER
                            }
                        }
                        if(ok)
                            profile.items.push_back(itemTemp);
                    }
                    itemElement = itemElement->NextSiblingElement("item");
                }
                idDuplicate.insert(profile.id);
                returnVar.second.push_back(profile);
                returnVar.first.push_back(startItem);
            }
        }
        else
            std::cerr << "Unable to open the xml file: " << file << ", is not an element: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
        startItem = startItem->NextSiblingElement("start");
    }
    return returnVar;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::vector<MonstersCollision> DatapackGeneralLoader::loadMonstersCollision(const std::string &file, const std::unordered_map<uint16_t, Item> &items,const std::vector<Event> &events)
{
    std::unordered_map<std::string,uint8_t> eventStringToId;
    std::unordered_map<std::string,std::unordered_map<std::string,uint8_t> > eventValueStringToId;
    {
        unsigned int index=0;
        unsigned int sub_index;
        while(index<events.size())
        {
            const Event &event=events.at(index);
            eventStringToId[event.name]=index;
            sub_index=0;
            while(sub_index<event.values.size())
            {
                eventValueStringToId[event.name][event.values.at(sub_index)]=sub_index;
                sub_index++;
            }
            index++;
        }
    }
    std::vector<MonstersCollision> returnVar;
    TiXmlDocument domDocument(file.c_str());
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="layers")
    {
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    bool ok;
    const TiXmlElement * monstersCollisionItem = root->FirstChildElement("monstersCollision");
    while(monstersCollisionItem!=NULL)
    {
        if(monstersCollisionItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            if(!monstersCollisionItem.hasAttribute("type"))
                std::cerr << "Have not the attribute type, into file: " << file << " at line " << monstersCollisionItem->Row() << std::endl;
            else if(!monstersCollisionItem.hasAttribute("monsterType"))
                std::cerr << "Have not the attribute monsterType, into: file: " << file << " at line " << monstersCollisionItem->Row() << std::endl;
            else
            {
                ok=true;
                MonstersCollision monstersCollision;
                if(monstersCollisionItem->Attribute("type")=="walkOn")
                    monstersCollision.type=MonstersCollisionType_WalkOn;
                else if(monstersCollisionItem->Attribute("type")=="actionOn")
                    monstersCollision.type=MonstersCollisionType_ActionOn;
                else
                {
                    ok=false;
                    std::cerr << "type is not walkOn or actionOn, into file: " << file << " at line " << monstersCollisionItem->Row() << std::endl;
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute("layer"))
                        monstersCollision.layer=monstersCollisionItem->Attribute("layer");
                }
                if(ok)
                {
                    if(monstersCollision.layer.empty() && monstersCollision.type!=MonstersCollisionType_WalkOn)//need specific layer name to do that's
                    {
                        ok=false;
                        std::cerr << "To have blocking layer by item, have specific layer name, into file: " << file << " at line " << monstersCollisionItem->Row() << std::endl;
                    }
                    else
                    {
                        monstersCollision.item=0;
                        if(monstersCollisionItem.hasAttribute("item"))
                        {
                            monstersCollision.item=stringtouint32(monstersCollisionItem->Attribute("item"),&ok);
                            if(!ok)
                                std::cerr << "item attribute is not a number, into file: " << file << " at line " << monstersCollisionItem->Row() << std::endl;
                            else if(items.find(monstersCollision.item)==items.cend())
                            {
                                ok=false;
                                std::cerr << "item is not into item list, into file: " << file << " at line " << monstersCollisionItem->Row() << std::endl;
                            }
                        }
                    }
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute("tile"))
                        monstersCollision.tile=monstersCollisionItem->Attribute("tile");
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute("background"))
                        monstersCollision.background=monstersCollisionItem->Attribute("background");
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute("monsterType"))
                    {
                        monstersCollision.defautMonsterTypeList=stringsplit(monstersCollisionItem->Attribute("monsterType"),';');
                        vectorRemoveEmpty(monstersCollision.defautMonsterTypeList);
                        vectorDuplicatesForSmallList(monstersCollision.defautMonsterTypeList);
                        monstersCollision.monsterTypeList=monstersCollision.defautMonsterTypeList;
                        //load the condition
                        const TiXmlElement * eventItem = monstersCollisionItem->FirstChildElement("event");
                        while(eventItem!=NULL)
                        {
                            if(eventItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                if(eventItem.hasAttribute("id") && eventItem.hasAttribute("value") && eventItem.hasAttribute("monsterType"))
                                {
                                    if(eventStringToId.find(eventItem->Attribute("id"))!=eventStringToId.cend())
                                    {
                                        if(eventValueStringToId.at(eventItem->Attribute("id")).find(eventItem->Attribute("value"))!=eventValueStringToId.at(eventItem->Attribute("id")).cend())
                                        {
                                            MonstersCollision::MonstersCollisionEvent event;
                                            event.event=eventStringToId.at(eventItem->Attribute("id"));
                                            event.event_value=eventValueStringToId.at(eventItem->Attribute("id")).at(eventItem->Attribute("value"));
                                            event.monsterTypeList=stringsplit(eventItem->Attribute("monsterType"),';');
                                            if(!event.monsterTypeList.empty())
                                            {
                                                monstersCollision.events.push_back(event);
                                                unsigned int index=0;
                                                while(index<event.monsterTypeList.size())
                                                {
                                                    if(!vectorcontainsAtLeastOne(monstersCollision.monsterTypeList,event.monsterTypeList.at(index)))
                                                        monstersCollision.monsterTypeList.push_back(event.monsterTypeList.at(index));
                                                    index++;
                                                }
                                            }
                                            else
                                                std::cerr << "monsterType is empty, into file: " << file << " at line " << eventItem->Row() << std::endl;
                                        }
                                        else
                                            std::cerr << "event value not found, into file: " << file << " at line " << eventItem->Row() << std::endl;
                                    }
                                    else
                                        std::cerr << "event not found, into file: " << file << " at line " << eventItem->Row() << std::endl;
                                }
                                else
                                    std::cerr << "event have missing attribute, into file: " << file << " at line " << eventItem->Row() << std::endl;
                            }
                            eventItem = eventItem->NextSiblingElement("event");
                        }
                    }
                }
                if(ok)
                {
                    unsigned int index=0;
                    while(index<returnVar.size())
                    {
                        if(returnVar.at(index).type==monstersCollision.type && returnVar.at(index).layer==monstersCollision.layer && returnVar.at(index).item==monstersCollision.item)
                        {
                            std::cerr << "similar monstersCollision previously found, into file: " << file << " at line " << monstersCollisionItem->Row() << std::endl;
                            ok=false;
                            break;
                        }
                        if(monstersCollision.type==MonstersCollisionType_WalkOn && returnVar.at(index).layer==monstersCollision.layer)
                        {
                            std::cerr << "You can't have different item for same layer in walkOn mode, into file: " << file << " at line " << monstersCollisionItem->Row() << std::endl;
                            ok=false;
                            break;
                        }
                        index++;
                    }
                }
                if(ok && !monstersCollision.monsterTypeList.empty())
                {
                    if(monstersCollision.type==MonstersCollisionType_WalkOn && monstersCollision.layer.empty() && monstersCollision.item==0)
                    {
                        if(returnVar.empty())
                            returnVar.push_back(monstersCollision);
                        else
                        {
                            returnVar.push_back(returnVar.back());
                            returnVar.front()=monstersCollision;
                        }
                    }
                    else
                        returnVar.push_back(monstersCollision);
                }
            }
        }
        monstersCollisionItem = monstersCollisionItem->NextSiblingElement("monstersCollision");
    }
    return returnVar;
}

LayersOptions DatapackGeneralLoader::loadLayersOptions(const std::string &file)
{
    LayersOptions returnVar;
    returnVar.zoom=2;
    TiXmlDocument domDocument(file.c_str());
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="layers")
    {
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found for reputation of the xml file" << std::endl;
        std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found for the xml file" << std::endl;
        return returnVar;
    }
    if(root->hasAttribute(QLatin1Literal("zoom")))
    {
        bool ok;
        returnVar.zoom=root->Attribute(QLatin1Literal("zoom")).toUShort(&ok);
        if(!ok)
        {
            returnVar.zoom=2;
            std::cerr << "Unable to open the xml file: " << file << ", zoom is not a number" << std::endl;
        }
        else
        {
            if(returnVar.zoom<1 || returnVar.zoom>4)
            {
                returnVar.zoom=2;
                std::cerr << "Unable to open the xml file: " << file << ", zoom out of range 1-4" << std::endl;
            }
        }
    }

    return returnVar;
}

std::vector<Event> DatapackGeneralLoader::loadEvents(const std::string &file)
{
    std::vector<Event> returnVar;

    TiXmlDocument domDocument(file.c_str());
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="events")
    {
        std::cerr << "Unable to open the file: " << file << ", \"events\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    const TiXmlElement * eventItem = root->FirstChildElement("event");
    while(eventItem!=NULL)
    {
        if(eventItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            if(!eventItem.hasAttribute("id"))
                std::cerr << "Have not the attribute id, into file: " << file << " at line " << eventItem->Row() << std::endl;
            else if(eventItem->Attribute("id").empty())
                std::cerr << "Have id empty, into file: " << file << " at line " << eventItem->Row() << std::endl;
            else
            {
                Event event;
                event.name=eventItem->Attribute("id");
                const TiXmlElement * valueItem = eventItem->FirstChildElement("value");
                while(valueItem!=NULL)
                {
                    if(valueItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                        event.values.push_back(valueItem.text());
                    valueItem = valueItem->NextSiblingElement("value");
                }
                if(!event.values.empty())
                    returnVar.push_back(event);
            }
        }
        eventItem = eventItem->NextSiblingElement("event");
    }
    return returnVar;
}

std::unordered_map<uint32_t,Shop> DatapackGeneralLoader::preload_shop(const std::string &file, const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<uint32_t,Shop> shops;

    TiXmlDocument domDocument(file.c_str());
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return shops;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="shops")
    {
        std::cerr << "Unable to open the file: " << file << ", \"shops\" root balise not found for reputation of the xml file" << std::endl;
        return shops;
    }

    //load the content
    bool ok;
    const TiXmlElement * shopItem = root->FirstChildElement("shop");
    while(shopItem!=NULL)
    {
        if(shopItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            if(shopItem.hasAttribute("id"))
            {
                uint32_t id=stringtouint32(shopItem->Attribute("id"),&ok);
                if(ok)
                {
                    if(shops.find(id)==shops.cend())
                    {
                        Shop shop;
                        const TiXmlElement * product = shopItem->FirstChildElement("product");
                        while(product!=NULL)
                        {
                            if(product->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                            {
                                if(product.hasAttribute("itemId"))
                                {
                                    uint32_t itemId=stringtouint32(product->Attribute("itemId"),&ok);
                                    if(!ok)
                                        std::cerr << "preload_shop() product attribute itemId is not a number for shops file: " << file << ", child->ValueStr(): " << shopItem->ValueStr() << " (at line: " << shopItem->Row() << ")" << std::endl;
                                    else
                                    {
                                        if(items.find(itemId)==items.cend())
                                            std::cerr << "preload_shop() product itemId in not into items list for shops file: " << file << ", child->ValueStr(): " << shopItem->ValueStr() << " (at line: " << shopItem->Row() << ")" << std::endl;
                                        else
                                        {
                                            uint32_t price=items.at(itemId).price;
                                            if(product.hasAttribute("overridePrice"))
                                            {
                                                price=stringtouint32(product->Attribute("overridePrice"),&ok);
                                                if(!ok)
                                                    price=items.at(itemId).price;
                                            }
                                            if(price==0)
                                                std::cerr << "preload_shop() item can't be into the shop with price == 0' for shops file: " << file << ", child->ValueStr(): " << shopItem->ValueStr() << " (at line: " << shopItem->Row() << ")" << std::endl;
                                            else
                                            {
                                                shop.prices.push_back(price);
                                                shop.items.push_back(itemId);
                                            }
                                        }
                                    }
                                }
                                else
                                    std::cerr << "preload_shop() material have not attribute itemId for shops file: " << file << ", child->ValueStr(): " << shopItem->ValueStr() << " (at line: " << shopItem->Row() << ")" << std::endl;
                            }
                            else
                                std::cerr << "preload_shop() material is not an element for shops file: " << file << ", child->ValueStr(): " << shopItem->ValueStr() << " (at line: " << shopItem->Row() << ")" << std::endl;
                            product = product->NextSiblingElement("product");
                        }
                        shops[id]=shop;
                    }
                    else
                        std::cerr << "Unable to open the shops file: " << file << ", child->ValueStr(): " << shopItem->ValueStr() << " (at line: " << shopItem->Row() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the shops file: " << file << ", id is not a number: child->ValueStr(): " << shopItem->ValueStr() << " (at line: " << shopItem->Row() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the shops file: " << file << ", have not the shops id: child->ValueStr(): " << shopItem->ValueStr() << " (at line: " << shopItem->Row() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the shops file: " << file << ", is not an element: child->ValueStr(): " << shopItem->ValueStr() << " (at line: " << shopItem->Row() << ")" << std::endl;
        shopItem = shopItem->NextSiblingElement("shop");
    }
    return shops;
}
#endif

std::vector<ServerProfile> DatapackGeneralLoader::loadServerProfileList(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file,const std::vector<Profile> &profileCommon)
{
    std::vector<ServerProfile> serverProfile=loadServerProfileListInternal(datapackPath,mainDatapackCode,file);
    //index of base profile
    std::unordered_set<std::string> profileId,serverProfileId;
    {
        unsigned int index=0;
        while(index<profileCommon.size())
        {
            //already deduplicated at loading
            profileId.insert(profileCommon.at(index).id);
            index++;
        }
    }
    //drop serverProfileList
    {
        unsigned int index=0;
        while(index<serverProfile.size())
        {
            if(profileId.find(serverProfile.at(index).id)!=profileId.cend())
            {
                serverProfileId.insert(serverProfile.at(index).id);
                index++;
            }
            else
            {
                std::cerr << "Profile xml file: " << file << ", found id \"" << serverProfile.at(index).id << "\" but not found in common, drop it" << std::endl;
                serverProfile.erase(serverProfile.begin()+index);
            }
        }
    }
    //add serverProfileList
    {
        unsigned int index=0;
        while(index<profileCommon.size())
        {
            if(serverProfileId.find(profileCommon.at(index).id)==serverProfileId.cend())
            {
                std::cerr << "Profile xml file: " << file << ", found common id \"" << profileCommon.at(index).id << "\" but not found in server, add it" << std::endl;
                ServerProfile serverProfileTemp;
                serverProfileTemp.id=profileCommon.at(index).id;
                serverProfileTemp.orientation=Orientation_bottom;
                serverProfileTemp.x=0;
                serverProfileTemp.y=0;
                serverProfile.push_back(serverProfileTemp);
            }
            index++;
        }
    }

    return serverProfile;
}

std::vector<ServerProfile> DatapackGeneralLoader::loadServerProfileListInternal(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file)
{
    std::unordered_set<std::string> idDuplicate;
    std::vector<ServerProfile> serverProfileList;

    TiXmlDocument domDocument(file.c_str());
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        const bool loadOkay=domDocument.LoadFile();
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument.ErrorRow() << ", column " << domDocument.ErrorCol() << ": " << domDocument.ErrorDesc() << std::endl;
            return serverProfileList;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const TiXmlElement * root = domDocument.RootElement();
    if(root->ValueStr()!="profile")
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found for reputation of the xml file" << std::endl;
        return serverProfileList;
    }

    //load the content
    bool ok;
    const TiXmlElement * startItem = root->FirstChildElement("start");
    while(startItem!=NULL)
    {
        if(startItem->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            ServerProfile serverProfile;
            serverProfile.orientation=Orientation_bottom;

            const TiXmlElement * map = startItem->FirstChildElement("map");
            if(map!=NULL && map->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT && map.hasAttribute("file") && map.hasAttribute("x") && map.hasAttribute("y"))
            {
                serverProfile.mapString=map->Attribute("file");
                if(!stringEndsWith(serverProfile.mapString,".tmx"))
                    serverProfile.mapString+=".tmx";
                if(!QFile::exists(QString::fromStdString(datapackPath+DATAPACK_BASE_PATH_MAPMAIN+mainDatapackCode+"/"+serverProfile.mapString)))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", map don't exists " << serverProfile.mapString << ": child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                    startItem = startItem->NextSiblingElement("start");
                    continue;
                }
                serverProfile.x=map->Attribute("x").toUShort(&ok);
                if(!ok)
                {
                    std::cerr << "Unable to open the xml file: " << file << ", map x is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                    startItem = startItem->NextSiblingElement("start");
                    continue;
                }
                serverProfile.y=map->Attribute("y").toUShort(&ok);
                if(!ok)
                {
                    std::cerr << "Unable to open the xml file: " << file << ", map y is not a number: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                    startItem = startItem->NextSiblingElement("start");
                    continue;
                }
            }
            else
            {
                std::cerr << "Unable to open the xml file: " << file << ", no correct map configuration: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                startItem = startItem->NextSiblingElement("start");
                continue;
            }

            if(startItem.hasAttribute("id"))
                serverProfile.id=startItem->Attribute("id");

            if(idDuplicate.find(serverProfile.id)!=idDuplicate.cend())
            {
                std::cerr << "Unable to open the xml file: " << file << ", id duplicate: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
                startItem = startItem->NextSiblingElement("start");
                continue;
            }

            if(!serverProfile.id.empty() && idDuplicate.find(serverProfile.id)==idDuplicate.cend())
            {
                idDuplicate.insert(serverProfile.id);
                serverProfileList.push_back(serverProfile);
            }
        }
        else
            std::cerr << "Unable to open the xml file: " << file << ", is not an element: child->ValueStr(): " << startItem->ValueStr() << " (at line: " << startItem->Row() << ")" << std::endl;
        startItem = startItem->NextSiblingElement("start");
    }

    return serverProfileList;
}
