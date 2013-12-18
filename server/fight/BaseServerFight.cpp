#include "BaseServerFight.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/CommonMap.h"
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
    GlobalServerData::serverPrivateVariables.monsterDrops=loadMonsterDrop(GlobalServerData::serverSettings.datapack_basePath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("monster.xml"),CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.monsters);

    DebugClass::debugConsole(QStringLiteral("%1 monster drop(s) loaded").arg(CommonDatapack::commonDatapack.monsters.size()));
}

void BaseServerFight::load_monsters_max_id()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id` FROM `monster` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;");
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
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
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
                    DebugClass::debugConsole(QStringLiteral("drop grass monster for the map: %1; because the monster %2 don't exists").arg(i.key()).arg(i.value()->grassMonster.at(index).id));
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
                    DebugClass::debugConsole(QStringLiteral("drop water monster for the map: %1; because the monster %2 don't exists").arg(i.key()).arg(i.value()->waterMonster.at(index).id));
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
                    DebugClass::debugConsole(QStringLiteral("drop cave monster for the map: %1; because the monster %2 don't exists").arg(i.key()).arg(i.value()->caveMonster.at(index).id));
                    GlobalServerData::serverPrivateVariables.map_list[i.key()]->caveMonster.clear();
                    break;
                }
                index++;
            }
        }
    }

    DebugClass::debugConsole(QStringLiteral("%1 monster(s) into %2 zone(s) for %3 map(s) checked").arg(monstersChecked).arg(zoneChecked).arg(GlobalServerData::serverPrivateVariables.map_list.size()));

}

QHash<quint32,MonsterDrops> BaseServerFight::loadMonsterDrop(const QString &file, QHash<quint32,Item> items,const QHash<quint32,Monster> &monsters)
{
    QDomDocument domDocument;
    QMultiHash<quint32,MonsterDrops> monsterDrops;
    //open and quick check the file
    if(DatapackGeneralLoader::xmlLoadedFile.contains(file))
        domDocument=DatapackGeneralLoader::xmlLoadedFile[file];
    else
    {
        QFile xmlFile(file);
        QByteArray xmlContent;
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            DebugClass::debugConsole(QStringLiteral("Unable to open the xml monsters drop file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return monsterDrops;
        }
        xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return monsterDrops;
        }
        DatapackGeneralLoader::xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("list"))
    {
        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(file));
        return monsterDrops;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(QStringLiteral("monster"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QStringLiteral("id")))
            {
                quint32 id=item.attribute(QStringLiteral("id")).toUInt(&ok);
                if(!ok)
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else if(!monsters.contains(id))
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id into the monster list, skip: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    QDomElement drops = item.firstChildElement(QStringLiteral("drops"));
                    if(!drops.isNull())
                    {
                        if(drops.isElement())
                        {
                            QDomElement drop = drops.firstChildElement(QStringLiteral("drop"));
                            while(!drop.isNull())
                            {
                                if(drop.isElement())
                                {
                                    if(drop.hasAttribute(QStringLiteral("item")))
                                    {
                                        MonsterDrops dropVar;
                                        if(drop.hasAttribute(QStringLiteral("quantity_min")))
                                        {
                                            dropVar.quantity_min=drop.attribute(QStringLiteral("quantity_min")).toUInt(&ok);
                                            if(!ok)
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, quantity_min is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                        else
                                            dropVar.quantity_min=1;
                                        if(ok)
                                        {
                                            if(drop.hasAttribute(QStringLiteral("quantity_max")))
                                            {
                                                dropVar.quantity_max=drop.attribute(QStringLiteral("quantity_max")).toUInt(&ok);
                                                if(!ok)
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, quantity_max is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                dropVar.quantity_max=1;
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_min<=0)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, dropVar.quantity_min is 0: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_max<=0)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, dropVar.quantity_max is 0: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.quantity_max<dropVar.quantity_min)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, dropVar.quantity_max<dropVar.quantity_min: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(drop.hasAttribute(QStringLiteral("luck")))
                                            {
                                                QString luck=drop.attribute(QStringLiteral("luck"));
                                                luck.remove(QStringLiteral("%"));
                                                dropVar.luck=luck.toUShort(&ok);
                                                if(!ok)
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, luck is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                dropVar.luck=100;
                                        }
                                        if(ok)
                                        {
                                            if(dropVar.luck<=0)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, luck is 0!: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            if(dropVar.luck>100)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, luck is greater than 100: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(drop.hasAttribute(QStringLiteral("item")))
                                            {
                                                dropVar.item=drop.attribute(QStringLiteral("item")).toUInt(&ok);
                                                if(!ok)
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, item is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                dropVar.luck=100;
                                        }
                                        if(ok)
                                        {
                                            if(!items.contains(dropVar.item))
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, the item %4 is not into the item list: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(dropVar.item));
                                            }
                                        }
                                        if(ok)
                                        {
                                            if(CommonSettings::commonSettings.rates_drop!=1.0)
                                            {
                                                dropVar.luck=dropVar.luck*CommonSettings::commonSettings.rates_drop;
                                                float targetAverage=((float)dropVar.quantity_min+(float)dropVar.quantity_max)/2.0;
                                                targetAverage=(targetAverage*dropVar.luck)/100.0;
                                                while(dropVar.luck>100)
                                                {
                                                    dropVar.quantity_max++;
                                                    float currentAverage=((float)dropVar.quantity_min+(float)dropVar.quantity_max)/2.0;
                                                    dropVar.luck=(100.0*targetAverage)/currentAverage;
                                                }
                                            }
                                            monsterDrops.insert(id,dropVar);
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, as not item attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                }
                                else
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                drop = drop.nextSiblingElement(QStringLiteral("drop"));
                            }
                        }
                        else
                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, drops balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    }
                }
            }
            else
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(QStringLiteral("monster"));
    }
    return monsterDrops;
}

void BaseServerFight::unload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops.clear();
}
