#include "DatapackGeneralLoader.h"
#include "GeneralVariable.h"
#include "CommonDatapack.h"
#include "CommonSettingsServer.h"
#include "FacilityLib.h"
#include "FacilityLibGeneral.h"
#include "cpp11addition.h"

#include <vector>
#include <iostream>

using namespace CatchChallenger;

std::vector<Reputation> DatapackGeneralLoader::loadReputation(const std::string &file)
{
    std::regex excludeFilterRegex("[\"']");
    std::regex typeRegex("^[a-z]{1,32}$");
    std::vector<Reputation> reputation;
    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return reputation;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return reputation;
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"reputations"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"reputations\" root balise not found for reputation of the xml file" << std::endl;
        return reputation;
    }

    //load the content
    bool ok;
    const CATCHCHALLENGER_XMLELEMENT * item = root->FirstChildElement("reputation");
    while(item!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(item))
        {
            if(item->Attribute("type")!=NULL)
            {
                std::vector<int32_t> point_list_positive,point_list_negative;
                std::vector<std::string> text_positive,text_negative;
                const CATCHCHALLENGER_XMLELEMENT * level = item->FirstChildElement("level");
                ok=true;
                while(level!=NULL && ok)
                {
                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(level))
                    {
                        if(level->Attribute("point")!=NULL)
                        {
                            const int32_t &point=stringtoint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(level->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("point"))),&ok);
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
                                            std::cerr << "Unable to open the file: " << file << ", reputation level with same number of point found!: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
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
                                            std::cerr << "Unable to open the file: " << file << ", reputation level with same number of point found!: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
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
                                std::cerr << "Unable to open the file: " << file << ", point is not number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                        }
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", point attribute not found: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                    level = level->NextSiblingElement("level");
                }
                std::sort(point_list_positive.begin(),point_list_positive.end());
                std::sort(point_list_negative.begin(),point_list_negative.end());
                std::reverse(point_list_negative.begin(),point_list_negative.end());
                if(ok)
                    if(point_list_positive.size()<2)
                    {
                        std::cerr << "Unable to open the file: " << file << ", reputation have to few level: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(!vectorcontainsAtLeastOne(point_list_positive,0))
                    {
                        std::cerr << "Unable to open the file: " << file << ", no starting level for the positive: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(!point_list_negative.empty() && !vectorcontainsAtLeastOne(point_list_negative,-1))
                    {
                        //std::cerr << "Unable to open the file: " << file << ", no starting level for the negative, first level need start with -1, fix by change range: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
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
                {
                    const std::string &type=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(item->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type")));
                    if(!std::regex_match(type,typeRegex))
                    {
                        std::cerr << "Unable to open the file: " << file << ", the type " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(item->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))) << " don't match wiuth the regex: ^[a-z]{1,32}$: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                        ok=false;
                    }
                    else
                    {
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
            }
            else
                std::cerr << "Unable to open the file: " << file << ", have not the item id: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
        item = item->NextSiblingElement("reputation");
    }

    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return reputation;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::unordered_map<uint16_t, Quest> DatapackGeneralLoader::loadQuests(const std::string &folder)
{
    bool ok;
    std::unordered_map<uint16_t, Quest> quests;
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
        const uint32_t &questId=stringtouint32(fileList.at(index).name,&ok);
        if(ok)
        {
            //add it, all seam ok
            std::pair<bool,Quest> returnedQuest=loadSingleQuest(fileList.at(index).absoluteFilePath+"/definition.xml");
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
    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return std::pair<bool,Quest>(false,quest);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return std::pair<bool,Quest>(false,quest);
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"quest"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"quest\" root balise not found for reputation of the xml file" << std::endl;
        return std::pair<bool,Quest>(false,quest);
    }

    //load the content
    bool ok;
    std::vector<uint16_t> defaultBots;
    quest.id=0;
    quest.repeatable=false;
    if(root->Attribute("repeatable")!=NULL)
        if(CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(root->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("repeatable"))),CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("yes"))
            || CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(root->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("repeatable"))),CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("true"))
           )
            quest.repeatable=true;
    if(root->Attribute("bot")!=NULL)
    {
        const std::vector<std::string> &tempStringList=stringsplit(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(root->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("bot"))),';');
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
    const CATCHCHALLENGER_XMLELEMENT * requirements = root->FirstChildElement("requirements");
    while(requirements!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(requirements))
        {
            //load requirements reputation
            {
                const CATCHCHALLENGER_XMLELEMENT * requirementsItem = requirements->FirstChildElement("reputation");
                while(requirementsItem!=NULL)
                {
                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(requirementsItem))
                    {
                        if(requirementsItem->Attribute("type")!=NULL && requirementsItem->Attribute("level")!=NULL)
                        {
                            if(reputationNameToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(requirementsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))))!=reputationNameToId.cend())
                            {
                                std::string stringLevel=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(requirementsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("level")));
                                bool positif=!stringStartWith(stringLevel,"-");
                                if(!positif)
                                    stringLevel.erase(0,1);
                                uint8_t level=stringtouint8(stringLevel,&ok);
                                if(ok)
                                {
                                    CatchChallenger::ReputationRequirements reputation;
                                    reputation.level=level;
                                    reputation.positif=positif;
                                    reputation.reputationId=reputationNameToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(requirementsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))));
                                    quest.requirements.reputation.push_back(reputation);
                                }
                                else
                                    std::cerr << "Unable to open the file: " << file << ", reputation is not a number " << stringLevel << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << requirementsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(requirementsItem) << ")" << std::endl;
                            }
                            else
                                std::cerr << "Has not the attribute: " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(requirementsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))) << ", reputation not found: child->CATCHCHALLENGER_XMLELENTVALUE(): " << requirementsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(requirementsItem) << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has not the attribute: type level, have not attribute type or level: child->CATCHCHALLENGER_XMLELENTVALUE(): " << requirementsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(requirementsItem) << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << requirementsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(requirementsItem) << ")" << std::endl;
                    requirementsItem = requirementsItem->NextSiblingElement("reputation");
                }
            }
            //load requirements quest
            {
                const CATCHCHALLENGER_XMLELEMENT * requirementsItem = requirements->FirstChildElement("quest");
                while(requirementsItem!=NULL)
                {
                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(requirementsItem))
                    {
                        if(requirementsItem->Attribute("id")!=NULL)
                        {
                            const uint32_t &questId=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(requirementsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                            if(ok)
                            {
                                QuestRequirements questNewEntry;
                                questNewEntry.quest=questId;
                                questNewEntry.inverse=false;
                                if(requirementsItem->Attribute("inverse")!=NULL)
                                    if(CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(requirementsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("inverse"))),"true"))
                                        questNewEntry.inverse=true;
                                quest.requirements.quests.push_back(questNewEntry);
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", requirement quest item id is not a number " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(requirementsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << requirementsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(requirementsItem) << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has attribute: %1, requirement quest item have not id attribute: child->CATCHCHALLENGER_XMLELENTVALUE(): " << requirementsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(requirementsItem) << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << requirementsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(requirementsItem) << ")" << std::endl;
                    requirementsItem = requirementsItem->NextSiblingElement("quest");
                }
            }
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << requirements->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(requirements) << ")" << std::endl;
        requirements = requirements->NextSiblingElement("requirements");
    }

    //load rewards
    const CATCHCHALLENGER_XMLELEMENT * rewards = root->FirstChildElement("rewards");
    while(rewards!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(rewards))
        {
            //load rewards reputation
            {
                const CATCHCHALLENGER_XMLELEMENT * reputationItem = rewards->FirstChildElement("reputation");
                while(reputationItem!=NULL)
                {
                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(reputationItem))
                    {
                        if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("point")!=NULL)
                        {
                            if(reputationNameToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))))!=reputationNameToId.cend())
                            {
                                const int32_t &point=stringtoint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("point"))),&ok);
                                if(ok)
                                {
                                    CatchChallenger::ReputationRewards reputation;
                                    reputation.reputationId=reputationNameToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))));
                                    reputation.point=point;
                                    quest.rewards.reputation.push_back(reputation);
                                }
                                else
                                    std::cerr << "Unable to open the file: " << file << ", quest rewards point is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", quest rewards point is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has not the attribute: type/point, quest rewards point have not type or point attribute: child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                    reputationItem = reputationItem->NextSiblingElement("reputation");
                }
            }
            //load rewards item
            {
                const CATCHCHALLENGER_XMLELEMENT * rewardsItem = rewards->FirstChildElement("item");
                while(rewardsItem!=NULL)
                {
                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(rewardsItem))
                    {
                        if(rewardsItem->Attribute("id")!=NULL)
                        {
                            CatchChallenger::Quest::Item item;
                            item.item=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(rewardsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                            item.quantity=1;
                            if(ok)
                            {
                                if(CommonDatapack::commonDatapack.items.item.find(item.item)==CommonDatapack::commonDatapack.items.item.cend())
                                {
                                    std::cerr << "Unable to open the file: " << file << ", rewards item id is not into the item list: " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(rewardsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << rewardsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(rewardsItem) << ")" << std::endl;
                                    return std::pair<bool,Quest>(false,quest);
                                }
                                if(rewardsItem->Attribute("quantity")!=NULL)
                                {
                                    item.quantity=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(rewardsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("quantity"))),&ok);
                                    if(!ok)
                                        item.quantity=1;
                                }
                                quest.rewards.items.push_back(item);
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", rewards item id is not a number: " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(rewardsItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << rewardsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(rewardsItem) << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has not the attribute: id, rewards item have not attribute id: child->CATCHCHALLENGER_XMLELENTVALUE(): " << rewardsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(rewardsItem) << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << rewardsItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(rewardsItem) << ")" << std::endl;
                    rewardsItem = rewardsItem->NextSiblingElement("item");
                }
            }
            //load rewards allow
            {
                const CATCHCHALLENGER_XMLELEMENT * allowItem = rewards->FirstChildElement("allow");
                while(allowItem!=NULL)
                {
                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(allowItem))
                    {
                        if(allowItem->Attribute("type")!=NULL)
                        {
                            if(CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(allowItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))),"clan"))
                                quest.rewards.allow.push_back(CatchChallenger::ActionAllow_Clan);
                            else
                                std::cerr << "Unable to open the file: " << file << ", allow type not understand " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(allowItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << allowItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(allowItem) << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has attribute: %1, rewards item have not attribute id: child->CATCHCHALLENGER_XMLELENTVALUE(): " << allowItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(allowItem) << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << allowItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(allowItem) << ")" << std::endl;
                    allowItem = allowItem->NextSiblingElement("allow");
                }
            }
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << rewards->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(rewards) << ")" << std::endl;
        rewards = rewards->NextSiblingElement("rewards");
    }

    std::unordered_map<uint8_t,CatchChallenger::Quest::Step> steps;
    //load step
    const CATCHCHALLENGER_XMLELEMENT * step = root->FirstChildElement("step");
    while(step!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(step))
        {
            if(step->Attribute("id")!=NULL)
            {
                const uint32_t &id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(step->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                if(ok)
                {
                    CatchChallenger::Quest::Step stepObject;
                    if(step->Attribute("bot")!=NULL)
                    {
                        const std::vector<std::string> &tempStringList=stringsplit(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(step->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("bot"))),';');
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
                        const CATCHCHALLENGER_XMLELEMENT * stepItem = step->FirstChildElement("item");
                        while(stepItem!=NULL)
                        {
                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(stepItem))
                            {
                                if(stepItem->Attribute("id")!=NULL)
                                {
                                    CatchChallenger::Quest::Item item;
                                    item.item=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(stepItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                                    item.quantity=1;
                                    if(ok)
                                    {
                                        if(CommonDatapack::commonDatapack.items.item.find(item.item)==CommonDatapack::commonDatapack.items.item.cend())
                                        {
                                            std::cerr << "Unable to open the file: " << file << ", rewards item id is not into the item list: " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(stepItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << stepItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(stepItem) << ")" << std::endl;
                                            return std::pair<bool,Quest>(false,quest);
                                        }
                                        if(stepItem->Attribute("quantity")!=NULL)
                                        {
                                            item.quantity=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(stepItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("quantity"))),&ok);
                                            if(!ok)
                                                item.quantity=1;
                                        }
                                        stepObject.requirements.items.push_back(item);
                                        if(stepItem->Attribute("monster")!=NULL && stepItem->Attribute("rate")!=NULL)
                                        {
                                            CatchChallenger::Quest::ItemMonster itemMonster;
                                            itemMonster.item=item.item;

                                            const std::vector<std::string> &tempStringList=stringsplit(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(stepItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("monster"))),';');
                                            unsigned int index=0;
                                            while(index<tempStringList.size())
                                            {
                                                const uint32_t &tempInt=stringtouint32(tempStringList.at(index),&ok);
                                                if(ok)
                                                    itemMonster.monsters.push_back(tempInt);
                                                index++;
                                            }

                                            std::string rateString=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(stepItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("rate")));
                                            stringreplaceOne(rateString,"%","");
                                            itemMonster.rate=stringtouint8(rateString,&ok);
                                            if(ok)
                                                stepObject.itemsMonster.push_back(itemMonster);
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the file: " << file << ", step id is not a number " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(stepItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << step->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(step) << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Has not the attribute: id, step have not id attribute: child->CATCHCHALLENGER_XMLELENTVALUE(): " << step->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(step) << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << step->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(step) << ")" << std::endl;
                            stepItem = stepItem->NextSiblingElement("item");
                        }
                    }
                    //do the fight
                    {
                        const CATCHCHALLENGER_XMLELEMENT * fightItem = step->FirstChildElement("fight");
                        while(fightItem!=NULL)
                        {
                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(fightItem))
                            {
                                if(fightItem->Attribute("id")!=NULL)
                                {
                                    const uint32_t &fightId=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(fightItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                                    if(ok)
                                        stepObject.requirements.fightId.push_back(fightId);
                                    else
                                        std::cerr << "Unable to open the file: " << file << ", step id is not a number " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(fightItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << fightItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(fightItem) << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Has attribute: %1, step have not id attribute: child->CATCHCHALLENGER_XMLELENTVALUE(): " << fightItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(fightItem) << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << fightItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(fightItem) << ")" << std::endl;
                            fightItem = fightItem->NextSiblingElement("fight");
                        }
                    }
                    steps[id]=stepObject;
                }
                else
                    std::cerr << "Unable to open the file: " << file << ", step id is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << step->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(step) << ")" << std::endl;
            }
            else
                std::cerr << "Has attribute: %1, step have not id attribute: child->CATCHCHALLENGER_XMLELENTVALUE(): " << step->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(step) << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << step->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(step) << ")" << std::endl;
        step = step->NextSiblingElement("step");
    }

    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif

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
    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return plants;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return plants;
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"plants"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"plants\" root balise not found for reputation of the xml file" << std::endl;
        return plants;
    }

    //load the content
    bool ok,ok2;
    const CATCHCHALLENGER_XMLELEMENT * plantItem = root->FirstChildElement("plant");
    while(plantItem!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(plantItem))
        {
            if(plantItem->Attribute("id")!=NULL && plantItem->Attribute("itemUsed")!=NULL)
            {
                const uint8_t &id=stringtouint8(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(plantItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                const uint32_t &itemUsed=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(plantItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("itemUsed"))),&ok2);
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
                            const CATCHCHALLENGER_XMLELEMENT * requirementsItem = plantItem->FirstChildElement("requirements");
                            if(requirementsItem!=NULL && CATCHCHALLENGER_XMLELENTISXMLELEMENT(requirementsItem))
                            {
                                const CATCHCHALLENGER_XMLELEMENT * reputationItem = requirementsItem->FirstChildElement("reputation");
                                while(reputationItem!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(reputationItem))
                                    {
                                        if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("level")!=NULL)
                                        {
                                            if(reputationNameToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))))!=reputationNameToId.cend())
                                            {
                                                ReputationRequirements reputationRequirements;
                                                std::string stringLevel=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("level")));
                                                reputationRequirements.positif=!stringStartWith(stringLevel,"-");
                                                if(!reputationRequirements.positif)
                                                    stringLevel.erase(0,1);
                                                reputationRequirements.level=stringtouint8(stringLevel,&ok);
                                                if(ok)
                                                {
                                                    reputationRequirements.reputationId=reputationNameToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))));
                                                    plant.requirements.reputation.push_back(reputationRequirements);
                                                }
                                            }
                                            else
                                                std::cerr << "Reputation type not found: " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))) << ", have not the id, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                    reputationItem = reputationItem->NextSiblingElement("reputation");
                                }
                            }
                        }
                        {
                            const CATCHCHALLENGER_XMLELEMENT * rewardsItem = plantItem->FirstChildElement("rewards");
                            if(rewardsItem!=NULL && CATCHCHALLENGER_XMLELENTISXMLELEMENT(rewardsItem))
                            {
                                const CATCHCHALLENGER_XMLELEMENT * reputationItem = rewardsItem->FirstChildElement("reputation");
                                while(reputationItem!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(reputationItem))
                                    {
                                        if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("point")!=NULL)
                                        {
                                            if(reputationNameToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))))!=reputationNameToId.cend())
                                            {
                                                ReputationRewards reputationRewards;
                                                reputationRewards.point=stringtoint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("point"))),&ok);
                                                if(ok)
                                                {
                                                    reputationRewards.reputationId=reputationNameToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))));
                                                    plant.rewards.reputation.push_back(reputationRewards);
                                                }
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                    reputationItem = reputationItem->NextSiblingElement("reputation");
                                }
                            }
                        }
                        ok=false;
                        const CATCHCHALLENGER_XMLELEMENT * quantity = plantItem->FirstChildElement("quantity");
                        if(quantity!=NULL)
                        {
                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(quantity))
                            {
                                const float &float_quantity=stringtofloat(quantity->GetText(),&ok2);
                                const int &integer_part=float_quantity;
                                float random_part=float_quantity-integer_part;
                                random_part*=RANDOM_FLOAT_PART_DIVIDER;
                                plant.fix_quantity=integer_part;
                                plant.random_quantity=random_part;
                                ok=ok2;
                            }
                        }
                        int intermediateTimeCount=0;
                        const CATCHCHALLENGER_XMLELEMENT * grow = plantItem->FirstChildElement("grow");
                        if(grow!=NULL)
                        {
                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(grow))
                            {
                                const CATCHCHALLENGER_XMLELEMENT * fruits = grow->FirstChildElement("fruits");
                                if(fruits!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(fruits))
                                    {
                                        plant.fruits_seconds=stringtouint32(fruits->GetText(),&ok2)*60;
                                        plant.sprouted_seconds=plant.fruits_seconds;
                                        plant.taller_seconds=plant.fruits_seconds;
                                        plant.flowering_seconds=plant.fruits_seconds;
                                        if(!ok2)
                                        {
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << fruits->GetText() << " child->CATCHCHALLENGER_XMLELENTVALUE(): " << fruits->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(fruits) << ")" << std::endl;
                                            ok=false;
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "Unable to parse the plants file: " << file << ", fruits is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << fruits->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(fruits) << ")" << std::endl;
                                    }
                                }
                                else
                                {
                                    ok=false;
                                    std::cerr << "Unable to parse the plants file: " << file << ", fruits is null: child->CATCHCHALLENGER_XMLELENTVALUE(): " << grow->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(grow) << ")" << std::endl;
                                }
                                const CATCHCHALLENGER_XMLELEMENT * sprouted = grow->FirstChildElement("sprouted");
                                if(sprouted!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(sprouted))
                                    {
                                        plant.sprouted_seconds=stringtouint32(sprouted->GetText(),&ok2)*60;
                                        if(!ok2)
                                        {
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << sprouted->GetText() << " child->CATCHCHALLENGER_XMLELENTVALUE(): " << sprouted->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(sprouted) << ")" << std::endl;
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << sprouted->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(sprouted) << ")" << std::endl;
                                }
                                const CATCHCHALLENGER_XMLELEMENT * taller = grow->FirstChildElement("taller");
                                if(taller!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(taller))
                                    {
                                        plant.taller_seconds=stringtouint32(taller->GetText(),&ok2)*60;
                                        if(!ok2)
                                        {
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << taller->GetText() << " child->CATCHCHALLENGER_XMLELENTVALUE(): " << taller->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(taller) << ")" << std::endl;
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        std::cerr << "Unable to parse the plants file: " << file << ", taller is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << taller->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(taller) << ")" << std::endl;
                                }
                                const CATCHCHALLENGER_XMLELEMENT * flowering = grow->FirstChildElement("flowering");
                                if(flowering!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(flowering))
                                    {
                                        plant.flowering_seconds=stringtouint32(flowering->GetText(),&ok2)*60;
                                        if(!ok2)
                                        {
                                            ok=false;
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << flowering->GetText() << " child->CATCHCHALLENGER_XMLELENTVALUE(): " << flowering->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(flowering) << ")" << std::endl;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        std::cerr << "Unable to parse the plants file: " << file << ", flowering is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << flowering->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(flowering) << ")" << std::endl;
                                }
                            }
                            else
                                std::cerr << "Unable to parse the plants file: " << file << ", grow is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): child->CATCHCHALLENGER_XMLELENTVALUE(): " << grow->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(grow) << ")" << std::endl;
                        }
                        else
                            std::cerr << "Unable to parse the plants file: " << file << ", grow is null: child->CATCHCHALLENGER_XMLELENTVALUE(): child->CATCHCHALLENGER_XMLELENTVALUE(): " << plantItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(plantItem) << ")" << std::endl;
                        if(ok)
                        {
                            bool needIntermediateTimeFix=false;
                            if(plant.flowering_seconds>=plant.fruits_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", flowering_seconds>=fruits_seconds: child->CATCHCHALLENGER_XMLELENTVALUE(): child->CATCHCHALLENGER_XMLELENTVALUE(): " << grow->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(grow) << ")" << std::endl;
                            }
                            if(plant.taller_seconds>=plant.flowering_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", taller_seconds>=flowering_seconds: child->CATCHCHALLENGER_XMLELENTVALUE(): " << grow->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(grow) << ")" << std::endl;
                            }
                            if(plant.sprouted_seconds>=plant.taller_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", sprouted_seconds>=taller_seconds: child->CATCHCHALLENGER_XMLELENTVALUE(): " << grow->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(grow) << ")" << std::endl;
                            }
                            if(plant.sprouted_seconds<=0)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", sprouted_seconds<=0: child->CATCHCHALLENGER_XMLELENTVALUE(): " << grow->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(grow) << ")" << std::endl;
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
                        std::cerr << "Unable to open the plants file: " << file << ", id number already set: child->CATCHCHALLENGER_XMLELENTVALUE(): " << plantItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(plantItem) << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the plants file: " << file << ", id is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << plantItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(plantItem) << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the plants file: " << file << ", have not the plant id: child->CATCHCHALLENGER_XMLELENTVALUE(): " << plantItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(plantItem) << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the plants file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << plantItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(plantItem) << ")" << std::endl;
        plantItem = plantItem->NextSiblingElement("plant");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return plants;
}

std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> > DatapackGeneralLoader::loadCraftingRecipes(const std::string &file,const std::unordered_map<uint16_t, Item> &items,uint16_t &crafingRecipesMaxId)
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
    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"recipes"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"recipes\" root balise not found for reputation of the xml file" << std::endl;
        return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
    }

    //load the content
    bool ok,ok2,ok3;
    const CATCHCHALLENGER_XMLELEMENT * recipeItem = root->FirstChildElement("recipe");
    while(recipeItem!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(recipeItem))
        {
            if(recipeItem->Attribute("id")!=NULL && recipeItem->Attribute("itemToLearn")!=NULL && recipeItem->Attribute("doItemId")!=NULL)
            {
                uint8_t success=100;
                if(recipeItem->Attribute("success")!=NULL)
                {
                    const uint8_t &tempShort=stringtouint8(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(recipeItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("success"))),&ok);
                    if(ok)
                    {
                        if(tempShort>100)
                            std::cerr << "preload_crafting_recipes() success can't be greater than 100 for crafting recipe file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                        else
                            success=tempShort;
                    }
                    else
                        std::cerr << "preload_crafting_recipes() success in not an number for crafting recipe file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                }
                uint16_t quantity=1;
                if(recipeItem->Attribute("quantity")!=NULL)
                {
                    const uint32_t &tempShort=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(recipeItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("quantity"))),&ok);
                    if(ok)
                    {
                        if(tempShort>65535)
                            std::cerr << "preload_crafting_recipes() quantity can't be greater than 65535 for crafting recipe file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                        else
                            quantity=tempShort;
                    }
                    else
                        std::cerr << "preload_crafting_recipes() quantity in not an number for crafting recipe file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                }

                const uint32_t &id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(recipeItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                const uint32_t &itemToLearn=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(recipeItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("itemToLearn"))),&ok2);
                const uint32_t &doItemId=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(recipeItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("doItemId"))),&ok3);
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
                            const CATCHCHALLENGER_XMLELEMENT * requirementsItem = recipeItem->FirstChildElement("requirements");
                            if(requirementsItem!=NULL && CATCHCHALLENGER_XMLELENTISXMLELEMENT(requirementsItem))
                            {
                                const CATCHCHALLENGER_XMLELEMENT * reputationItem = requirementsItem->FirstChildElement("reputation");
                                while(reputationItem!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(reputationItem))
                                    {
                                        if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("level")!=NULL)
                                        {
                                            if(reputationNameToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))))!=reputationNameToId.cend())
                                            {
                                                ReputationRequirements reputationRequirements;
                                                std::string stringLevel=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("level")));
                                                reputationRequirements.positif=!stringStartWith(stringLevel,"-");
                                                if(!reputationRequirements.positif)
                                                    stringLevel.erase(0,1);
                                                reputationRequirements.level=stringtouint8(stringLevel,&ok);
                                                if(ok)
                                                {
                                                    reputationRequirements.reputationId=reputationNameToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))));
                                                    recipe.requirements.reputation.push_back(reputationRequirements);
                                                }
                                            }
                                            else
                                                std::cerr << "Reputation type not found: " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))) << ", have not the id, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                    reputationItem = reputationItem->NextSiblingElement("reputation");
                                }
                            }
                        }
                        {
                            const CATCHCHALLENGER_XMLELEMENT * rewardsItem = recipeItem->FirstChildElement("rewards");
                            if(rewardsItem!=NULL && CATCHCHALLENGER_XMLELENTISXMLELEMENT(rewardsItem))
                            {
                                const CATCHCHALLENGER_XMLELEMENT * reputationItem = rewardsItem->FirstChildElement("reputation");
                                while(reputationItem!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(reputationItem))
                                    {
                                        if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("point")!=NULL)
                                        {
                                            if(reputationNameToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))))!=reputationNameToId.cend())
                                            {
                                                ReputationRewards reputationRewards;
                                                reputationRewards.point=stringtoint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("point"))),&ok);
                                                if(ok)
                                                {
                                                    reputationRewards.reputationId=reputationNameToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))));
                                                    recipe.rewards.reputation.push_back(reputationRewards);
                                                }
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                    reputationItem = reputationItem->NextSiblingElement("reputation");
                                }
                            }
                        }
                        const CATCHCHALLENGER_XMLELEMENT * material = recipeItem->FirstChildElement("material");
                        while(material!=NULL && ok)
                        {
                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(material))
                            {
                                if(material->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("itemId"))!=NULL)
                                {
                                    const uint32_t &itemId=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(material->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("itemId"))),&ok2);
                                    if(!ok2)
                                    {
                                        ok=false;
                                        std::cerr << "preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << material->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(material) << ")" << std::endl;
                                        break;
                                    }
                                    uint16_t quantity=1;
                                    if(material->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("quantity"))!=NULL)
                                    {
                                        const uint32_t &tempShort=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(material->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("quantity"))),&ok2);
                                        if(ok2)
                                        {
                                            if(tempShort>65535)
                                            {
                                                ok=false;
                                                std::cerr << "preload_crafting_recipes() material quantity can't be greater than 65535 for crafting recipe file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << material->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(material) << ")" << std::endl;
                                                break;
                                            }
                                            else
                                                quantity=tempShort;
                                        }
                                        else
                                        {
                                            ok=false;
                                            std::cerr << "preload_crafting_recipes() material quantity in not an number for crafting recipe file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << material->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(material) << ")" << std::endl;
                                            break;
                                        }
                                    }
                                    if(items.find(itemId)==items.cend())
                                    {
                                        ok=false;
                                        std::cerr << "preload_crafting_recipes() material itemId in not into items list for crafting recipe file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << material->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(material) << ")" << std::endl;
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
                                        std::cerr << "id of item already into resource or product list: %1: child->CATCHCHALLENGER_XMLELENTVALUE(): " << material->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(material) << ")" << std::endl;
                                    }
                                    else
                                    {
                                        if(recipe.doItemId==newMaterial.item)
                                        {
                                            std::cerr << "id of item already into resource or product list: %1: child->CATCHCHALLENGER_XMLELENTVALUE(): " << material->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(material) << ")" << std::endl;
                                            ok=false;
                                        }
                                        else
                                            recipe.materials.push_back(newMaterial);
                                    }
                                }
                                else
                                    std::cerr << "preload_crafting_recipes() material have not attribute itemId for crafting recipe file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << material->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(material) << ")" << std::endl;
                            }
                            else
                                std::cerr << "preload_crafting_recipes() material is not an element for crafting recipe file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << material->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(material) << ")" << std::endl;
                            material = material->NextSiblingElement("material");
                        }
                        if(ok)
                        {
                            if(items.find(recipe.itemToLearn)==items.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() itemToLearn is not into items list for crafting recipe file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(items.find(recipe.doItemId)==items.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() doItemId is not into items list for crafting recipe file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.find(recipe.itemToLearn)!=itemToCrafingRecipes.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() itemToLearn already used to learn another recipe: " << itemToCrafingRecipes.at(recipe.doItemId) << ": file: " << file << " child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(recipe.itemToLearn==recipe.doItemId)
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() the product of the recipe can't be them self: " << id << ": file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.find(recipe.doItemId)!=itemToCrafingRecipes.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() the product of the recipe can't be a recipe: " << itemToCrafingRecipes.at(recipe.doItemId) << ": file: " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(crafingRecipesMaxId<id)
                                crafingRecipesMaxId=id;
                            crafingRecipes[id]=recipe;
                            itemToCrafingRecipes[recipe.itemToLearn]=id;
                        }
                    }
                    else
                        std::cerr << "Unable to open the crafting recipe file: " << file << ", id number already set: child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the crafting recipe file: " << file << ", id is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the crafting recipe file: " << file << ", have not the crafting recipe id: child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the crafting recipe file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << recipeItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(recipeItem) << ")" << std::endl;
        recipeItem = recipeItem->NextSiblingElement("recipe");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
}

std::unordered_map<uint16_t,Industry> DatapackGeneralLoader::loadIndustries(const std::string &folder,const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<uint16_t,Industry> industries;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &fileList=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    unsigned int file_index=0;
    while(file_index<fileList.size())
    {
        const std::string &file=fileList.at(file_index).absoluteFilePath;
        CATCHCHALLENGER_XMLDOCUMENT *domDocument;
        //open and quick check the file
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        else
        {
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
            #else
            domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
            #endif
            const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
            if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
            {
                std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
        }
        #endif
        const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
        if(root==NULL)
        {
            std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
            file_index++;
            continue;
        }
        if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"industries"))
        {
            std::cerr << "Unable to open the file: " << file << ", \"industries\" root balise not found for reputation of the xml file" << std::endl;
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2,ok3;
        const CATCHCHALLENGER_XMLELEMENT * industryItem = root->FirstChildElement("industrialrecipe");
        while(industryItem!=NULL)
        {
            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(industryItem))
            {
                if(industryItem->Attribute("id")!=NULL && industryItem->Attribute("time")!=NULL && industryItem->Attribute("cycletobefull")!=NULL)
                {
                    Industry industry;
                    const uint32_t &id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(industryItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                    industry.time=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(industryItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("time"))),&ok2);
                    industry.cycletobefull=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(industryItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("cycletobefull"))),&ok3);
                    if(ok && ok2 && ok3)
                    {
                        if(industries.find(id)==industries.cend())
                        {
                            if(industry.time<60*5)
                            {
                                std::cerr << "the time need be greater than 5*60 seconds to not slow down the server: " << industry.time << ", " << file << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                industry.time=60*5;
                            }
                            if(industry.cycletobefull<1)
                            {
                                std::cerr << "cycletobefull need be greater than 0: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                industry.cycletobefull=1;
                            }
                            else if(industry.cycletobefull>65535)
                            {
                                std::cerr << "cycletobefull need be lower to 10 to not slow down the server, use the quantity: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                industry.cycletobefull=10;
                            }
                            //resource
                            {
                                const CATCHCHALLENGER_XMLELEMENT * resourceItem = industryItem->FirstChildElement("resource");
                                ok=true;
                                while(resourceItem!=NULL && ok)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(resourceItem))
                                    {
                                        Industry::Resource resource;
                                        resource.quantity=1;
                                        if(resourceItem->Attribute("quantity")!=NULL)
                                        {
                                            resource.quantity=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(resourceItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("quantity"))),&ok);
                                            if(!ok)
                                                std::cerr << "quantity is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                        }
                                        if(ok)
                                        {
                                            if(resourceItem->Attribute("id")!=NULL)
                                            {
                                                resource.item=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(resourceItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                                                if(!ok)
                                                    std::cerr << "id is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                                else if(items.find(resource.item)==items.cend())
                                                {
                                                    ok=false;
                                                    std::cerr << "id is not into the item list: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
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
                                                        std::cerr << "id of item already into resource or product list: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
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
                                                            std::cerr << "id of item already into resource or product list: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                                        }
                                                        else
                                                            industry.resources.push_back(resource);
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                std::cerr << "have not the id attribute: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "is not a elements: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                    }
                                    resourceItem = resourceItem->NextSiblingElement("resource");
                                }
                            }

                            //product
                            if(ok)
                            {
                                const CATCHCHALLENGER_XMLELEMENT * productItem = industryItem->FirstChildElement("product");
                                ok=true;
                                while(productItem!=NULL && ok)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(productItem))
                                    {
                                        Industry::Product product;
                                        product.quantity=1;
                                        if(productItem->Attribute("quantity")!=NULL)
                                        {
                                            product.quantity=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(productItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("quantity"))),&ok);
                                            if(!ok)
                                                std::cerr << "quantity is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                        }
                                        if(ok)
                                        {
                                            if(productItem->Attribute("id")!=NULL)
                                            {
                                                product.item=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(productItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                                                if(!ok)
                                                    std::cerr << "id is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                                else if(items.find(product.item)==items.cend())
                                                {
                                                    ok=false;
                                                    std::cerr << "id is not into the item list: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
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
                                                        std::cerr << "id of item already into resource or product list: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
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
                                                            std::cerr << "id of item already into resource or product list: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                                        }
                                                        else
                                                            industry.products.push_back(product);
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                std::cerr << "have not the id attribute: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "is not a elements: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                    }
                                    productItem = productItem->NextSiblingElement("product");
                                }
                            }

                            //add
                            if(ok)
                            {
                                if(industry.products.empty() || industry.resources.empty())
                                    std::cerr << "product or resources is empty: child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                                else
                                    industries[id]=industry;
                            }
                        }
                        else
                            std::cerr << "Unable to open the industries id number already set: file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the industries id is not a number: file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the industries have not the id: file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the industries is not an element: file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << industryItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(industryItem) << ")" << std::endl;
            industryItem = industryItem->NextSiblingElement("industrialrecipe");
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
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
    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return industriesLink;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return industriesLink;
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"industries"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"industries\" root balise not found for reputation of the xml file" << std::endl;
        return industriesLink;
    }

    //load the content
    bool ok,ok2;
    const CATCHCHALLENGER_XMLELEMENT * linkItem = root->FirstChildElement("link");
    while(linkItem!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(linkItem))
        {
            if(linkItem->Attribute("industrialrecipe")!=NULL && linkItem->Attribute("industry")!=NULL)
            {
                const uint32_t &industry_id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(linkItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("industrialrecipe"))),&ok);
                const uint32_t &factory_id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(linkItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("industry"))),&ok2);
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
                                    const CATCHCHALLENGER_XMLELEMENT * requirementsItem = linkItem->FirstChildElement("requirements");
                                    if(requirementsItem!=NULL && CATCHCHALLENGER_XMLELENTISXMLELEMENT(requirementsItem))
                                    {
                                        const CATCHCHALLENGER_XMLELEMENT * reputationItem = requirementsItem->FirstChildElement("reputation");
                                        while(reputationItem!=NULL)
                                        {
                                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(reputationItem))
                                            {
                                                if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("level")!=NULL)
                                                {
                                                    if(reputationNameToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))))!=reputationNameToId.cend())
                                                    {
                                                        ReputationRequirements reputationRequirements;
                                                        std::string stringLevel=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("level")));
                                                        reputationRequirements.positif=!stringStartWith(stringLevel,"-");
                                                        if(!reputationRequirements.positif)
                                                            stringLevel.erase(0,1);
                                                        reputationRequirements.level=stringtouint8(stringLevel,&ok);
                                                        if(ok)
                                                        {
                                                            reputationRequirements.reputationId=reputationNameToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))));
                                                            industryLink->requirements.reputation.push_back(reputationRequirements);
                                                        }
                                                    }
                                                    else
                                                        std::cerr << "Reputation type not found: have not the id, child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                                }
                                                else
                                                    std::cerr << "Unable to open the industries link have not the id, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the industries link is not a element, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                            reputationItem = reputationItem->NextSiblingElement("reputation");
                                        }
                                    }
                                }
                                {
                                    const CATCHCHALLENGER_XMLELEMENT * rewardsItem = linkItem->FirstChildElement("rewards");
                                    if(rewardsItem!=NULL && CATCHCHALLENGER_XMLELENTISXMLELEMENT(rewardsItem))
                                    {
                                        const CATCHCHALLENGER_XMLELEMENT * reputationItem = rewardsItem->FirstChildElement("reputation");
                                        while(reputationItem!=NULL)
                                        {
                                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(reputationItem))
                                            {
                                                if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("point")!=NULL)
                                                {
                                                    if(reputationNameToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))))!=reputationNameToId.cend())
                                                    {
                                                        ReputationRewards reputationRewards;
                                                        reputationRewards.point=stringtoint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("point"))),&ok);
                                                        if(ok)
                                                        {
                                                            reputationRewards.reputationId=reputationNameToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))));
                                                            industryLink->rewards.reputation.push_back(reputationRewards);
                                                        }
                                                    }
                                                }
                                                else
                                                    std::cerr << "Unable to open the industries link have not the id, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the industries link is not a element, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << reputationItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(reputationItem) << ")" << std::endl;
                                            reputationItem = reputationItem->NextSiblingElement("reputation");
                                        }
                                    }
                                }
                            }
                        }
                        else
                            std::cerr << "Industry id for factory is not found: " << industry_id << ", file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << linkItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(linkItem) << ")" << std::endl;
                    }
                    else
                        std::cerr << "Factory already found: " << factory_id << ", file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << linkItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(linkItem) << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the industries link the attribute is not a number, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << linkItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(linkItem) << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the industries link have not the id, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << linkItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(linkItem) << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the industries link is not a element, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << linkItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(linkItem) << ")" << std::endl;
        linkItem = linkItem->NextSiblingElement("link");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return industriesLink;
}

ItemFull DatapackGeneralLoader::loadItems(const std::string &folder,const std::unordered_map<uint8_t,Buff> &monsterBuffs)
{
    #ifdef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    (void)monsterBuffs;
    #endif
    ItemFull items;
    items.itemMaxId=0;
    const std::vector<std::string> &fileList=FacilityLibGeneral::listFolder(folder+'/');
    unsigned int file_index=0;
    while(file_index<fileList.size())
    {
        if(!stringEndsWith(fileList.at(file_index),".xml"))
        {
            file_index++;
            continue;
        }
        const std::string &file=folder+fileList.at(file_index);
        CATCHCHALLENGER_XMLDOCUMENT *domDocument;
        //open and quick check the file
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        else
        {
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
            #else
            domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
            #endif
            const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
            if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
            {
                std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
        }
        #endif
        const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
        if(root==NULL)
        {
            file_index++;
            continue;
        }
        if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"items"))
        {
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        const CATCHCHALLENGER_XMLELEMENT * item = root->FirstChildElement("item");
        while(item!=NULL)
        {
            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(item))
            {
                if(item->Attribute("id")!=NULL)
                {
                    const uint32_t &id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(item->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                    if(ok)
                    {
                        if(items.item.find(id)==items.item.cend())
                        {
                            if(items.itemMaxId<id)
                                items.itemMaxId=id;
                            //load the price
                            {
                                if(item->Attribute("price")!=NULL)
                                {
                                    bool ok;
                                    items.item[id].price=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(item->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("price"))),&ok);
                                    if(!ok)
                                    {
                                        std::cerr << "price is not a number, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                                        items.item[id].price=0;
                                    }
                                }
                                else
                                {
                                    /*if(!item->Attribute("quest")!=NULL || item->Attribute("quest")!="yes")
                                        std::cerr << "For parse item: Price not found, default to 0 (not sellable): child->CATCHCHALLENGER_XMLELENTVALUE(): %1 (%2 at line: %3)").arg(item->CATCHCHALLENGER_XMLELENTVALUE()).arg(file).arg(CATCHCHALLENGER_XMLELENTATLINE(item));*/
                                    items.item[id].price=0;
                                }
                            }
                            //load the consumeAtUse
                            {
                                if(item->Attribute("consumeAtUse")!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(item->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("consumeAtUse"))),"false"))
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
                                const CATCHCHALLENGER_XMLELEMENT * trapItem = item->FirstChildElement("trap");
                                if(trapItem!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(trapItem))
                                    {
                                        Trap trap;
                                        trap.bonus_rate=1.0;
                                        if(trapItem->Attribute("bonus_rate")!=NULL)
                                        {
                                            float bonus_rate=stringtofloat(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(trapItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("bonus_rate"))),&ok);
                                            if(ok)
                                                trap.bonus_rate=bonus_rate;
                                            else
                                                std::cerr << "Unable to open the file: bonus_rate is not a number, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << trapItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(trapItem) << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the file: trap have not the attribute bonus_rate, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << trapItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(trapItem) << ")" << std::endl;
                                        items.trap[id]=trap;
                                        haveAnEffect=true;
                                    }
                                }
                            }
                            //load the repel
                            if(!haveAnEffect)
                            {
                                const CATCHCHALLENGER_XMLELEMENT * repelItem = item->FirstChildElement("repel");
                                if(repelItem!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(repelItem))
                                    {
                                        if(repelItem->Attribute("step")!=NULL)
                                        {
                                            const uint32_t &step=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(repelItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("step"))),&ok);
                                            if(ok)
                                            {
                                                if(step>0)
                                                {
                                                    items.repel[id]=step;
                                                    haveAnEffect=true;
                                                }
                                                else
                                                    std::cerr << "Unable to open the file: step is not greater than 0, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << repelItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(repelItem) << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the file: step is not a number, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << repelItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(repelItem) << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the file: repel have not the attribute step, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << repelItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(repelItem) << ")" << std::endl;
                                    }
                                }
                            }
                            //load the monster effect
                            if(!haveAnEffect)
                            {
                                {
                                    const CATCHCHALLENGER_XMLELEMENT * hpItem = item->FirstChildElement("hp");
                                    while(hpItem!=NULL)
                                    {
                                        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(hpItem))
                                        {
                                            if(hpItem->Attribute("add")!=NULL)
                                            {
                                                if(CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(hpItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("add"))),"all"))
                                                {
                                                    MonsterItemEffect monsterItemEffect;
                                                    monsterItemEffect.type=MonsterItemEffectType_AddHp;
                                                    monsterItemEffect.value=-1;
                                                    items.monsterItemEffect[id].push_back(monsterItemEffect);
                                                }
                                                else
                                                {
                                                    std::string addString=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(hpItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("add")));
                                                    if(addString.find("%")==std::string::npos)//todo this part
                                                    {
                                                        const int32_t &add=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(hpItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("add"))),&ok);
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
                                                                std::cerr << "Unable to open the file, add is not greater than 0, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << hpItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(hpItem) << ")" << std::endl;
                                                        }
                                                        else
                                                            std::cerr << "Unable to open the file, add is not a number, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << hpItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(hpItem) << ")" << std::endl;
                                                    }
                                                }
                                            }
                                            else
                                                std::cerr << "Unable to open the file, hp have not the attribute add, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << hpItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(hpItem) << ")" << std::endl;
                                        }
                                        hpItem = hpItem->NextSiblingElement("hp");
                                    }
                                }
                                #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                                {
                                    const CATCHCHALLENGER_XMLELEMENT * buffItem = item->FirstChildElement("buff");
                                    while(buffItem!=NULL)
                                    {
                                        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(buffItem))
                                        {
                                            if(buffItem->Attribute("remove")!=NULL)
                                            {
                                                if(CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(buffItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("remove"))),"all"))
                                                {
                                                    MonsterItemEffect monsterItemEffect;
                                                    monsterItemEffect.type=MonsterItemEffectType_RemoveBuff;
                                                    monsterItemEffect.value=-1;
                                                    items.monsterItemEffect[id].push_back(monsterItemEffect);
                                                }
                                                else
                                                {
                                                    const int32_t &remove=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(buffItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("remove"))),&ok);
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
                                                                std::cerr << "Unable to open the file, buff item to remove is not found, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << buffItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(buffItem) << ")" << std::endl;
                                                        }
                                                        else
                                                            std::cerr << "Unable to open the file, step is not greater than 0, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << buffItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(buffItem) << ")" << std::endl;
                                                    }
                                                    else
                                                        std::cerr << "Unable to open the file, step is not a number, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << buffItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(buffItem) << ")" << std::endl;
                                                }
                                            }
                                            /// \todo
                                             /* else
                                                std::cerr << "Unable to open the file: " << file << ", buff have not the attribute know attribute like remove: child->CATCHCHALLENGER_XMLELENTVALUE(): %2 (at line: %3)").arg(file).arg(buffItem->CATCHCHALLENGER_XMLELENTVALUE()).arg(CATCHCHALLENGER_XMLELENTATLINE(buffItem));*/
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
                                const CATCHCHALLENGER_XMLELEMENT * levelItem = item->FirstChildElement("level");
                                while(levelItem!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(levelItem))
                                    {
                                        if(levelItem->Attribute("up")!=NULL)
                                        {
                                            const uint32_t &levelUp=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(levelItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("up"))),&ok);
                                            if(!ok)
                                                std::cerr << "Unable to open the file, level up is not possitive number, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << levelItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(levelItem) << ")" << std::endl;
                                            else if(levelUp<=0)
                                                std::cerr << "Unable to open the file, level up is greater than 0, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << levelItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(levelItem) << ")" << std::endl;
                                            else
                                            {
                                                MonsterItemEffectOutOfFight monsterItemEffectOutOfFight;
                                                monsterItemEffectOutOfFight.type=MonsterItemEffectTypeOutOfFight_AddLevel;
                                                monsterItemEffectOutOfFight.value=levelUp;
                                                items.monsterItemEffectOutOfFight[id].push_back(monsterItemEffectOutOfFight);
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the file, level have not the attribute up, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << levelItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(levelItem) << ")" << std::endl;
                                    }
                                    levelItem = levelItem->NextSiblingElement("level");
                                }
                            }
                        }
                        else
                            std::cerr << "Unable to open the file, id number already set, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file, id is not a number, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the file, have not the item id, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the file, is not an element, file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
            item = item->NextSiblingElement("item");
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        file_index++;
    }
    return items;
}
#endif

std::vector<std::string> DatapackGeneralLoader::loadSkins(const std::string &folder)
{
    return FacilityLibGeneral::skinIdList(folder);
}

std::pair<std::vector<const CATCHCHALLENGER_XMLELEMENT *>, std::vector<Profile> > DatapackGeneralLoader::loadProfileList(const std::string &datapackPath, const std::string &file,
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
    std::pair<std::vector<const CATCHCHALLENGER_XMLELEMENT *>, std::vector<Profile> > returnVar;
    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return returnVar;
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"profile"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    bool ok;
    const CATCHCHALLENGER_XMLELEMENT * startItem = root->FirstChildElement("start");
    while(startItem!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(startItem))
        {
            Profile profile;
            profile.cash=0;

            if(startItem->Attribute("id")!=NULL)
                profile.databaseId=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(startItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id")));

            if(idDuplicate.find(profile.databaseId)!=idDuplicate.cend())
            {
                std::cerr << "Unable to open the xml file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                startItem = startItem->NextSiblingElement("start");
                continue;
            }

            if(!profile.databaseId.empty() && idDuplicate.find(profile.databaseId)==idDuplicate.cend())
            {
                const CATCHCHALLENGER_XMLELEMENT * forcedskin = startItem->FirstChildElement("forcedskin");

                std::vector<std::string> forcedskinList;
                if(forcedskin!=NULL && CATCHCHALLENGER_XMLELENTISXMLELEMENT(forcedskin) && forcedskin->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("value"))!=NULL)
                    forcedskinList=stringsplit(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(forcedskin->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("value"))),';');
                else
                    forcedskinList=defaultforcedskinList;
                {
                    unsigned int index=0;
                    while(index<forcedskinList.size())
                    {
                        if(skinNameToId.find(forcedskinList.at(index))!=skinNameToId.cend())
                            profile.forcedskin.push_back(skinNameToId.at(forcedskinList.at(index)));
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", skin " << forcedskinList.at(index) << " don't exists: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                        index++;
                    }
                }
                unsigned int index=0;
                while(index<profile.forcedskin.size())
                {
                    if(!CatchChallenger::FacilityLibGeneral::isDir(datapackPath+DATAPACK_BASE_PATH_SKIN+CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index))))
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", skin value: " << forcedskinList.at(index) << " don't exists into: into " << datapackPath << DATAPACK_BASE_PATH_SKIN << CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index)) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                        profile.forcedskin.erase(profile.forcedskin.begin()+index);
                    }
                    else
                        index++;
                }

                profile.cash=0;
                const CATCHCHALLENGER_XMLELEMENT * cash = startItem->FirstChildElement("cash");
                if(cash!=NULL && CATCHCHALLENGER_XMLELENTISXMLELEMENT(cash) && cash->Attribute("value")!=NULL)
                {
                    profile.cash=stringtodouble(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(cash->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("value"))),&ok);
                    if(!ok)
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", cash is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                        profile.cash=0;
                    }
                }
                const CATCHCHALLENGER_XMLELEMENT * monstersElementGroup = startItem->FirstChildElement("monstergroup");
                while(monstersElementGroup!=NULL)
                {
                    std::vector<Profile::Monster> monsters_list;
                    const CATCHCHALLENGER_XMLELEMENT * monstersElement = monstersElementGroup->FirstChildElement("monster");
                    while(monstersElement!=NULL)
                    {
                        Profile::Monster monster;
                        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(monstersElement) && monstersElement->Attribute("id")!=NULL && monstersElement->Attribute("level")!=NULL && monstersElement->Attribute("captured_with")!=NULL)
                        {
                            monster.id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", monster id is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                            if(ok)
                            {
                                monster.level=stringtouint8(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("level"))),&ok);
                                if(!ok)
                                    std::cerr << "Unable to open the xml file: " << file << ", monster level is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                            }
                            if(ok)
                            {
                                if(monster.level==0 || monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                                    std::cerr << "Unable to open the xml file: " << file << ", monster level is not into the range: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                            }
                            if(ok)
                            {
                                monster.captured_with=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("captured_with"))),&ok);
                                if(!ok)
                                    std::cerr << "Unable to open the xml file: " << file << ", captured_with is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                            }
                            if(ok)
                            {
                                if(monsters.find(monster.id)==monsters.cend())
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", starter don't found the monster " << monster.id << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                                    ok=false;
                                }
                            }
                            #ifndef CATCHCHALLENGER_CLASS_MASTER
                            if(ok)
                            {
                                if(items.find(monster.captured_with)==items.cend())
                                    std::cerr << "Unable to open the xml file: " << file << ", starter don't found the monster capture item " << monster.id << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                            }
                            #endif // CATCHCHALLENGER_CLASS_MASTER
                            if(ok)
                                monsters_list.push_back(monster);
                        }
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", no monster attribute to load: child->CATCHCHALLENGER_XMLELENTVALUE(): " << monstersElement->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(monstersElement) << ")" << std::endl;
                        monstersElement = monstersElement->NextSiblingElement("monster");
                    }
                    if(monsters_list.empty())
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", no monster to load: child->CATCHCHALLENGER_XMLELENTVALUE(): " << monstersElement->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(monstersElement) << ")" << std::endl;
                        startItem = startItem->NextSiblingElement("start");
                        continue;
                    }
                    profile.monstergroup.push_back(monsters_list);
                    monstersElementGroup = monstersElementGroup->NextSiblingElement("monstergroup");
                }
                if(profile.monstergroup.empty())
                {
                    std::cerr << "Unable to open the xml file: " << file << ", no monstergroup to load: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                    startItem = startItem->NextSiblingElement("start");
                    continue;
                }
                const CATCHCHALLENGER_XMLELEMENT * reputationElement = startItem->FirstChildElement("reputation");
                while(reputationElement!=NULL)
                {
                    Profile::Reputation reputationTemp;
                    reputationTemp.internalIndex=255;
                    reputationTemp.level=-1;
                    reputationTemp.point=0xFFFFFFFF;
                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(reputationElement) && reputationElement->Attribute("type")!=NULL && reputationElement->Attribute("level")!=NULL)
                    {
                        reputationTemp.level=stringtoint8(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("level"))),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", reputation level is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                        if(ok)
                        {
                            if(reputationNameToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))))==reputationNameToId.cend())
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", reputation type not found " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                                ok=false;
                            }
                            if(ok)
                            {
                                reputationTemp.internalIndex=reputationNameToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))));
                                if(reputationTemp.level==0)
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", reputation level is useless if level 0: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                                    ok=false;
                                }
                                else if(reputationTemp.level<0)
                                {
                                    if((-reputationTemp.level)>(int32_t)reputations.at(reputationTemp.internalIndex).reputation_negative.size())
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", reputation level is lower than minimal level for " << reputationElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type")) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                                        ok=false;
                                    }
                                }
                                else// if(reputationTemp.level>0)
                                {
                                    if((reputationTemp.level)>=(int32_t)reputations.at(reputationTemp.internalIndex).reputation_positive.size())
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", reputation level is higther than maximal level for " << reputationElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type")) << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                                        ok=false;
                                    }
                                }
                            }
                            if(ok)
                            {
                                reputationTemp.point=0;
                                if(reputationElement->Attribute("point")!=NULL)
                                {
                                    reputationTemp.point=stringtoint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(reputationElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("point"))),&ok);
                                    std::cerr << "Unable to open the xml file: " << file << ", reputation point is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                                    if(ok)
                                    {
                                        if((reputationTemp.point>0 && reputationTemp.level<0) || (reputationTemp.point<0 && reputationTemp.level>=0))
                                            std::cerr << "Unable to open the xml file: " << file << ", reputation point is not negative/positive like the level: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                                    }
                                }
                            }
                        }
                        if(ok)
                            profile.reputations.push_back(reputationTemp);
                    }
                    reputationElement = reputationElement->NextSiblingElement("reputation");
                }
                const CATCHCHALLENGER_XMLELEMENT * itemElement = startItem->FirstChildElement("item");
                while(itemElement!=NULL)
                {
                    Profile::Item itemTemp;
                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(itemElement) && itemElement->Attribute("id")!=NULL)
                    {
                        itemTemp.id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(itemElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", item id is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                        if(ok)
                        {
                            itemTemp.quantity=0;
                            if(itemElement->Attribute("quantity")!=NULL)
                            {
                                itemTemp.quantity=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(itemElement->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("quantity"))),&ok);
                                if(!ok)
                                    std::cerr << "Unable to open the xml file: " << file << ", item quantity is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                                if(ok)
                                {
                                    if(itemTemp.quantity==0)
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", item quantity is null: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                                        ok=false;
                                    }
                                }
                                #ifndef CATCHCHALLENGER_CLASS_MASTER
                                if(ok)
                                {
                                    if(items.find(itemTemp.id)==items.cend())
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", item not found as know item " << itemTemp.id << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
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
                idDuplicate.insert(profile.databaseId);
                returnVar.second.push_back(profile);
                returnVar.first.push_back(startItem);
            }
        }
        else
            std::cerr << "Unable to open the xml file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
        startItem = startItem->NextSiblingElement("start");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return returnVar;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::vector<MonstersCollision> DatapackGeneralLoader::loadMonstersCollision(const std::string &file, const std::unordered_map<uint16_t, Item> &items,const std::vector<Event> &events)
{
    std::unordered_map<std::string,uint8_t> eventStringToId;
    std::unordered_map<std::string,std::unordered_map<std::string,uint8_t> > eventListingToId;
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
                eventListingToId[event.name][event.values.at(sub_index)]=sub_index;
                sub_index++;
            }
            index++;
        }
    }
    std::vector<MonstersCollision> returnVar;
    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return returnVar;
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"layers"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    bool ok;
    const CATCHCHALLENGER_XMLELEMENT * monstersCollisionItem = root->FirstChildElement("monstersCollision");
    while(monstersCollisionItem!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(monstersCollisionItem))
        {
            if(monstersCollisionItem->Attribute("type")==NULL)
                std::cerr << "Have not the attribute type, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(monstersCollisionItem) << std::endl;
            else if(monstersCollisionItem->Attribute("monsterType")==NULL)
                std::cerr << "Have not the attribute monsterType, into: file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(monstersCollisionItem) << std::endl;
            else
            {
                ok=true;
                MonstersCollision monstersCollision;
                if(CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersCollisionItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))),"walkOn"))
                    monstersCollision.type=MonstersCollisionType_WalkOn;
                else if(CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersCollisionItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))),"actionOn"))
                    monstersCollision.type=MonstersCollisionType_ActionOn;
                else
                {
                    ok=false;
                    std::cerr << "type is not walkOn or actionOn, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(monstersCollisionItem) << std::endl;
                }
                if(ok)
                {
                    if(monstersCollisionItem->Attribute("layer")!=NULL)
                        monstersCollision.layer=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersCollisionItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("layer")));
                }
                if(ok)
                {
                    if(monstersCollision.layer.empty() && monstersCollision.type!=MonstersCollisionType_WalkOn)//need specific layer name to do that's
                    {
                        ok=false;
                        std::cerr << "To have blocking layer by item, have specific layer name, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(monstersCollisionItem) << std::endl;
                    }
                    else
                    {
                        monstersCollision.item=0;
                        if(monstersCollisionItem->Attribute("item")!=NULL)
                        {
                            monstersCollision.item=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersCollisionItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("item"))),&ok);
                            if(!ok)
                                std::cerr << "item attribute is not a number, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(monstersCollisionItem) << std::endl;
                            else if(items.find(monstersCollision.item)==items.cend())
                            {
                                ok=false;
                                std::cerr << "item is not into item list, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(monstersCollisionItem) << std::endl;
                            }
                        }
                    }
                }
                if(ok)
                {
                    if(monstersCollisionItem->Attribute("tile")!=NULL)
                        monstersCollision.tile=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersCollisionItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("tile")));
                }
                if(ok)
                {
                    if(monstersCollisionItem->Attribute("background")!=NULL)
                        monstersCollision.background=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersCollisionItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("background")));
                }
                if(ok)
                {
                    if(monstersCollisionItem->Attribute("monsterType")!=NULL)
                    {
                        monstersCollision.defautMonsterTypeList=stringsplit(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(monstersCollisionItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("monsterType"))),';');
                        vectorRemoveEmpty(monstersCollision.defautMonsterTypeList);
                        vectorRemoveDuplicatesForSmallList(monstersCollision.defautMonsterTypeList);
                        monstersCollision.monsterTypeList=monstersCollision.defautMonsterTypeList;
                        //load the condition
                        const CATCHCHALLENGER_XMLELEMENT * eventItem = monstersCollisionItem->FirstChildElement("event");
                        while(eventItem!=NULL)
                        {
                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(eventItem))
                            {
                                if(eventItem->Attribute("id")!=NULL && eventItem->Attribute("value")!=NULL && eventItem->Attribute("monsterType")!=NULL)
                                {
                                    if(eventStringToId.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(eventItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))))!=eventStringToId.cend())
                                    {
                                        const auto & list=eventListingToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(eventItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))));
                                        if(list.find(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(eventItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("value"))))!=list.cend())
                                        {
                                            MonstersCollision::MonstersCollisionEvent event;
                                            event.event=eventStringToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(eventItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))));
                                            event.event_value=eventListingToId.at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(eventItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id")))).at(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(eventItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("value"))));
                                            event.monsterTypeList=stringsplit(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(eventItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("monsterType"))),';');
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
                                                std::cerr << "monsterType is empty, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(eventItem) << std::endl;
                                        }
                                        else
                                            std::cerr << "event value not found, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(eventItem) << std::endl;
                                    }
                                    else
                                        std::cerr << "event not found, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(eventItem) << std::endl;
                                }
                                else
                                    std::cerr << "event have missing attribute, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(eventItem) << std::endl;
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
                            std::cerr << "similar monstersCollision previously found, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(monstersCollisionItem) << std::endl;
                            ok=false;
                            break;
                        }
                        if(monstersCollision.type==MonstersCollisionType_WalkOn && returnVar.at(index).layer==monstersCollision.layer)
                        {
                            std::cerr << "You can't have different item for same layer in walkOn mode, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(monstersCollisionItem) << std::endl;
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
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return returnVar;
}

LayersOptions DatapackGeneralLoader::loadLayersOptions(const std::string &file)
{
    LayersOptions returnVar;
    returnVar.zoom=2;
    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return returnVar;
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"layers"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found for reputation of the xml file" << std::endl;
        std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found for the xml file" << std::endl;
        return returnVar;
    }
    if(root->Attribute("zoom")!=NULL)
    {
        bool ok;
        returnVar.zoom=stringtouint8(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(root->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("zoom"))),&ok);
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
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return returnVar;
}

std::vector<Event> DatapackGeneralLoader::loadEvents(const std::string &file)
{
    std::vector<Event> returnVar;

    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return returnVar;
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"events"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"events\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    const CATCHCHALLENGER_XMLELEMENT * eventItem = root->FirstChildElement("event");
    while(eventItem!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(eventItem))
        {
            if(eventItem->Attribute("id")==NULL)
                std::cerr << "Have not the attribute id, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(eventItem) << std::endl;
            else
            {
                std::string idString=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(eventItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id")));
                if(idString.empty())
                    std::cerr << "Have id empty, into file: " << file << " at line " << CATCHCHALLENGER_XMLELENTATLINE(eventItem) << std::endl;
                else
                {
                    Event event;
                    event.name=idString;
                    const CATCHCHALLENGER_XMLELEMENT * valueItem = eventItem->FirstChildElement("value");
                    while(valueItem!=NULL)
                    {
                        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(valueItem))
                            event.values.push_back(valueItem->GetText());
                        valueItem = valueItem->NextSiblingElement("value");
                    }
                    if(!event.values.empty())
                        returnVar.push_back(event);
                }
            }
        }
        eventItem = eventItem->NextSiblingElement("event");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return returnVar;
}

std::unordered_map<uint32_t,Shop> DatapackGeneralLoader::preload_shop(const std::string &file, const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<uint32_t,Shop> shops;

    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return shops;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return shops;
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"shops"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"shops\" root balise not found for reputation of the xml file" << std::endl;
        return shops;
    }

    //load the content
    bool ok;
    const CATCHCHALLENGER_XMLELEMENT * shopItem = root->FirstChildElement("shop");
    while(shopItem!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(shopItem))
        {
            if(shopItem->Attribute("id")!=NULL)
            {
                uint32_t id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(shopItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                if(ok)
                {
                    if(shops.find(id)==shops.cend())
                    {
                        Shop shop;
                        const CATCHCHALLENGER_XMLELEMENT * product = shopItem->FirstChildElement("product");
                        while(product!=NULL)
                        {
                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(product))
                            {
                                if(product->Attribute("itemId")!=NULL)
                                {
                                    uint32_t itemId=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(product->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("itemId"))),&ok);
                                    if(!ok)
                                        std::cerr << "preload_shop() product attribute itemId is not a number for shops file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << shopItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(shopItem) << ")" << std::endl;
                                    else
                                    {
                                        if(items.find(itemId)==items.cend())
                                            std::cerr << "preload_shop() product itemId in not into items list for shops file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << shopItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(shopItem) << ")" << std::endl;
                                        else
                                        {
                                            uint32_t price=items.at(itemId).price;
                                            if(product->Attribute("overridePrice")!=NULL)
                                            {
                                                price=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(product->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("overridePrice"))),&ok);
                                                if(!ok)
                                                    price=items.at(itemId).price;
                                            }
                                            if(price==0)
                                                std::cerr << "preload_shop() item can't be into the shop with price == 0' for shops file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << shopItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(shopItem) << ")" << std::endl;
                                            else
                                            {
                                                shop.prices.push_back(price);
                                                shop.items.push_back(itemId);
                                            }
                                        }
                                    }
                                }
                                else
                                    std::cerr << "preload_shop() material have not attribute itemId for shops file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << shopItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(shopItem) << ")" << std::endl;
                            }
                            else
                                std::cerr << "preload_shop() material is not an element for shops file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << shopItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(shopItem) << ")" << std::endl;
                            product = product->NextSiblingElement("product");
                        }
                        shops[id]=shop;
                    }
                    else
                        std::cerr << "Unable to open the shops file: " << file << ", child->CATCHCHALLENGER_XMLELENTVALUE(): " << shopItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(shopItem) << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the shops file: " << file << ", id is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << shopItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(shopItem) << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the shops file: " << file << ", have not the shops id: child->CATCHCHALLENGER_XMLELENTVALUE(): " << shopItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(shopItem) << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the shops file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << shopItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(shopItem) << ")" << std::endl;
        shopItem = shopItem->NextSiblingElement("shop");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return shops;
}
#endif

std::vector<ServerSpecProfile> DatapackGeneralLoader::loadServerProfileList(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file,const std::vector<Profile> &profileCommon)
{
    std::vector<ServerSpecProfile> serverProfile=loadServerProfileListInternal(datapackPath,mainDatapackCode,file);
    //index of base profile
    std::unordered_set<std::string> profileId,serverProfileId;
    {
        unsigned int index=0;
        while(index<profileCommon.size())
        {
            const Profile &profile=profileCommon.at(index);
            //already deduplicated at loading
            profileId.insert(profile.databaseId);
            index++;
        }
    }
    //drop serverProfileList
    {
        unsigned int index=0;
        while(index<serverProfile.size())
        {
            const ServerSpecProfile &serverSpecProfile=serverProfile.at(index);
            if(profileId.find(serverSpecProfile.databaseId)!=profileId.cend())
            {
                serverProfileId.insert(serverSpecProfile.databaseId);
                index++;
            }
            else
            {
                std::cerr << "Profile xml file: " << file << ", found id \"" << serverSpecProfile.databaseId << "\" but not found in common, drop it" << std::endl;
                serverProfile.erase(serverProfile.begin()+index);
            }
        }
    }
    //add serverProfileList
    {
        unsigned int index=0;
        while(index<profileCommon.size())
        {
            const Profile &profile=profileCommon.at(index);
            if(serverProfileId.find(profile.databaseId)==serverProfileId.cend())
            {
                std::cerr << "Profile xml file: " << file << ", found common id \"" << profile.databaseId << "\" but not found in server, add it" << std::endl;
                std::cerr << "Mostly due datapack/player/start.xml entry not found into datapack/internal/map/main/official/start.xml" << std::endl;
                /*ServerSpecProfile serverProfileTemp;
                serverProfileTemp.databaseId=profile.databaseId;
                serverProfileTemp.orientation=Orientation_bottom;
                serverProfileTemp.x=0;
                serverProfileTemp.y=0;
                serverProfile.push_back(serverProfileTemp);*/
            }
            index++;
        }
    }

    return serverProfile;
}

std::vector<ServerSpecProfile> DatapackGeneralLoader::loadServerProfileListInternal(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file)
{
    std::unordered_set<std::string> idDuplicate;
    std::vector<ServerSpecProfile> serverProfileList;

    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return serverProfileList;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return serverProfileList;
    }
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"profile"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found for reputation of the xml file" << std::endl;
        return serverProfileList;
    }

    //load the content
    bool ok;
    const CATCHCHALLENGER_XMLELEMENT * startItem = root->FirstChildElement("start");
    while(startItem!=NULL)
    {
        if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(startItem))
        {
            ServerSpecProfile serverProfile;
            serverProfile.orientation=Orientation_bottom;

            const CATCHCHALLENGER_XMLELEMENT * map = startItem->FirstChildElement("map");
            if(map!=NULL && CATCHCHALLENGER_XMLELENTISXMLELEMENT(map) && map->Attribute("file")!=NULL && map->Attribute("x")!=NULL && map->Attribute("y")!=NULL)
            {
                serverProfile.mapString=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(map->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("file")));
                if(!stringEndsWith(serverProfile.mapString,".tmx"))
                    serverProfile.mapString+=".tmx";
                if(!CatchChallenger::FacilityLibGeneral::isFile(datapackPath+DATAPACK_BASE_PATH_MAPMAIN+mainDatapackCode+'/'+serverProfile.mapString))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", map don't exists " << serverProfile.mapString << ": child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                    {
                        std::cerr << "Into the starter the map \"" << serverProfile.mapString << "\" is not found, fix it (abort)" << std::endl;
                        abort();
                    }
                    startItem = startItem->NextSiblingElement("start");
                    continue;
                }
                serverProfile.x=stringtouint8(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(map->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("x"))),&ok);
                if(!ok)
                {
                    std::cerr << "Unable to open the xml file: " << file << ", map x is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                    startItem = startItem->NextSiblingElement("start");
                    continue;
                }
                serverProfile.y=stringtouint8(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(map->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("y"))),&ok);
                if(!ok)
                {
                    std::cerr << "Unable to open the xml file: " << file << ", map y is not a number: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                    startItem = startItem->NextSiblingElement("start");
                    continue;
                }
            }
            else
            {
                std::cerr << "Unable to open the xml file: " << file << ", no correct map configuration: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                startItem = startItem->NextSiblingElement("start");
                continue;
            }

            if(startItem->Attribute("id")!=NULL)
                serverProfile.databaseId=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(startItem->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id")));

            if(idDuplicate.find(serverProfile.databaseId)!=idDuplicate.cend())
            {
                std::cerr << "Unable to open the xml file: " << file << ", id duplicate: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
                startItem = startItem->NextSiblingElement("start");
                continue;
            }

            if(!serverProfile.databaseId.empty() && idDuplicate.find(serverProfile.databaseId)==idDuplicate.cend())
            {
                idDuplicate.insert(serverProfile.databaseId);
                serverProfileList.push_back(serverProfile);
            }
        }
        else
            std::cerr << "Unable to open the xml file: " << file << ", is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << startItem->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(startItem) << ")" << std::endl;
        startItem = startItem->NextSiblingElement("start");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return serverProfileList;
}


#ifndef CATCHCHALLENGER_CLASS_MASTER
//global to drop useless communication as 100% item luck or to have more informations into client and knowleg on bot
std::unordered_map<uint16_t,std::vector<MonsterDrops> > DatapackGeneralLoader::loadMonsterDrop(const std::string &folder,
                                                                                               const std::unordered_map<uint16_t, Item> &items,
                                                                                               const std::unordered_map<uint16_t,Monster> &monsters)
{
    std::unordered_map<uint16_t,std::vector<MonsterDrops> > monsterDrops;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &fileList=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    unsigned int file_index=0;
    while(file_index<(uint32_t)fileList.size())
    {
        const std::string &file=fileList.at(file_index).absoluteFilePath;
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }

        CATCHCHALLENGER_XMLDOCUMENT *domDocument;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        //open and quick check the file
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        else
        {
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
            #else
            domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
            #endif
            const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
            if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
            {
                std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
        }
        #endif
        const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
        if(root==NULL)
        {
            file_index++;
            continue;
        }
        if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"monsters"))
        {
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        const CATCHCHALLENGER_XMLELEMENT * item = root->FirstChildElement("monster");
        while(item!=NULL)
        {
            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(item))
            {
                if(item->Attribute("id")!=NULL)
                {
                    const uint16_t &id=stringtouint16(item->Attribute("id"),&ok);
                    if(!ok)
                        std::cerr << "Unable to open the xml file: " << file << ", id not a number: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                    else if(monsters.find(id)==monsters.cend())
                        std::cerr << "Unable to open the xml file: " << file << ", id into the monster list, skip: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                    else
                    {
                        const CATCHCHALLENGER_XMLELEMENT * drops = item->FirstChildElement("drops");
                        if(drops!=NULL)
                        {
                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(drops))
                            {
                                const CATCHCHALLENGER_XMLELEMENT * drop = drops->FirstChildElement("drop");
                                while(drop!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(drop))
                                    {
                                        if(drop->Attribute("item")!=NULL)
                                        {
                                            MonsterDrops dropVar;
                                            dropVar.item=0;
                                            if(drop->Attribute("quantity_min")!=NULL)
                                            {
                                                dropVar.quantity_min=stringtouint32(drop->Attribute("quantity_min"),&ok);
                                                if(!ok)
                                                    std::cerr << "Unable to open the xml file: " << file << ", quantity_min is not a number: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                            }
                                            else
                                                dropVar.quantity_min=1;
                                            if(ok)
                                            {
                                                if(drop->Attribute("quantity_max")!=NULL)
                                                {
                                                    dropVar.quantity_max=stringtouint32(drop->Attribute("quantity_max"),&ok);
                                                    if(!ok)
                                                        std::cerr << "Unable to open the xml file: " << file << ", quantity_max is not a number: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                                else
                                                    dropVar.quantity_max=1;
                                            }
                                            if(ok)
                                            {
                                                if(dropVar.quantity_min<=0)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_min is 0: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(dropVar.quantity_max<=0)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max is 0: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(dropVar.quantity_max<dropVar.quantity_min)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max<dropVar.quantity_min: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(drop->Attribute("luck")!=NULL)
                                                {
                                                    std::string luck=drop->Attribute("luck");
                                                    if(!luck.empty())
                                                        if(luck.back()=='%')
                                                            luck.resize(luck.size()-1);
                                                    dropVar.luck=stringtouint32(luck,&ok);
                                                    if(!ok)
                                                        std::cerr << "Unable to open the xml file: " << file << ", luck is not a number: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                    else if(dropVar.luck==0)
                                                    {
                                                        std::cerr << "Unable to open the xml file: " << file << ", luck can't be 0: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                        ok=false;
                                                    }
                                                }
                                                else
                                                    dropVar.luck=100;
                                            }
                                            if(ok)
                                            {
                                                if(dropVar.luck<=0)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", luck is 0!: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                                if(dropVar.luck>100)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", luck is greater than 100: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(drop->Attribute("item")!=NULL)
                                                {
                                                    dropVar.item=stringtouint32(drop->Attribute("item"),&ok);
                                                    if(!ok)
                                                        std::cerr << "Unable to open the xml file: " << file << ", item is not a number: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                                else
                                                    dropVar.luck=100;
                                            }
                                            if(ok)
                                            {
                                                if(items.find(dropVar.item)==items.cend())
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", the item " << dropVar.item << " is not into the item list: child.tagName(): %2 (at line: %3)" << std::endl;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(CommonSettingsServer::commonSettingsServer.rates_drop!=1.0)
                                                {
                                                    if(CommonSettingsServer::commonSettingsServer.rates_drop==0)
                                                    {
                                                        std::cerr << "CommonSettingsServer::commonSettingsServer.rates_drop==0 durring loading the drop, reset to 1" << std::endl;
                                                        CommonSettingsServer::commonSettingsServer.rates_drop=1;
                                                    }
                                                    dropVar.luck=dropVar.luck*CommonSettingsServer::commonSettingsServer.rates_drop;
                                                    float targetAverage=((float)dropVar.quantity_min+(float)dropVar.quantity_max)/2.0;
                                                    targetAverage=(targetAverage*dropVar.luck)/100.0;
                                                    while(dropVar.luck>100)
                                                    {
                                                        dropVar.quantity_max++;
                                                        float currentAverage=((float)dropVar.quantity_min+(float)dropVar.quantity_max)/2.0;
                                                        dropVar.luck=(100.0*targetAverage)/currentAverage;
                                                    }
                                                }
                                                monsterDrops[id].push_back(dropVar);
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the xml file: " << file << ", as not item attribute: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", effect balise is not an element: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                    drop = drop->NextSiblingElement("drop");
                                }
                            }
                            else
                                std::cerr << "Unable to open the xml file: " << file << ", drops balise is not an element: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                        }
                    }
                }
                else
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster id: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
            item = item->NextSiblingElement("monster");
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        file_index++;
    }
    return monsterDrops;
}
#endif
