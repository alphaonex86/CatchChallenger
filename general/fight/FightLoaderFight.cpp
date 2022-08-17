#include "FightLoader.hpp"
#ifndef EPOLLCATCHCHALLENGERSERVER
#include "../../general/base/CommonDatapack.hpp"
#endif
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/cpp11addition.hpp"
#include <iostream>

using namespace CatchChallenger;

#ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
std::unordered_map<uint16_t,BotFight> FightLoader::loadFight(const std::string &folder, const std::unordered_map<uint16_t,Monster> &monsters, const std::unordered_map<uint16_t, Skill> &monsterSkills, const std::unordered_map<uint16_t, Item> &items,uint16_t &botFightsMaxId)
{
    std::unordered_map<uint16_t,BotFight> botFightList;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &fileList=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    unsigned int index_file=0;
    while(index_file<fileList.size())
    {
        const std::string &file=fileList.at(index_file).absoluteFilePath;
        tinyxml2::XMLDocument *domDocument;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        //open and quick check the file
        if(CommonDatapack::commonDatapack.get_xmlLoadedFile().find(file)!=CommonDatapack::commonDatapack.get_xmlLoadedFile().cend())
            domDocument=&CommonDatapack::commonDatapack.get_xmlLoadedFile_rw()[file];
        else
        {
            domDocument=&CommonDatapack::commonDatapack.get_xmlLoadedFile_rw()[file];
            #else
            domDocument=new tinyxml2::XMLDocument();
            #endif
            const auto loadOkay = domDocument->LoadFile(file.c_str());
            if(loadOkay!=0)
            {
                std::cerr << file+", "+domDocument->ErrorName() << std::endl;
                index_file++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
        }
        #endif
        const tinyxml2::XMLElement * root = domDocument->RootElement();
        if(root==NULL)
        {
            index_file++;
            continue;
        }
        if(root->Name()==NULL)
        {
            std::cerr << "Unable to open the xml file: " << file << ", \"fights\" root balise not found 2 for the xml file" << std::endl;
            index_file++;
            continue;
        }
        if(strcmp(root->Name(),"fights")!=0)
        {
            std::cerr << "Unable to open the xml file: " << file << ", \"fights\" root balise not found for the xml file" << std::endl;
            index_file++;
            continue;
        }

        //load the content
        bool ok;
        const tinyxml2::XMLElement * item = root->FirstChildElement("fight");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                const uint16_t &id=stringtouint16(item->Attribute("id"),&ok);
                if(ok)
                {
                    bool entryValid=true;
                    CatchChallenger::BotFight botFight;
                    botFight.cash=0;
                    {
                        const tinyxml2::XMLElement * monster = item->FirstChildElement("monster");
                        while(entryValid && monster!=NULL)
                        {
                            if(monster->Attribute("id")==NULL)
                                std::cerr << "Has not attribute \"id\": Name(): " << monster->Name() << std::endl;
                            else
                            {
                                CatchChallenger::BotFight::BotFightMonster botFightMonster;
                                botFightMonster.level=1;
                                botFightMonster.id=stringtouint16(monster->Attribute("id"),&ok);
                                if(ok)
                                {
                                    if(monsters.find(botFightMonster.id)==monsters.cend())
                                    {
                                        entryValid=false;
                                        std::cerr << "Monster not found into the monster list: " << botFightMonster.id << " into the Name(): " << monster->Name() << "), file: " << file << std::endl;
                                        break;
                                    }
                                    if(monster->Attribute("level")!=NULL)
                                    {
                                        botFightMonster.level=stringtouint8(monster->Attribute("level"),&ok);
                                        if(!ok)
                                        {
                                            std::cerr << "The level is not a number: type: " << monster->Attribute("type") << " Name(): " << monster->Name() << "), file: " << file << std::endl;
                                            botFightMonster.level=1;
                                        }
                                        if(botFightMonster.level<1)
                                        {
                                            std::cerr << "Can't be 0 or negative: type: " << monster->Attribute("type") << " Name(): " << monster->Name() << "), file: " << file << std::endl;
                                            botFightMonster.level=1;
                                        }
                                    }
                                    const tinyxml2::XMLElement * attack = monster->FirstChildElement("attack");
                                    while(entryValid && attack!=NULL)
                                    {
                                        uint8_t attackLevel=1;
                                        if(attack->Attribute("id")==NULL)
                                            std::cerr << "Has not attribute \"type\": Name(): " << attack->Name() << ", file: " << file << std::endl;
                                        else
                                        {
                                            const uint16_t attackId=stringtouint16(attack->Attribute("id"),&ok);
                                            if(ok)
                                            {
                                                if(monsterSkills.find(attackId)==monsterSkills.cend())
                                                {
                                                    entryValid=false;
                                                    std::cerr << "Monster attack not found: %1 into the Name(): " << attack->Name() << ", file: " << file << std::endl;
                                                    break;
                                                }
                                                if(attack->Attribute("level")!=NULL)
                                                {
                                                    attackLevel=stringtouint8(attack->Attribute("level"),&ok);
                                                    if(!ok)
                                                    {
                                                        std::cerr << "The level is not a number: Name(): " << attack->Name() << ", file: " << file << std::endl;
                                                        entryValid=false;
                                                        break;
                                                    }
                                                    if(attackLevel<1)
                                                    {
                                                        std::cerr << "Can't be 0 or negative: Name(): " << attack->Name() << ", file: " << file << std::endl;
                                                        entryValid=false;
                                                        break;
                                                    }
                                                }
                                                if(attackLevel>monsterSkills.at(attackId).level.size())
                                                {
                                                    std::cerr << "Level out of range: Name(): " << attack->Name() << ", file: " << file << std::endl;
                                                    entryValid=false;
                                                    break;
                                                }
                                                CatchChallenger::PlayerMonster::PlayerSkill botFightAttack;
                                                botFightAttack.skill=attackId;
                                                botFightAttack.level=attackLevel;
                                                botFightMonster.attacks.push_back(botFightAttack);
                                            }
                                        }
                                        attack = attack->NextSiblingElement("attack");
                                    }
                                    if(botFightMonster.attacks.empty())
                                        botFightMonster.attacks=loadDefaultAttack(botFightMonster.id,botFightMonster.level,monsters,monsterSkills);
                                    if(botFightMonster.attacks.empty())
                                    {
                                        std::cerr << "Empty attack list: Name(): " << monster->Name() << "), file: " << file << std::endl;
                                        entryValid=false;
                                        break;
                                    }
                                    botFight.monsters.push_back(botFightMonster);
                                }
                            }
                            monster = monster->NextSiblingElement("monster");
                        }
                    }
                    {
                        const tinyxml2::XMLElement * gain = item->FirstChildElement("gain");
                        while(entryValid && gain!=NULL)
                        {
                            if(gain->Attribute("cash")!=NULL)
                            {
                                const uint32_t &cash=stringtouint32(gain->Attribute("cash"),&ok)*CommonSettingsServer::commonSettingsServer.rates_gold/1000;
                                if(ok)
                                    botFight.cash+=cash;
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", unknow cash text: child->Name(): " << item->Name() << std::endl;
                            }
                            else if(gain->Attribute("item")!=NULL)
                            {
                                BotFight::Item itemVar;
                                itemVar.quantity=1;
                                itemVar.id=stringtouint16(gain->Attribute("item"),&ok);
                                if(ok)
                                {
                                    if(items.find(itemVar.id)!=items.cend())
                                    {
                                        if(gain->Attribute("quantity")!=NULL)
                                        {
                                            itemVar.quantity=stringtouint32(gain->Attribute("quantity"),&ok);
                                            if(!ok || itemVar.quantity<1)
                                            {
                                                itemVar.quantity=1;
                                                std::cerr << "Unable to open the xml file: " << file << ", quantity value is wrong: child->Name(): " << item->Name() << std::endl;
                                            }
                                        }
                                        botFight.items.push_back(itemVar);
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", item not found: child->Name(): " << item->Name() << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", unknow item id text: child->Name(): " << item->Name() << std::endl;
                            }
                            else
                                std::cerr << "unknown fight gain: file: " << file << " child->Name(): " << gain->Name() << std::endl;
                            gain = gain->NextSiblingElement("gain");
                        }
                    }
                    if(entryValid)
                    {
                        if(botFightList.find(id)==botFightList.cend())
                        {
                            if(!botFight.monsters.empty())
                            {
                                if(botFightsMaxId<id)
                                    botFightsMaxId=id;
                                botFightList[id]=botFight;
                            }
                            else
                                std::cerr << "Monster list is empty to open the xml file: " << file << ", id already found: child->Name(): " << item->Name() << std::endl;
                        }
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", id already found: child->Name(): " << item->Name() << std::endl;
                    }
                }
                else
                    std::cerr << "Unable to open the xml file: " << file << ", id is not a number: child->Name(): " << item->Name() << std::endl;
            }
            item = item->NextSiblingElement("fight");
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        index_file++;
    }
    return botFightList;
}
#endif
