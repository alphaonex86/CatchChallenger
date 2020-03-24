#include "FightLoader.hpp"
#ifndef EPOLLCATCHCHALLENGERSERVER
#include "../../general/base/CommonDatapack.hpp"
#endif
#include <iostream>

using namespace CatchChallenger;

bool CatchChallenger::operator<(const Monster::AttackToLearn &entry1, const Monster::AttackToLearn &entry2)
{
    if(entry1.learnAtLevel!=entry2.learnAtLevel)
        return entry1.learnAtLevel < entry2.learnAtLevel;
    else
        return entry1.learnSkill < entry2.learnSkill;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::vector<Type> FightLoader::loadTypes(const std::string &file)
{
    std::unordered_map<std::string,uint8_t> nameToId;
    std::vector<Type> types;
    tinyxml2::XMLDocument *domDocument;
    //open and quick check the file
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
            std::cerr << file+", "+domDocument->ErrorName() << std::endl;
            return types;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return types;
    }
    if(root->Name()==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", \"types\" root balise not found 2 for the xml file" << std::endl;
        return types;
    }
    if(strcmp(root->Name(),"types")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"types\" root balise not found for the xml file" << std::endl;
        return types;
    }

    //load the content
    bool ok;
    {
        std::unordered_set<std::string> duplicate;
        const tinyxml2::XMLElement * typeItem = root->FirstChildElement("type");
        while(typeItem!=NULL)
        {
            if(typeItem->Attribute("name")!=NULL)
            {
                std::string name=std::string(typeItem->Attribute("name"));
                if(duplicate.find(name)==duplicate.cend())
                {
                    duplicate.insert(name);
                    Type type;
                    type.name=name;
                    nameToId[type.name]=static_cast<uint8_t>(types.size());
                    types.push_back(type);
                }
                else
                    std::cerr << "Unable to open the file: " << file << ", name is already set for type: child->Name(): " << typeItem->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the file: " << file << ", have not the item id: child->Name(): " << typeItem->Name() << std::endl;
            typeItem = typeItem->NextSiblingElement("type");
        }
    }
    {
        std::unordered_set<std::string> duplicate;
        const tinyxml2::XMLElement * typeItem = root->FirstChildElement("type");
        while(typeItem!=NULL)
        {
            if(typeItem->Attribute("name")!=NULL)
            {
                std::string name=std::string(typeItem->Attribute("name"));
                if(duplicate.find(name)==duplicate.cend())
                {
                    duplicate.insert(name);
                    const tinyxml2::XMLElement * multiplicator = typeItem->FirstChildElement("multiplicator");
                    while(multiplicator!=NULL)
                    {
                        if(multiplicator->Attribute("number")!=NULL && multiplicator->Attribute("to")!=NULL)
                        {
                            float number=stringtofloat(multiplicator->Attribute("number"),&ok);
                            std::vector<std::string> to=stringsplit(multiplicator->Attribute("to"),';');
                            if(ok && (static_cast<double>(number)==2.0 || static_cast<double>(number)==0.5 || static_cast<double>(number)==0.0))
                            {
                                unsigned int index=0;
                                while(index<to.size())
                                {
                                    if(nameToId.find(to.at(index))!=nameToId.cend())
                                    {
                                        const std::string &typeName=to.at(index);
                                        if(number==0)
                                            types[nameToId.at(name)].multiplicator[nameToId.at(typeName)]=0;
                                        else if(number>1)
                                            types[nameToId.at(name)].multiplicator[nameToId.at(typeName)]=number;
                                        else
                                            types[nameToId.at(name)].multiplicator[nameToId.at(typeName)]=static_cast<float>(-(1.0/static_cast<double>(number)));
                                    }
                                    else
                                        std::cerr << "Unable to open the file: " << file << ", name is not into list: " << to.at(index) << " is not found: child->Name(): " << multiplicator->Name() << std::endl;
                                    index++;
                                }
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", name is already set for type: child->Name(): " << multiplicator->Name() << std::endl;
                        }
                        else
                            std::cerr << "Unable to open the file: " << file << ", have not the item id: child->Name(): " << multiplicator->Name() << std::endl;
                        multiplicator = multiplicator->NextSiblingElement("multiplicator");
                    }
                }
                else
                    std::cerr << "Unable to open the file: " << file << ", name is already set for type: child->Name(): " << typeItem->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the file: " << file << ", have not the item id: child->Name(): " << typeItem->Name() << std::endl;
            typeItem = typeItem->NextSiblingElement("type");
        }
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return types;
}
#endif

#ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
std::unordered_map<uint16_t/*item*/, std::unordered_map<uint16_t/*monster*/,uint16_t/*evolveTo*/> > FightLoader::loadMonsterEvolutionItems(const std::unordered_map<uint16_t,Monster> &monsters)
{
    std::unordered_map<uint16_t, std::unordered_map<uint16_t,uint16_t> > evolutionItem;
    auto i=monsters.begin();
    while(i!=monsters.cend())
    {
        unsigned int index=0;
        while(index<i->second.evolutions.size())
        {
            if(i->second.evolutions.at(index).type==Monster::EvolutionType_Item)
                evolutionItem[i->second.evolutions.at(index).data.item][i->first]=i->second.evolutions.at(index).evolveTo;
            index++;
        }
        ++i;
    }
    return evolutionItem;
}
#endif

#ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
std::unordered_map<uint16_t/*item*/, std::unordered_set<uint16_t/*monster*/> > FightLoader::loadMonsterItemToLearn(const std::unordered_map<uint16_t,Monster> &monsters,const std::unordered_map<uint16_t/*item*/, std::unordered_map<uint16_t/*monster*/,uint16_t/*evolveTo*/> > &evolutionItem)
{
    std::unordered_map<uint16_t/*item*/, std::unordered_set<uint16_t/*monster*/> > learnItem;
    auto i=monsters.cbegin();
    while(i!=monsters.cend())
    {
        auto j=i->second.learnByItem.cbegin();
        while(j!=i->second.learnByItem.cend())
        {
            if(evolutionItem.find(j->first)==evolutionItem.cend())
                learnItem[j->first].insert(i->first);
            else
                std::cerr << "The item " << j->first << " can't be used to learn because already used to evolv" << std::endl;
            ++j;
        }
        ++i;
    }
    return learnItem;
}
#endif

std::vector<PlayerMonster::PlayerSkill> FightLoader::loadDefaultAttack(const uint16_t &monsterId,const uint8_t &level, const std::unordered_map<uint16_t,Monster> &monsters, const std::unordered_map<uint16_t, Skill> &monsterSkills)
{
    std::vector<CatchChallenger::PlayerMonster::PlayerSkill> skills;
    std::vector<CatchChallenger::Monster::AttackToLearn> attack=monsters.at(monsterId).learn;
    unsigned int index=0;
    while(index<attack.size())
    {
        if(attack.at(index).learnAtLevel<=level)
        {
            CatchChallenger::PlayerMonster::PlayerSkill temp;
            temp.level=attack.at(index).learnSkillLevel;
            temp.skill=attack.at(index).learnSkill;
            temp.endurance=0;
            if(monsterSkills.find(temp.skill)!=monsterSkills.cend())
                if(temp.level<=monsterSkills.at(temp.skill).level.size() && temp.level>0)
                    temp.endurance=monsterSkills.at(temp.skill).level.at(temp.level-1).endurance;
            skills.push_back(temp);
        }
        index++;
    }
    while(skills.size()>4)
        skills.erase(skills.begin());
    return skills;
}
