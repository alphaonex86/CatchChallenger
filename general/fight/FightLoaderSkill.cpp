#include "FightLoader.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/cpp11addition.hpp"
#ifndef EPOLLCATCHCHALLENGERSERVER
#include "../../general/base/CommonDatapack.hpp"
#endif
#include <iostream>

using namespace CatchChallenger;

std::unordered_map<uint16_t,Skill> FightLoader::loadMonsterSkill(std::unordered_map<std::string,CATCHCHALLENGER_TYPE_SKILL> &tempNameToSkillId,const std::string &folder
                                                   #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                   , const std::unordered_map<uint8_t, Buff> &monsterBuffs
                                                   , const std::vector<Type> &types
                                                   #endif
                                                   )
{
    std::unordered_map<uint16_t,Skill> monsterSkills;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &fileList=
            CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    unsigned int file_index=0;
    while(file_index<fileList.size())
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
        #endif // CATCHCHALLENGER_CLASS_MASTER
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
            std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found 2 for the xml file" << std::endl;
            file_index++;
            continue;
        }
        if(strcmp(root->Name(),"skills")!=0)
        {
            std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found for the xml file" << std::endl;
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        bool ok2;
        #endif
        const tinyxml2::XMLElement * item = root->FirstChildElement("skill");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                uint16_t id=stringtouint16(item->Attribute("id"),&ok);
                if(ok && monsterSkills.find(id)!=monsterSkills.cend())
                    std::cerr << "Unable to open the xml file: " << file << ", id already found: child->Name(): " << item->Name() << std::endl;
                else if(ok)
                {
                    const tinyxml2::XMLElement * name = item->FirstChildElement("name");
                    while(item!=NULL)
                    {
                        if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                        {
                            tempNameToSkillId[name->GetText()]=id;
                            break;
                        }
                        name = name->NextSiblingElement("name");
                    }
                    std::unordered_map<uint8_t,Skill::SkillList> levelDef;
                    const tinyxml2::XMLElement * effect = item->FirstChildElement("effect");
                    if(effect!=NULL)
                    {
                        const tinyxml2::XMLElement * level = effect->FirstChildElement("level");
                        while(level!=NULL)
                        {
                            if(level->Attribute("number")!=NULL)
                            {
                                uint32_t sp=0;
                                if(level->Attribute("sp")!=NULL)
                                {
                                    sp=stringtouint32(level->Attribute("sp"),&ok);
                                    if(!ok)
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", sp is not number: child->Name(): " << level->Name() << std::endl;
                                        sp=0;
                                    }
                                }
                                uint8_t endurance=40;
                                if(level->Attribute("endurance")!=NULL)
                                {
                                    endurance=stringtouint8(level->Attribute("endurance"),&ok);
                                    if(!ok)
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", endurance is not number: child->Name(): " << level->Name() << std::endl;
                                        endurance=40;
                                    }
                                    if(endurance<1)
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", endurance lower than 1: child->Name(): " << level->Name() << std::endl;
                                        endurance=40;
                                    }
                                }
                                uint8_t number=0;
                                if(ok)
                                    number=stringtouint8(level->Attribute("number"),&ok);
                                if(ok)
                                {
                                    levelDef[number].sp_to_learn=sp;
                                    levelDef[number].endurance=endurance;
                                    if(number>0)
                                    {
                                        #ifndef CATCHCHALLENGER_CLASS_MASTER
                                        {
                                            const tinyxml2::XMLElement * life = level->FirstChildElement("life");
                                            while(life!=NULL)
                                            {
                                                Skill::Life effect;
                                                effect.effect.on=ApplyOn_Nobody;
                                                effect.effect.quantity=0;
                                                effect.effect.type=QuantityType_Quantity;
                                                effect.success=0;
                                                if(life->Attribute("applyOn")!=NULL)
                                                {
                                                    const std::string &applyOn=life->Attribute("applyOn");
                                                    if(applyOn=="aloneEnemy")
                                                        effect.effect.on=ApplyOn_AloneEnemy;
                                                    else if(applyOn=="themself")
                                                        effect.effect.on=ApplyOn_Themself;
                                                    else if(applyOn=="allEnemy")
                                                        effect.effect.on=ApplyOn_AllEnemy;
                                                    else if(applyOn=="allAlly")
                                                        effect.effect.on=ApplyOn_AllAlly;
                                                    else if(applyOn=="nobody")
                                                        effect.effect.on=ApplyOn_Nobody;
                                                    else
                                                    {
                                                        std::cerr << "Unable to open the xml file: " << file << ", applyOn tag wrong " << life->Attribute("applyOn") << ": child->Name(): " << item->Name() << std::endl;
                                                        effect.effect.on=ApplyOn_AloneEnemy;
                                                    }
                                                }
                                                else
                                                    effect.effect.on=ApplyOn_AloneEnemy;
                                                std::string text;
                                                if(life->Attribute("quantity")!=NULL)
                                                    text=life->Attribute("quantity");
                                                if(stringEndsWith(text,"%"))
                                                    effect.effect.type=QuantityType_Percent;
                                                else
                                                    effect.effect.type=QuantityType_Quantity;
                                                stringreplaceOne(text,"%","");
                                                stringreplaceOne(text,"+","");
                                                effect.effect.quantity=stringtoint32(text,&ok);
                                                effect.success=100;
                                                if(life->Attribute("success")!=NULL)
                                                {
                                                    std::string success=life->Attribute("success");
                                                    stringreplaceOne(success,"%","");
                                                    effect.success=stringtouint8(success,&ok2);
                                                    if(!ok2)
                                                    {
                                                        std::cerr << "Unable to open the xml file: " << file << ", success wrong corrected to 100%: child->Name(): " << item->Name() << std::endl;
                                                        effect.success=100;
                                                    }
                                                }
                                                if(ok)
                                                {
                                                    if(effect.effect.quantity!=0)
                                                        levelDef[number].life.push_back(effect);
                                                }
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", " << text << " is not a number: child->Name(): " << item->Name() << std::endl;
                                                life = life->NextSiblingElement("life");
                                            }
                                        }
                                        #endif // CATCHCHALLENGER_CLASS_MASTER
                                        #ifndef CATCHCHALLENGER_CLASS_MASTER
                                        {
                                            const tinyxml2::XMLElement * buff = level->FirstChildElement("buff");
                                            while(buff!=NULL)
                                            {
                                                if(buff->Attribute("id")!=NULL)
                                                {
                                                    const uint8_t idBuff=stringtouint8(buff->Attribute("id"),&ok);
                                                    if(ok)
                                                    {
                                                        Skill::Buff effect;
                                                        effect.effect.buff=0;
                                                        effect.effect.level=0;
                                                        effect.effect.on=ApplyOn_Nobody;
                                                        effect.success=0;
                                                        if(buff->Attribute("applyOn")!=NULL)
                                                        {
                                                            const std::string &applyOn=buff->Attribute("applyOn");
                                                            if(applyOn=="aloneEnemy")
                                                                effect.effect.on=ApplyOn_AloneEnemy;
                                                            else if(applyOn=="themself")
                                                                effect.effect.on=ApplyOn_Themself;
                                                            else if(applyOn=="allEnemy")
                                                                effect.effect.on=ApplyOn_AllEnemy;
                                                            else if(applyOn=="allAlly")
                                                                effect.effect.on=ApplyOn_AllAlly;
                                                            else if(applyOn=="nobody")
                                                                effect.effect.on=ApplyOn_Nobody;
                                                            else
                                                            {
                                                                std::cerr << "Unable to open the xml file: " << file << ", applyOn tag wrong "
                                                                          << buff->Attribute("applyOn") << ": child->Name(): " << item->Name() << std::endl;
                                                                effect.effect.on=ApplyOn_AloneEnemy;
                                                            }
                                                        }
                                                        else
                                                            effect.effect.on=ApplyOn_AloneEnemy;
                                                        if(monsterBuffs.find(idBuff)==monsterBuffs.cend())
                                                            std::cerr << "Unable to open the xml file: " << file << ", this buff id is not found: " << idBuff << ": child->Name(): " << item->Name() << std::endl;
                                                        else
                                                        {
                                                            effect.effect.level=1;
                                                            ok2=true;
                                                            if(buff->Attribute("level")!=NULL)
                                                            {
                                                                const std::string &level=buff->Attribute("level");
                                                                effect.effect.level=stringtouint8(level,&ok2);
                                                                if(!ok2)
                                                                    std::cerr << "Unable to open the xml file: " << file << ", level wrong: " <<
                                                                                 buff->Attribute("level") << " child->Name(): " << item->Name() << std::endl;
                                                                if(effect.effect.level<=0)
                                                                {
                                                                    ok2=false;
                                                                    std::cerr << "Unable to open the xml file: " << file << ", level need be egal or greater than 1: child->Name(): " << item->Name() << std::endl;
                                                                }
                                                            }
                                                            if(ok2)
                                                            {
                                                                if(monsterBuffs.at(idBuff).level.size()<effect.effect.level)
                                                                    std::cerr << "Unable to open the xml file: " << file << ", level needed: " << effect.effect.level << ", level max found: " << monsterBuffs.at(idBuff).level.size() << ": child->Name(): " << item->Name() << std::endl;
                                                                else
                                                                {
                                                                    effect.effect.buff=idBuff;
                                                                    effect.success=100;
                                                                    if(buff->Attribute("success")!=NULL)
                                                                    {
                                                                        std::string success=buff->Attribute("success");
                                                                        stringreplaceOne(success,"%","");
                                                                        effect.success=stringtouint8(success,&ok2);
                                                                        if(!ok2)
                                                                        {
                                                                            std::cerr << "Unable to open the xml file: " << file << ", success wrong corrected to 100%: child->Name(): " << item->Name() << std::endl;
                                                                            effect.success=100;
                                                                        }
                                                                    }
                                                                    levelDef[number].buff.push_back(effect);
                                                                }
                                                            }
                                                        }
                                                    }
                                                    else
                                                        std::cerr << "Unable to open the xml file: " << file << ", have not tag id: child->Name(): " << item->Name() << std::endl;
                                                }
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", have not tag id: child->Name(): " << item->Name() << std::endl;
                                                buff = buff->NextSiblingElement("buff");
                                            }
                                        }
                                        #endif // CATCHCHALLENGER_CLASS_MASTER
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", level need be egal or greater than 1: child->Name(): " << item->Name() << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", number tag is not a number: child->Name(): " << item->Name() << std::endl;
                            }
                            level = level->NextSiblingElement("level");
                        }
                        if(levelDef.size()==0)
                            std::cerr << "Unable to open the xml file: " << file << ", 0 level found: child->Name(): " << item->Name() << std::endl;
                        else
                        {
                            monsterSkills[id].type=255;
                            #ifndef CATCHCHALLENGER_CLASS_MASTER
                            if(item->Attribute("type")!=NULL)
                            {
                                if(typeNameToId.find(item->Attribute("type"))!=typeNameToId.cend())
                                    monsterSkills[id].type=typeNameToId.at(item->Attribute("type"));
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", type not found: " << item->Attribute("type") << ": child->Name(): " << item->Name() << std::endl;
                            }
                            #endif // CATCHCHALLENGER_CLASS_MASTER
                        }
                        //order by level to learn
                        uint8_t index=1;
                        while(levelDef.find(index)!=levelDef.cend())
                        {
                            /*if(levelDef.value(index).buff.empty() && levelDef.value(index).life.empty())
                                std::cerr << "Unable to open the xml file: " << file << ", no effect loaded for skill %4 at level %5, missing level to continue: child->Name(): %2 (at line: %3)").arg(file).arg(item->Name()).arg(CATCHCHALLENGER_XMLELENTATLINE(item)).arg(id).arg(index));*/
                            monsterSkills[id].level.push_back(levelDef.at(index));
                            levelDef.erase(index);
                            index++;
                        }
                        if(levelDef.size()>0)
                            std::cerr << "Unable to open the xml file: " << file << ", level up to " << index << " loaded, missing level to continue: child->Name(): " << item->Name() << std::endl;
                    }
                    /*else
                        std::cerr << "Unable to open the xml file: " << file << ", have not effect balise: child->Name(): " << item->Name() << std::endl;*/
                }
                else
                    std::cerr << "Unable to open the xml file: " << file << ", id is not a number: child->Name(): " << item->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", have not the skill id: child->Name(): " << item->Name() << std::endl;
            item = item->NextSiblingElement("skill");
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        file_index++;
    }

    //check the default attack
    if(monsterSkills.find(0)==monsterSkills.cend())
        std::cerr << "Warning: no default monster attack if no more attack" << std::endl;
    else if(monsterSkills.at(0).level.empty())
    {
        monsterSkills.erase(0);
        std::cerr << "Warning: no level for default monster attack if no more attack" << std::endl;
    }
    else
    {
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        if(monsterSkills.at(0).level.front().life.empty())
        {
            monsterSkills.erase(0);
            std::cerr << "Warning: no life effect for the default attack" << std::endl;
        }
        else
        {
            const Skill &skill=monsterSkills.at(0);
            const Skill::SkillList &skillList=skill.level.front();
            const size_t &list_size=skillList.life.size();
            unsigned int index=0;
            while(index<list_size)
            {
                const Skill::Life &life=skillList.life.at(index);
                if(life.success==100 && life.effect.on==ApplyOn_AloneEnemy && life.effect.quantity<0)
                    break;
                index++;
            }
            if(index==list_size)
            {
                const Skill::Life &life=skillList.life.front();
                monsterSkills.erase(0);
                std::cerr << "Warning: no valid life effect for the default attack (id: 0): success=100%: " << life.success << ", on=ApplyOn_AloneEnemy: " << life.effect.on << ", quantity<0: " << life.effect.quantity << " for skill" << std::endl;
            }
        }
        #endif
    }

    return monsterSkills;
}
