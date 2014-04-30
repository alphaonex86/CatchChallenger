#include "BaseServerCrafting.h"
#include "MapServerCrafting.h"
#include "../VariableServer.h"
#include "../base/MapServer.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../base/SqlFunction.h"
#include "../base/GlobalServerData.h"

#include <QSqlError>
#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>

using namespace CatchChallenger;

QString BaseServerCrafting::text_dottmx=QLatin1Literal(".tmx");
QString BaseServerCrafting::text_shops=QLatin1Literal("shops");
QString BaseServerCrafting::text_shop=QLatin1Literal("shop");
QString BaseServerCrafting::text_id=QLatin1Literal("id");
QString BaseServerCrafting::text_product=QLatin1Literal("product");
QString BaseServerCrafting::text_itemId=QLatin1Literal("itemId");
QString BaseServerCrafting::text_overridePrice=QLatin1Literal("overridePrice");

void BaseServerCrafting::preload_the_plant_on_map()
{
    int plant_on_the_map=0;
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `map`,`x`,`y`,`plant`,`character`,`plant_timestamps` FROM `plant`");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT map,x,y,plant,character,plant_timestamps FROM plant");
        break;
    }
    QSqlQuery plantOnMapQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!plantOnMapQuery.exec(queryText))
        DebugClass::debugConsole(plantOnMapQuery.lastQuery()+": "+plantOnMapQuery.lastError().text());
    while(plantOnMapQuery.next())
    {
        bool ok;
        QString map=plantOnMapQuery.value(0).toString();
        if(!map.endsWith(BaseServerCrafting::text_dottmx))
            map+=BaseServerCrafting::text_dottmx;
        if(!GlobalServerData::serverPrivateVariables.map_list.contains(map))
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the map not exists: %1").arg(map));
            continue;
        }
        if(static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.value(map))->plants.size()>=255)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the map have 255 or more plant: %1").arg(map));
            continue;
        }
        const quint8 &x=plantOnMapQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the x is not a number"));
            continue;
        }
        if(x>=GlobalServerData::serverPrivateVariables.map_list.value(map)->width)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the x>%1 for the map %2: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map)->width).arg(map).arg(x));
            continue;
        }
        const quint8 &y=plantOnMapQuery.value(2).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the y is not a number"));
            continue;
        }
        if(y>=GlobalServerData::serverPrivateVariables.map_list.value(map)->height)
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because the y>%1 for the map %2: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map)->height).arg(map).arg(y));
            continue;
        }
        const quint8 &plant=plantOnMapQuery.value(3).toUInt(&ok);
        if(!ok)
            continue;
        if(!CommonDatapack::commonDatapack.plants.contains(plant))
        {
            DebugClass::debugConsole(QStringLiteral("Plant dropped to not block the player, due to missing plant into the list: %1").arg(plant));
            remove_plant_on_map(map,x,y);
            continue;
        }
        const quint32 &character=plantOnMapQuery.value(4).toUInt(&ok);
        if(!ok)
            continue;
        if(!MoveOnTheMap::isDirt(*GlobalServerData::serverPrivateVariables.map_list.value(map),x,y))
        {
            DebugClass::debugConsole(QStringLiteral("Plant ignored because is not into dirt layer: %1 (%2,%3)").arg(map).arg(x).arg(y));
            continue;
        }
        const quint64 &plant_timestamps=plantOnMapQuery.value(5).toULongLong(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Plant timestamps is not a number: %1 (%2,%3)").arg(map).arg(x).arg(y));
            continue;
        }

        //plant_timestamps
        MapServerCrafting::PlantOnMap plantOnMap;
        plantOnMap.x=x;
        plantOnMap.y=y;
        plantOnMap.plant=plant;
        plantOnMap.character=character;
        plantOnMap.mature_at=plant_timestamps+CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds;
        plantOnMap.player_owned_expire_at=plant_timestamps+CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds+60*60*24;
        static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map])->plants << plantOnMap;
        #ifdef DEBUG_MESSAGE_MAP_PLANTS
        DebugClass::debugConsole(QStringLiteral("put on the map: %1 (%2,%3) the plant: %4, owned by played id: %5, mature at: %6 (%7+%8)")
                                 .arg(map).arg(x).arg(y)
                                 .arg(plant)
                                 .arg(character)
                                 .arg(plantOnMap.mature_at).arg(plant_timestamps).arg(CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds)
                                 );
        #endif
        plant_on_the_map++;
    }

    DebugClass::debugConsole(QStringLiteral("%1 plant(s) on the map loaded").arg(plant_on_the_map));
}

void BaseServerCrafting::preload_shop()
{
    const QString &file=GlobalServerData::serverSettings.datapack_basePath+QStringLiteral(DATAPACK_BASE_PATH_SHOP)+QStringLiteral("shop.xml");
    QDomDocument domDocument;
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile shopFile(file);
        QByteArray xmlContent;
        if(!shopFile.open(QIODevice::ReadOnly))
        {
            DebugClass::debugConsole(QStringLiteral("Unable to open the shops file: %1, error: %2").arg(file).arg(shopFile.errorString()));
            return;
        }
        xmlContent=shopFile.readAll();
        shopFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            DebugClass::debugConsole(QStringLiteral("Unable to open the shops file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return;
        }
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=BaseServerCrafting::text_shops)
    {
        DebugClass::debugConsole(QStringLiteral("Unable to open the shops file: %1, \"shops\" root balise not found for the xml file").arg(file));
        return;
    }

    //load the content
    bool ok;
    QDomElement shopItem = root.firstChildElement(BaseServerCrafting::text_shop);
    while(!shopItem.isNull())
    {
        if(shopItem.isElement())
        {
            if(shopItem.hasAttribute(BaseServerCrafting::text_id))
            {
                quint32 id=shopItem.attribute(BaseServerCrafting::text_id).toUInt(&ok);
                if(ok)
                {
                    if(!GlobalServerData::serverPrivateVariables.shops.contains(id))
                    {
                        Shop shop;
                        QDomElement product = shopItem.firstChildElement(BaseServerCrafting::text_product);
                        while(!product.isNull())
                        {
                            if(product.isElement())
                            {
                                if(product.hasAttribute(BaseServerCrafting::text_itemId))
                                {
                                    quint32 itemId=product.attribute(BaseServerCrafting::text_itemId).toUInt(&ok);
                                    if(!ok)
                                        DebugClass::debugConsole(QStringLiteral("preload_shop() product attribute itemId is not a number for shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                    else
                                    {
                                        if(!CommonDatapack::commonDatapack.items.item.contains(itemId))
                                            DebugClass::debugConsole(QStringLiteral("preload_shop() product itemId in not into items list for shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                        else
                                        {
                                            quint32 price=CommonDatapack::commonDatapack.items.item.value(itemId).price;
                                            if(product.hasAttribute(BaseServerCrafting::text_overridePrice))
                                            {
                                                price=product.attribute(BaseServerCrafting::text_overridePrice).toUInt(&ok);
                                                if(!ok)
                                                    price=CommonDatapack::commonDatapack.items.item.value(itemId).price;
                                            }
                                            shop.prices << price;
                                            shop.items << itemId;
                                        }
                                    }
                                }
                                else
                                    DebugClass::debugConsole(QStringLiteral("preload_shop() material have not attribute itemId for shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                            }
                            else
                                DebugClass::debugConsole(QStringLiteral("preload_shop() material is not an element for shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                            product = product.nextSiblingElement(BaseServerCrafting::text_product);
                        }
                        GlobalServerData::serverPrivateVariables.shops[id]=shop;
                    }
                    else
                        DebugClass::debugConsole(QStringLiteral("Unable to open the shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QStringLiteral("Unable to open the shops file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
            }
            else
                DebugClass::debugConsole(QStringLiteral("Unable to open the shops file: %1, have not the shops id: child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
        }
        else
            DebugClass::debugConsole(QStringLiteral("Unable to open the shops file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
        shopItem = shopItem.nextSiblingElement(BaseServerCrafting::text_shop);
    }

    DebugClass::debugConsole(QStringLiteral("%1 shops(s) loaded").arg(GlobalServerData::serverPrivateVariables.shops.size()));
}

void BaseServerCrafting::remove_plant_on_map(const QString &map,const quint8 &x,const quint8 &y)
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("DELETE FROM `plant` WHERE `map`=\"%1\" AND `x`=%2 AND `y`=%3")
                .arg(SqlFunction::quoteSqlVariable(map))
                .arg(x)
                .arg(y);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        queryText=QStringLiteral("DELETE FROM plant WHERE map=\"%1\" AND x=%2 AND y=%3")
            .arg(SqlFunction::quoteSqlVariable(map))
            .arg(x)
            .arg(y);
        break;
    }
    QSqlQuery removePlantOnMapQuery(queryText,*GlobalServerData::serverPrivateVariables.db);
    Q_UNUSED(removePlantOnMapQuery);
}

void BaseServerCrafting::unload_the_plant_on_map()
{
}

void BaseServerCrafting::unload_shop()
{
    GlobalServerData::serverPrivateVariables.shops.clear();
}
