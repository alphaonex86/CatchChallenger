#include "FightLoader.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#ifndef EPOLLCATCHCHALLENGERSERVER
#include "../../general/base/CommonDatapack.hpp"
#endif
#include "../../general/base/cpp11addition.hpp"
#include <iostream>

using namespace CatchChallenger;

#ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
std::unordered_map<uint8_t,Buff> FightLoader::loadMonsterBuff(const std::string &folder)
{
    std::unordered_map<uint8_t,Buff> monsterBuffs;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &fileList=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    unsigned int file_index=0;
    while(file_index<fileList.size())
    {
        const std::string &file=fileList.at(file_index).absoluteFilePath;
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
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
        if(strcmp(root->Name(),"buffs")!=0)
        {
            std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found for the xml file" << std::endl;
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        const tinyxml2::XMLElement * item = root->FirstChildElement("buff");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                uint8_t id=stringtouint8(item->Attribute("id"),&ok);
                if(ok && monsterBuffs.find(id)!=monsterBuffs.cend())
                    std::cerr << "Unable to open the xml file: " << file << ", id already found: child->Name(): " << item->Name() << std::endl;
                else if(ok)
                {
                    Buff::Duration general_duration=Buff::Duration_ThisFight;
                    uint8_t general_durationNumberOfTurn=0;
                    float general_capture_bonus=1.0;
                    if(item->Attribute("capture_bonus")!=NULL)
                    {
                       general_capture_bonus=stringtofloat(item->Attribute("capture_bonus"),&ok);
                        if(!ok)
                        {
                            std::cerr << "Unable to open the xml file: " << file << ", capture_bonus is not a number: child->Name(): " << item->Name() << std::endl;
                            general_capture_bonus=1.0;
                        }
                    }
                    if(item->Attribute("duration")!=NULL)
                    {
                        if(strcmp(item->Attribute("duration"),"Always")==0)
                            general_duration=Buff::Duration_Always;
                        else if(strcmp(item->Attribute("duration"),"NumberOfTurn")==0)
                        {
                            if(item->Attribute("durationNumberOfTurn")!=NULL)
                            {
                                general_durationNumberOfTurn=stringtouint8(item->Attribute("durationNumberOfTurn"),&ok);
                                if(!ok)
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", durationNumberOfTurn is not a number: child->Name(): " << item->Name() << std::endl;
                                    general_durationNumberOfTurn=3;
                                }
                                if(general_durationNumberOfTurn<=0)
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", durationNumberOfTurn is egal to 0: child->Name(): " << item->Name() << std::endl;
                                    general_durationNumberOfTurn=3;
                                }
                            }
                            else
                                general_durationNumberOfTurn=3;
                            general_duration=Buff::Duration_NumberOfTurn;
                        }
                        else if(strcmp(item->Attribute("duration"),"ThisFight")==0)
                            general_duration=Buff::Duration_ThisFight;
                        else
                        {
                            std::cerr << "Unable to open the xml file: " << file << ", attribute duration have wrong value \"" <<
                                         item->Attribute("duration") << "\" is not a number: child->Name(): " << item->Name() << std::endl;
                            general_duration=Buff::Duration_ThisFight;
                        }
                    }
                    std::unordered_map<uint8_t,Buff::GeneralEffect> levelDef;
                    const tinyxml2::XMLElement * effect = item->FirstChildElement("effect");
                    if(effect!=NULL)
                    {
                        const tinyxml2::XMLElement * level = effect->FirstChildElement("level");
                        while(level!=NULL)
                        {
                            if(level->Attribute("number")!=NULL)
                            {
                                uint8_t number=stringtouint8(level->Attribute("number"),&ok);
                                if(ok)
                                {
                                    if(number>0)
                                    {
                                        Buff::Duration duration=general_duration;
                                        uint8_t durationNumberOfTurn=general_durationNumberOfTurn;
                                        float capture_bonus=general_capture_bonus;
                                        if(item->Attribute("capture_bonus")!=NULL)
                                        {
                                           capture_bonus=stringtofloat(item->Attribute("capture_bonus"),&ok);
                                            if(!ok)
                                            {
                                                std::cerr << "Unable to open the xml file: " << file << ", capture_bonus is not a number: child->Name(): " << item->Name() << std::endl;
                                                capture_bonus=general_capture_bonus;
                                            }
                                        }
                                        if(item->Attribute("duration")!=NULL)
                                        {
                                            if(strcmp(item->Attribute("duration"),"Always")==0)
                                                duration=Buff::Duration_Always;
                                            else if(strcmp(item->Attribute("duration"),"NumberOfTurn")==0)
                                            {
                                                if(item->Attribute("durationNumberOfTurn")!=NULL)
                                                {
                                                    durationNumberOfTurn=stringtouint8(item->Attribute("durationNumberOfTurn"),&ok);
                                                    if(!ok)
                                                    {
                                                        std::cerr << "Unable to open the xml file: " << file << ", durationNumberOfTurn is not a number: child->Name(): " << item->Name() << std::endl;
                                                        durationNumberOfTurn=general_durationNumberOfTurn;
                                                    }
                                                }
                                                else
                                                    durationNumberOfTurn=general_durationNumberOfTurn;
                                                duration=Buff::Duration_NumberOfTurn;
                                            }
                                            else if(strcmp(item->Attribute("duration"),"ThisFight")==0)
                                                duration=Buff::Duration_ThisFight;
                                            else
                                            {
                                                std::cerr << "Unable to open the xml file: " << file << ", attribute duration have wrong value \"" << item->Attribute("duration") << "\" is not a number: child->Name(): " << item->Name() << std::endl;
                                                duration=general_duration;
                                            }
                                        }
                                        levelDef[number].duration=duration;
                                        levelDef[number].durationNumberOfTurn=durationNumberOfTurn;
                                        levelDef[number].capture_bonus=capture_bonus;



                                        const tinyxml2::XMLElement * inFight = level->FirstChildElement("inFight");
                                        while(inFight!=NULL)
                                        {
                                            ok=true;
                                            Buff::Effect effect;
                                            std::string text;
                                            if(inFight->Attribute("hp")!=NULL)
                                            {
                                                text=inFight->Attribute("hp");
                                                effect.on=Buff::Effect::EffectOn_HP;
                                            }
                                            else if(inFight->Attribute("defense")!=NULL)
                                            {
                                                text=inFight->Attribute("defense");
                                                effect.on=Buff::Effect::EffectOn_Defense;
                                            }
                                            else if(inFight->Attribute("attack")!=NULL)
                                            {
                                                text=inFight->Attribute("attack");
                                                effect.on=Buff::Effect::EffectOn_Attack;
                                            }
                                            else
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", not know attribute balise: child->Name(): "
                                                          << inFight->Name() << std::endl;
                                            }
                                            if(ok)
                                            {
                                                if(stringEndsWith(text,"%"))
                                                    effect.type=QuantityType_Percent;
                                                else
                                                    effect.type=QuantityType_Quantity;
                                                stringreplaceOne(text,"%","");
                                                stringreplaceOne(text,"+","");
                                                effect.quantity=stringtoint32(text,&ok);
                                                if(ok)
                                                    levelDef[number].fight.push_back(effect);
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", \"" << text <<
                                                                 "\" something is wrong, or is not a number, or not into hp or defense balise: child->Name(): "
                                                              << inFight->Name() << std::endl;
                                            }
                                            inFight = inFight->NextSiblingElement("inFight");
                                        }
                                        const tinyxml2::XMLElement * inWalk = level->FirstChildElement("inWalk");
                                        while(inWalk!=NULL)
                                        {
                                            if(inWalk->Attribute("steps")!=NULL)
                                            {
                                                uint32_t steps=stringtouint32(inWalk->Attribute("steps"),&ok);
                                                if(ok)
                                                {
                                                    Buff::EffectInWalk effect;
                                                    effect.steps=steps;
                                                    std::string text;
                                                    if(inWalk->Attribute("hp")!=NULL)
                                                    {
                                                        text=inWalk->Attribute("hp");
                                                        effect.effect.on=Buff::Effect::EffectOn_HP;
                                                    }
                                                    else if(inWalk->Attribute("defense")!=NULL)
                                                    {
                                                        text=inWalk->Attribute("defense");
                                                        effect.effect.on=Buff::Effect::EffectOn_Defense;
                                                    }
                                                    else
                                                        std::cerr << "Unable to open the xml file: " << file << ", not action found: child->Name(): " << item->Name() << std::endl;
                                                    if(stringEndsWith(text,"%"))
                                                        effect.effect.type=QuantityType_Percent;
                                                    else
                                                        effect.effect.type=QuantityType_Quantity;
                                                    stringreplaceOne(text,"%","");
                                                    stringreplaceOne(text,"+","");
                                                    effect.effect.quantity=stringtoint32(text,&ok);
                                                    if(ok)
                                                        levelDef[number].walk.push_back(effect);
                                                    else
                                                        std::cerr << "Unable to open the xml file: " << file << ", " << text << " is not a number: child->Name(): " << item->Name() << std::endl;
                                                }
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", have not tag steps: child->Name(): " << item->Name() << std::endl;
                                            }
                                            inWalk = inWalk->NextSiblingElement("inWalk");
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", level need be egal or greater than 1: child->Name(): " << item->Name() << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", number tag is not a number: child->Name(): " << item->Name() << std::endl;
                            }
                            level = level->NextSiblingElement("level");
                        }
                        uint8_t index=1;
                        while(levelDef.find(index)!=levelDef.cend())
                        {
                            /*if(levelDef.value(index).fight.empty() && levelDef.value(index).walk.empty())
                                std::cerr << "Unable to open the xml file: " << file << ", no effect loaded for buff %4 at level %5, missing level to continue: child->Name(): %2 (at line: %3)").arg(file).arg(item->Name()).arg(CATCHCHALLENGER_XMLELENTATLINE(item)).arg(id).arg(index));*/
                            monsterBuffs[id].level.push_back(levelDef.at(index));
                            levelDef.erase(index);
                            index++;
                        }
                        if(levelDef.size()>0)
                            std::cerr << "Unable to open the xml file: " << file << ", level up to " << index << " loaded, missing level to continue: child->Name(): " << item->Name() << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the xml file: " << file << ", have not effet balise: child->Name(): " << item->Name() << std::endl;
                }
                else
                    std::cerr << "Unable to open the xml file: " << file << ", id is not a number: child->Name(): " << item->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", have not the buff id: child->Name(): " << item->Name() << std::endl;
            item = item->NextSiblingElement("buff");
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        file_index++;
    }
    return monsterBuffs;
}
#endif
