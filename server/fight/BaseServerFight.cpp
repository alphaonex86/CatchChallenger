#include "../base/BaseServer.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonMap.h"
#include "../../general/fight/FightLoader.h"
#include "../../general/base/tinyXML/tinyxml.h"
#include "../base/GlobalServerData.h"
#include "../VariableServer.h"
#include "../base/DatabaseFunction.h"

#include <string>
#include <vector>

using namespace CatchChallenger;

void BaseServer::preload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops=loadMonsterDrop(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml",CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.monsters);

    std::cout << CommonDatapack::commonDatapack.monsters.size() << " monster drop(s) loaded" << std::endl;
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void BaseServer::load_sql_monsters_max_id()
{
    std::cout << GlobalServerData::serverPrivateVariables.cityStatusList.size() << " SQL city loaded" << std::endl;

    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxMonsterId=1;
    std::string queryText;
    switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id` FROM `monster` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM monster ORDER BY id DESC LIMIT 1;";
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&BaseServer::load_monsters_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        abort();//stop because can't do the first db access
        load_clan_max_id();
    }
    return;
}

void BaseServer::load_monsters_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_monsters_max_id_return();
}

void BaseServer::load_monsters_max_id_return()
{
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        bool ok;
        const uint32_t &maxMonsterId=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
        {
            std::cerr << "Max monster id is failed to convert to number" << std::endl;
            continue;
        }
        else
            if(maxMonsterId>=GlobalServerData::serverPrivateVariables.maxMonsterId)
                GlobalServerData::serverPrivateVariables.maxMonsterId=maxMonsterId+1;
    }
    load_clan_max_id();
}
#endif

std::unordered_map<uint16_t,std::vector<MonsterDrops> > BaseServer::loadMonsterDrop(const std::string &file, std::unordered_map<uint16_t,Item> items,const std::unordered_map<uint16_t,Monster> &monsters)
{
    TiXmlDocument *domDocument;
    std::unordered_map<uint16_t,std::vector<MonsterDrops> > monsterDrops;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new TiXmlDocument();
        #endif
        const bool loadOkay=domDocument->LoadFile(file);
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file.c_str() << ", Parse error at line " << domDocument->ErrorRow() << ", column " << domDocument->ErrorCol() << ": " << domDocument->ErrorDesc() << std::endl;
            return monsterDrops;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const TiXmlElement * root = domDocument->RootElement();;
    if(root->ValueStr()!=BaseServer::text_monsters)
    {
        std::cerr << "Unable to open the xml file: " << file << ", \"monsters\" root balise not found for the xml file" << std::endl;
        return monsterDrops;
    }

    //load the content
    bool ok;
    const TiXmlElement * item = root->FirstChildElement(BaseServer::text_monster);
    while(item!=NULL)
    {
        if(item->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
        {
            if(item->Attribute(BaseServer::text_id)!=NULL)
            {
                const uint16_t &id=stringtouint16(*item->Attribute(BaseServer::text_id),&ok);
                if(!ok)
                    std::cerr << "Unable to open the xml file: " << file << ", id not a number: child.tagName(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                else if(monsters.find(id)==monsters.cend())
                    std::cerr << "Unable to open the xml file: " << file << ", id into the monster list, skip: child.tagName(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                else
                {
                    const TiXmlElement * drops = item->FirstChildElement(BaseServer::text_drops);
                    if(drops!=NULL)
                    {
                        if(drops->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                        {
                            const TiXmlElement * drop = drops->FirstChildElement(BaseServer::text_drop);
                            while(drop!=NULL)
                            {
                                if(drop->Type()==TiXmlNode::NodeType::TINYXML_ELEMENT)
                                {
                                    if(drop->Attribute(BaseServer::text_item)!=NULL)
                                    {
                                        MonsterDrops dropVar;
                                        dropVar.item=0;
                                        if(drop->Attribute(BaseServer::text_quantity_min)!=NULL)
                                        {
                                            dropVar.quantity_min=stringtouint32(*drop->Attribute(BaseServer::text_quantity_min),&ok);
                                            if(!ok)
                                                std::cerr << "Unable to open the xml file: " << file << ", quantity_min is not a number: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                        }
                                        else
                                            dropVar.quantity_min=1;
                                        if(ok)
                                        {
                                            if(drop->Attribute(BaseServer::text_quantity_max)!=NULL)
                                            {
                                                dropVar.quantity_max=stringtouint32(*drop->Attribute(BaseServer::text_quantity_max),&ok);
                                                if(!ok)
                                                    std::cerr << "Unable to open the xml file: " << file << ", quantity_max is not a number: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                            }
                                            else
                                                dropVar.quantity_max=1;
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_min<=0)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_min is 0: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_max<=0)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max is 0: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_max<dropVar.quantity_min)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max<dropVar.quantity_min: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(drop->Attribute(BaseServer::text_luck)!=NULL)
                                            {
                                                std::string luck=*drop->Attribute(BaseServer::text_luck);
                                                if(!luck.empty())
                                                    if(luck.back()=='%')
                                                        luck.resize(luck.size()-1);
                                                dropVar.luck=stringtouint32(luck,&ok);
                                                if(!ok)
                                                    std::cerr << "Unable to open the xml file: " << file << ", luck is not a number: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                            }
                                            else
                                                dropVar.luck=100;
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.luck<=0)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", luck is 0!: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                            }
                                            if(dropVar.luck>100)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", luck is greater than 100: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(drop->Attribute(BaseServer::text_item)!=NULL)
                                            {
                                                dropVar.item=stringtouint32(*drop->Attribute(BaseServer::text_item),&ok);
                                                if(!ok)
                                                    std::cerr << "Unable to open the xml file: " << file << ", item is not a number: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                            }
                                            else
                                                dropVar.luck=100;
                                        }
                                        if(ok)
                                        {
                                            if(items.find(dropVar.item)==items.cend())
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", the item " << dropVar.item << " is not into the item list: child.tagName(): %2 (at line: %3)" << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(CommonSettingsServer::commonSettingsServer.rates_drop!=1.0)
                                            {
                                                dropVar.luck=dropVar.luck*CommonSettingsServer::commonSettingsServer.rates_drop;
                                                float targetAverage=((float)dropVar.quantity_min+(float)dropVar.quantity_max)/2.0;
                                                targetAverage=(targetAverage*dropVar.luck)/100.0;
                                                while(dropVar.luck>100)
                                                {
                                                    dropVar.quantity_max++;
                                                    float currentAverage=((float)dropVar.quantity_min+(float)dropVar.quantity_max)/2.0;
                                                    dropVar.luck=(100.0*targetAverage)/currentAverage;
                                                }
                                            }
                                            monsterDrops[id].push_back(dropVar);
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", as not item attribute: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", effect balise is not an element: child.tagName(): " << drop->ValueStr() << " (at line: " << drop->Row() << ")" << std::endl;
                                drop = drop->NextSiblingElement(BaseServer::text_drop);
                            }
                        }
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", drops balise is not an element: child.tagName(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
                    }
                }
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster id: child.tagName(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): " << item->ValueStr() << " (at line: " << item->Row() << ")" << std::endl;
        item = item->NextSiblingElement(BaseServer::text_monster);
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return monsterDrops;
}

void BaseServer::unload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops.clear();
}
