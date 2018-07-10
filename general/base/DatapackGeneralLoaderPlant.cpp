#include "DatapackGeneralLoader.h"
#include "../../general/base/CommonDatapack.h"
#include <iostream>

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::unordered_map<uint8_t, Plant> DatapackGeneralLoader::loadPlants(const std::string &file)
{
    std::unordered_map<std::string,uint8_t> reputationNameToId;
    {
        uint8_t index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    std::unordered_map<uint8_t, Plant> plants;
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
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return plants;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return plants;
    }
    if(strcmp(root->Name(),"plants")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"plants\" root balise not found for reputation of the xml file" << std::endl;
        return plants;
    }

    //load the content
    bool ok,ok2;
    const tinyxml2::XMLElement * plantItem = root->FirstChildElement("plant");
    while(plantItem!=NULL)
    {
        if(plantItem->Attribute("id")!=NULL && plantItem->Attribute("itemUsed")!=NULL)
        {
            const uint8_t &id=stringtouint8(plantItem->Attribute("id"),&ok);
            const uint16_t &itemUsed=stringtouint16(plantItem->Attribute("itemUsed"),&ok2);
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
                        const tinyxml2::XMLElement * requirementsItem = plantItem->FirstChildElement("requirements");
                        if(requirementsItem!=NULL)
                        {
                            const tinyxml2::XMLElement * reputationItem = requirementsItem->FirstChildElement("reputation");
                            while(reputationItem!=NULL)
                            {
                                if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("level")!=NULL)
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
                                        std::cerr << "Reputation type not found: " << reputationItem->Attribute("type") << ", have not the id, child->Name(): " << reputationItem->Name() << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->Name(): " << reputationItem->Name() << std::endl;
                                reputationItem = reputationItem->NextSiblingElement("reputation");
                            }
                        }
                    }
                    {
                        const tinyxml2::XMLElement * rewardsItem = plantItem->FirstChildElement("rewards");
                        if(rewardsItem!=NULL)
                        {
                            const tinyxml2::XMLElement * reputationItem = rewardsItem->FirstChildElement("reputation");
                            while(reputationItem!=NULL)
                            {
                                if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("point")!=NULL)
                                {
                                    if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                                    {
                                        ReputationRewards reputationRewards;
                                        reputationRewards.point=stringtoint32(reputationItem->Attribute("point"),&ok);
                                        if(ok)
                                        {
                                            reputationRewards.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                            plant.rewards.reputation.push_back(reputationRewards);
                                        }
                                    }
                                }
                                else
                                    std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->Name(): " << reputationItem->Name() << std::endl;
                                reputationItem = reputationItem->NextSiblingElement("reputation");
                            }
                        }
                    }
                    ok=false;
                    const tinyxml2::XMLElement * quantity = plantItem->FirstChildElement("quantity");
                    if(quantity!=NULL)
                    {
                        if(quantity->GetText()!=NULL)
                        {
                            const float &float_quantity=stringtofloat(quantity->GetText(),&ok2);
                            const int &integer_part=float_quantity;
                            float random_part=float_quantity-integer_part;
                            random_part*=RANDOM_FLOAT_PART_DIVIDER;
                            plant.fix_quantity=static_cast<uint16_t>(integer_part);
                            plant.random_quantity=random_part;
                            ok=ok2;
                        }
                    }
                    int intermediateTimeCount=0;
                    const tinyxml2::XMLElement * grow = plantItem->FirstChildElement("grow");
                    if(grow!=NULL)
                    {
                        const tinyxml2::XMLElement * fruits = grow->FirstChildElement("fruits");
                        if(fruits!=NULL)
                        {
                            if(fruits->GetText()!=NULL)
                            {
                                plant.fruits_seconds=stringtouint32(fruits->GetText(),&ok2)*60;
                                plant.sprouted_seconds=static_cast<uint16_t>(plant.fruits_seconds);
                                plant.taller_seconds=static_cast<uint16_t>(plant.fruits_seconds);
                                plant.flowering_seconds=static_cast<uint16_t>(plant.fruits_seconds);
                                if(!ok2)
                                {
                                    std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << fruits->GetText() << " child->Name(): " << fruits->Name() << std::endl;
                                    ok=false;
                                }
                            }
                            else
                            {
                                ok=false;
                                std::cerr << "Unable to parse the plants file: " << file << ", fruits is not an element: child->Name(): " << fruits->Name() << std::endl;
                            }
                        }
                        else
                        {
                            ok=false;
                            std::cerr << "Unable to parse the plants file: " << file << ", fruits is null: child->Name(): " << grow->Name() << std::endl;
                        }
                        const tinyxml2::XMLElement * sprouted = grow->FirstChildElement("sprouted");
                        if(sprouted!=NULL)
                        {
                            if(sprouted->GetText()!=NULL)
                            {
                                plant.sprouted_seconds=stringtouint16(sprouted->GetText(),&ok2)*60;
                                if(!ok2)
                                {
                                    std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << sprouted->GetText() << " child->Name(): " << sprouted->Name() << std::endl;
                                    ok=false;
                                }
                                else
                                    intermediateTimeCount++;
                            }
                            else
                                std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not an element: child->Name(): " << sprouted->Name() << std::endl;
                        }
                        const tinyxml2::XMLElement * taller = grow->FirstChildElement("taller");
                        if(taller!=NULL)
                        {
                            if(taller->GetText()!=NULL)
                            {
                                plant.taller_seconds=stringtouint16(taller->GetText(),&ok2)*60;
                                if(!ok2)
                                {
                                    std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << taller->GetText() << " child->Name(): " << taller->Name() << std::endl;
                                    ok=false;
                                }
                                else
                                    intermediateTimeCount++;
                            }
                            else
                                std::cerr << "Unable to parse the plants file: " << file << ", taller is not an element: child->Name(): " << taller->Name() << std::endl;
                        }
                        const tinyxml2::XMLElement * flowering = grow->FirstChildElement("flowering");
                        if(flowering!=NULL)
                        {
                            if(flowering->GetText()!=NULL)
                            {
                                plant.flowering_seconds=stringtouint16(flowering->GetText(),&ok2)*60;
                                if(!ok2)
                                {
                                    ok=false;
                                    std::cerr << "Unable to parse the plants file: " << file << ", sprouted is not a number: " << flowering->GetText() << " child->Name(): " << flowering->Name() << std::endl;
                                }
                                else
                                    intermediateTimeCount++;
                            }
                            else
                                std::cerr << "Unable to parse the plants file: " << file << ", flowering is not an element: child->Name(): " << flowering->Name() << std::endl;
                        }
                    }
                    else
                        std::cerr << "Unable to parse the plants file: " << file << ", grow is null: child->Name(): child->Name(): " << plantItem->Name() << std::endl;
                    if(ok)
                    {
                        bool needIntermediateTimeFix=false;
                        if(plant.flowering_seconds>=plant.fruits_seconds)
                        {
                            needIntermediateTimeFix=true;
                            if(intermediateTimeCount>=3)
                                std::cerr << "Warning when parse the plants file: " << file << ", flowering_seconds>=fruits_seconds: child->Name(): child->Name(): " << grow->Name() << std::endl;
                        }
                        if(plant.taller_seconds>=plant.flowering_seconds)
                        {
                            needIntermediateTimeFix=true;
                            if(intermediateTimeCount>=3)
                                std::cerr << "Warning when parse the plants file: " << file << ", taller_seconds>=flowering_seconds: child->Name(): " << grow->Name() << std::endl;
                        }
                        if(plant.sprouted_seconds>=plant.taller_seconds)
                        {
                            needIntermediateTimeFix=true;
                            if(intermediateTimeCount>=3)
                                std::cerr << "Warning when parse the plants file: " << file << ", sprouted_seconds>=taller_seconds: child->Name(): " << grow->Name() << std::endl;
                        }
                        if(plant.sprouted_seconds<=0)
                        {
                            needIntermediateTimeFix=true;
                            if(intermediateTimeCount>=3)
                                std::cerr << "Warning when parse the plants file: " << file << ", sprouted_seconds<=0: child->Name(): " << grow->Name() << std::endl;
                        }
                        if(needIntermediateTimeFix)
                        {
                            plant.flowering_seconds=static_cast<uint16_t>(plant.fruits_seconds*3/4);
                            plant.taller_seconds=static_cast<uint16_t>(plant.fruits_seconds*2/4);
                            plant.sprouted_seconds=static_cast<uint16_t>(plant.fruits_seconds*1/4);
                        }
                        plants[id]=plant;
                    }
                }
                else
                    std::cerr << "Unable to open the plants file: " << file << ", id number already set: child->Name(): " << plantItem->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the plants file: " << file << ", id is not a number: child->Name(): " << plantItem->Name() << std::endl;
        }
        else
            std::cerr << "Unable to open the plants file: " << file << ", have not the plant id: child->Name(): " << plantItem->Name() << std::endl;
        plantItem = plantItem->NextSiblingElement("plant");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return plants;
}
#endif
