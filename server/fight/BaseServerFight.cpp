#include "BaseServerFight.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/Map.h"
#include "../../general/fight/FightLoader.h"
#include "../base/GlobalServerData.h"
#include "../VariableServer.h"

#include <QFile>
#include <QString>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QSqlError>

using namespace CatchChallenger;

void BaseServerFight::preload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops=loadMonsterDrop(GlobalServerData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml",CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.monsters);

    DebugClass::debugConsole(QString("%1 monster drop(s) loaded").arg(CommonDatapack::commonDatapack.monsters.size()));
}

void BaseServerFight::load_monsters_max_id()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;");
        break;
    }
    QSqlQuery maxMonsterIdQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!maxMonsterIdQuery.exec(queryText))
        DebugClass::debugConsole(maxMonsterIdQuery.lastQuery()+": "+maxMonsterIdQuery.lastError().text());
    GlobalServerData::serverPrivateVariables.maxMonsterId=0;
    while(maxMonsterIdQuery.next())
    {
        bool ok;
        GlobalServerData::serverPrivateVariables.maxMonsterId=maxMonsterIdQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("Max monster id is failed to convert to number"));
            GlobalServerData::serverPrivateVariables.maxMonsterId=0;
            continue;
        }
    }
}

void BaseServerFight::check_monsters_map()
{
    quint32 monstersChecked=0,zoneChecked=0;
    int index;

    QHashIterator<QString,Map *> i(GlobalServerData::serverPrivateVariables.map_list);
    while (i.hasNext()) {
        i.next();
        if(!i.value()->grassMonster.isEmpty())
        {
            monstersChecked+=i.value()->grassMonster.size();
            zoneChecked++;
            index=0;
            while(index<i.value()->grassMonster.size())
            {
                if(!CommonDatapack::commonDatapack.monsters.contains(i.value()->grassMonster.at(index).id))
                {
                    DebugClass::debugConsole(QString("drop grass monster for the map: %1; because the monster %2 don't exists").arg(i.key()).arg(i.value()->grassMonster.at(index).id));
                    GlobalServerData::serverPrivateVariables.map_list[i.key()]->grassMonster.clear();
                    break;
                }
                index++;
            }
        }
        if(!i.value()->waterMonster.isEmpty())
        {
            monstersChecked+=i.value()->waterMonster.size();
            zoneChecked++;
            index=0;
            while(index<i.value()->waterMonster.size())
            {
                if(!CommonDatapack::commonDatapack.monsters.contains(i.value()->waterMonster.at(index).id))
                {
                    DebugClass::debugConsole(QString("drop water monster for the map: %1; because the monster %2 don't exists").arg(i.key()).arg(i.value()->waterMonster.at(index).id));
                    GlobalServerData::serverPrivateVariables.map_list[i.key()]->waterMonster.clear();
                    break;
                }
                index++;
            }
        }
        if(!i.value()->caveMonster.isEmpty())
        {
            monstersChecked+=i.value()->caveMonster.size();
            zoneChecked++;
            index=0;
            while(index<i.value()->grassMonster.size())
            {
                if(!CommonDatapack::commonDatapack.monsters.contains(i.value()->caveMonster.at(index).id))
                {
                    DebugClass::debugConsole(QString("drop cave monster for the map: %1; because the monster %2 don't exists").arg(i.key()).arg(i.value()->caveMonster.at(index).id));
                    GlobalServerData::serverPrivateVariables.map_list[i.key()]->caveMonster.clear();
                    break;
                }
                index++;
            }
        }
    }

    DebugClass::debugConsole(QString("%1 monster(s) into %2 zone(s) for %3 map(s) checked").arg(monstersChecked).arg(zoneChecked).arg(GlobalServerData::serverPrivateVariables.map_list.size()));

}

QHash<quint32,MonsterDrops> BaseServerFight::loadMonsterDrop(const QString &file, QHash<quint32,Item> items,const QHash<quint32,Monster> &monsters)
{
    QMultiHash<quint32,MonsterDrops> monsterDrops;
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml monsters drop file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return monsterDrops;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return monsterDrops;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return monsterDrops;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("monster");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toUInt(&ok);
                if(!ok)
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else if(!monsters.contains(id))
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id into the monster list, skip: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    QDomElement drops = item.firstChildElement("drops");
                    if(!drops.isNull())
                    {
                        if(drops.isElement())
                        {
                            QDomElement drop = drops.firstChildElement("drop");
                            while(!drop.isNull())
                            {
                                if(drop.isElement())
                                {
                                    if(drop.hasAttribute("item"))
                                    {
                                        MonsterDrops dropVar;
                                        if(drop.hasAttribute("quantity_min"))
                                        {
                                            dropVar.quantity_min=drop.attribute("quantity_min").toUInt(&ok);
                                            if(!ok)
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, quantity_min is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                        else
                                            dropVar.quantity_min=1;
                                        if(ok)
                                        {
                                            if(drop.hasAttribute("quantity_max"))
                                            {
                                                dropVar.quantity_max=drop.attribute("quantity_max").toUInt(&ok);
                                                if(!ok)
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, quantity_max is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                dropVar.quantity_max=1;
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_min<=0)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, dropVar.quantity_min is 0: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_max<=0)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, dropVar.quantity_max is 0: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_max<dropVar.quantity_min)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, dropVar.quantity_max<dropVar.quantity_min: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(drop.hasAttribute("luck"))
                                            {
                                                QString luck=drop.attribute("luck");
                                                luck.remove("%");
                                                dropVar.luck=luck.toUShort(&ok);
                                                if(!ok)
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, luck is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                dropVar.luck=100;
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.luck<=0)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, luck is 0!: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            if(dropVar.luck>100)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, luck is greater than 100: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(drop.hasAttribute("item"))
                                            {
                                                dropVar.item=drop.attribute("item").toUInt(&ok);
                                                if(!ok)
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, item is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                dropVar.luck=100;
                                        }
                                        if(ok)
                                        {
                                            if(!items.contains(dropVar.item))
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, the item is not into the item list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                        }
                                        if(ok)
                                            monsterDrops.insert(id,dropVar);
                                    }
                                    else
                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, as not item attribute: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                }
                                else
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                drop = drop.nextSiblingElement("drop");
                            }
                        }
                        else
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, drops balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                }
            }
            else
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("monster");
    }
    return monsterDrops;
}

void BaseServerFight::unload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops.clear();
}
