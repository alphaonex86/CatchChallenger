#include "../base/BaseServer.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonMap.h"
#include "../../general/fight/FightLoader.h"
#ifdef CATCHCHALLENGER_XLMPARSER_TINYXML1
#include "../../general/base/tinyXML/tinyxml.h"
#elif defined(CATCHCHALLENGER_XLMPARSER_TINYXML2)
#include "../../general/base/tinyXML2/tinyxml2.h"
#endif
#include "../base/GlobalServerData.h"
#include "../VariableServer.h"
#include "../base/DatabaseFunction.h"

#include <string>
#include <vector>

using namespace CatchChallenger;

void BaseServer::preload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops=loadMonsterDrop(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS,CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.monsters);

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

std::unordered_map<uint16_t,std::vector<MonsterDrops> > BaseServer::loadMonsterDrop(const std::string &folder, std::unordered_map<uint16_t,Item> items,const std::unordered_map<uint16_t,Monster> &monsters)
{
    std::unordered_map<uint16_t,std::vector<MonsterDrops> > monsterDrops;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &fileList=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    unsigned int file_index=0;
    while(file_index<(uint32_t)fileList.size())
    {
        const std::string &file=fileList.at(file_index).absoluteFilePath;
        if(!stringEndsWith(file,CACHEDSTRING_dotxml))
        {
            file_index++;
            continue;
        }

        CATCHCHALLENGER_XMLDOCUMENT *domDocument;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        //open and quick check the file
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        else
        {
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
            #else
            domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
            #endif
            const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
            if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
            {
                std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
        }
        #endif
        const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
        if(root==NULL)
        {
            file_index++;
            continue;
        }
        if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),XMLCACHEDSTRING_monsters))
        {
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        const CATCHCHALLENGER_XMLELEMENT * item = root->FirstChildElement(XMLCACHEDSTRING_monster);
        while(item!=NULL)
        {
            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(item))
            {
                if(item->Attribute(XMLCACHEDSTRING_id)!=NULL)
                {
                    const uint16_t &id=stringtouint16(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(item->Attribute(XMLCACHEDSTRING_id)),&ok);
                    if(!ok)
                        std::cerr << "Unable to open the xml file: " << file << ", id not a number: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                    else if(monsters.find(id)==monsters.cend())
                        std::cerr << "Unable to open the xml file: " << file << ", id into the monster list, skip: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                    else
                    {
                        const CATCHCHALLENGER_XMLELEMENT * drops = item->FirstChildElement(XMLCACHEDSTRING_drops);
                        if(drops!=NULL)
                        {
                            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(drops))
                            {
                                const CATCHCHALLENGER_XMLELEMENT * drop = drops->FirstChildElement(XMLCACHEDSTRING_drop);
                                while(drop!=NULL)
                                {
                                    if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(drop))
                                    {
                                        if(drop->Attribute(XMLCACHEDSTRING_item)!=NULL)
                                        {
                                            MonsterDrops dropVar;
                                            dropVar.item=0;
                                            if(drop->Attribute(XMLCACHEDSTRING_quantity_min)!=NULL)
                                            {
                                                dropVar.quantity_min=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(drop->Attribute(XMLCACHEDSTRING_quantity_min)),&ok);
                                                if(!ok)
                                                    std::cerr << "Unable to open the xml file: " << file << ", quantity_min is not a number: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                            }
                                            else
                                                dropVar.quantity_min=1;
                                            if(ok)
                                            {
                                                if(drop->Attribute(XMLCACHEDSTRING_quantity_max)!=NULL)
                                                {
                                                    dropVar.quantity_max=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(drop->Attribute(XMLCACHEDSTRING_quantity_max)),&ok);
                                                    if(!ok)
                                                        std::cerr << "Unable to open the xml file: " << file << ", quantity_max is not a number: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                                else
                                                    dropVar.quantity_max=1;
                                            }
                                            if(ok)
                                            {
                                                if(dropVar.quantity_min<=0)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_min is 0: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(dropVar.quantity_max<=0)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max is 0: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(dropVar.quantity_max<dropVar.quantity_min)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max<dropVar.quantity_min: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(drop->Attribute(XMLCACHEDSTRING_luck)!=NULL)
                                                {
                                                    std::string luck=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(drop->Attribute(XMLCACHEDSTRING_luck));
                                                    if(!luck.empty())
                                                        if(luck.back()=='%')
                                                            luck.resize(luck.size()-1);
                                                    dropVar.luck=stringtouint32(luck,&ok);
                                                    if(!ok)
                                                        std::cerr << "Unable to open the xml file: " << file << ", luck is not a number: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                                else
                                                    dropVar.luck=100;
                                            }
                                            if(ok)
                                            {
                                                if(dropVar.luck<=0)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", luck is 0!: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                                if(dropVar.luck>100)
                                                {
                                                    ok=false;
                                                    std::cerr << "Unable to open the xml file: " << file << ", luck is greater than 100: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(drop->Attribute(XMLCACHEDSTRING_item)!=NULL)
                                                {
                                                    dropVar.item=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(drop->Attribute(XMLCACHEDSTRING_item)),&ok);
                                                    if(!ok)
                                                        std::cerr << "Unable to open the xml file: " << file << ", item is not a number: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
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
                                            std::cerr << "Unable to open the xml file: " << file << ", as not item attribute: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", effect balise is not an element: child.tagName(): " << drop->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(drop) << ")" << std::endl;
                                    drop = drop->NextSiblingElement(XMLCACHEDSTRING_drop);
                                }
                            }
                            else
                                std::cerr << "Unable to open the xml file: " << file << ", drops balise is not an element: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
                        }
                    }
                }
                else
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster id: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): " << item->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(item) << ")" << std::endl;
            item = item->NextSiblingElement(XMLCACHEDSTRING_monster);
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        file_index++;
    }
    return monsterDrops;
}

void BaseServer::unload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops.clear();
}
