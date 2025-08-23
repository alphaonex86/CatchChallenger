#include "DatapackGeneralLoader.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/tinyXML2/customtinyxml2.hpp"
#include <iostream>

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, Quest> DatapackGeneralLoader::loadQuests(const std::string &folder, const std::unordered_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId)
{
    bool ok;
    std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, Quest> quests;
    //open and quick check the file
    const std::vector<FacilityLibGeneral::InodeDescriptor> &fileList=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder,CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
    unsigned int index=0;
    while(index<fileList.size())
    {
        if(!CatchChallenger::FacilityLibGeneral::isFile(fileList.at(index).absoluteFilePath+"/definition.xml"))
        {
            index++;
            continue;
        }
        const uint16_t &questId=stringtouint16(fileList.at(index).name,&ok);
        if(ok)
        {
            //add it, all seam ok
            std::pair<bool,Quest> returnedQuest=loadSingleQuest(fileList.at(index).absoluteFilePath+"/definition.xml",mapPathToId);
            if(returnedQuest.first==true)
            {
                returnedQuest.second.id=questId;
                if(quests.find(returnedQuest.second.id)!=quests.cend())
                    std::cerr << "The quest with id: " << returnedQuest.second.id << " is already found, disable: " << fileList.at(index).absoluteFilePath << "/definition.xml" << std::endl;
                else
                    quests[returnedQuest.second.id]=returnedQuest.second;
            }
        }
        else
            std::cerr << "Unable to open the folder: " << fileList.at(index).absoluteFilePath << ", because is folder name is not a number" << std::endl;
        index++;
    }
    return quests;
}

std::pair<bool,Quest> DatapackGeneralLoader::loadSingleQuest(const std::string &file, const std::unordered_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId)
{
    std::unordered_map<std::string,uint8_t> reputationNameToId;
    {
        uint8_t index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            const Reputation &reputation=CommonDatapack::commonDatapack.reputation.at(index);
            reputationNameToId[reputation.name]=index;
            index++;
        }
    }
    CatchChallenger::Quest quest;
    quest.id=0;
    tinyxml2::XMLDocument *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new tinyxml2::XMLDocument();
        #endif
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return std::pair<bool,Quest>(false,quest);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return std::pair<bool,Quest>(false,quest);
    }
    if(root->Name()==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", \"quest\" root balise not found 2 for reputation of the xml file" << std::endl;
        return std::pair<bool,Quest>(false,quest);
    }
    if(strcmp(root->Name(),"quest")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"quest\" root balise not found for reputation of the xml file" << std::endl;
        return std::pair<bool,Quest>(false,quest);
    }

    //load the content
    bool ok;
    CATCHCHALLENGER_TYPE_BOTID botToTalkBotId=0;
    CATCHCHALLENGER_TYPE_MAPID botToTalkMapId=65535;
    quest.id=0;
    quest.repeatable=false;
    if(root->Attribute("repeatable")!=NULL)
        if(strcmp(root->Attribute("repeatable"),"yes")==0 ||
           strcmp(root->Attribute("repeatable"),"true")==0)
            quest.repeatable=true;
    if(root->Attribute("bot")!=NULL && root->Attribute("map")!=NULL)
    {
        const CATCHCHALLENGER_TYPE_BOTID tempInt=stringtouint8(root->Attribute("bot"),&ok);
        if(ok)
        {
            botToTalkBotId=tempInt;
            const std::string tempS(root->Attribute("map"));
            if(mapPathToId.find(tempS)!=mapPathToId.cend())
                botToTalkMapId=mapPathToId.at(tempS);
            else
            {
                std::cerr << "Unable to found the file: " << tempS << ", \"map\" into map list" << std::endl;
                return std::pair<bool,Quest>(false,quest);
            }
        }
        else
        {
            std::cerr << "Unable to open the file: " << file << ", \"bot\" is not a number" << std::endl;
            return std::pair<bool,Quest>(false,quest);
        }
    }
    else
    {
        std::cerr << "Unable to open the file: " << file << ", \"bot\" or \"map\" root balise not found for reputation of the xml file" << std::endl;
        return std::pair<bool,Quest>(false,quest);
    }

    //load requirements
    const tinyxml2::XMLElement * requirements = root->FirstChildElement("requirements");
    while(requirements!=NULL)
    {
        //load requirements reputation
        {
            const tinyxml2::XMLElement * requirementsItem = requirements->FirstChildElement("reputation");
            while(requirementsItem!=NULL)
            {
                if(requirementsItem->Attribute("type")!=NULL && requirementsItem->Attribute("level")!=NULL)
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
                            std::cerr << "Unable to open the file: " << file << ", reputation is not a number " << stringLevel << ": child->Name(): " << requirementsItem->Name() << std::endl;
                    }
                    else
                        std::cerr << "Has not the attribute: " << requirementsItem->Attribute("type")
                                  << ", reputation not found: child->Name(): " << requirementsItem->Name() << std::endl;
                }
                else
                    std::cerr << "Has not the attribute: type level, have not attribute type or level: child->Name(): " << requirementsItem->Name() << std::endl;
                requirementsItem = requirementsItem->NextSiblingElement("reputation");
            }
        }
        //load requirements quest
        {
            const tinyxml2::XMLElement * requirementsItem = requirements->FirstChildElement("quest");
            while(requirementsItem!=NULL)
            {
                if(requirementsItem->Attribute("id")!=NULL)
                {
                    const uint16_t &questId=stringtouint16(requirementsItem->Attribute("id"),&ok);
                    if(ok)
                    {
                        QuestRequirements questNewEntry;
                        questNewEntry.quest=questId;
                        questNewEntry.inverse=false;
                        if(requirementsItem->Attribute("inverse")!=NULL)
                            if(strcmp(requirementsItem->Attribute("inverse"),"true")==0)
                                questNewEntry.inverse=true;
                        quest.requirements.quests.push_back(questNewEntry);
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", requirement quest item id is not a number " <<
                                     requirementsItem->Attribute("id") << ": child->Name(): " << requirementsItem->Name() << std::endl;
                }
                else
                    std::cerr << "Has attribute: %1, requirement quest item have not id attribute: child->Name(): " << requirementsItem->Name() << std::endl;
                requirementsItem = requirementsItem->NextSiblingElement("quest");
            }
        }
        requirements = requirements->NextSiblingElement("requirements");
    }

    //load rewards
    const tinyxml2::XMLElement * rewards = root->FirstChildElement("rewards");
    while(rewards!=NULL)
    {
        //load rewards reputation
        {
            const tinyxml2::XMLElement * reputationItem = rewards->FirstChildElement("reputation");
            while(reputationItem!=NULL)
            {
                if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("point")!=NULL)
                {
                    if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                    {
                        const int32_t &point=stringtoint32(reputationItem->Attribute("point"),&ok);
                        if(ok)
                        {
                            CatchChallenger::ReputationRewards reputation;
                            reputation.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                            reputation.point=point;
                            quest.rewards.reputation.push_back(reputation);
                        }
                        else
                            std::cerr << "Unable to open the file: " << file << ", quest rewards point is not a number: child->Name(): " << reputationItem->Name() << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", quest rewards point is not a number: child->Name(): " << reputationItem->Name() << std::endl;
                }
                else
                    std::cerr << "Has not the attribute: type/point, quest rewards point have not type or point attribute: child->Name(): " << reputationItem->Name() << std::endl;
                reputationItem = reputationItem->NextSiblingElement("reputation");
            }
        }
        //load rewards item
        {
            const tinyxml2::XMLElement * rewardsItem = rewards->FirstChildElement("item");
            while(rewardsItem!=NULL)
            {
                if(rewardsItem->Attribute("id")!=NULL)
                {
                    CatchChallenger::Quest::Item item;
                    item.item=stringtouint16(rewardsItem->Attribute("id"),&ok);
                    item.quantity=1;
                    if(ok)
                    {
                        if(CommonDatapack::commonDatapack.items.item.find(item.item)==CommonDatapack::commonDatapack.items.item.cend())
                        {
                            std::cerr << "Unable to open the file: " << file << ", rewards item id is not into the item list: "
                                      << rewardsItem->Attribute("id") << ": child->Name(): " << rewardsItem->Name() << std::endl;
                            return std::pair<bool,Quest>(false,quest);
                        }
                        if(rewardsItem->Attribute("quantity")!=NULL)
                        {
                            item.quantity=stringtouint32(rewardsItem->Attribute("quantity"),&ok);
                            if(!ok)
                                item.quantity=1;
                        }
                        quest.rewards.items.push_back(item);
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", rewards item id is not a number: "
                                  << rewardsItem->Attribute("id") << ": child->Name(): " << rewardsItem->Name() << std::endl;
                }
                else
                    std::cerr << "Has not the attribute: id, rewards item have not attribute id: child->Name(): " << rewardsItem->Name() << std::endl;
                rewardsItem = rewardsItem->NextSiblingElement("item");
            }
        }
        //load rewards allow
        {
            const tinyxml2::XMLElement * allowItem = rewards->FirstChildElement("allow");
            while(allowItem!=NULL)
            {
                if(allowItem->Attribute("type")!=NULL)
                {
                    if(strcmp(allowItem->Attribute("type"),"clan")==0)
                        quest.rewards.allow.push_back(CatchChallenger::ActionAllow_Clan);
                    else
                        std::cerr << "Unable to open the file: " << file << ", allow type not understand " <<
                                     allowItem->Attribute("id") << ": child->Name(): " << allowItem->Name() << std::endl;
                }
                else
                    std::cerr << "Has attribute: %1, rewards item have not attribute id: child->Name(): " << allowItem->Name() << std::endl;
                allowItem = allowItem->NextSiblingElement("allow");
            }
        }
        rewards = rewards->NextSiblingElement("rewards");
    }

    std::unordered_map<uint8_t,CatchChallenger::Quest::Step> steps;
    //load step
    const tinyxml2::XMLElement * step = root->FirstChildElement("step");
    while(step!=NULL)
    {
        int16_t id=1;
        if(step->Attribute("id")!=NULL)
        {
            id=stringtouint8(step->Attribute("id"),&ok);
            if(!ok)
            {
                std::cerr << "Unable to open the file: " << file << ", step id is not a number: child->Name(): " << step->Name() << std::endl;
                id=-1;
            }
        }
        if(id>=0)
        {
            CatchChallenger::Quest::Step stepObject;
            if(step->Attribute("bot")!=NULL && step->Attribute("map")!=NULL)
            {
                const CATCHCHALLENGER_TYPE_BOTID &tempInt=stringtouint8(step->Attribute("bot"),&ok);
                if(ok)
                {
                    std::string tempS(step->Attribute("map"));
                    if(mapPathToId.find(tempS)!=mapPathToId.cend())
                    {
                        stepObject.botToTalkBotId=tempInt;
                        stepObject.botToTalkMapId=mapPathToId.at(tempS);
                    }
                    else
                        std::cerr << "Unable to found the file: " << tempS << ", \"map\" into map list" << std::endl;
                }
                else
                    std::cerr << "Unable to open the file: " << file << ", step bot count is not number: " << step->Attribute("bot") << std::endl;
            }
            else
            {
                stepObject.botToTalkBotId=botToTalkBotId;
                stepObject.botToTalkMapId=botToTalkMapId;
            }
            //do the item
            {
                const tinyxml2::XMLElement * stepItem = step->FirstChildElement("item");
                while(stepItem!=NULL)
                {
                    if(stepItem->Attribute("id")!=NULL)
                    {
                        CatchChallenger::Quest::Item item;
                        item.item=stringtouint16(stepItem->Attribute("id"),&ok);
                        item.quantity=1;
                        if(ok)
                        {
                            if(CommonDatapack::commonDatapack.items.item.find(item.item)==CommonDatapack::commonDatapack.items.item.cend())
                            {
                                std::cerr << "Unable to open the file: " << file << ", rewards item id is not into the item list: "
                                          << stepItem->Attribute("id") << ": child->Name(): " << stepItem->Name() << std::endl;
                                return std::pair<bool,Quest>(false,quest);
                            }
                            if(stepItem->Attribute("quantity")!=NULL)
                            {
                                item.quantity=stringtouint32(stepItem->Attribute("quantity"),&ok);
                                if(!ok)
                                    item.quantity=1;
                            }
                            stepObject.requirements.items.push_back(item);
                            if(stepItem->Attribute("monster")!=NULL && stepItem->Attribute("rate")!=NULL)
                            {
                                CatchChallenger::Quest::ItemMonster itemMonster;
                                itemMonster.item=item.item;

                                const std::vector<std::string> &tempStringList=stringsplit(stepItem->Attribute("monster"),';');
                                unsigned int index=0;
                                while(index<tempStringList.size())
                                {
                                    const uint16_t &tempInt=stringtouint16(tempStringList.at(index),&ok);
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
                            std::cerr << "Unable to open the file: " << file << ", step id is not a number " << stepItem->Attribute("id") << ": child->Name(): " << step->Name() << std::endl;
                    }
                    else
                        std::cerr << "Has not the attribute: id, step have not id attribute: child->Name(): " << step->Name() << std::endl;
                    stepItem = stepItem->NextSiblingElement("item");
                }
            }
            //do the fight
            {
                const tinyxml2::XMLElement * fightItem = step->FirstChildElement("fight");
                while(fightItem!=NULL)
                {
                    if(fightItem->Attribute("bot")!=NULL && fightItem->Attribute("map")!=NULL)
                    {
                        const CATCHCHALLENGER_TYPE_BOTID &fightId=stringtouint8(fightItem->Attribute("bot"),&ok);
                        if(ok)
                        {
                            std::string tempS(fightItem->Attribute("map"));
                            if(mapPathToId.find(tempS)!=mapPathToId.cend())
                                stepObject.requirements.fights[mapPathToId.at(tempS)].insert(fightId);
                            else
                                std::cerr << "Unable to found the file: " << tempS << ", \"map\" into map list" << std::endl;
                        }
                        else
                            std::cerr << "Unable to open the file: " << file << ", step id is not a number "
                                      << fightItem->Attribute("id") << ": child->Name(): " << fightItem->Name() << std::endl;
                    }
                    else
                        std::cerr << "Has attribute: %1, step have not id attribute: child->Name(): " << fightItem->Name() << std::endl;
                    fightItem = fightItem->NextSiblingElement("fight");
                }
            }
            steps[id]=stepObject;
        }
        step = step->NextSiblingElement("step");
    }

    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif

    //sort the step
    uint8_t indexLoop=1;
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
#endif
