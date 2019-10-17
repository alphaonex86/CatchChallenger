#include "FightLoader.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonSettingsServer.h"
#ifndef EPOLLCATCHCHALLENGERSERVER
#include "../../general/base/CommonDatapack.h"
#endif
#include <iostream>
#include <cmath>

using namespace CatchChallenger;

std::unordered_map<uint16_t,Monster> FightLoader::loadMonster(const std::string &folder, const std::unordered_map<uint16_t, Skill> &monsterSkills
                                                #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                , const std::vector<Type> &types, const std::unordered_map<uint16_t, Item> &items,
                                                uint16_t &monstersMaxId
                                                #endif
                                                )
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(types.size()>255)
    {
        std::cerr << "FightLoader::loadMonster() types.size()>255" << std::endl;
        abort();
    }
    #endif
    #endif
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    if(CommonSettingsServer::commonSettingsServer.rates_xp_pow==0)
    {
        std::cerr << "CommonSettingsServer::commonSettingsServer.rates_xp_pow==0 abort" << std::endl;
        abort();
    }
    #endif
    std::unordered_map<uint16_t,Monster> monsters;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &fileList=
            CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    unsigned int file_index=0;
    while(file_index<(uint32_t)fileList.size())
    {
        const std::string &file=fileList.at(file_index).absoluteFilePath;
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        std::unordered_map<std::string,uint8_t> typeNameToId;
        {
            uint8_t index=0;
            while(index<types.size())
            {
                typeNameToId[types.at(index).name]=index;
                index++;
            }
        }
        #endif
        tinyxml2::XMLDocument *domDocument;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        //open and quick check the file
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
                std::cerr << file+", "+domDocument->ErrorName() << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
        }
        #endif
        const tinyxml2::XMLElement * root = domDocument->RootElement();
        if(root==NULL)
        {
            file_index++;
            continue;
        }
        if(root->Name()==NULL)
        {
            file_index++;
            continue;
        }
        if(strcmp(root->Name(),"monsters")!=0)
        {
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2;
        tinyxml2::XMLElement * item = const_cast<tinyxml2::XMLElement *>(root->FirstChildElement("monster"));
        while(item!=NULL)
        {
            bool attributeIsOk=true;
            if(item->Attribute("id")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"id\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            #ifndef CATCHCHALLENGER_CLASS_MASTER
            if(item->Attribute("egg_step")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"egg_step\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            if(item->Attribute("xp_for_max_level")==NULL && item->Attribute("xp_max")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"xp_for_max_level\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            else
            {
                if(item->Attribute("xp_for_max_level")==NULL)
                    item->SetAttribute("xp_for_max_level",item->Attribute("xp_max"));
            }
            if(item->Attribute("hp")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"hp\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            if(item->Attribute("attack")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"attack\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            if(item->Attribute("defense")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"defense\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            if(item->Attribute("special_attack")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"special_attack\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            if(item->Attribute("special_defense")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"special_defense\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            if(item->Attribute("speed")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"speed\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            if(item->Attribute("give_sp")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"give_sp\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            if(item->Attribute("give_xp")==NULL)
            {
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"give_xp\": child->Name(): " << item->Name() << std::endl;
                attributeIsOk=false;
            }
            #endif // CATCHCHALLENGER_CLASS_MASTER
            if(attributeIsOk)
            {
                Monster monster;
                #ifndef CATCHCHALLENGER_CLASS_MASTER
                monster.catch_rate=100;
                #endif
                uint16_t id=stringtouint16(item->Attribute("id"),&ok);
                if(!ok)
                    std::cerr << "Unable to open the xml file: " << file << ", id not a number: child->Name(): " << item->Name() << std::endl;
                else if(monsters.find(id)!=monsters.cend())
                    std::cerr << "Unable to open the xml file: " << file << ", id already found: child->Name(): " << item->Name() << std::endl;
                else
                {
                    #ifndef CATCHCHALLENGER_CLASS_MASTER
                    if(item->Attribute("catch_rate")!=NULL)
                    {
                        bool ok2;
                        uint8_t catch_rate=stringtouint8(item->Attribute("catch_rate"),&ok2);
                        if(ok2)
                        {
                            //if(catch_rate<=255)
                                monster.catch_rate=catch_rate;
                            /*else
                                std::cerr << "Unable to open the xml file: " << file << ", catch_rate is not a number: " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(item->Attribute(XMLCACHEDSTRING_catch_rate)) << " child->Name(): " << item->Name() << std::endl;*/
                        }
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", catch_rate is not a number: " << item->Attribute("catch_rate") << " child->Name(): " << item->Name() << std::endl;
                    }
                    if(item->Attribute("type")!=NULL)
                    {
                        const std::vector<std::string> &typeList=stringsplit(item->Attribute("type"),';');
                        unsigned int index=0;
                        while(index<typeList.size())
                        {
                            if(typeNameToId.find(typeList.at(index))!=typeNameToId.cend())
                                monster.type.push_back(typeNameToId.at(typeList.at(index)));
                            else
                                std::cerr << "Unable to open the xml file: " << file << ", type not found into the list: " << item->Attribute("type") << " child->Name(): " << item->Name() << std::endl;
                            index++;
                        }
                    }
                    if(item->Attribute("type2")!=NULL)
                    {
                        const std::vector<std::string> &typeList=stringsplit(item->Attribute("type2"),';');
                        unsigned int index=0;
                        while(index<typeList.size())
                        {
                            if(typeNameToId.find(typeList.at(index))!=typeNameToId.cend())
                                monster.type.push_back(typeNameToId.at(typeList.at(index)));
                            else
                                std::cerr << "Unable to open the xml file: " << file << ", type not found into the list: " << item->Attribute("type2") << " child->Name(): " << item->Name() << std::endl;
                            index++;
                        }
                    }
                    #ifdef CATCHCHALLENGER_CLIENT
                    double &powerVar=monster.powerVar;
                    #else
                    double powerVar=1.0;
                    #endif
                    powerVar=1.0;
                    if(ok)
                    {
                        if(item->Attribute("pow")!=NULL)
                        {
                            powerVar=stringtodouble(item->Attribute("pow"),&ok);
                            if(!ok)
                            {
                                powerVar=1.0;
                                std::cerr << "Unable to open the xml file: " << file << ", pow is not a double: child->Name(): " << item->Name() << std::endl;
                                ok=true;
                            }
                            if(powerVar<=1.0)
                            {
                                powerVar=1.0;
                                std::cerr << "Unable to open the xml file: " << file << ", pow is too low: child->Name(): " << item->Name() << std::endl;
                            }
                            if(powerVar>=10.0)
                            {
                                powerVar=1.0;
                                std::cerr << "Unable to open the xml file: " << file << ", pow is too hight: child->Name(): " << item->Name() << std::endl;
                            }
                        }
                    }
                    #ifndef CATCHCHALLENGER_CLASS_MASTER
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                    powerVar*=static_cast<double>(CommonSettingsServer::commonSettingsServer.rates_xp_pow)/1000.0;
                    #endif
                    #endif
                    if(ok)
                    {
                        monster.egg_step=stringtouint32(item->Attribute("egg_step"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", egg_step is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    if(ok)
                    {
                        monster.xp_for_max_level=stringtouint32(item->Attribute("xp_for_max_level"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", xp_for_max_level is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    #endif
                    if(ok)
                    {
                        monster.stat.hp=stringtouint32(item->Attribute("hp"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", hp is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    #ifndef CATCHCHALLENGER_CLASS_MASTER
                    if(ok)
                    {
                        monster.stat.attack=stringtouint32(item->Attribute("attack"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", attack is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    if(ok)
                    {
                        monster.stat.defense=stringtouint32(item->Attribute("defense"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", defense is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    if(ok)
                    {
                        monster.stat.special_attack=stringtouint32(item->Attribute("special_attack"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", special_attack is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    if(ok)
                    {
                        monster.stat.special_defense=stringtouint32(item->Attribute("special_defense"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", special_defense is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    if(ok)
                    {
                        monster.stat.speed=stringtouint32(item->Attribute("speed"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", speed is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                    if(CommonSettingsServer::commonSettingsServer.rates_xp<=0)
                    {
                        std::cerr << "CommonSettingsServer::commonSettingsServer.rates_xp can't be null" << std::endl;
                        abort();
                    }
                    if(ok)
                    {
                        monster.give_xp=stringtouint32(item->Attribute("give_xp"),&ok)*CommonSettingsServer::commonSettingsServer.rates_xp/1000;
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", give_xp is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    if(ok)
                    {
                        monster.give_sp=stringtouint32(item->Attribute("give_sp"),&ok)*CommonSettingsServer::commonSettingsServer.rates_xp/1000;
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", give_sp is not number: child->Name(): " << item->Name() << std::endl;
                    }
                    #else
                    monster.give_xp=0;
                    monster.give_sp=0;
                    #endif
                    #endif
                    if(ok)
                    {
                        if(item->Attribute("ratio_gender")!=NULL)
                        {
                            std::string ratio_gender=std::string(item->Attribute("ratio_gender"));
                            stringreplaceOne(ratio_gender,"percent","");
                            monster.ratio_gender=stringtoint8(ratio_gender,&ok2);
                            if(!ok2)
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", ratio_gender is not number: child->Name(): " << item->Name() << std::endl;
                                monster.ratio_gender=50;
                            }
                            if(monster.ratio_gender<-1 || monster.ratio_gender>100)
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", ratio_gender is not in range of -1, 100: child->Name(): " << item->Name() << std::endl;
                                monster.ratio_gender=50;
                            }
                        }
                        monster.ratio_gender=50;
                    }
                    if(ok)
                    {
                        {
                            const tinyxml2::XMLElement * attack_list = item->FirstChildElement("attack_list");
                            if(attack_list!=NULL)
                            {
                                tinyxml2::XMLElement * attack = const_cast<tinyxml2::XMLElement *>(attack_list->FirstChildElement("attack"));
                                while(attack!=NULL)
                                {
                                    if(attack->Attribute("skill")!=NULL || attack->Attribute("id")!=NULL)
                                    {
                                        ok=true;
                                        if(attack->Attribute("skill")==NULL)
                                            attack->SetAttribute("skill",attack->Attribute("id"));
                                        Monster::AttackToLearn attackVar;
                                        if(attack->Attribute("skill_level")!=NULL || attack->Attribute("attack_level")!=NULL)
                                        {
                                            if(attack->Attribute("skill_level")==NULL)
                                                attack->SetAttribute("skill_level",attack->Attribute("attack_level"));
                                            attackVar.learnSkillLevel=stringtouint8(attack->Attribute("skill_level"),&ok);
                                            if(!ok)
                                                std::cerr << "Unable to open the xml file: " << file << ", skill_level is not a number: child->Name(): " << attack->Name() << std::endl;
                                        }
                                        else
                                            attackVar.learnSkillLevel=1;
                                        if(ok)
                                        {
                                            attackVar.learnSkill=stringtouint16(attack->Attribute("skill"),&ok);
                                            if(!ok)
                                                std::cerr << "Unable to open the xml file: " << file << ", skill is not a number: child->Name(): " << attack->Name() << std::endl;
                                        }
                                        if(ok)
                                        {
                                            if(monsterSkills.find(attackVar.learnSkill)==monsterSkills.cend())
                                            {
                                                std::cerr << "Unable to open the xml file: " << file << ", attack is not into attack loaded: child->Name(): " << attack->Name() << std::endl;
                                                ok=false;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(attackVar.learnSkillLevel<=0 || attackVar.learnSkillLevel>(uint32_t)monsterSkills.at(attackVar.learnSkill).level.size())
                                            {
                                                std::cerr << "Unable to open the xml file: " << file << ", attack level is not in range 1-" << monsterSkills.at(attackVar.learnSkill).level.size() << ": child->Name(): " << attack->Name() << std::endl;
                                                ok=false;
                                            }
                                        }
                                        if(attack->Attribute("level")!=NULL)
                                        {
                                            if(ok)
                                            {
                                                attackVar.learnAtLevel=stringtouint8(attack->Attribute("level") ,&ok);
                                                if(ok)
                                                {
                                                    unsigned int index=0;
                                                    //if it's the first lean don't need previous learn
                                                    if(attackVar.learnSkillLevel>1)
                                                    {
                                                        index=0;
                                                        while(index<monster.learn.size())
                                                        {
                                                            if(monster.learn.at(index).learnSkillLevel==(attackVar.learnSkillLevel-1) && monster.learn.at(index).learnSkill==attackVar.learnSkill)
                                                                break;
                                                            index++;
                                                        }
                                                        if(index==monster.learn.size())
                                                        {
                                                            std::cerr << "Unable to open the xml file: " << file << ", attack " << attackVar.learnSkill << " with level " << attackVar.learnSkillLevel << " can't be added because not same attack with previous level: child->Name(): " << attack->Name() << std::endl;
                                                            ok=false;
                                                        }
                                                    }

                                                    //check if can learn
                                                    if(ok)
                                                    {
                                                        index=0;
                                                        while(index<monster.learn.size())
                                                        {
                                                            if(attackVar.learnSkillLevel>1 && (monster.learn.at(index).learnSkillLevel-1)==attackVar.learnSkillLevel)
                                                                ok=true;
                                                            if(monster.learn.at(index).learnSkillLevel==attackVar.learnSkillLevel && monster.learn.at(index).learnSkill==attackVar.learnSkill)
                                                            {
                                                                std::cerr << "Unable to open the xml file: " << file << ", attack already do for this level for skill " << attackVar.learnSkill << " at level " << attackVar.learnSkillLevel << " for monster " << id << ": child->Name(): " << attack->Name() << std::endl;
                                                                ok=false;
                                                                break;
                                                            }
                                                            if(monster.learn.at(index).learnSkill==attackVar.learnSkill && monster.learn.at(index).learnSkillLevel==attackVar.learnSkillLevel)
                                                            {
                                                                std::cerr << "Unable to open the xml file: " << file << ", this attack level is already found " << attackVar.learnSkill << ", level: " << attackVar.learnSkillLevel << " for attack: " << index << ": child->Name(): " << attack->Name() << std::endl;
                                                                ok=false;
                                                                break;
                                                            }
                                                            index++;
                                                        }
                                                        if(ok)
                                                            monster.learn.push_back(attackVar);
                                                    }
                                                    else
                                                        std::cerr << "Unable to open the xml file: " << file << ", no way to learn " << attackVar.learnSkill << ", level: " << attackVar.learnSkillLevel << " for attack: ?: child->Name(): " << attack->Name() << std::endl;
                                                }
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", level is not a number: child->Name(): " << attack->Name() << std::endl;
                                            }
                                        }
                                        else
                                        {
                                            #ifndef CATCHCHALLENGER_CLASS_MASTER
                                            if(attack->Attribute("byitem")!=NULL)
                                            {
                                                uint16_t itemId=0;
                                                if(ok)
                                                {
                                                    itemId=stringtouint16(attack->Attribute("byitem"),&ok);
                                                    if(!ok)
                                                        std::cerr << "Unable to open the xml file: " << file << ", item to learn is not a number " << attack->Attribute("byitem") << ": child->Name(): " << attack->Name() << std::endl;
                                                }
                                                if(ok)
                                                {
                                                    if(items.find(itemId)==items.cend())
                                                    {
                                                        std::cerr << "Unable to open the xml file: " << file << ", item to learn not found " << itemId << ": child->Name(): " << attack->Name() << std::endl;
                                                        ok=false;
                                                    }
                                                }
                                                if(ok)
                                                {
                                                    if(monster.learnByItem.find(itemId)!=monster.learnByItem.cend())
                                                    {
                                                        std::cerr << "Unable to open the xml file: " << file << ", item to learn is already used " << itemId << ": child->Name(): " << attack->Name() << std::endl;
                                                        ok=false;
                                                    }
                                                }
                                                if(ok)
                                                {
                                                    Monster::AttackToLearnByItem tempEntry;
                                                    tempEntry.learnSkill=attackVar.learnSkill;
                                                    tempEntry.learnSkillLevel=attackVar.learnSkillLevel;
                                                    monster.learnByItem[itemId]=tempEntry;
                                                }
                                            }
                                            else
                                            {
                                                std::cerr << "Unable to open the xml file: " << file << ", level and byitem is not found: child->Name(): " << attack->Name() << std::endl;
                                                ok=false;
                                            }
                                            #endif // CATCHCHALLENGER_CLASS_MASTER
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", missing arguements (level or skill): child->Name(): " << attack->Name() << std::endl;
                                    attack = attack->NextSiblingElement("attack");
                                }
                                std::sort(monster.learn.begin(),monster.learn.end());
                            }
                            else
                                std::cerr << "Unable to open the xml file: " << file << ", have not attack_list: child->Name(): " << item->Name() << std::endl;
                        }
                        #ifndef CATCHCHALLENGER_CLASS_MASTER
                        {
                            const tinyxml2::XMLElement * evolutionsItem = item->FirstChildElement("evolutions");
                            if(evolutionsItem!=NULL)
                            {
                                const tinyxml2::XMLElement * evolutionItem = evolutionsItem->FirstChildElement("evolution");
                                while(evolutionItem!=NULL)
                                {
                                    if(evolutionItem->Attribute("type")!=NULL && (
                                           evolutionItem->Attribute("level")!=NULL ||
                                           (strcmp(evolutionItem->Attribute("type"),"trade")==0 && evolutionItem->Attribute("evolveTo")!=NULL) ||
                                           (strcmp(evolutionItem->Attribute("type"),"item")==0 && evolutionItem->Attribute("item")!=NULL)
                                           )
                                       )
                                    {
                                        ok=true;
                                        Monster::Evolution evolutionVar;
                                        const std::string &typeText=std::string(evolutionItem->Attribute("type"));
                                        if(typeText!="trade")
                                        {
                                            if(typeText=="item")
                                                evolutionVar.data.item=stringtouint16(evolutionItem->Attribute("item"),&ok);
                                            else//level
                                                evolutionVar.data.level=stringtouint8(evolutionItem->Attribute("level"),&ok);
                                            if(!ok)
                                                std::cerr << "Unable to open the xml file: " << file << ", level is not a number: child->Name(): " << evolutionItem->Name() << std::endl;
                                        }
                                        else
                                        {
                                            evolutionVar.data.level=0;
                                            evolutionVar.data.item=0;
                                        }
                                        if(ok)
                                        {
                                            evolutionVar.evolveTo=stringtouint16(evolutionItem->Attribute("evolveTo"),&ok);
                                            if(!ok)
                                                std::cerr << "Unable to open the xml file: " << file << ", evolveTo is not a number: child->Name(): " << evolutionItem->Name() << std::endl;
                                        }
                                        if(ok)
                                        {
                                            if(typeText=="level")
                                                evolutionVar.type=Monster::EvolutionType_Level;
                                            else if(typeText=="item")
                                                evolutionVar.type=Monster::EvolutionType_Item;
                                            else if(typeText=="trade")
                                                evolutionVar.type=Monster::EvolutionType_Trade;
                                            else
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", unknown evolution type: " << typeText << " child->Name(): " << evolutionItem->Name() << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(typeText=="level" && (evolutionVar.data.level<0 || evolutionVar.data.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX))
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", level out of range: " << evolutionVar.data.level << " child->Name(): " << evolutionItem->Name() << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(typeText=="item")
                                            {
                                                if(items.find(evolutionVar.data.item)==items.cend())
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", unknown evolution item: " << evolutionVar.data.item << " child->Name(): " << evolutionItem->Name() << std::endl;
                                                }
                                            }
                                        }
                                        if(ok)
                                            monster.evolutions.push_back(evolutionVar);
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", missing arguements (level or skill): child->Name(): " << evolutionItem->Name() << std::endl;
                                    evolutionItem = evolutionItem->NextSiblingElement("evolution");
                                }
                            }
                        }
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(powerVar==0)
                        {
                            std::cerr << "powerVar==0" << std::endl;
                            abort();
                        }
                        #endif
                        int index=0;
                        while(index<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                        {
                            uint64_t xp_for_this_level=std::pow(index+1,powerVar);
                            uint64_t xp_for_max_level=monster.xp_for_max_level;
                            uint64_t max_xp=std::pow(CATCHCHALLENGER_MONSTER_LEVEL_MAX,powerVar);
                            uint64_t tempXp=xp_for_this_level*xp_for_max_level/max_xp;
                            if(tempXp<1)
                                tempXp=1;
                            /*#ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(id==7 && monster.level_to_xp.size()==20)
                                std::cout << "Example level to xp: " << tempXp << std::endl;
                            #endif*/
                            monster.level_to_xp.push_back(static_cast<uint32_t>(tempXp));
                            index++;
                        }
                        #ifdef DEBUG_MESSAGE_MONSTER_XP_LOAD
                        index=0;
                        while(index<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                        {
                            int give_xp=(monster.give_xp*(index+1))/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                            std::cerr << "monster: " << id << ", xp " << index+1 << " for the level: " << monster.level_to_xp.at(index) << ", monster for this level: " << monster.level_to_xp.at(index)/give_xp << std::endl;
                            index++;
                        }
                        std::cerr << "monster.level_to_xp.size(): " << monster.level_to_xp.size() << std::endl;
                        #endif

                        #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(monster.give_xp!=0)
                            if((monster.xp_for_max_level*CommonSettingsServer::commonSettingsServer.rates_xp/1000/monster.give_xp)>150)
                                std::cerr << "Warning: you need more than " << monster.xp_for_max_level/monster.give_xp << " monster(s) to pass the last level, prefer do that's with the rate for the monster id: " << id << std::endl;
                        #endif
                        #endif

                        if(monstersMaxId<id)
                            monstersMaxId=id;
                        #endif // CATCHCHALLENGER_CLASS_MASTER
                        monsters[id]=monster;
                    }
                    else
                        std::cerr << "Unable to open the xml file: " << file << ", one of the attribute is wrong or is not a number: child->Name(): " << item->Name() << std::endl;
                }
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster id: child->Name(): " << item->Name() << std::endl;
            item = item->NextSiblingElement("monster");
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        file_index++;
    }

    #ifndef CATCHCHALLENGER_CLASS_MASTER
    //check the evolveTo
    auto i = monsters.begin();
    while(i!=monsters.cend())
    {
        unsigned int index=0;
        bool evolutionByLevel=false,evolutionByTrade=false;
        while(index<i->second.evolutions.size())
        {
            std::unordered_set<uint32_t> itemUse;
            if(i->second.evolutions.at(index).type==Monster::EvolutionType_Level)
            {
                if(evolutionByLevel)
                {
                    std::cerr << "The monster " << i->first << " have already evolution by level" << std::endl;
                    i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                    continue;
                }
                evolutionByLevel=true;
            }
            if(i->second.evolutions.at(index).type==Monster::EvolutionType_Trade)
            {
                if(evolutionByTrade)
                {
                    std::cerr << "The monster " << i->first << " have already evolution by trade" << std::endl;
                    i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                    continue;
                }
                evolutionByTrade=true;
            }
            if(i->second.evolutions.at(index).type==Monster::EvolutionType_Item)
            {
                if(itemUse.find(i->second.evolutions.at(index).data.item)!=itemUse.cend())
                {
                    std::cerr << "The monster " << i->first << " have already evolution with this item" << std::endl;
                    i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                    continue;
                }
                itemUse.insert(i->second.evolutions.at(index).data.item);
            }
            if(i->second.evolutions.at(index).evolveTo==i->first)
            {
                std::cerr << "The monster " << i->first << " can't evolve into them self" << std::endl;
                i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                continue;
            }
            else if(monsters.find(i->second.evolutions.at(index).evolveTo)==monsters.cend())
            {
                std::cerr << "The monster " << i->second.evolutions.at(index).evolveTo << " for the evolution of " << i->first << " can't be found" << std::endl;
                i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                continue;
            }
            index++;
        }
        ++i;
    }
    #endif

    return monsters;
}
