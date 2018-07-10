#include "DatapackGeneralLoader.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsServer.h"
#include <iostream>

using namespace CatchChallenger;

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
        if(strcmp(root->Name(),"monsters")!=0)
        {
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        const tinyxml2::XMLElement * item = root->FirstChildElement("monster");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                const uint16_t &id=stringtouint16(item->Attribute("id"),&ok);
                if(!ok)
                    std::cerr << "Unable to open the xml file: " << file << ", id not a number: child.tagName(): " << item->Name() << std::endl;
                else if(monsters.find(id)==monsters.cend())
                    std::cerr << "Unable to open the xml file: " << file << ", id into the monster list, skip: child.tagName(): " << item->Name() << std::endl;
                else
                {
                    const tinyxml2::XMLElement * drops = item->FirstChildElement("drops");
                    if(drops!=NULL)
                    {
                        const tinyxml2::XMLElement * drop = drops->FirstChildElement("drop");
                        while(drop!=NULL)
                        {
                            if(drop->Attribute("item")!=NULL)
                            {
                                MonsterDrops dropVar;
                                dropVar.item=0;
                                if(drop->Attribute("quantity_min")!=NULL)
                                {
                                    dropVar.quantity_min=stringtouint32(drop->Attribute("quantity_min"),&ok);
                                    if(!ok)
                                        std::cerr << "Unable to open the xml file: " << file << ", quantity_min is not a number: child.tagName(): " << drop->Name() << std::endl;
                                }
                                else
                                    dropVar.quantity_min=1;
                                if(ok)
                                {
                                    if(drop->Attribute("quantity_max")!=NULL)
                                    {
                                        dropVar.quantity_max=stringtouint32(drop->Attribute("quantity_max"),&ok);
                                        if(!ok)
                                            std::cerr << "Unable to open the xml file: " << file << ", quantity_max is not a number: child.tagName(): " << drop->Name() << std::endl;
                                    }
                                    else
                                        dropVar.quantity_max=1;
                                }
                                if(ok)
                                {
                                    if(dropVar.quantity_min<=0)
                                    {
                                        ok=false;
                                        std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_min is 0: child.tagName(): " << drop->Name() << std::endl;
                                    }
                                }
                                if(ok)
                                {
                                    if(dropVar.quantity_max<=0)
                                    {
                                        ok=false;
                                        std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max is 0: child.tagName(): " << drop->Name() << std::endl;
                                    }
                                }
                                if(ok)
                                {
                                    if(dropVar.quantity_max<dropVar.quantity_min)
                                    {
                                        ok=false;
                                        std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max<dropVar.quantity_min: child.tagName(): " << drop->Name() << std::endl;
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
                                        dropVar.luck=stringtouint8(luck,&ok);
                                        if(!ok)
                                            std::cerr << "Unable to open the xml file: " << file << ", luck is not a number: child.tagName(): " << drop->Name() << std::endl;
                                        else if(dropVar.luck==0)
                                        {
                                            std::cerr << "Unable to open the xml file: " << file << ", luck can't be 0: child.tagName(): " << drop->Name() << std::endl;
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
                                        std::cerr << "Unable to open the xml file: " << file << ", luck is 0!: child.tagName(): " << drop->Name() << std::endl;
                                    }
                                    if(dropVar.luck>100)
                                    {
                                        ok=false;
                                        std::cerr << "Unable to open the xml file: " << file << ", luck is greater than 100: child.tagName(): " << drop->Name() << std::endl;
                                    }
                                }
                                if(ok)
                                {
                                    if(drop->Attribute("item")!=NULL)
                                    {
                                        dropVar.item=stringtouint16(drop->Attribute("item"),&ok);
                                        if(!ok)
                                            std::cerr << "Unable to open the xml file: " << file << ", item is not a number: child.tagName(): " << drop->Name() << std::endl;
                                    }
                                    else
                                        dropVar.luck=100;
                                }
                                if(ok)
                                {
                                    if(items.find(dropVar.item)==items.cend())
                                    {
                                        ok=false;
                                        std::cerr << "Unable to open the xml file: " << file << ", the item " << dropVar.item << " is not into the item list: child.tagName(): %2" << std::endl;
                                    }
                                }
                                if(ok)
                                {
                                    if(static_cast<double>(CommonSettingsServer::commonSettingsServer.rates_drop)!=1.0)
                                    {
                                        if(CommonSettingsServer::commonSettingsServer.rates_drop==0)
                                        {
                                            std::cerr << "CommonSettingsServer::commonSettingsServer.rates_drop==0 durring loading the drop, reset to 1" << std::endl;
                                            CommonSettingsServer::commonSettingsServer.rates_drop=1;
                                        }
                                        dropVar.luck=dropVar.luck*CommonSettingsServer::commonSettingsServer.rates_drop;
                                        double targetAverage=static_cast<double>(((double)dropVar.quantity_min+(double)dropVar.quantity_max)/2.0);
                                        targetAverage=static_cast<double>((targetAverage*dropVar.luck)/100.0);
                                        while(dropVar.luck>100)
                                        {
                                            dropVar.quantity_max++;
                                            double currentAverage=static_cast<double>(((double)dropVar.quantity_min+(double)dropVar.quantity_max)/2.0);
                                            dropVar.luck=static_cast<double>((100.0*targetAverage)/currentAverage);
                                        }
                                    }
                                    monsterDrops[id].push_back(dropVar);
                                }
                            }
                            else
                                std::cerr << "Unable to open the xml file: " << file << ", as not item attribute: child.tagName(): " << drop->Name() << std::endl;
                            drop = drop->NextSiblingElement("drop");
                        }
                    }
                }
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster id: child.tagName(): " << item->Name() << std::endl;
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
