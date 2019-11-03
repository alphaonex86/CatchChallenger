#include "DatapackGeneralLoader.h"
#include <iostream>
#ifndef EPOLLCATCHCHALLENGERSERVER
#include "../../general/base/CommonDatapack.h"
#endif

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::vector<MonstersCollisionTemp> DatapackGeneralLoader::loadMonstersCollision(const std::string &file, const std::unordered_map<uint16_t, Item> &items,const std::vector<Event> &events)
{
    std::unordered_map<std::string,uint8_t> eventStringToId;
    std::unordered_map<std::string,std::unordered_map<std::string,uint8_t> > eventListingToId;
    {
        uint8_t index=0;
        uint8_t sub_index;
        while(index<events.size())
        {
            const Event &event=events.at(index);
            eventStringToId[event.name]=index;
            sub_index=0;
            while(sub_index<event.values.size())
            {
                eventListingToId[event.name][event.values.at(sub_index)]=sub_index;
                sub_index++;
            }
            index++;
        }
    }
    std::vector<MonstersCollisionTemp> returnVar;
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
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found 2 for reputation of the xml file" << std::endl;
        return returnVar;
    }
    if(strcmp(root->Name(),"layers")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    bool ok;
    const tinyxml2::XMLElement * monstersCollisionItem = root->FirstChildElement("monstersCollision");
    while(monstersCollisionItem!=NULL)
    {
        if(monstersCollisionItem->Attribute("type")==NULL)
            std::cerr << "Have not the attribute type, into file: " << file << std::endl;
        else if(monstersCollisionItem->Attribute("monsterType")==NULL)
            std::cerr << "Have not the attribute monsterType, into: file: " << file << std::endl;
        else
        {
            ok=true;
            MonstersCollisionTemp monstersCollision;
            if(strcmp(monstersCollisionItem->Attribute("type"),"walkOn")==0)
                monstersCollision.type=MonstersCollisionType_WalkOn;
            else if(strcmp(monstersCollisionItem->Attribute("type"),"actionOn")==0)
                monstersCollision.type=MonstersCollisionType_ActionOn;
            else
            {
                ok=false;
                std::cerr << "type is not walkOn or actionOn, into file: " << file << std::endl;
            }
            if(ok)
            {
                if(monstersCollisionItem->Attribute("layer")!=NULL)
                    monstersCollision.layer=monstersCollisionItem->Attribute("layer");
            }
            if(ok)
            {
                if(monstersCollision.layer.empty() && monstersCollision.type!=MonstersCollisionType_WalkOn)//need specific layer name to do that's
                {
                    ok=false;
                    std::cerr << "To have blocking layer by item, have specific layer name, into file: " << file << std::endl;
                }
                else
                {
                    monstersCollision.item=0;
                    if(monstersCollisionItem->Attribute("item")!=NULL)
                    {
                        monstersCollision.item=stringtouint16(monstersCollisionItem->Attribute("item"),&ok);
                        if(!ok)
                            std::cerr << "item attribute is not a number, into file: " << file << std::endl;
                        else if(items.find(monstersCollision.item)==items.cend())
                        {
                            ok=false;
                            std::cerr << "item is not into item list, into file: " << file << std::endl;
                        }
                    }
                }
            }
            if(ok)
            {
                if(monstersCollisionItem->Attribute("tile")!=NULL)
                    monstersCollision.tile=monstersCollisionItem->Attribute("tile");
            }
            if(ok)
            {
                if(monstersCollisionItem->Attribute("background")!=NULL)
                    monstersCollision.background=monstersCollisionItem->Attribute("background");
            }
            if(ok)
            {
                if(monstersCollisionItem->Attribute("monsterType")!=NULL)
                {
                    monstersCollision.defautMonsterTypeList=stringsplit(monstersCollisionItem->Attribute("monsterType"),';');
                    vectorRemoveEmpty(monstersCollision.defautMonsterTypeList);
                    vectorRemoveDuplicatesForSmallList(monstersCollision.defautMonsterTypeList);
                    monstersCollision.monsterTypeList=monstersCollision.defautMonsterTypeList;
                    //load the condition
                    const tinyxml2::XMLElement * eventItem = monstersCollisionItem->FirstChildElement("event");
                    while(eventItem!=NULL)
                    {
                        if(eventItem->Attribute("id")!=NULL && eventItem->Attribute("value")!=NULL && eventItem->Attribute("monsterType")!=NULL)
                        {
                            if(eventStringToId.find(eventItem->Attribute("id"))!=eventStringToId.cend())
                            {
                                const auto & list=eventListingToId.at(eventItem->Attribute("id"));
                                if(list.find(eventItem->Attribute("value"))!=list.cend())
                                {
                                    MonstersCollisionTemp::MonstersCollisionEvent event;
                                    event.event=eventStringToId.at(eventItem->Attribute("id"));
                                    event.event_value=eventListingToId.at(eventItem->Attribute("id")).at(eventItem->Attribute("value"));
                                    event.monsterTypeList=stringsplit(eventItem->Attribute("monsterType"),';');
                                    if(!event.monsterTypeList.empty())
                                    {
                                        monstersCollision.events.push_back(event);
                                        unsigned int index=0;
                                        while(index<event.monsterTypeList.size())
                                        {
                                            if(!vectorcontainsAtLeastOne(monstersCollision.monsterTypeList,event.monsterTypeList.at(index)))
                                                monstersCollision.monsterTypeList.push_back(event.monsterTypeList.at(index));
                                            index++;
                                        }
                                    }
                                    else
                                        std::cerr << "monsterType is empty, into file: " << file << std::endl;
                                }
                                else
                                    std::cerr << "event value not found, into file: " << file << std::endl;
                            }
                            else
                                std::cerr << "event not found, into file: " << file << std::endl;
                        }
                        else
                            std::cerr << "event have missing attribute, into file: " << file << std::endl;
                        eventItem = eventItem->NextSiblingElement("event");
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
                        std::cerr << "similar monstersCollision previously found, into file: " << file << std::endl;
                        ok=false;
                        break;
                    }
                    if(monstersCollision.type==MonstersCollisionType_WalkOn && returnVar.at(index).layer==monstersCollision.layer)
                    {
                        std::cerr << "You can't have different item for same layer in walkOn mode, into file: " << file << std::endl;
                        ok=false;
                        break;
                    }
                    index++;
                }
            }
            if(ok && !monstersCollision.monsterTypeList.empty())
            {
                if(monstersCollision.type==MonstersCollisionType_WalkOn && monstersCollision.layer.empty() && monstersCollision.item==0)
                {
                    if(returnVar.empty())
                        returnVar.push_back(monstersCollision);
                    else
                    {
                        returnVar.push_back(returnVar.back());
                        returnVar.front()=monstersCollision;
                    }
                }
                else
                    returnVar.push_back(monstersCollision);
            }
        }
        monstersCollisionItem = monstersCollisionItem->NextSiblingElement("monstersCollision");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return returnVar;
}

LayersOptions DatapackGeneralLoader::loadLayersOptions(const std::string &file)
{
    LayersOptions returnVar;
    returnVar.zoom=2;
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
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found 2 for reputation of the xml file" << std::endl;
        std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found 2 for the xml file" << std::endl;
        return returnVar;
    }
    if(strcmp(root->Name(),"layers")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"layers\" root balise not found for reputation of the xml file" << std::endl;
        std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found for the xml file" << std::endl;
        return returnVar;
    }
    if(root->Attribute("zoom")!=NULL)
    {
        bool ok;
        returnVar.zoom=stringtouint8(root->Attribute("zoom"),&ok);
        if(!ok)
        {
            returnVar.zoom=2;
            std::cerr << "Unable to open the xml file: " << file << ", zoom is not a number" << std::endl;
        }
        else
        {
            if(returnVar.zoom<1 || returnVar.zoom>4)
            {
                returnVar.zoom=2;
                std::cerr << "Unable to open the xml file: " << file << ", zoom out of range 1-4" << std::endl;
            }
        }
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return returnVar;
}

std::vector<Event> DatapackGeneralLoader::loadEvents(const std::string &file)
{
    std::vector<Event> returnVar;

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
        std::cerr << "Unable to open the file: " << file << ", \"events\" root balise not found 2 for reputation of the xml file" << std::endl;
        return returnVar;
    }
    if(strcmp(root->Name(),"events")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"events\" root balise not found for reputation of the xml file" << std::endl;
        return returnVar;
    }

    //load the content
    const tinyxml2::XMLElement * eventItem = root->FirstChildElement("event");
    while(eventItem!=NULL)
    {
        if(eventItem->Attribute("id")==NULL)
            std::cerr << "Have not the attribute id, into file: " << file << std::endl;
        else
        {
            std::string idString=eventItem->Attribute("id");
            if(idString.empty())
                std::cerr << "Have id empty, into file: " << file << std::endl;
            else
            {
                Event event;
                event.name=idString;
                const tinyxml2::XMLElement * valueItem = eventItem->FirstChildElement("value");
                while(valueItem!=NULL)
                {
                    if(valueItem->GetText()!=NULL)
                        event.values.push_back(valueItem->GetText());
                    valueItem = valueItem->NextSiblingElement("value");
                }
                if(!event.values.empty())
                    returnVar.push_back(event);
            }
        }
        eventItem = eventItem->NextSiblingElement("event");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return returnVar;
}

/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
std::unordered_map<uint16_t, Shop> DatapackGeneralLoader::preload_shop(const std::string &file, const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<uint16_t,Shop> shops;

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
            return shops;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return shops;
    }
    if(root->Name()==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", \"shops\" root balise not found 2 for reputation of the xml file" << std::endl;
        return shops;
    }
    if(strcmp(root->Name(),"shops")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"shops\" root balise not found for reputation of the xml file" << std::endl;
        return shops;
    }

    //load the content
    bool ok;
    const tinyxml2::XMLElement * shopItem = root->FirstChildElement("shop");
    while(shopItem!=NULL)
    {
        if(shopItem->Attribute("id")!=NULL)
        {
            const uint16_t id=stringtouint16(shopItem->Attribute("id"),&ok);
            if(ok)
            {
                if(shops.find(id)==shops.cend())
                {
                    Shop shop;
                    const tinyxml2::XMLElement * product = shopItem->FirstChildElement("product");
                    while(product!=NULL)
                    {
                        if(product->Attribute("itemId")!=NULL)
                        {
                            uint16_t itemId=stringtouint16(product->Attribute("itemId"),&ok);
                            if(!ok)
                                std::cerr << "preload_shop() product attribute itemId is not a number for shops file: " << file << ", child->Name(): " << shopItem->Name() << std::endl;
                            else
                            {
                                if(items.find(itemId)==items.cend())
                                    std::cerr << "preload_shop() product itemId in not into items list for shops file: " << file << ", child->Name(): " << shopItem->Name() << std::endl;
                                else
                                {
                                    uint32_t price=items.at(itemId).price;
                                    if(product->Attribute("overridePrice")!=NULL)
                                    {
                                        price=stringtouint32(product->Attribute("overridePrice"),&ok);
                                        if(!ok)
                                            price=items.at(itemId).price;
                                    }
                                    if(price==0)
                                        std::cerr << "preload_shop() item can't be into the shop with price == 0' for shops file: " << file << ", child->Name(): " << shopItem->Name() << std::endl;
                                    else
                                    {
                                        shop.prices.push_back(price);
                                        shop.items.push_back(itemId);
                                    }
                                }
                            }
                        }
                        else
                            std::cerr << "preload_shop() material have not attribute itemId for shops file: " << file << ", child->Name(): " << shopItem->Name() << std::endl;
                        product = product->NextSiblingElement("product");
                    }
                    shops[id]=shop;
                }
                else
                    std::cerr << "Unable to open the shops file: " << file << ", child->Name(): " << shopItem->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the shops file: " << file << ", id is not a number: child->Name(): " << shopItem->Name() << std::endl;
        }
        else
            std::cerr << "Unable to open the shops file: " << file << ", have not the shops id: child->Name(): " << shopItem->Name() << std::endl;
        shopItem = shopItem->NextSiblingElement("shop");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return shops;
}
#endif
