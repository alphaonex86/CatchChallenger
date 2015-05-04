#include "../base/BaseServer.h"
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

using namespace CatchChallenger;

void BaseServer::preload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops=loadMonsterDrop(GlobalServerData::serverSettings.datapack_basePath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("monster.xml"),CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.monsters);

    DebugClass::debugConsole(QStringLiteral("%1 monster drop(s) loaded").arg(CommonDatapack::commonDatapack.monsters.size()));
}

void BaseServer::load_sql_monsters_max_id()
{
    DebugClass::debugConsole(QStringLiteral("%1 SQL city loaded").arg(GlobalServerData::serverPrivateVariables.cityStatusList.size()));

    //start to 0 due to pre incrementation before use
    GlobalServerData::serverPrivateVariables.maxMonsterId=0;
    QString queryText;
    switch(GlobalServerData::serverPrivateVariables.db.databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id` FROM `monster` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id FROM monster ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::load_monsters_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        abort();//stop because can't do the first db access
        load_sql_monsters_warehouse_max_id();
    }
    return;
}

void BaseServer::load_monsters_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_monsters_max_id_return();
}

void BaseServer::load_monsters_max_id_return()
{
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        //not +1 because incremented before use
        const quint32 &maxMonsterId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
            continue;
        }
        else
            if(maxMonsterId>GlobalServerData::serverPrivateVariables.maxMonsterId)
                GlobalServerData::serverPrivateVariables.maxMonsterId=maxMonsterId;
    }
    load_sql_monsters_warehouse_max_id();
}

void BaseServer::load_sql_monsters_warehouse_max_id()
{
    QString queryText;
    switch(GlobalServerData::serverPrivateVariables.db.databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id` FROM `monster_warehouse` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id FROM monster_warehouse ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id FROM monster_warehouse ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::load_monsters_warehouse_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        abort();//stop because can't do the first db access
        load_sql_monsters_market_max_id();
    }
    return;
}

void BaseServer::load_monsters_warehouse_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_monsters_warehouse_max_id_return();
}

void BaseServer::load_monsters_warehouse_max_id_return()
{
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        //not +1 because incremented before use
        const quint32 &maxMonsterId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
            continue;
        }
        else
            if(maxMonsterId>GlobalServerData::serverPrivateVariables.maxMonsterId)
                GlobalServerData::serverPrivateVariables.maxMonsterId=maxMonsterId;
    }
    load_sql_monsters_market_max_id();
}

void BaseServer::load_sql_monsters_market_max_id()
{
    QString queryText;
    switch(GlobalServerData::serverPrivateVariables.db.databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id` FROM `monster_market` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id FROM monster_market ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id FROM monster_market ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::load_monsters_market_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        abort();//stop because can't do the first db access
        load_clan_max_id();
    }
    return;
}

void BaseServer::load_monsters_market_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_monsters_market_max_id_return();
}

void BaseServer::load_monsters_market_max_id_return()
{
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        //not +1 because incremented before use
        const quint32 &maxMonsterId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
            continue;
        }
        else
            if(maxMonsterId>GlobalServerData::serverPrivateVariables.maxMonsterId)
                GlobalServerData::serverPrivateVariables.maxMonsterId=maxMonsterId;
    }
    load_clan_max_id();
}

QHash<quint16,MonsterDrops> BaseServer::loadMonsterDrop(const QString &file, QHash<quint16,Item> items,const QHash<quint16,Monster> &monsters)
{
    QDomDocument domDocument;
    QMultiHash<quint16,MonsterDrops> monsterDrops;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
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
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=BaseServer::text_monsters)
    {
        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"monsters\" root balise not found for the xml file").arg(file));
        return monsterDrops;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(BaseServer::text_monster);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(BaseServer::text_id))
            {
                const quint16 &id=item.attribute(BaseServer::text_id).toUShort(&ok);
                if(!ok)
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else if(!monsters.contains(id))
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id into the monster list, skip: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    QDomElement drops = item.firstChildElement(BaseServer::text_drops);
                    if(!drops.isNull())
                    {
                        if(drops.isElement())
                        {
                            QDomElement drop = drops.firstChildElement(BaseServer::text_drop);
                            while(!drop.isNull())
                            {
                                if(drop.isElement())
                                {
                                    if(drop.hasAttribute(BaseServer::text_item))
                                    {
                                        MonsterDrops dropVar;
                                        if(drop.hasAttribute(BaseServer::text_quantity_min))
                                        {
                                            dropVar.quantity_min=drop.attribute(BaseServer::text_quantity_min).toUInt(&ok);
                                            if(!ok)
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, quantity_min is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                        else
                                            dropVar.quantity_min=1;
                                        if(ok)
                                        {
                                            if(drop.hasAttribute(BaseServer::text_quantity_max))
                                            {
                                                dropVar.quantity_max=drop.attribute(BaseServer::text_quantity_max).toUInt(&ok);
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
                                            if(drop.hasAttribute(BaseServer::text_luck))
                                            {
                                                QString luck=drop.attribute(BaseServer::text_luck);
                                                luck.remove(BaseServer::text_percent);
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
                                            if(drop.hasAttribute(BaseServer::text_item))
                                            {
                                                dropVar.item=drop.attribute(BaseServer::text_item).toUInt(&ok);
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
                                            monsterDrops.insert(id,dropVar);
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, as not item attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                }
                                else
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                drop = drop.nextSiblingElement(BaseServer::text_drop);
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
        item = item.nextSiblingElement(BaseServer::text_monster);
    }
    return monsterDrops;
}

void BaseServer::unload_monsters_drops()
{
    GlobalServerData::serverPrivateVariables.monsterDrops.clear();
}
