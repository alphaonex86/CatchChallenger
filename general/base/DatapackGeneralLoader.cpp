#include "DatapackGeneralLoader.h"
#include "GeneralVariable.h"
#include "CommonDatapack.h"
#include "FacilityLib.h"
#include "FacilityLibGeneral.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfoList>
#include <QDir>
#include <iostream>

using namespace CatchChallenger;

std::vector<Reputation> DatapackGeneralLoader::loadReputation(const std::string &file)
{
    std::regex excludeFilterRegex("[\"']");
    std::regex typeRegex("^[a-z]{1,32}$");
    QDomDocument domDocument;
    std::vector<Reputation> reputation;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        //open and quick check the file
        QFile itemsFile(QString::fromStdString(file));
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << itemsFile.errorString().toStdString() << std::endl;
            return reputation;
        }
        const QByteArray &xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return reputation;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        std::cerr << "Unable to open the file: " << file << ", \"list\" root balise not found for reputation of the xml file" << std::endl;
        return reputation;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("reputation");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("type"))
            {
                std::vector<int32_t> point_list_positive,point_list_negative;
                std::vector<std::string> text_positive,text_negative;
                QDomElement level = item.firstChildElement("level");
                ok=true;
                while(!level.isNull() && ok)
                {
                    if(level.isElement())
                    {
                        if(level.hasAttribute("point"))
                        {
                            const int32_t &point=level.attribute("point").toInt(&ok);
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
                                            std::cerr << "Unable to open the file: " << file << ", reputation level with same number of point found!: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
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
                                            std::cerr << "Unable to open the file: " << file << ", reputation level with same number of point found!: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
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
                                std::cerr << "Unable to open the file: " << file << ", point is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", point attribute not found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    level = level.nextSiblingElement("level");
                }
                qSort(point_list_positive);
                qSort(point_list_negative.end(),point_list_negative.begin());
                if(ok)
                    if(point_list_positive.size()<2)
                    {
                        std::cerr << "Unable to open the file: " << file << ", reputation have to few level: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(!vectorcontainsAtLeastOne(point_list_positive,0))
                    {
                        std::cerr << "Unable to open the file: " << file << ", no starting level for the positive: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(!point_list_negative.empty() && !vectorcontainsAtLeastOne(point_list_negative,-1))
                    {
                        //std::cerr << "Unable to open the file: " << file << ", no starting level for the negative, first level need start with -1, fix by change range: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
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
                    if(!std::regex_match(item.attribute("type").toStdString(),typeRegex))
                    {
                        std::cerr << "Unable to open the file: " << file << ", the type " << item.attribute("type").toStdString() << " don't match wiuth the regex: ^[a-z]{1,32}$: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        ok=false;
                    }
                if(ok)
                {
                    const std::string &type=item.attribute("type").toStdString();
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
                std::cerr << "Unable to open the file: " << file << ", have not the item id: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
        item = item.nextSiblingElement("reputation");
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
        const uint32_t &questId=entryList.at(index).fileName().toUInt(&ok);
        if(ok)
        {
            //add it, all seam ok
            std::pair<bool,Quest> returnedQuest=loadSingleQuest(entryList.at(index).absoluteFilePath().toStdString()+"/definition.xml");
            if(returnedQuest.first==true)
            {
                returnedQuest.second.id=questId;
                if(quests.find(returnedQuest.second.id)!=quests.cend())
                    std::cerr << "The quest with id: " << returnedQuest.second.id << " is already found, disable: " << entryList.at(index).absoluteFilePath().toStdString() << "/definition.xml" << std::endl;
                else
                    quests[returnedQuest.second.id]=returnedQuest.second;
            }
        }
        else
            std::cerr << "Unable to open the folder: " << entryList.at(index).fileName().toStdString() << ", because is folder name is not a number" << std::endl;
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
    QDomDocument domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        #endif
        QFile itemsFile(QString::fromStdString(file));
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << itemsFile.errorString().toStdString() << std::endl;
            return std::pair<bool,Quest>(false,quest);
        }
        const QByteArray &xmlContent=itemsFile.readAll();
        itemsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return std::pair<bool,Quest>(false,quest);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="quest")
    {
        std::cerr << "Unable to open the file: " << file << ", \"quest\" root balise not found for reputation of the xml file" << std::endl;
        return std::pair<bool,Quest>(false,quest);
    }

    //load the content
    bool ok;
    std::vector<uint16_t> defaultBots;
    quest.id=0;
    quest.repeatable=false;
    if(root.hasAttribute("repeatable"))
        if(root.attribute("repeatable")=="yes" || root.attribute("repeatable")=="true")
            quest.repeatable=true;
    if(root.hasAttribute("bot"))
    {
        const std::vector<std::string> &tempStringList=stringsplit(root.attribute("bot").toStdString(),';');
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
    QDomElement requirements = root.firstChildElement("requirements");
    while(!requirements.isNull())
    {
        if(requirements.isElement())
        {
            //load requirements reputation
            {
                QDomElement requirementsItem = requirements.firstChildElement("reputation");
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute("type") && requirementsItem.hasAttribute("level"))
                        {
                            if(reputationNameToId.find(requirementsItem.attribute("type").toStdString())!=reputationNameToId.cend())
                            {
                                std::string stringLevel=requirementsItem.attribute("level").toStdString();
                                bool positif=!stringStartWith(stringLevel,"-");
                                if(!positif)
                                    stringLevel.erase(0,1);
                                uint8_t level=stringtouint8(stringLevel,&ok);
                                if(ok)
                                {
                                    CatchChallenger::ReputationRequirements reputation;
                                    reputation.level=level;
                                    reputation.positif=positif;
                                    reputation.reputationId=reputationNameToId.at(requirementsItem.attribute("type").toStdString());
                                    quest.requirements.reputation.push_back(reputation);
                                }
                                else
                                    std::cerr << "Unable to open the file: " << file << ", reputation is not a number " << stringLevel << ": child.tagName(): " << requirementsItem.tagName().toStdString() << " (at line: " << requirementsItem.lineNumber() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Has not the attribute: " << requirementsItem.attribute("type").toStdString() << ", reputation not found: child.tagName(): " << requirementsItem.tagName().toStdString() << " (at line: " << requirementsItem.lineNumber() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has not the attribute: type level, have not attribute type or level: child.tagName(): " << requirementsItem.tagName().toStdString() << " (at line: " << requirementsItem.lineNumber() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << requirementsItem.tagName().toStdString() << " (at line: " << requirementsItem.lineNumber() << ")" << std::endl;
                    requirementsItem = requirementsItem.nextSiblingElement("reputation");
                }
            }
            //load requirements quest
            {
                QDomElement requirementsItem = requirements.firstChildElement("quest");
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute("id"))
                        {
                            const uint32_t &questId=requirementsItem.attribute("id").toUInt(&ok);
                            if(ok)
                            {
                                QuestRequirements questNewEntry;
                                questNewEntry.quest=questId;
                                questNewEntry.inverse=false;
                                if(requirementsItem.hasAttribute("inverse"))
                                    if(requirementsItem.attribute("inverse")=="true")
                                        questNewEntry.inverse=true;
                                quest.requirements.quests.push_back(questNewEntry);
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", requirement quest item id is not a number " << requirementsItem.attribute("id").toStdString() << ": child.tagName(): " << requirementsItem.tagName().toStdString() << " (at line: " << requirementsItem.lineNumber() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has attribute: %1, requirement quest item have not id attribute: child.tagName(): " << requirementsItem.tagName().toStdString() << " (at line: " << requirementsItem.lineNumber() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << requirementsItem.tagName().toStdString() << " (at line: " << requirementsItem.lineNumber() << ")" << std::endl;
                    requirementsItem = requirementsItem.nextSiblingElement("quest");
                }
            }
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << requirements.tagName().toStdString() << " (at line: " << requirements.lineNumber() << ")" << std::endl;
        requirements = requirements.nextSiblingElement("requirements");
    }

    //load rewards
    QDomElement rewards = root.firstChildElement("rewards");
    while(!rewards.isNull())
    {
        if(rewards.isElement())
        {
            //load rewards reputation
            {
                QDomElement reputationItem = rewards.firstChildElement("reputation");
                while(!reputationItem.isNull())
                {
                    if(reputationItem.isElement())
                    {
                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("point"))
                        {
                            if(reputationNameToId.find(reputationItem.attribute("type").toStdString())!=reputationNameToId.cend())
                            {
                                const int32_t &point=reputationItem.attribute("point").toInt(&ok);
                                if(ok)
                                {
                                    CatchChallenger::ReputationRewards reputation;
                                    reputation.reputationId=reputationNameToId.at(reputationItem.attribute("type").toStdString());
                                    reputation.point=point;
                                    quest.rewards.reputation.push_back(reputation);
                                }
                                else
                                    std::cerr << "Unable to open the file: " << file << ", quest rewards point is not a number: child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", quest rewards point is not a number: child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has not the attribute: type/point, quest rewards point have not type or point attribute: child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                    reputationItem = reputationItem.nextSiblingElement("reputation");
                }
            }
            //load rewards item
            {
                QDomElement rewardsItem = rewards.firstChildElement("item");
                while(!rewardsItem.isNull())
                {
                    if(rewardsItem.isElement())
                    {
                        if(rewardsItem.hasAttribute("id"))
                        {
                            CatchChallenger::Quest::Item item;
                            item.item=rewardsItem.attribute("id").toUInt(&ok);
                            item.quantity=1;
                            if(ok)
                            {
                                if(CommonDatapack::commonDatapack.items.item.find(item.item)==CommonDatapack::commonDatapack.items.item.cend())
                                {
                                    std::cerr << "Unable to open the file: " << file << ", rewards item id is not into the item list: " << rewardsItem.attribute("id").toStdString() << ": child.tagName(): " << rewardsItem.tagName().toStdString() << " (at line: " << rewardsItem.lineNumber() << ")" << std::endl;
                                    return std::pair<bool,Quest>(false,quest);
                                }
                                if(rewardsItem.hasAttribute("quantity"))
                                {
                                    item.quantity=rewardsItem.attribute("quantity").toUInt(&ok);
                                    if(!ok)
                                        item.quantity=1;
                                }
                                quest.rewards.items.push_back(item);
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", rewards item id is not a number: " << rewardsItem.attribute("id").toStdString() << ": child.tagName(): " << rewardsItem.tagName().toStdString() << " (at line: " << rewardsItem.lineNumber() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has not the attribute: id, rewards item have not attribute id: child.tagName(): " << rewardsItem.tagName().toStdString() << " (at line: " << rewardsItem.lineNumber() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << rewardsItem.tagName().toStdString() << " (at line: " << rewardsItem.lineNumber() << ")" << std::endl;
                    rewardsItem = rewardsItem.nextSiblingElement("item");
                }
            }
            //load rewards allow
            {
                QDomElement allowItem = rewards.firstChildElement("allow");
                while(!allowItem.isNull())
                {
                    if(allowItem.isElement())
                    {
                        if(allowItem.hasAttribute("type"))
                        {
                            if(allowItem.attribute("type")=="clan")
                                quest.rewards.allow.push_back(CatchChallenger::ActionAllow_Clan);
                            else
                                std::cerr << "Unable to open the file: " << file << ", allow type not understand " << allowItem.attribute("id").toStdString() << ": child.tagName(): " << allowItem.tagName().toStdString() << " (at line: " << allowItem.lineNumber() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Has attribute: %1, rewards item have not attribute id: child.tagName(): " << allowItem.tagName().toStdString() << " (at line: " << allowItem.lineNumber() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << allowItem.tagName().toStdString() << " (at line: " << allowItem.lineNumber() << ")" << std::endl;
                    allowItem = allowItem.nextSiblingElement("allow");
                }
            }
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << rewards.tagName().toStdString() << " (at line: " << rewards.lineNumber() << ")" << std::endl;
        rewards = rewards.nextSiblingElement("rewards");
    }

    std::unordered_map<uint8_t,CatchChallenger::Quest::Step> steps;
    //load step
    QDomElement step = root.firstChildElement("step");
    while(!step.isNull())
    {
        if(step.isElement())
        {
            if(step.hasAttribute("id"))
            {
                const uint32_t &id=step.attribute("id").toUInt(&ok);
                if(ok)
                {
                    CatchChallenger::Quest::Step stepObject;
                    if(step.hasAttribute("bot"))
                    {
                        const std::vector<std::string> &tempStringList=stringsplit(step.attribute("bot").toStdString(),';');
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
                        QDomElement stepItem = step.firstChildElement("item");
                        while(!stepItem.isNull())
                        {
                            if(stepItem.isElement())
                            {
                                if(stepItem.hasAttribute("id"))
                                {
                                    CatchChallenger::Quest::Item item;
                                    item.item=stepItem.attribute("id").toUInt(&ok);
                                    item.quantity=1;
                                    if(ok)
                                    {
                                        if(CommonDatapack::commonDatapack.items.item.find(item.item)==CommonDatapack::commonDatapack.items.item.cend())
                                        {
                                            std::cerr << "Unable to open the file: " << file << ", rewards item id is not into the item list: " << stepItem.attribute("id").toStdString() << ": child.tagName(): " << stepItem.tagName().toStdString() << " (at line: " << stepItem.lineNumber() << ")" << std::endl;
                                            return std::pair<bool,Quest>(false,quest);
                                        }
                                        if(stepItem.hasAttribute("quantity"))
                                        {
                                            item.quantity=stepItem.attribute("quantity").toUInt(&ok);
                                            if(!ok)
                                                item.quantity=1;
                                        }
                                        stepObject.requirements.items.push_back(item);
                                        if(stepItem.hasAttribute("monster") && stepItem.hasAttribute("rate"))
                                        {
                                            CatchChallenger::Quest::ItemMonster itemMonster;
                                            itemMonster.item=item.item;

                                            const std::vector<std::string> &tempStringList=stringsplit(stepItem.attribute("monster").toStdString(),';');
                                            unsigned int index=0;
                                            while(index<tempStringList.size())
                                            {
                                                const uint32_t &tempInt=stringtouint32(tempStringList.at(index),&ok);
                                                if(ok)
                                                    itemMonster.monsters.push_back(tempInt);
                                                index++;
                                            }

                                            std::string rateString=stepItem.attribute("rate").toStdString();
                                            stringreplaceOne(rateString,"%","");
                                            itemMonster.rate=stringtouint8(rateString,&ok);
                                            if(ok)
                                                stepObject.itemsMonster.push_back(itemMonster);
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the file: " << file << ", step id is not a number " << stepItem.attribute("id").toStdString() << ": child.tagName(): " << step.tagName().toStdString() << " (at line: " << step.lineNumber() << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Has not the attribute: id, step have not id attribute: child.tagName(): " << step.tagName().toStdString() << " (at line: " << step.lineNumber() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << step.tagName().toStdString() << " (at line: " << step.lineNumber() << ")" << std::endl;
                            stepItem = stepItem.nextSiblingElement("item");
                        }
                    }
                    //do the fight
                    {
                        QDomElement fightItem = step.firstChildElement("fight");
                        while(!fightItem.isNull())
                        {
                            if(fightItem.isElement())
                            {
                                if(fightItem.hasAttribute("id"))
                                {
                                    const uint32_t &fightId=fightItem.attribute("id").toUInt(&ok);
                                    if(ok)
                                        stepObject.requirements.fightId.push_back(fightId);
                                    else
                                        std::cerr << "Unable to open the file: " << file << ", step id is not a number " << fightItem.attribute("id").toStdString() << ": child.tagName(): " << fightItem.tagName().toStdString() << " (at line: " << fightItem.lineNumber() << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Has attribute: %1, step have not id attribute: child.tagName(): " << fightItem.tagName().toStdString() << " (at line: " << fightItem.lineNumber() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << fightItem.tagName().toStdString() << " (at line: " << fightItem.lineNumber() << ")" << std::endl;
                            fightItem = fightItem.nextSiblingElement("fight");
                        }
                    }
                    steps[id]=stepObject;
                }
                else
                    std::cerr << "Unable to open the file: " << file << ", step id is not a number: child.tagName(): " << step.tagName().toStdString() << " (at line: " << step.lineNumber() << ")" << std::endl;
            }
            else
                std::cerr << "Has attribute: %1, step have not id attribute: child.tagName(): " << step.tagName().toStdString() << " (at line: " << step.lineNumber() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << step.tagName().toStdString() << " (at line: " << step.lineNumber() << ")" << std::endl;
        step = step.nextSiblingElement("step");
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
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile plantsFile(QString::fromStdString(file));
        if(!plantsFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << plantsFile.errorString().toStdString() << std::endl;
            return plants;
        }
        const QByteArray &xmlContent=plantsFile.readAll();
        plantsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return plants;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="plants")
    {
        std::cerr << "Unable to open the file: " << file << ", \"plants\" root balise not found for reputation of the xml file" << std::endl;
        return plants;
    }

    //load the content
    bool ok,ok2;
    QDomElement plantItem = root.firstChildElement("plant");
    while(!plantItem.isNull())
    {
        if(plantItem.isElement())
        {
            if(plantItem.hasAttribute("id") && plantItem.hasAttribute("itemUsed"))
            {
                const uint8_t &id=plantItem.attribute("id").toUShort(&ok);
                const uint32_t &itemUsed=plantItem.attribute("itemUsed").toUInt(&ok2);
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
                            QDomElement requirementsItem = plantItem.firstChildElement("requirements");
                            if(!requirementsItem.isNull() && requirementsItem.isElement())
                            {
                                QDomElement reputationItem = requirementsItem.firstChildElement("reputation");
                                while(!reputationItem.isNull())
                                {
                                    if(reputationItem.isElement())
                                    {
                                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("level"))
                                        {
                                            if(reputationNameToId.find(reputationItem.attribute("type").toStdString())!=reputationNameToId.cend())
                                            {
                                                ReputationRequirements reputationRequirements;
                                                std::string stringLevel=reputationItem.attribute("level").toStdString();
                                                reputationRequirements.positif=!stringStartWith(stringLevel,"-");
                                                if(!reputationRequirements.positif)
                                                    stringLevel.erase(0,1);
                                                reputationRequirements.level=stringtouint8(stringLevel,&ok);
                                                if(ok)
                                                {
                                                    reputationRequirements.reputationId=reputationNameToId.at(reputationItem.attribute("type").toStdString());
                                                    plant.requirements.reputation.push_back(reputationRequirements);
                                                }
                                            }
                                            else
                                                std::cerr << "Reputation type not found: " << reputationItem.attribute("type").toStdString() << ", have not the id, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                    reputationItem = reputationItem.nextSiblingElement("reputation");
                                }
                            }
                        }
                        {
                            QDomElement rewardsItem = plantItem.firstChildElement("rewards");
                            if(!rewardsItem.isNull() && rewardsItem.isElement())
                            {
                                QDomElement reputationItem = rewardsItem.firstChildElement("reputation");
                                while(!reputationItem.isNull())
                                {
                                    if(reputationItem.isElement())
                                    {
                                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("point"))
                                        {
                                            if(reputationNameToId.find(reputationItem.attribute("type").toStdString())!=reputationNameToId.cend())
                                            {
                                                ReputationRewards reputationRewards;
                                                reputationRewards.point=reputationItem.attribute("point").toInt(&ok);
                                                if(ok)
                                                {
                                                    reputationRewards.reputationId=reputationNameToId.at(reputationItem.attribute("type").toStdString());
                                                    plant.rewards.reputation.push_back(reputationRewards);
                                                }
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                    reputationItem = reputationItem.nextSiblingElement("reputation");
                                }
                            }
                        }
                        ok=false;
                        QDomElement quantity = plantItem.firstChildElement("quantity");
                        if(!quantity.isNull())
                        {
                            if(quantity.isElement())
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
                        QDomElement grow = plantItem.firstChildElement("grow");
                        if(!grow.isNull())
                        {
                            if(grow.isElement())
                            {
                                const QDomElement &fruits = grow.firstChildElement("fruits");
                                if(!fruits.isNull())
                                {
                                    if(fruits.isElement())
                                    {
                                        plant.fruits_seconds=fruits.text().toUInt(&ok2)*60;
                                        plant.sprouted_seconds=plant.fruits_seconds;
                                        plant.taller_seconds=plant.fruits_seconds;
                                        plant.flowering_seconds=plant.fruits_seconds;
                                        if(!ok2)
                                        {
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << fruits.text().toStdString() << " child.tagName(): " << fruits.tagName().toStdString() << " (at line: " << fruits.lineNumber() << ")" << std::endl;
                                            ok=false;
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "Unable to parse the plants file: " << file << ", fruits is not an element: child.tagName(): " << fruits.tagName().toStdString() << " (at line: " << fruits.lineNumber() << ")" << std::endl;
                                    }
                                }
                                else
                                {
                                    ok=false;
                                    std::cerr << "Unable to parse the plants file: " << file << ", fruits is null: child.tagName(): " << fruits.tagName().toStdString() << " (at line: " << fruits.lineNumber() << ")" << std::endl;
                                }
                                const QDomElement &sprouted = grow.firstChildElement("sprouted");
                                if(!sprouted.isNull())
                                {
                                    if(sprouted.isElement())
                                    {
                                        plant.sprouted_seconds=sprouted.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << sprouted.text().toStdString() << " child.tagName(): " << sprouted.tagName().toStdString() << " (at line: " << sprouted.lineNumber() << ")" << std::endl;
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not an element: child.tagName(): " << sprouted.tagName().toStdString() << " (at line: " << sprouted.lineNumber() << ")" << std::endl;
                                }
                                const QDomElement &taller = grow.firstChildElement("taller");
                                if(!taller.isNull())
                                {
                                    if(taller.isElement())
                                    {
                                        plant.taller_seconds=taller.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << taller.text().toStdString() << " child.tagName(): " << taller.tagName().toStdString() << " (at line: " << taller.lineNumber() << ")" << std::endl;
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        std::cerr << "Unable to parse the plants file: " << file << ", taller is not an element: child.tagName(): " << taller.tagName().toStdString() << " (at line: " << taller.lineNumber() << ")" << std::endl;
                                }
                                const QDomElement &flowering = grow.firstChildElement("flowering");
                                if(!flowering.isNull())
                                {
                                    if(flowering.isElement())
                                    {
                                        plant.flowering_seconds=flowering.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            ok=false;
                                            std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << flowering.text().toStdString() << " child.tagName(): " << flowering.tagName().toStdString() << " (at line: " << flowering.lineNumber() << ")" << std::endl;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        std::cerr << "Unable to parse the plants file: " << file << ", flowering is not an element: child.tagName(): " << flowering.tagName().toStdString() << " (at line: " << flowering.lineNumber() << ")" << std::endl;
                                }
                            }
                            else
                                std::cerr << "Unable to parse the plants file: " << file << ", grow is not an element: child.tagName(): child.tagName(): " << grow.tagName().toStdString() << " (at line: " << grow.lineNumber() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Unable to parse the plants file: " << file << ", grow is null: child.tagName(): child.tagName(): " << grow.tagName().toStdString() << " (at line: " << grow.lineNumber() << ")" << std::endl;
                        if(ok)
                        {
                            bool needIntermediateTimeFix=false;
                            if(plant.flowering_seconds>=plant.fruits_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", flowering_seconds>=fruits_seconds: child.tagName(): child.tagName(): " << grow.tagName().toStdString() << " (at line: " << grow.lineNumber() << ")" << std::endl;
                            }
                            if(plant.taller_seconds>=plant.flowering_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", taller_seconds>=flowering_seconds: child.tagName(): " << grow.tagName().toStdString() << " (at line: " << grow.lineNumber() << ")" << std::endl;
                            }
                            if(plant.sprouted_seconds>=plant.taller_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", sprouted_seconds>=taller_seconds: child.tagName(): " << grow.tagName().toStdString() << " (at line: " << grow.lineNumber() << ")" << std::endl;
                            }
                            if(plant.sprouted_seconds<=0)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    std::cerr << "Warning when parse the plants file: " << file << ", sprouted_seconds<=0: child.tagName(): " << grow.tagName().toStdString() << " (at line: " << grow.lineNumber() << ")" << std::endl;
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
                        std::cerr << "Unable to open the plants file: " << file << ", id number already set: child.tagName(): " << plantItem.tagName().toStdString() << " (at line: " << plantItem.lineNumber() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the plants file: " << file << ", id is not a number: child.tagName(): " << plantItem.tagName().toStdString() << " (at line: " << plantItem.lineNumber() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the plants file: " << file << ", have not the plant id: child.tagName(): " << plantItem.tagName().toStdString() << " (at line: " << plantItem.lineNumber() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the plants file: " << file << ", is not an element: child.tagName(): " << plantItem.tagName().toStdString() << " (at line: " << plantItem.lineNumber() << ")" << std::endl;
        plantItem = plantItem.nextSiblingElement("plant");
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
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile craftingRecipesFile(QString::fromStdString(file));
        if(!craftingRecipesFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << craftingRecipesFile.errorString().toStdString() << std::endl;
            return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
        }
        const QByteArray &xmlContent=craftingRecipesFile.readAll();
        craftingRecipesFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="recipes")
    {
        std::cerr << "Unable to open the file: " << file << ", \"recipes\" root balise not found for reputation of the xml file" << std::endl;
        return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
    }

    //load the content
    bool ok,ok2,ok3;
    QDomElement recipeItem = root.firstChildElement("recipe");
    while(!recipeItem.isNull())
    {
        if(recipeItem.isElement())
        {
            if(recipeItem.hasAttribute("id") && recipeItem.hasAttribute("itemToLearn") && recipeItem.hasAttribute("doItemId"))
            {
                uint8_t success=100;
                if(recipeItem.hasAttribute("success"))
                {
                    const uint8_t &tempShort=recipeItem.attribute("success").toUShort(&ok);
                    if(ok)
                    {
                        if(tempShort>100)
                            std::cerr << "preload_crafting_recipes() success can't be greater than 100 for crafting recipe file: " << file << ", child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                        else
                            success=tempShort;
                    }
                    else
                        std::cerr << "preload_crafting_recipes() success in not an number for crafting recipe file: " << file << ", child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                }
                uint16_t quantity=1;
                if(recipeItem.hasAttribute("quantity"))
                {
                    const uint32_t &tempShort=recipeItem.attribute("quantity").toUInt(&ok);
                    if(ok)
                    {
                        if(tempShort>65535)
                            std::cerr << "preload_crafting_recipes() quantity can't be greater than 65535 for crafting recipe file: " << file << ", child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                        else
                            quantity=tempShort;
                    }
                    else
                        std::cerr << "preload_crafting_recipes() quantity in not an number for crafting recipe file: " << file << ", child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                }

                const uint32_t &id=recipeItem.attribute("id").toUInt(&ok);
                const uint32_t &itemToLearn=recipeItem.attribute("itemToLearn").toUInt(&ok2);
                const uint32_t &doItemId=recipeItem.attribute("doItemId").toUInt(&ok3);
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
                            QDomElement requirementsItem = recipeItem.firstChildElement("requirements");
                            if(!requirementsItem.isNull() && requirementsItem.isElement())
                            {
                                QDomElement reputationItem = requirementsItem.firstChildElement("reputation");
                                while(!reputationItem.isNull())
                                {
                                    if(reputationItem.isElement())
                                    {
                                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("level"))
                                        {
                                            if(reputationNameToId.find(reputationItem.attribute("type").toStdString())!=reputationNameToId.cend())
                                            {
                                                ReputationRequirements reputationRequirements;
                                                std::string stringLevel=reputationItem.attribute("level").toStdString();
                                                reputationRequirements.positif=!stringStartWith(stringLevel,"-");
                                                if(!reputationRequirements.positif)
                                                    stringLevel.erase(0,1);
                                                reputationRequirements.level=stringtouint8(stringLevel,&ok);
                                                if(ok)
                                                {
                                                    reputationRequirements.reputationId=reputationNameToId.at(reputationItem.attribute("type").toStdString());
                                                    recipe.requirements.reputation.push_back(reputationRequirements);
                                                }
                                            }
                                            else
                                                std::cerr << "Reputation type not found: " << reputationItem.attribute("type").toStdString() << ", have not the id, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                    reputationItem = reputationItem.nextSiblingElement("reputation");
                                }
                            }
                        }
                        {
                            QDomElement rewardsItem = recipeItem.firstChildElement("rewards");
                            if(!rewardsItem.isNull() && rewardsItem.isElement())
                            {
                                QDomElement reputationItem = rewardsItem.firstChildElement("reputation");
                                while(!reputationItem.isNull())
                                {
                                    if(reputationItem.isElement())
                                    {
                                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("point"))
                                        {
                                            if(reputationNameToId.find(reputationItem.attribute("type").toStdString())!=reputationNameToId.cend())
                                            {
                                                ReputationRewards reputationRewards;
                                                reputationRewards.point=reputationItem.attribute("point").toInt(&ok);
                                                if(ok)
                                                {
                                                    reputationRewards.reputationId=reputationNameToId.at(reputationItem.attribute("type").toStdString());
                                                    recipe.rewards.reputation.push_back(reputationRewards);
                                                }
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child.tagName(): " << reputationItem.tagName().toStdString() << " (at line: " << reputationItem.lineNumber() << ")" << std::endl;
                                    reputationItem = reputationItem.nextSiblingElement("reputation");
                                }
                            }
                        }
                        QDomElement material = recipeItem.firstChildElement("material");
                        while(!material.isNull() && ok)
                        {
                            if(material.isElement())
                            {
                                if(material.hasAttribute("itemId"))
                                {
                                    const uint32_t &itemId=material.attribute("itemId").toUInt(&ok2);
                                    if(!ok2)
                                    {
                                        ok=false;
                                        std::cerr << "preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: " << file << ": child.tagName(): " << material.tagName().toStdString() << " (at line: " << material.lineNumber() << ")" << std::endl;
                                        break;
                                    }
                                    uint16_t quantity=1;
                                    if(material.hasAttribute("quantity"))
                                    {
                                        const uint32_t &tempShort=material.attribute("quantity").toUInt(&ok2);
                                        if(ok2)
                                        {
                                            if(tempShort>65535)
                                            {
                                                ok=false;
                                                std::cerr << "preload_crafting_recipes() material quantity can't be greater than 65535 for crafting recipe file: " << file << ": child.tagName(): " << material.tagName().toStdString() << " (at line: " << material.lineNumber() << ")" << std::endl;
                                                break;
                                            }
                                            else
                                                quantity=tempShort;
                                        }
                                        else
                                        {
                                            ok=false;
                                            std::cerr << "preload_crafting_recipes() material quantity in not an number for crafting recipe file: " << file << ": child.tagName(): " << material.tagName().toStdString() << " (at line: " << material.lineNumber() << ")" << std::endl;
                                            break;
                                        }
                                    }
                                    if(items.find(itemId)==items.cend())
                                    {
                                        ok=false;
                                        std::cerr << "preload_crafting_recipes() material itemId in not into items list for crafting recipe file: " << file << ": child.tagName(): " << material.tagName().toStdString() << " (at line: " << material.lineNumber() << ")" << std::endl;
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
                                        std::cerr << "id of item already into resource or product list: %1: child.tagName(): " << material.tagName().toStdString() << " (at line: " << material.lineNumber() << ")" << std::endl;
                                    }
                                    else
                                    {
                                        if(recipe.doItemId==newMaterial.item)
                                        {
                                            std::cerr << "id of item already into resource or product list: %1: child.tagName(): " << material.tagName().toStdString() << " (at line: " << material.lineNumber() << ")" << std::endl;
                                            ok=false;
                                        }
                                        else
                                            recipe.materials.push_back(newMaterial);
                                    }
                                }
                                else
                                    std::cerr << "preload_crafting_recipes() material have not attribute itemId for crafting recipe file: " << file << ": child.tagName(): " << material.tagName().toStdString() << " (at line: " << material.lineNumber() << ")" << std::endl;
                            }
                            else
                                std::cerr << "preload_crafting_recipes() material is not an element for crafting recipe file: " << file << ": child.tagName(): " << material.tagName().toStdString() << " (at line: " << material.lineNumber() << ")" << std::endl;
                            material = material.nextSiblingElement("material");
                        }
                        if(ok)
                        {
                            if(items.find(recipe.itemToLearn)==items.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() itemToLearn is not into items list for crafting recipe file: " << file << ": child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(items.find(recipe.doItemId)==items.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() doItemId is not into items list for crafting recipe file: " << file << ": child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.find(recipe.itemToLearn)!=itemToCrafingRecipes.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() itemToLearn already used to learn another recipe: " << itemToCrafingRecipes.at(recipe.doItemId) << ": file: " << file << " child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(recipe.itemToLearn==recipe.doItemId)
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() the product of the recipe can't be them self: " << id << ": file: " << file << ": child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.find(recipe.doItemId)!=itemToCrafingRecipes.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() the product of the recipe can't be a recipe: " << itemToCrafingRecipes.at(recipe.doItemId) << ": file: " << file << ": child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                            }
                        }
                        if(ok)
                        {
                            crafingRecipes[id]=recipe;
                            itemToCrafingRecipes[recipe.itemToLearn]=id;
                        }
                    }
                    else
                        std::cerr << "Unable to open the crafting recipe file: " << file << ", id number already set: child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the crafting recipe file: " << file << ", id is not a number: child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the crafting recipe file: " << file << ", have not the crafting recipe id: child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the crafting recipe file: " << file << ", is not an element: child.tagName(): " << recipeItem.tagName().toStdString() << " (at line: " << recipeItem.lineNumber() << ")" << std::endl;
        recipeItem = recipeItem.nextSiblingElement("recipe");
    }
    return std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
}

std::unordered_map<uint16_t,Industry> DatapackGeneralLoader::loadIndustries(const std::string &folder,const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<uint16_t,Industry> industries;
    QDir dir(folder);
    const QFileInfoList &fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        QDomDocument domDocument;
        const std::string &file=fileList.at(file_index).absoluteFilePath();
        //open and quick check the file
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            #endif
            QFile industryFile(file);
            if(!industryFile.open(QIODevice::ReadOnly))
            {
                std::cerr << "Unable to open the file: " << file << ", error: " << industryFile.errorString().toStdString() << std::endl;
                file_index++;
                continue;
            }
            const QByteArray &xmlContent=industryFile.readAll();
            industryFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        const QDomElement &root = domDocument.documentElement();
        if(root.tagName()!="industries")
        {
            std::cerr << "Unable to open the file: " << file << ", \"industries\" root balise not found for reputation of the xml file" << std::endl;
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2,ok3;
        QDomElement industryItem = root.firstChildElement("industrialrecipe");
        while(!industryItem.isNull())
        {
            if(industryItem.isElement())
            {
                if(industryItem.hasAttribute("id") && industryItem.hasAttribute("time") && industryItem.hasAttribute("cycletobefull"))
                {
                    Industry industry;
                    const uint32_t &id=industryItem.attribute("id").toUInt(&ok);
                    industry.time=industryItem.attribute("time").toUInt(&ok2);
                    industry.cycletobefull=industryItem.attribute("cycletobefull").toUInt(&ok3);
                    if(ok && ok2 && ok3)
                    {
                        if(!industries.contains(id))
                        {
                            if(industry.time<60*5)
                            {
                                std::cerr << "the time need be greater than 5*60 seconds to not slow down the server: %4, %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber()).arg(industry.time);
                                industry.time=60*5;
                            }
                            if(industry.cycletobefull<1)
                            {
                                std::cerr << "cycletobefull need be greater than 0: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                industry.cycletobefull=1;
                            }
                            else if(industry.cycletobefull>65535)
                            {
                                std::cerr << "cycletobefull need be lower to 10 to not slow down the server, use the quantity: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                industry.cycletobefull=10;
                            }
                            //resource
                            {
                                QDomElement resourceItem = industryItem.firstChildElement("resource");
                                ok=true;
                                while(!resourceItem.isNull() && ok)
                                {
                                    if(resourceItem.isElement())
                                    {
                                        Industry::Resource resource;
                                        resource.quantity=1;
                                        if(resourceItem.hasAttribute("quantity"))
                                        {
                                            resource.quantity=resourceItem.attribute("quantity").toUInt(&ok);
                                            if(!ok)
                                                std::cerr << "quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(resourceItem.hasAttribute("id"))
                                            {
                                                resource.item=resourceItem.attribute("id").toUInt(&ok);
                                                if(!ok)
                                                    std::cerr << "id is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                else if(!items.contains(resource.item))
                                                {
                                                    ok=false;
                                                    std::cerr << "id is not into the item list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
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
                                                        std::cerr << "id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
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
                                                            std::cerr << "id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                        }
                                                        else
                                                            industry.resources << resource;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                std::cerr << "have not the id attribute: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "is not a elements: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                    }
                                    resourceItem = resourceItem.nextSiblingElement("resource");
                                }
                            }

                            //product
                            if(ok)
                            {
                                QDomElement productItem = industryItem.firstChildElement("product");
                                ok=true;
                                while(!productItem.isNull() && ok)
                                {
                                    if(productItem.isElement())
                                    {
                                        Industry::Product product;
                                        product.quantity=1;
                                        if(productItem.hasAttribute("quantity"))
                                        {
                                            product.quantity=productItem.attribute("quantity").toUInt(&ok);
                                            if(!ok)
                                                std::cerr << "quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(productItem.hasAttribute("id"))
                                            {
                                                product.item=productItem.attribute("id").toUInt(&ok);
                                                if(!ok)
                                                    std::cerr << "id is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                else if(!items.contains(product.item))
                                                {
                                                    ok=false;
                                                    std::cerr << "id is not into the item list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
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
                                                        std::cerr << "id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
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
                                                            std::cerr << "id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                        }
                                                        else
                                                            industry.products << product;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                std::cerr << "have not the id attribute: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "is not a elements: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                    }
                                    productItem = productItem.nextSiblingElement("product");
                                }
                            }

                            //add
                            if(ok)
                            {
                                if(industry.products.isEmpty() || industry.resources.isEmpty())
                                    std::cerr << "product or resources is empty: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                else
                                    industries[id]=industry;
                            }
                        }
                        else
                            std::cerr << "Unable to open the industries file: " << file << ", id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                    }
                    else
                        std::cerr << "Unable to open the industries file: " << file << ", id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                }
                else
                    std::cerr << "Unable to open the industries file: " << file << ", have not the id: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
            }
            else
                std::cerr << "Unable to open the industries file: " << file << ", is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
            industryItem = industryItem.nextSiblingElement("industrialrecipe");
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
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile industriesLinkFile(file);
        if(!industriesLinkFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << industriesLinkFile.errorString().toStdString() << std::endl;
            return industriesLink;
        }
        const QByteArray &xmlContent=industriesLinkFile.readAll();
        industriesLinkFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return industriesLink;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="industries")
    {
        std::cerr << "Unable to open the file: " << file << ", \"industries\" root balise not found for reputation of the xml file" << std::endl;
        return industriesLink;
    }

    //load the content
    bool ok,ok2;
    QDomElement linkItem = root.firstChildElement("link");
    while(!linkItem.isNull())
    {
        if(linkItem.isElement())
        {
            if(linkItem.hasAttribute("industrialrecipe") && linkItem.hasAttribute("industry"))
            {
                const uint32_t &industry_id=linkItem.attribute("industrialrecipe").toUInt(&ok);
                const uint32_t &factory_id=linkItem.attribute("industry").toUInt(&ok2);
                if(ok && ok2)
                {
                    if(!industriesLink.contains(factory_id))
                    {
                        if(industries.contains(industry_id))
                        {
                            industriesLink[factory_id].industry=industry_id;
                            IndustryLink *industryLink=&industriesLink[factory_id];
                            {
                                {
                                    QDomElement requirementsItem = linkItem.firstChildElement("requirements");
                                    if(!requirementsItem.isNull() && requirementsItem.isElement())
                                    {
                                        QDomElement reputationItem = requirementsItem.firstChildElement("reputation");
                                        while(!reputationItem.isNull())
                                        {
                                            if(reputationItem.isElement())
                                            {
                                                if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("level"))
                                                {
                                                    if(reputationNameToId.contains(reputationItem.attribute("type")))
                                                    {
                                                        ReputationRequirements reputationRequirements;
                                                        std::string stringLevel=reputationItem.attribute("level");
                                                        reputationRequirements.positif=!stringLevel.startsWith("-");
                                                        if(!reputationRequirements.positif)
                                                            stringLevel.remove(0,1);
                                                        reputationRequirements.level=stringLevel.toUShort(&ok);
                                                        if(ok)
                                                        {
                                                            reputationRequirements.reputationId=reputationNameToId.at(reputationItem.attribute("type"));
                                                            industryLink->requirements.reputation << reputationRequirements;
                                                        }
                                                    }
                                                    else
                                                        std::cerr << "Reputation type not found: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                                }
                                                else
                                                    std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                            }
                                            else
                                                std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                            reputationItem = reputationItem.nextSiblingElement("reputation");
                                        }
                                    }
                                }
                                {
                                    QDomElement rewardsItem = linkItem.firstChildElement("rewards");
                                    if(!rewardsItem.isNull() && rewardsItem.isElement())
                                    {
                                        QDomElement reputationItem = rewardsItem.firstChildElement("reputation");
                                        while(!reputationItem.isNull())
                                        {
                                            if(reputationItem.isElement())
                                            {
                                                if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("point"))
                                                {
                                                    if(reputationNameToId.contains(reputationItem.attribute("type")))
                                                    {
                                                        ReputationRewards reputationRewards;
                                                        reputationRewards.point=reputationItem.attribute("point").toInt(&ok);
                                                        if(ok)
                                                        {
                                                            reputationRewards.reputationId=reputationNameToId.at(reputationItem.attribute("type"));
                                                            industryLink->rewards.reputation << reputationRewards;
                                                        }
                                                    }
                                                }
                                                else
                                                    std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                            }
                                            else
                                                std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                            reputationItem = reputationItem.nextSiblingElement("reputation");
                                        }
                                    }
                                }
                            }
                        }
                        else
                            std::cerr << "Industry id for factory is not found: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
                    }
                    else
                        std::cerr << "Factory already found: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
                }
                else
                    std::cerr << "Unable to open the industries link file: " << file << ", the attribute is not a number, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
            }
            else
                std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
        }
        else
            std::cerr << "Unable to open the industries link file: " << file << ", is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
        linkItem = linkItem.nextSiblingElement("link");
    }
    return industriesLink;
}

ItemFull DatapackGeneralLoader::loadItems(const std::string &folder,const std::unordered_map<uint8_t,Buff> &monsterBuffs)
{
    #ifdef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    (void)monsterBuffs;
    #endif
    ItemFull items;
    QDir dir(folder);
    const std::vector<std::string> &fileList=FacilityLibGeneral::listFolder(dir.absolutePath()+"/");
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).endsWith(".xml"))
        {
            file_index++;
            continue;
        }
        const std::string &file=folder+fileList.at(file_index);
        if(!QFileInfo(file).isFile())
        {
            file_index++;
            continue;
        }
        QDomDocument domDocument;
        //open and quick check the file
        if(!file.endsWith(".xml"))
        {
            file_index++;
            continue;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            #endif
            QFile itemsFile(file);
            if(!itemsFile.open(QIODevice::ReadOnly))
            {
                std::cerr << "Unable to open the file: " << file << ", error: " << itemsFile.errorString().toStdString() << std::endl;
                file_index++;
                continue;
            }
            const QByteArray &xmlContent=itemsFile.readAll();
            itemsFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        const QDomElement &root = domDocument.documentElement();
        if(root.tagName()!="items")
        {
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        QDomElement item = root.firstChildElement("item");
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute("id"))
                {
                    const uint32_t &id=item.attribute("id").toUInt(&ok);
                    if(ok)
                    {
                        if(!items.item.contains(id))
                        {
                            //load the price
                            {
                                if(item.hasAttribute("price"))
                                {
                                    bool ok;
                                    items.item[id].price=item.attribute("price").toUInt(&ok);
                                    if(!ok)
                                    {
                                        std::cerr << "price is not a number: child.tagName(): %1 (at line: %2)").arg(item.tagName()).arg(item.lineNumber());
                                        items.item[id].price=0;
                                    }
                                }
                                else
                                {
                                    /*if(!item.hasAttribute("quest") || item.attribute("quest")!="yes")
                                        std::cerr << "For parse item: Price not found, default to 0 (not sellable): child.tagName(): %1 (%2 at line: %3)").arg(item.tagName()).arg(file).arg(item.lineNumber());*/
                                    items.item[id].price=0;
                                }
                            }
                            //load the consumeAtUse
                            {
                                if(item.hasAttribute("consumeAtUse"))
                                {
                                    if(item.attribute("consumeAtUse")=="false")
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
                                QDomElement trapItem = item.firstChildElement("trap");
                                if(!trapItem.isNull())
                                {
                                    if(trapItem.isElement())
                                    {
                                        Trap trap;
                                        trap.bonus_rate=1.0;
                                        if(trapItem.hasAttribute("bonus_rate"))
                                        {
                                            float bonus_rate=trapItem.attribute("bonus_rate").toFloat(&ok);
                                            if(ok)
                                                trap.bonus_rate=bonus_rate;
                                            else
                                                std::cerr << "Unable to open the file: " << file << ", bonus_rate is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(trapItem.tagName()).arg(trapItem.lineNumber());
                                        }
                                        else
                                            std::cerr << "Unable to open the file: " << file << ", trap have not the attribute bonus_rate: child.tagName(): %2 (at line: %3)").arg(file).arg(trapItem.tagName()).arg(trapItem.lineNumber());
                                        items.trap[id]=trap;
                                        haveAnEffect=true;
                                    }
                                }
                            }
                            //load the repel
                            if(!haveAnEffect)
                            {
                                QDomElement repelItem = item.firstChildElement("repel");
                                if(!repelItem.isNull())
                                {
                                    if(repelItem.isElement())
                                    {
                                        if(repelItem.hasAttribute("step"))
                                        {
                                            const uint32_t &step=repelItem.attribute("step").toUInt(&ok);
                                            if(ok)
                                            {
                                                if(step>0)
                                                {
                                                    items.repel[id]=step;
                                                    haveAnEffect=true;
                                                }
                                                else
                                                    std::cerr << "Unable to open the file: " << file << ", step is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                            }
                                            else
                                                std::cerr << "Unable to open the file: " << file << ", step is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                        }
                                        else
                                            std::cerr << "Unable to open the file: " << file << ", repel have not the attribute step: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                    }
                                }
                            }
                            //load the monster effect
                            if(!haveAnEffect)
                            {
                                {
                                    QDomElement hpItem = item.firstChildElement("hp");
                                    while(!hpItem.isNull())
                                    {
                                        if(hpItem.isElement())
                                        {
                                            if(hpItem.hasAttribute("add"))
                                            {
                                                if(hpItem.attribute("add")=="all")
                                                {
                                                    MonsterItemEffect monsterItemEffect;
                                                    monsterItemEffect.type=MonsterItemEffectType_AddHp;
                                                    monsterItemEffect.value=-1;
                                                    items.monsterItemEffect.insert(id,monsterItemEffect);
                                                }
                                                else
                                                {
                                                    if(!hpItem.attribute("add").contains("%"))//todo this part
                                                    {
                                                        const int32_t &add=hpItem.attribute("add").toUInt(&ok);
                                                        if(ok)
                                                        {
                                                            if(add>0)
                                                            {
                                                                MonsterItemEffect monsterItemEffect;
                                                                monsterItemEffect.type=MonsterItemEffectType_AddHp;
                                                                monsterItemEffect.value=add;
                                                                items.monsterItemEffect.insert(id,monsterItemEffect);
                                                            }
                                                            else
                                                                std::cerr << "Unable to open the file: " << file << ", add is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                                        }
                                                        else
                                                            std::cerr << "Unable to open the file: " << file << ", add is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                                    }
                                                }
                                            }
                                            else
                                                std::cerr << "Unable to open the file: " << file << ", hp have not the attribute add: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                        }
                                        hpItem = hpItem.nextSiblingElement("hp");
                                    }
                                }
                                #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                                {
                                    QDomElement buffItem = item.firstChildElement("buff");
                                    while(!buffItem.isNull())
                                    {
                                        if(buffItem.isElement())
                                        {
                                            if(buffItem.hasAttribute("remove"))
                                            {
                                                if(buffItem.attribute("remove")=="all")
                                                {
                                                    MonsterItemEffect monsterItemEffect;
                                                    monsterItemEffect.type=MonsterItemEffectType_RemoveBuff;
                                                    monsterItemEffect.value=-1;
                                                    items.monsterItemEffect.insert(id,monsterItemEffect);
                                                }
                                                else
                                                {
                                                    const int32_t &remove=buffItem.attribute("remove").toUInt(&ok);
                                                    if(ok)
                                                    {
                                                        if(remove>0)
                                                        {
                                                            if(monsterBuffs.contains(remove))
                                                            {
                                                                MonsterItemEffect monsterItemEffect;
                                                                monsterItemEffect.type=MonsterItemEffectType_RemoveBuff;
                                                                monsterItemEffect.value=remove;
                                                                items.monsterItemEffect.insert(id,monsterItemEffect);
                                                            }
                                                            else
                                                                std::cerr << "Unable to open the file: " << file << ", buff item to remove is not found: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                                        }
                                                        else
                                                            std::cerr << "Unable to open the file: " << file << ", step is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                                    }
                                                    else
                                                        std::cerr << "Unable to open the file: " << file << ", step is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                                }
                                            }
                                            /// \todo
                                             /* else
                                                std::cerr << "Unable to open the file: " << file << ", buff have not the attribute know attribute like remove: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());*/
                                        }
                                        buffItem = buffItem.nextSiblingElement("buff");
                                    }
                                }
                                #endif
                                if(items.monsterItemEffect.contains(id))
                                    haveAnEffect=true;
                            }
                            //load the monster offline effect
                            if(!haveAnEffect)
                            {
                                QDomElement levelItem = item.firstChildElement("level");
                                while(!levelItem.isNull())
                                {
                                    if(levelItem.isElement())
                                    {
                                        if(levelItem.hasAttribute("up"))
                                        {
                                            const uint32_t &levelUp=levelItem.attribute("up").toUInt(&ok);
                                            if(!ok)
                                                std::cerr << "Unable to open the file: " << file << ", level up is not possitive number: child.tagName(): %2 (at line: %3)").arg(file).arg(levelItem.tagName()).arg(levelItem.lineNumber());
                                            else if(levelUp<=0)
                                                std::cerr << "Unable to open the file: " << file << ", level up is greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(levelItem.tagName()).arg(levelItem.lineNumber());
                                            else
                                            {
                                                MonsterItemEffectOutOfFight monsterItemEffectOutOfFight;
                                                monsterItemEffectOutOfFight.type=MonsterItemEffectTypeOutOfFight_AddLevel;
                                                monsterItemEffectOutOfFight.value=levelUp;
                                                items.monsterItemEffectOutOfFight.insert(id,monsterItemEffectOutOfFight);
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the file: " << file << ", level have not the attribute up: child.tagName(): %2 (at line: %3)").arg(file).arg(levelItem.tagName()).arg(levelItem.lineNumber());
                                    }
                                    levelItem = levelItem.nextSiblingElement("level");
                                }
                            }
                        }
                        else
                            std::cerr << "Unable to open the file: " << file << ", id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
                }
                else
                    std::cerr << "Unable to open the file: " << file << ", have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
            item = item.nextSiblingElement("item");
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

std::pair<std::vector<QDomElement>, std::vector<Profile> > DatapackGeneralLoader::loadProfileList(const std::string &datapackPath, const std::string &file,
                                                                                  #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                                                  const std::unordered_map<uint16_t, Item> &items,
                                                                                  #endif // CATCHCHALLENGER_CLASS_MASTER
                                                                                  const std::unordered_map<uint16_t,Monster> &monsters,const std::vector<Reputation> &reputations)
{
    Std::unordered_Set<std::string> idDuplicate;
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
            defaultforcedskinList << CommonDatapack::commonDatapack.skins.at(index);
            index++;
        }
    }
    std::pair<std::vector<QDomElement>, std::vector<Profile> > returnVar;
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
            return returnVar;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="profile")
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    bool ok;
    QDomElement startItem = root.firstChildElement("start");
    while(!startItem.isNull())
    {
        if(startItem.isElement())
        {
            Profile profile;

            if(startItem.hasAttribute("id"))
                profile.id=startItem.attribute("id");

            if(idDuplicate.contains(profile.id))
            {
                std::cerr << "Unable to open the xml file: " << file << ", id duplicate: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement("start");
                continue;
            }

            if(!profile.id.isEmpty() && !idDuplicate.contains(profile.id))
            {
                const QDomElement &forcedskin = startItem.firstChildElement("forcedskin");

                std::vector<std::string> forcedskinList;
                if(!forcedskin.isNull() && forcedskin.isElement() && forcedskin.hasAttribute("value"))
                    forcedskinList=forcedskin.attribute("value").split(";");
                else
                    forcedskinList=defaultforcedskinList;
                {
                    unsigned int index=0;
                    while(index<forcedskinList.size())
                    {
                        if(skinNameToId.contains(forcedskinList.at(index)))
                            profile.forcedskin << skinNameToId.at(forcedskinList.at(index));
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", skin %4 don't exists: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(forcedskinList.at(index)));
                        index++;
                    }
                }
                unsigned int index=0;
                while(index<profile.forcedskin.size())
                {
                    if(!QFile::exists(datapackPath+QLatin1String(DATAPACK_BASE_PATH_SKIN)+CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index))))
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", skin %4 don't exists into: %5: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(profile.forcedskin.at(index)).arg(datapackPath+QLatin1String(DATAPACK_BASE_PATH_SKIN)+CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index))));
                        profile.forcedskin.removeAt(index);
                    }
                    else
                        index++;
                }

                profile.cash=0;
                const QDomElement &cash = startItem.firstChildElement("cash");
                if(!cash.isNull() && cash.isElement() && cash.hasAttribute("value"))
                {
                    profile.cash=cash.attribute("value").toULongLong(&ok);
                    if(!ok)
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", cash is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        profile.cash=0;
                    }
                }
                QDomElement monstersElement = startItem.firstChildElement("monster");
                while(!monstersElement.isNull())
                {
                    Profile::Monster monster;
                    if(monstersElement.isElement() && monstersElement.hasAttribute("id") && monstersElement.hasAttribute("level") && monstersElement.hasAttribute("captured_with"))
                    {
                        monster.id=monstersElement.attribute("id").toUInt(&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", monster id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        if(ok)
                        {
                            monster.level=monstersElement.attribute("level").toUShort(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", monster level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        }
                        if(ok)
                        {
                            if(monster.level==0 || monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                                std::cerr << "Unable to open the xml file: " << file << ", monster level is not into the range: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.captured_with=monstersElement.attribute("captured_with").toUInt(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", captured_with is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        }
                        if(ok)
                        {
                            if(!monsters.contains(monster.id))
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", starter don't found the monster %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(monster.id));
                                ok=false;
                            }
                        }
                        if(ok)
                        {
                            #ifndef CATCHCHALLENGER_CLASS_MASTER
                            if(!items.contains(monster.captured_with))
                                std::cerr << "Unable to open the xml file: " << file << ", starter don't found the monster capture item %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(monster.id));
                            #endif // CATCHCHALLENGER_CLASS_MASTER
                        }
                        if(ok)
                            profile.monsters << monster;
                    }
                    monstersElement = monstersElement.nextSiblingElement("monster");
                }
                if(profile.monsters.empty())
                {
                    std::cerr << "Unable to open the xml file: " << file << ", not monster to load: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement("start");
                    continue;
                }
                QDomElement reputationElement = startItem.firstChildElement("reputation");
                while(!reputationElement.isNull())
                {
                    Profile::Reputation reputationTemp;
                    if(reputationElement.isElement() && reputationElement.hasAttribute("type") && reputationElement.hasAttribute("level"))
                    {
                        reputationTemp.level=reputationElement.attribute("level").toShort(&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", reputation level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        if(ok)
                        {
                            if(!reputationNameToId.contains(reputationElement.attribute("type")))
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", reputation type not found %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(reputationElement.attribute("type")));
                                ok=false;
                            }
                            if(ok)
                            {
                                reputationTemp.reputationId=reputationNameToId.at(reputationElement.attribute("type"));
                                if(reputationTemp.level==0)
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", reputation level is useless if level 0: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                    ok=false;
                                }
                                else if(reputationTemp.level<0)
                                {
                                    if((-reputationTemp.level)>reputations.at(reputationTemp.reputationId).reputation_negative.size())
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", reputation level is lower than minimal level for %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(reputationElement.attribute("type")));
                                        ok=false;
                                    }
                                }
                                else// if(reputationTemp.level>0)
                                {
                                    if((reputationTemp.level)>=reputations.at(reputationTemp.reputationId).reputation_positive.size())
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", reputation level is higther than maximal level for %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(reputationElement.attribute("type")));
                                        ok=false;
                                    }
                                }
                            }
                            if(ok)
                            {
                                reputationTemp.point=0;
                                if(reputationElement.hasAttribute("point"))
                                {
                                    reputationTemp.point=reputationElement.attribute("point").toInt(&ok);
                                    std::cerr << "Unable to open the xml file: " << file << ", reputation point is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                    if(ok)
                                    {
                                        if((reputationTemp.point>0 && reputationTemp.level<0) || (reputationTemp.point<0 && reputationTemp.level>=0))
                                            std::cerr << "Unable to open the xml file: " << file << ", reputation point is not negative/positive like the level: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                    }
                                }
                            }
                        }
                        if(ok)
                            profile.reputation << reputationTemp;
                    }
                    reputationElement = reputationElement.nextSiblingElement("reputation");
                }
                QDomElement itemElement = startItem.firstChildElement("item");
                while(!itemElement.isNull())
                {
                    Profile::Item itemTemp;
                    if(itemElement.isElement() && itemElement.hasAttribute("id"))
                    {
                        itemTemp.id=itemElement.attribute("id").toUInt(&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", item id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        if(ok)
                        {
                            itemTemp.quantity=0;
                            if(itemElement.hasAttribute("quantity"))
                            {
                                itemTemp.quantity=itemElement.attribute("quantity").toUInt(&ok);
                                if(!ok)
                                    std::cerr << "Unable to open the xml file: " << file << ", item quantity is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                if(ok)
                                {
                                    if(itemTemp.quantity==0)
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", item quantity is null: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                        ok=false;
                                    }
                                }
                                #ifndef CATCHCHALLENGER_CLASS_MASTER
                                if(ok)
                                {
                                    if(!items.contains(itemTemp.id))
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", item not found as know item %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(itemTemp.id));
                                        ok=false;
                                    }
                                }
                                #endif // CATCHCHALLENGER_CLASS_MASTER
                            }
                        }
                        if(ok)
                            profile.items << itemTemp;
                    }
                    itemElement = itemElement.nextSiblingElement("item");
                }
                idDuplicate << profile.id;
                returnVar.second << profile;
                returnVar.first << startItem;
            }
        }
        else
            std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
        startItem = startItem.nextSiblingElement("start");
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
        int sub_index;
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
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
            return returnVar;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="layers")
    {
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    bool ok;
    QDomElement monstersCollisionItem = root.firstChildElement("monstersCollision");
    while(!monstersCollisionItem.isNull())
    {
        if(monstersCollisionItem.isElement())
        {
            if(!monstersCollisionItem.hasAttribute("type"))
                std::cerr << "Have not the attribute type, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
            else if(!monstersCollisionItem.hasAttribute("monsterType"))
                std::cerr << "Have not the attribute monsterType, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
            else
            {
                ok=true;
                MonstersCollision monstersCollision;
                if(monstersCollisionItem.attribute("type")=="walkOn")
                    monstersCollision.type=MonstersCollisionType_WalkOn;
                else if(monstersCollisionItem.attribute("type")=="actionOn")
                    monstersCollision.type=MonstersCollisionType_ActionOn;
                else
                {
                    ok=false;
                    std::cerr << "type is not walkOn or actionOn, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute("layer"))
                        monstersCollision.layer=monstersCollisionItem.attribute("layer");
                }
                if(ok)
                {
                    if(monstersCollision.layer.isEmpty() && monstersCollision.type!=MonstersCollisionType_WalkOn)//need specific layer name to do that's
                    {
                        ok=false;
                        std::cerr << "To have blocking layer by item, have specific layer name, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                    }
                    else
                    {
                        monstersCollision.item=0;
                        if(monstersCollisionItem.hasAttribute("item"))
                        {
                            monstersCollision.item=monstersCollisionItem.attribute("item").toUInt(&ok);
                            if(!ok)
                                std::cerr << "item attribute is not a number, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                            else if(!items.contains(monstersCollision.item))
                            {
                                ok=false;
                                std::cerr << "item is not into item list, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                            }
                        }
                    }
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute("tile"))
                        monstersCollision.tile=monstersCollisionItem.attribute("tile");
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute("background"))
                        monstersCollision.background=monstersCollisionItem.attribute("background");
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute("monsterType"))
                    {
                        monstersCollision.defautMonsterTypeList=monstersCollisionItem.attribute("monsterType").split(";",std::string::SkipEmptyParts);
                        monstersCollision.defautMonsterTypeList.removeDuplicates();
                        monstersCollision.monsterTypeList=monstersCollision.defautMonsterTypeList;
                        //load the condition
                        QDomElement eventItem = monstersCollisionItem.firstChildElement("event");
                        while(!eventItem.isNull())
                        {
                            if(eventItem.isElement())
                            {
                                if(eventItem.hasAttribute("id") && eventItem.hasAttribute("value") && eventItem.hasAttribute("monsterType"))
                                {
                                    if(eventStringToId.contains(eventItem.attribute("id")))
                                    {
                                        if(eventValueStringToId.at(eventItem.attribute("id")).contains(eventItem.attribute("value")))
                                        {
                                            MonstersCollision::MonstersCollisionEvent event;
                                            event.event=eventStringToId.at(eventItem.attribute("id"));
                                            event.event_value=eventValueStringToId.at(eventItem.attribute("id")).at(eventItem.attribute("value"));
                                            event.monsterTypeList=eventItem.attribute("monsterType").split(";",std::string::SkipEmptyParts);
                                            if(!event.monsterTypeList.isEmpty())
                                            {
                                                monstersCollision.events << event;
                                                unsigned int index=0;
                                                while(index<event.monsterTypeList.size())
                                                {
                                                    if(!monstersCollision.monsterTypeList.contains(event.monsterTypeList.at(index)))
                                                        monstersCollision.monsterTypeList << event.monsterTypeList.at(index);
                                                    index++;
                                                }
                                            }
                                            else
                                                std::cerr << "monsterType is empty, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
                                        }
                                        else
                                            std::cerr << "event value not found, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
                                    }
                                    else
                                        std::cerr << "event not found, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
                                }
                                else
                                    std::cerr << "event have missing attribute, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
                            }
                            eventItem = eventItem.nextSiblingElement("event");
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
                            std::cerr << "similar monstersCollision previously found, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                            ok=false;
                            break;
                        }
                        if(monstersCollision.type==MonstersCollisionType_WalkOn && returnVar.at(index).layer==monstersCollision.layer)
                        {
                            std::cerr << "You can't have different item for same layer in walkOn mode, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                            ok=false;
                            break;
                        }
                        index++;
                    }
                }
                if(ok && !monstersCollision.monsterTypeList.isEmpty())
                {
                    if(monstersCollision.type==MonstersCollisionType_WalkOn && monstersCollision.layer.isEmpty() && monstersCollision.item==0)
                    {
                        if(returnVar.isEmpty())
                            returnVar << monstersCollision;
                        else
                        {
                            returnVar << returnVar.last();
                            returnVar.first()=monstersCollision;
                        }
                    }
                    else
                        returnVar << monstersCollision;
                }
            }
        }
        monstersCollisionItem = monstersCollisionItem.nextSiblingElement("monstersCollision");
    }
    return returnVar;
}

LayersOptions DatapackGeneralLoader::loadLayersOptions(const std::string &file)
{
    LayersOptions returnVar;
    returnVar.zoom=2;
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
            return returnVar;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="layers")
    {
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found for reputation of the xml file" << std::endl;
        std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found for the xml file").arg(file));
        return returnVar;
    }
    if(root.hasAttribute(QLatin1Literal("zoom")))
    {
        bool ok;
        returnVar.zoom=root.attribute(QLatin1Literal("zoom")).toUShort(&ok);
        if(!ok)
        {
            returnVar.zoom=2;
            std::cerr << "Unable to open the xml file: " << file << ", zoom is not a number").arg(file));
        }
        else
        {
            if(returnVar.zoom<1 || returnVar.zoom>4)
            {
                returnVar.zoom=2;
                std::cerr << "Unable to open the xml file: " << file << ", zoom out of range 1-4").arg(file));
            }
        }
    }

    return returnVar;
}

std::vector<Event> DatapackGeneralLoader::loadEvents(const std::string &file)
{
    std::vector<Event> returnVar;

    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
            return returnVar;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="events")
    {
        std::cerr << "Unable to open the file: " << file << ", \"events\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    QDomElement eventItem = root.firstChildElement("event");
    while(!eventItem.isNull())
    {
        if(eventItem.isElement())
        {
            if(!eventItem.hasAttribute("id"))
                std::cerr << "Have not the attribute id, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
            else if(eventItem.attribute("id").isEmpty())
                std::cerr << "Have id empty, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
            else
            {
                Event event;
                event.name=eventItem.attribute("id");
                QDomElement valueItem = eventItem.firstChildElement("value");
                while(!valueItem.isNull())
                {
                    if(valueItem.isElement())
                        event.values << valueItem.text();
                    valueItem = valueItem.nextSiblingElement("value");
                }
                if(!event.values.isEmpty())
                    returnVar << event;
            }
        }
        eventItem = eventItem.nextSiblingElement("event");
    }
    return returnVar;
}

std::unordered_map<uint32_t,Shop> DatapackGeneralLoader::preload_shop(const std::string &file, const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<uint32_t,Shop> shops;

    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile shopFile(file);
        QByteArray xmlContent;
        if(!shopFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << shopFile.errorString().toStdString() << std::endl;
            return shops;
        }
        xmlContent=shopFile.readAll();
        shopFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return shops;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="shops")
    {
        std::cerr << "Unable to open the file: " << file << ", \"shops\" root balise not found for reputation of the xml file" << std::endl;
        return shops;
    }

    //load the content
    bool ok;
    QDomElement shopItem = root.firstChildElement("shop");
    while(!shopItem.isNull())
    {
        if(shopItem.isElement())
        {
            if(shopItem.hasAttribute("id"))
            {
                uint32_t id=shopItem.attribute("id").toUInt(&ok);
                if(ok)
                {
                    if(!shops.contains(id))
                    {
                        Shop shop;
                        QDomElement product = shopItem.firstChildElement("product");
                        while(!product.isNull())
                        {
                            if(product.isElement())
                            {
                                if(product.hasAttribute("itemId"))
                                {
                                    uint32_t itemId=product.attribute("itemId").toUInt(&ok);
                                    if(!ok)
                                        std::cerr << "preload_shop() product attribute itemId is not a number for shops file: " << file << ", child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                    else
                                    {
                                        if(!items.contains(itemId))
                                            std::cerr << "preload_shop() product itemId in not into items list for shops file: " << file << ", child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                        else
                                        {
                                            uint32_t price=items.at(itemId).price;
                                            if(product.hasAttribute("overridePrice"))
                                            {
                                                price=product.attribute("overridePrice").toUInt(&ok);
                                                if(!ok)
                                                    price=items.at(itemId).price;
                                            }
                                            if(price==0)
                                                std::cerr << "preload_shop() item can't be into the shop with price == 0' for shops file: " << file << ", child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                            else
                                            {
                                                shop.prices << price;
                                                shop.items << itemId;
                                            }
                                        }
                                    }
                                }
                                else
                                    std::cerr << "preload_shop() material have not attribute itemId for shops file: " << file << ", child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                            }
                            else
                                std::cerr << "preload_shop() material is not an element for shops file: " << file << ", child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                            product = product.nextSiblingElement("product");
                        }
                        shops[id]=shop;
                    }
                    else
                        std::cerr << "Unable to open the shops file: " << file << ", child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                }
                else
                    std::cerr << "Unable to open the shops file: " << file << ", id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
            }
            else
                std::cerr << "Unable to open the shops file: " << file << ", have not the shops id: child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
        }
        else
            std::cerr << "Unable to open the shops file: " << file << ", is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
        shopItem = shopItem.nextSiblingElement("shop");
    }
    return shops;
}
#endif

std::vector<ServerProfile> DatapackGeneralLoader::loadServerProfileList(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file,const std::vector<Profile> &profileCommon)
{
    std::vector<ServerProfile> serverProfile=loadServerProfileListInternal(datapackPath,mainDatapackCode,file);
    //index of base profile
    Std::unordered_Set<std::string> profileId,serverProfileId;
    {
        unsigned int index=0;
        while(index<profileCommon.size())
        {
            //already deduplicated at loading
            profileId << profileCommon.at(index).id;
            index++;
        }
    }
    //drop serverProfileList
    {
        unsigned int index=0;
        while(index<serverProfile.size())
        {
            if(profileId.contains(serverProfile.at(index).id))
            {
                serverProfileId << serverProfile.at(index).id;
                index++;
            }
            else
            {
                std::cerr << "Profile xml file: " << file << ", found id \"%2\" but not found in common, drop it").arg(file).arg(serverProfile.at(index).id));
                serverProfile.removeAt(index);
            }
        }
    }
    //add serverProfileList
    {
        unsigned int index=0;
        while(index<profileCommon.size())
        {
            if(!serverProfileId.contains(profileCommon.at(index).id))
            {
                std::cerr << "Profile xml file: " << file << ", found common id \"%2\" but not found in server, add it").arg(file).arg(profileCommon.at(index).id));
                ServerProfile serverProfileTemp;
                serverProfileTemp.id=profileCommon.at(index).id;
                serverProfileTemp.orientation=Orientation_bottom;
                serverProfileTemp.x=0;
                serverProfileTemp.y=0;
                serverProfile << serverProfileTemp;
            }
            index++;
        }
    }

    return serverProfile;
}

std::vector<ServerProfile> DatapackGeneralLoader::loadServerProfileListInternal(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file)
{
    Std::unordered_Set<std::string> idDuplicate;
    std::vector<ServerProfile> serverProfileList;

    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
            return serverProfileList;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return serverProfileList;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="profile")
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found for reputation of the xml file" << std::endl;
        return serverProfileList;
    }

    //load the content
    bool ok;
    QDomElement startItem = root.firstChildElement("start");
    while(!startItem.isNull())
    {
        if(startItem.isElement())
        {
            ServerProfile serverProfile;
            serverProfile.orientation=Orientation_bottom;

            const QDomElement &map = startItem.firstChildElement("map");
            if(!map.isNull() && map.isElement() && map.hasAttribute("file") && map.hasAttribute("x") && map.hasAttribute("y"))
            {
                serverProfile.mapString=map.attribute("file");
                if(!serverProfile.mapString.endsWith(".tmx"))
                    serverProfile.mapString+=".tmx";
                if(!QFile::exists(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_MAPMAIN).arg(mainDatapackCode)+serverProfile.mapString))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", map don't exists %2: child.tagName(): %3 (at line: %4)").arg(file).arg(serverProfile.mapString).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement("start");
                    continue;
                }
                serverProfile.x=map.attribute("x").toUShort(&ok);
                if(!ok)
                {
                    std::cerr << "Unable to open the xml file: " << file << ", map x is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement("start");
                    continue;
                }
                serverProfile.y=map.attribute("y").toUShort(&ok);
                if(!ok)
                {
                    std::cerr << "Unable to open the xml file: " << file << ", map y is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement("start");
                    continue;
                }
            }
            else
            {
                std::cerr << "Unable to open the xml file: " << file << ", no correct map configuration: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement("start");
                continue;
            }

            if(startItem.hasAttribute("id"))
                serverProfile.id=startItem.attribute("id");

            if(idDuplicate.contains(serverProfile.id))
            {
                std::cerr << "Unable to open the xml file: " << file << ", id duplicate: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement("start");
                continue;
            }

            if(!serverProfile.id.isEmpty() && !idDuplicate.contains(serverProfile.id))
            {
                idDuplicate << serverProfile.id;
                serverProfileList << serverProfile;
            }
        }
        else
            std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
        startItem = startItem.nextSiblingElement("start");
    }

    return serverProfileList;
}
