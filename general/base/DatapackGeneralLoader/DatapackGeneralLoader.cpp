#include "DatapackGeneralLoader.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/tinyXML2/customtinyxml2.hpp"

#include <iostream>

using namespace CatchChallenger;

std::vector<std::string> DatapackGeneralLoader::loadSkins(const std::string &folder)
{
    return FacilityLibGeneral::skinIdList(folder);
}

std::pair<std::vector<const tinyxml2::XMLElement *>, std::vector<Profile> > DatapackGeneralLoader::loadProfileList(const std::string &datapackPath, const std::string &file,
                                                                                  #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                                                  const std::unordered_map<uint16_t, Item> &items,
                                                                                  #endif // CATCHCHALLENGER_CLASS_MASTER
                                                                                  const std::unordered_map<uint16_t,Monster> &monsters,const std::vector<Reputation> &reputations)
{
    //DatapackClientLoader::DatapackClientLoader.skins=CatchChallenger::FacilityLibGeneral::skinIdList(datapackPath+DATAPACK_BASE_PATH_SKIN);
    CommonDatapack::commonDatapack.skins=DatapackGeneralLoader::loadSkins(datapackPath+DATAPACK_BASE_PATH_SKIN);
    std::unordered_set<std::string> idDuplicate;
    std::unordered_map<std::string,uint8_t> reputationNameToId;
    {
        uint8_t index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    std::vector<std::string> defaultforcedskinList;
    std::unordered_map<std::string,uint8_t> skinNameToId;
    {
        uint8_t index=0;
        while(index<CommonDatapack::commonDatapack.skins.size())
        {
            skinNameToId[CommonDatapack::commonDatapack.skins.at(index)]=index;
            defaultforcedskinList.push_back(CommonDatapack::commonDatapack.skins.at(index));
            index++;
        }
    }
    std::pair<std::vector<const tinyxml2::XMLElement *>, std::vector<Profile> > returnVar;
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
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return returnVar;
    }
    if(root->Name()==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found 2 for reputation of the xml file" << std::endl;
        return returnVar;
    }
    if(strcmp(root->Name(),"profile")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    bool ok;
    const tinyxml2::XMLElement * startItem = root->FirstChildElement("start");
    while(startItem!=NULL)
    {
        Profile profile;
        profile.cash=0;

        if(startItem->Attribute("id")!=NULL)
            profile.databaseId=startItem->Attribute("id");

        if(idDuplicate.find(profile.databaseId)!=idDuplicate.cend())
        {
            std::cerr << "Unable to open the xml file: " << file << ", child->Name(): " << startItem->Name() << std::endl;
            startItem = startItem->NextSiblingElement("start");
            continue;
        }

        if(!profile.databaseId.empty() && idDuplicate.find(profile.databaseId)==idDuplicate.cend())
        {
            const tinyxml2::XMLElement * forcedskin = startItem->FirstChildElement("forcedskin");

            std::vector<std::string> forcedskinList;
            if(forcedskin!=NULL && forcedskin->Attribute("value")!=NULL)
                forcedskinList=stringsplit(forcedskin->Attribute("value"),';');
            else
                forcedskinList=defaultforcedskinList;
            {
                unsigned int index=0;
                while(index<forcedskinList.size())
                {
                    if(skinNameToId.find(forcedskinList.at(index))!=skinNameToId.cend())
                        profile.forcedskin.push_back(skinNameToId.at(forcedskinList.at(index)));
                    else
                        std::cerr << "Unable to open the xml file: " << file << ", skin " << forcedskinList.at(index) << " don't exists: child->Name(): " << startItem->Name() << std::endl;
                    index++;
                }
            }
            unsigned int index=0;
            while(index<profile.forcedskin.size())
            {
                if(!CatchChallenger::FacilityLibGeneral::isDir(datapackPath+DATAPACK_BASE_PATH_SKIN+CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index))))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", skin value: " << forcedskinList.at(index) << " don't exists into: into " << datapackPath << DATAPACK_BASE_PATH_SKIN << CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index)) << ": child->Name(): " << startItem->Name() << std::endl;
                    profile.forcedskin.erase(profile.forcedskin.begin()+index);
                }
                else
                    index++;
            }

            profile.cash=0;
            const tinyxml2::XMLElement * cash = startItem->FirstChildElement("cash");
            if(cash!=NULL && cash->Attribute("value")!=NULL)
            {
                profile.cash=stringtodouble(cash->Attribute("value"),&ok);
                if(!ok)
                {
                    std::cerr << "Unable to open the xml file: " << file << ", cash is not a number: child->Name(): " << startItem->Name() << std::endl;
                    profile.cash=0;
                }
            }
            const tinyxml2::XMLElement * monstersElementGroup = startItem->FirstChildElement("monstergroup");
            while(monstersElementGroup!=NULL)
            {
                std::vector<Profile::Monster> monsters_list;
                const tinyxml2::XMLElement * monstersElement = monstersElementGroup->FirstChildElement("monster");
                while(monstersElement!=NULL)
                {
                    Profile::Monster monster;
                    if(monstersElement->Attribute("id")!=NULL && monstersElement->Attribute("level")!=NULL &&
                            monstersElement->Attribute("captured_with")!=NULL)
                    {
                        monster.id=stringtouint16(monstersElement->Attribute("id"),&ok);
                        if(!ok)
                            std::cerr << "Unable to open the xml file: " << file << ", monster id is not a number: child->Name(): " << startItem->Name() << std::endl;
                        if(ok)
                        {
                            monster.level=stringtouint8(monstersElement->Attribute("level"),&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", monster level is not a number: child->Name(): " << startItem->Name() << std::endl;
                        }
                        if(ok)
                        {
                            if(monster.level==0 || monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                                std::cerr << "Unable to open the xml file: " << file << ", monster level is not into the range: child->Name(): " << startItem->Name() << std::endl;
                        }
                        if(ok)
                        {
                            monster.captured_with=stringtouint16(monstersElement->Attribute("captured_with"),&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", captured_with is not a number: child->Name(): " << startItem->Name() << std::endl;
                        }
                        if(ok)
                        {
                            if(monsters.find(monster.id)==monsters.cend())
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", starter don't found the monster " << monster.id << ": child->Name(): " << startItem->Name() << std::endl;
                                ok=false;
                            }
                        }
                        #ifndef CATCHCHALLENGER_CLASS_MASTER
                        if(ok)
                        {
                            if(items.find(monster.captured_with)==items.cend())
                                std::cerr << "Unable to open the xml file: " << file << ", starter don't found the monster capture item " << monster.id << ": child->Name(): " << startItem->Name() << std::endl;
                        }
                        #endif // CATCHCHALLENGER_CLASS_MASTER
                        if(ok)
                            monsters_list.push_back(monster);
                    }
                    else
                        std::cerr << "Unable to open the xml file: " << file << ", no monster attribute to load: child->Name(): " << monstersElement->Name() << std::endl;
                    monstersElement = monstersElement->NextSiblingElement("monster");
                }
                if(monsters_list.empty())
                {
                    if(monstersElement==nullptr)
                        std::cerr << "Unable to open the xml file: " << file << ", no monster to load: child->Name(): monstersElement==nullptr" << std::endl;
                    else
                        std::cerr << "Unable to open the xml file: " << file << ", no monster to load: child->Name(): " << monstersElement->Name() << std::endl;
                    startItem = startItem->NextSiblingElement("start");
                    continue;
                }
                profile.monstergroup.push_back(monsters_list);
                monstersElementGroup = monstersElementGroup->NextSiblingElement("monstergroup");
            }
            if(profile.monstergroup.empty())
            {
                std::cerr << "Unable to open the xml file: " << file << ", no monstergroup to load: child->Name(): " << startItem->Name() << std::endl;
                startItem = startItem->NextSiblingElement("start");
                continue;
            }
            const tinyxml2::XMLElement * reputationElement = startItem->FirstChildElement("reputation");
            while(reputationElement!=NULL)
            {
                Profile::Reputation reputationTemp;
                reputationTemp.internalIndex=255;
                reputationTemp.level=-1;
                reputationTemp.point=0xFFFFFFFF;
                if(reputationElement->Attribute("type")!=NULL && reputationElement->Attribute("level")!=NULL)
                {
                    reputationTemp.level=stringtoint8(reputationElement->Attribute("level"),&ok);
                    if(!ok)
                        std::cerr << "Unable to open the xml file: " << file << ", reputation level is not a number: child->Name(): " << startItem->Name() << std::endl;
                    if(ok)
                    {
                        if(reputationNameToId.find(reputationElement->Attribute("type"))==reputationNameToId.cend())
                        {
                            std::cerr << "Unable to open the xml file: " << file << ", reputation type not found " << reputationElement->Attribute("type") << ": child->Name(): " << startItem->Name() << std::endl;
                            ok=false;
                        }
                        if(ok)
                        {
                            reputationTemp.internalIndex=reputationNameToId.at(reputationElement->Attribute("type"));
                            if(reputationTemp.level==0)
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", reputation level is useless if level 0: child->Name(): " << startItem->Name() << std::endl;
                                ok=false;
                            }
                            else if(reputationTemp.level<0)
                            {
                                if((-reputationTemp.level)>(int32_t)reputations.at(reputationTemp.internalIndex).reputation_negative.size())
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", reputation level is lower than minimal level for "
                                              << reputationElement->Attribute("type") << ": child->Name(): " << startItem->Name() << std::endl;
                                    ok=false;
                                }
                            }
                            else// if(reputationTemp.level>0)
                            {
                                if((reputationTemp.level)>=(int32_t)reputations.at(reputationTemp.internalIndex).reputation_positive.size())
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", reputation level is higther than maximal level for "
                                              << reputationElement->Attribute("type") << ": child->Name(): " << startItem->Name() << std::endl;
                                    ok=false;
                                }
                            }
                        }
                        if(ok)
                        {
                            reputationTemp.point=0;
                            if(reputationElement->Attribute("point")!=NULL)
                            {
                                reputationTemp.point=stringtoint32(reputationElement->Attribute("point"),&ok);
                                std::cerr << "Unable to open the xml file: " << file << ", reputation point is not a number: child->Name(): " << startItem->Name() << std::endl;
                                if(ok)
                                {
                                    if((reputationTemp.point>0 && reputationTemp.level<0) || (reputationTemp.point<0 && reputationTemp.level>=0))
                                        std::cerr << "Unable to open the xml file: " << file << ", reputation point is not negative/positive like the level: child->Name(): " << startItem->Name() << std::endl;
                                }
                            }
                        }
                    }
                    if(ok)
                        profile.reputations.push_back(reputationTemp);
                }
                reputationElement = reputationElement->NextSiblingElement("reputation");
            }
            const tinyxml2::XMLElement * itemElement = startItem->FirstChildElement("item");
            while(itemElement!=NULL)
            {
                Profile::Item itemTemp;
                if(itemElement->Attribute("id")!=NULL)
                {
                    itemTemp.id=stringtouint16(itemElement->Attribute("id"),&ok);
                    if(!ok)
                        std::cerr << "Unable to open the xml file: " << file << ", item id is not a number: child->Name(): " << startItem->Name() << std::endl;
                    if(ok)
                    {
                        itemTemp.quantity=0;
                        if(itemElement->Attribute("quantity")!=NULL)
                        {
                            itemTemp.quantity=stringtouint32(itemElement->Attribute("quantity"),&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", item quantity is not a number: child->Name(): " << startItem->Name() << std::endl;
                            if(ok)
                            {
                                if(itemTemp.quantity==0)
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", item quantity is null: child->Name(): " << startItem->Name() << std::endl;
                                    ok=false;
                                }
                            }
                            #ifndef CATCHCHALLENGER_CLASS_MASTER
                            if(ok)
                            {
                                if(items.find(itemTemp.id)==items.cend())
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", item not found as know item " << itemTemp.id << ": child->Name(): " << startItem->Name() << std::endl;
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
        startItem = startItem->NextSiblingElement("start");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return returnVar;
}

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
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return serverProfileList;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return serverProfileList;
    }
    if(root->Name()==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found 2 for reputation of the xml file" << std::endl;
        return serverProfileList;
    }
    if(strcmp(root->Name(),"profile")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"profile\" root balise not found for reputation of the xml file" << std::endl;
        return serverProfileList;
    }

    //load the content
    bool ok;
    const tinyxml2::XMLElement * startItem = root->FirstChildElement("start");
    while(startItem!=NULL)
    {
        ServerSpecProfile serverProfile;
        serverProfile.orientation=Orientation_bottom;

        const tinyxml2::XMLElement * map = startItem->FirstChildElement("map");
        if(map!=NULL && map->Attribute("file")!=NULL && map->Attribute("x")!=NULL && map->Attribute("y")!=NULL)
        {
            serverProfile.mapString=map->Attribute("file");
            if(!stringEndsWith(serverProfile.mapString,".tmx"))
                serverProfile.mapString+=".tmx";
            if(!CatchChallenger::FacilityLibGeneral::isFile(datapackPath+DATAPACK_BASE_PATH_MAPMAIN+mainDatapackCode+'/'+serverProfile.mapString))
            {
                std::cerr << "Unable to open the xml file: " << file << ", map don't exists " << serverProfile.mapString << ": child->Name(): " << startItem->Name() << std::endl;
                {
                    std::cerr << "Into the starter the map \"" << serverProfile.mapString << "\" is not found, fix it (abort)" << std::endl;
                    abort();
                }
                startItem = startItem->NextSiblingElement("start");
                continue;
            }
            serverProfile.x=stringtouint8(map->Attribute("x"),&ok);
            if(!ok)
            {
                std::cerr << "Unable to open the xml file: " << file << ", map x is not a number: child->Name(): " << startItem->Name() << std::endl;
                startItem = startItem->NextSiblingElement("start");
                continue;
            }
            serverProfile.y=stringtouint8(map->Attribute("y"),&ok);
            if(!ok)
            {
                std::cerr << "Unable to open the xml file: " << file << ", map y is not a number: child->Name(): " << startItem->Name() << std::endl;
                startItem = startItem->NextSiblingElement("start");
                continue;
            }
        }
        else
        {
            std::cerr << "Unable to open the xml file: " << file << ", no correct map configuration: child->Name(): " << startItem->Name() << std::endl;
            startItem = startItem->NextSiblingElement("start");
            continue;
        }

        if(startItem->Attribute("id")!=NULL)
            serverProfile.databaseId=startItem->Attribute("id");

        if(idDuplicate.find(serverProfile.databaseId)!=idDuplicate.cend())
        {
            std::cerr << "Unable to open the xml file: " << file << ", id duplicate: child->Name(): " << startItem->Name() << std::endl;
            startItem = startItem->NextSiblingElement("start");
            continue;
        }

        if(!serverProfile.databaseId.empty() && idDuplicate.find(serverProfile.databaseId)==idDuplicate.cend())
        {
            idDuplicate.insert(serverProfile.databaseId);
            serverProfileList.push_back(serverProfile);
        }
        startItem = startItem->NextSiblingElement("start");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return serverProfileList;
}
