#include "DatapackGeneralLoader.hpp"
#include "../FacilityLibGeneral.hpp"
#include "../cpp11addition.hpp"
#include "../CommonDatapack.hpp"
#include "../../tinyXML2/customtinyxml2.hpp"
#include <iostream>

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_MASTER
void DatapackGeneralLoader::loadItems(catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_ITEM> &tempNameToItemId,const std::string &folder,const catchchallenger_datapack_map<uint8_t,Buff> &monsterBuffs,
        catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, std::vector<MonsterItemEffect> > &monsterItemEffect,
        catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, std::vector<MonsterItemEffectOutOfFight> > &monsterItemEffectOutOfFight,
        catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Item> &outItems,
        CATCHCHALLENGER_TYPE_ITEM &itemMaxId,
        catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, uint32_t> &repel,
        catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Trap> &trap)
{
    #ifdef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    (void)monsterBuffs;
    #endif
    itemMaxId=0;
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
        tinyxml2::XMLDocument *domDocument;
        //open and quick check the file
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.has_xmlLoadedFile(file))
            domDocument=&CommonDatapack::commonDatapack.get_xmlLoadedFile_rw(file);
        else
        {
            domDocument=&CommonDatapack::commonDatapack.get_xmlLoadedFile_rw(file);
            #else
            domDocument=new tinyxml2::XMLDocument();
            #endif
            const tinyxml2::XMLError loadOkay = domDocument->LoadFile(file.c_str());
            if(loadOkay!=0)
            {
                std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
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
        if(strcmp(root->Name(),"items")!=0)
        {
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        const tinyxml2::XMLElement * item = root->FirstChildElement("item");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                const uint16_t &id=stringtouint16(item->Attribute("id"),&ok);
                if(ok)
                {
                    const tinyxml2::XMLElement * name = item->FirstChildElement("name");
                    while(item!=NULL)
                    {
                        if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                        {
                            tempNameToItemId[str_tolower(name->GetText())]=id;
                            break;
                        }
                        name = name->NextSiblingElement("name");
                    }
                    if(outItems.find(id)==outItems.cend())
                    {
                        if(itemMaxId<id)
                            itemMaxId=id;
                        //load the price
                        {
                            if(item->Attribute("price")!=NULL)
                            {
                                bool ok;
                                outItems[id].price=stringtouint32(item->Attribute("price"),&ok);
                                if(!ok)
                                {
                                    std::cerr << "price is not a number, file: " << file << ", child->Name(): " << item->Name() << std::endl;
                                    outItems[id].price=0;
                                }
                            }
                            else
                                outItems[id].price=0;
                        }
                        //load the consumeAtUse
                        {
                            if(item->Attribute("consumeAtUse")!=NULL)
                            {
                                if(strcmp(item->Attribute("consumeAtUse"),"false")==0)
                                    outItems[id].consumeAtUse=false;
                                else
                                    outItems[id].consumeAtUse=true;
                            }
                            else
                                outItems[id].consumeAtUse=true;
                        }
                        bool haveAnEffect=false;
                        //load the trap
                        if(!haveAnEffect)
                        {
                            const tinyxml2::XMLElement * trapItem = item->FirstChildElement("trap");
                            if(trapItem!=NULL)
                            {
                                Trap trapEntry;
                                trapEntry.bonus_rate=1.0;
                                if(trapItem->Attribute("bonus_rate")!=NULL)
                                {
                                    float bonus_rate=stringtofloat(trapItem->Attribute("bonus_rate"),&ok);
                                    if(ok)
                                        trapEntry.bonus_rate=bonus_rate;
                                    else
                                        std::cerr << "Unable to open the file: bonus_rate is not a number, file: " << file << ", child->Name(): " << trapItem->Name() << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the file: trap have not the attribute bonus_rate, file: " << file << ", child->Name(): " << trapItem->Name() << std::endl;
                                trap[id]=trapEntry;
                                haveAnEffect=true;
                            }
                        }
                        //load the repel
                        if(!haveAnEffect)
                        {
                            const tinyxml2::XMLElement * repelItem = item->FirstChildElement("repel");
                            if(repelItem!=NULL)
                            {
                                if(repelItem->Attribute("step")!=NULL)
                                {
                                    const uint32_t &step=stringtouint32(repelItem->Attribute("step"),&ok);
                                    if(ok)
                                    {
                                        if(step>0)
                                        {
                                            repel[id]=step;
                                            haveAnEffect=true;
                                        }
                                        else
                                            std::cerr << "Unable to open the file: step is not greater than 0, file: " << file << ", child->Name(): " << repelItem->Name() << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the file: step is not a number, file: " << file << ", child->Name(): " << repelItem->Name() << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the file: repel have not the attribute step, file: " << file << ", child->Name(): " << repelItem->Name() << std::endl;
                            }
                        }
                        //load the monster effect
                        if(!haveAnEffect)
                        {
                            {
                                const tinyxml2::XMLElement * hpItem = item->FirstChildElement("hp");
                                while(hpItem!=NULL)
                                {
                                    if(hpItem->Attribute("add")!=NULL)
                                    {
                                        if(strcmp(hpItem->Attribute("add"),"all")==0)
                                        {
                                            MonsterItemEffect effectEntry;
                                            effectEntry.type=MonsterItemEffectType_AddHp;
                                            effectEntry.data.hp=-1;
                                            monsterItemEffect[id].push_back(effectEntry);
                                        }
                                        else
                                        {
                                            std::string addString=hpItem->Attribute("add");
                                            if(addString.find("%")==std::string::npos)//todo this part
                                            {
                                                const int32_t &add=stringtouint32(hpItem->Attribute("add"),&ok);
                                                if(ok)
                                                {
                                                    if(add>0)
                                                    {
                                                        MonsterItemEffect effectEntry;
                                                        effectEntry.type=MonsterItemEffectType_AddHp;
                                                        effectEntry.data.hp=add;
                                                        monsterItemEffect[id].push_back(effectEntry);
                                                    }
                                                    else
                                                        std::cerr << "Unable to open the file, add is not greater than 0, file: " << file << ", child->Name(): " << hpItem->Name() << std::endl;
                                                }
                                                else
                                                    std::cerr << "Unable to open the file, add is not a number, file: " << file << ", child->Name(): " << hpItem->Name() << std::endl;
                                            }
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the file, hp have not the attribute add, file: " << file << ", child->Name(): " << hpItem->Name() << std::endl;
                                    hpItem = hpItem->NextSiblingElement("hp");
                                }
                            }
                            #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                            {
                                const tinyxml2::XMLElement * buffItem = item->FirstChildElement("buff");
                                while(buffItem!=NULL)
                                {
                                    if(buffItem->Attribute("remove")!=NULL)
                                    {
                                        if(strcmp(buffItem->Attribute("remove"),"all")==0)
                                        {
                                            MonsterItemEffect effectEntry;
                                            effectEntry.type=MonsterItemEffectType_RemoveBuff;
                                            effectEntry.data.buff=-1;
                                            monsterItemEffect[id].push_back(effectEntry);
                                        }
                                        else
                                        {
                                            const int8_t &removebuffid=stringtouint8(buffItem->Attribute("remove"),&ok);
                                            if(ok)
                                            {
                                                if(removebuffid>0)
                                                {
                                                    if(monsterBuffs.find(removebuffid)!=monsterBuffs.cend())
                                                    {
                                                        MonsterItemEffect effectEntry;
                                                        effectEntry.type=MonsterItemEffectType_RemoveBuff;
                                                        effectEntry.data.buff=removebuffid;
                                                        monsterItemEffect[id].push_back(effectEntry);
                                                    }
                                                    else
                                                        std::cerr << "Unable to open the file, buff item to remove is not found, file: " << file << ", child->Name(): " << buffItem->Name() << std::endl;
                                                }
                                                else
                                                    std::cerr << "Unable to open the file, step is not greater than 0, file: " << file << ", child->Name(): " << buffItem->Name() << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the file, step is not a number, file: " << file << ", child->Name(): " << buffItem->Name() << std::endl;
                                        }
                                    }
                                    /// \todo
                                     /* else
                                        std::cerr << "Unable to open the file: " << file << ", buff have not the attribute know attribute like remove: child->Name(): %2 (at line: %3)").arg(file).arg(buffItem->Name()).arg(CATCHCHALLENGER_XMLELENTATLINE(buffItem));*/
                                    buffItem = buffItem->NextSiblingElement("buff");
                                }
                            }
                            #endif
                            if(monsterItemEffect.find(id)!=monsterItemEffect.cend())
                                haveAnEffect=true;
                        }
                        //load the monster offline effect
                        if(!haveAnEffect)
                        {
                            const tinyxml2::XMLElement * levelItem = item->FirstChildElement("level");
                            while(levelItem!=NULL)
                            {
                                if(levelItem->Attribute("up")!=NULL)
                                {
                                    const uint8_t &levelUp=stringtouint8(levelItem->Attribute("up"),&ok);
                                    if(!ok)
                                        std::cerr << "Unable to open the file, level up is not possitive number, file: " << file << ", child->Name(): " << levelItem->Name() << std::endl;
                                    else if(levelUp<=0)
                                        std::cerr << "Unable to open the file, level up is greater than 0, file: " << file << ", child->Name(): " << levelItem->Name() << std::endl;
                                    else
                                    {
                                        MonsterItemEffectOutOfFight effectOutOfFightEntry;
                                        effectOutOfFightEntry.type=MonsterItemEffectTypeOutOfFight_AddLevel;
                                        effectOutOfFightEntry.data.level=levelUp;
                                        monsterItemEffectOutOfFight[id].push_back(effectOutOfFightEntry);
                                    }
                                }
                                else
                                    std::cerr << "Unable to open the file, level have not the attribute up, file: " << file << ", child->Name(): " << levelItem->Name() << std::endl;
                                levelItem = levelItem->NextSiblingElement("level");
                            }
                        }
                    }
                    else
                        std::cerr << "Unable to open the file, id number already set, file: " << file << ", child->Name(): " << item->Name() << std::endl;
                }
                else
                    std::cerr << "Unable to open the file, id is not a number, file: " << file << ", child->Name(): " << item->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the file, have not the item id, file: " << file << ", child->Name(): " << item->Name() << std::endl;
            item = item->NextSiblingElement("item");
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        file_index++;
    }
}
#endif
