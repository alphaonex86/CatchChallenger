#include "../base/BaseServer.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonMap.h"
#include "../../general/fight/FightLoader.h"
#include "../base/GlobalServerData.h"
#include "../VariableServer.h"

#include <QFile>
#include <string>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>

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
        const uint32_t &maxMonsterId=stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
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
    QDomDocument domDocument;
    std::unordered_map<uint16_t,std::vector<MonsterDrops> > monsterDrops;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile xmlFile(QString::fromStdString(file));
        QByteArray xmlContent;
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the xml monsters drop file: " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
            return monsterDrops;
        }
        xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the xml file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return monsterDrops;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName().toStdString()!=BaseServer::text_monsters)
    {
        std::cerr << "Unable to open the xml file: " << file << ", \"monsters\" root balise not found for the xml file" << std::endl;
        return monsterDrops;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(QString::fromStdString(BaseServer::text_monster));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QString::fromStdString(BaseServer::text_id)))
            {
                const uint16_t &id=item.attribute(QString::fromStdString(BaseServer::text_id)).toUShort(&ok);
                if(!ok)
                    std::cerr << "Unable to open the xml file: " << file << ", id not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                else if(monsters.find(id)==monsters.cend())
                    std::cerr << "Unable to open the xml file: " << file << ", id into the monster list, skip: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                else
                {
                    QDomElement drops = item.firstChildElement(QString::fromStdString(BaseServer::text_drops));
                    if(!drops.isNull())
                    {
                        if(drops.isElement())
                        {
                            QDomElement drop = drops.firstChildElement(QString::fromStdString(BaseServer::text_drop));
                            while(!drop.isNull())
                            {
                                if(drop.isElement())
                                {
                                    if(drop.hasAttribute(QString::fromStdString(BaseServer::text_item)))
                                    {
                                        MonsterDrops dropVar;
                                        if(drop.hasAttribute(QString::fromStdString(BaseServer::text_quantity_min)))
                                        {
                                            dropVar.quantity_min=drop.attribute(QString::fromStdString(BaseServer::text_quantity_min)).toUInt(&ok);
                                            if(!ok)
                                                std::cerr << "Unable to open the xml file: " << file << ", quantity_min is not a number: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                        }
                                        else
                                            dropVar.quantity_min=1;
                                        if(ok)
                                        {
                                            if(drop.hasAttribute(QString::fromStdString(BaseServer::text_quantity_max)))
                                            {
                                                dropVar.quantity_max=drop.attribute(QString::fromStdString(BaseServer::text_quantity_max)).toUInt(&ok);
                                                if(!ok)
                                                    std::cerr << "Unable to open the xml file: " << file << ", quantity_max is not a number: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                            }
                                            else
                                                dropVar.quantity_max=1;
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_min<=0)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_min is 0: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_max<=0)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max is 0: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_max<dropVar.quantity_min)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", dropVar.quantity_max<dropVar.quantity_min: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(drop.hasAttribute(QString::fromStdString(BaseServer::text_luck)))
                                            {
                                                std::string luck=drop.attribute(QString::fromStdString(BaseServer::text_luck)).toStdString();
                                                if(!luck.empty())
                                                    if(luck.back()=='%')
                                                        luck.resize(luck.size()-1);
                                                dropVar.luck=stringtouint32(luck,&ok);
                                                if(!ok)
                                                    std::cerr << "Unable to open the xml file: " << file << ", luck is not a number: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                            }
                                            else
                                                dropVar.luck=100;
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.luck<=0)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", luck is 0!: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                            }
                                            if(dropVar.luck>100)
                                            {
                                                ok=false;
                                                std::cerr << "Unable to open the xml file: " << file << ", luck is greater than 100: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(drop.hasAttribute(QString::fromStdString(BaseServer::text_item)))
                                            {
                                                dropVar.item=drop.attribute(QString::fromStdString(BaseServer::text_item)).toUInt(&ok);
                                                if(!ok)
                                                    std::cerr << "Unable to open the xml file: " << file << ", item is not a number: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
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
                                        std::cerr << "Unable to open the xml file: " << file << ", as not item attribute: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", effect balise is not an element: child.tagName(): " << drop.tagName().toStdString() << " (at line: " << drop.lineNumber() << ")" << std::endl;
                                drop = drop.nextSiblingElement(QString::fromStdString(BaseServer::text_drop));
                            }
                        }
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", drops balise is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    }
                }
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", have not the monster id: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
        }
        else
            std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
        item = item.nextSiblingElement(QString::fromStdString(BaseServer::text_monster));
    }
    return monsterDrops;
}

void BaseServer::unload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops.clear();
}
