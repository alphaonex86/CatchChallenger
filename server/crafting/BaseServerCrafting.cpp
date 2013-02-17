#include "BaseServerCrafting.h"
#include "MapServerCrafting.h"
#include "../VariableServer.h"
#include "../base/MapServer.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../base/SqlFunction.h"
#include "../base/GlobalServerData.h"

#include <QSqlError>
#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>

using namespace CatchChallenger;

void BaseServerCrafting::preload_the_plant()
{
    //open and quick check the file
    QFile plantsFile(GlobalServerData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_PLANTS+"plants.xml");
    QByteArray xmlContent;
    if(!plantsFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(plantsFile.fileName()).arg(plantsFile.errorString()));
        return;
    }
    xmlContent=plantsFile.readAll();
    plantsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(plantsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="plants")
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(plantsFile.fileName()));
        return;
    }

    //load the content
    bool ok,ok2;
    QDomElement plantItem = root.firstChildElement("plant");
    while(!plantItem.isNull())
    {
        if(plantItem.isElement())
        {
            if(plantItem.hasAttribute("id") && plantItem.hasAttribute("itemUsed"))
            {
                quint8 id=plantItem.attribute("id").toUShort(&ok);
                quint32 itemUsed=plantItem.attribute("itemUsed").toULongLong(&ok2);
                if(ok && ok2)
                {
                    if(!GlobalServerData::serverPrivateVariables.plants.contains(id))
                    {
                        ok=true;
                        Plant plant;
                        plant.itemUsed=itemUsed;
                        QDomElement grow = plantItem.firstChildElement("grow");
                        if(!grow.isNull())
                        {
                            if(grow.isElement())
                            {
                                QDomElement fruits = grow.firstChildElement("fruits");
                                if(!fruits.isNull())
                                {
                                    if(fruits.isElement())
                                    {
                                        plant.mature_seconds=fruits.text().toULongLong(&ok2)*60;
                                        if(!ok2)
                                        {
                                            ok=false;
                                            DebugClass::debugConsole(QString("preload_the_plant() fruits in not an number for xml file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(QString("preload_the_plant() fruit is not element for xml file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                                }
                                else
                                    DebugClass::debugConsole(QString("preload_the_plant() fruit is null for xml file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                            }
                            else
                                DebugClass::debugConsole(QString("preload_the_plant() grow is not an element for xml file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                        }
                        else
                            DebugClass::debugConsole(QString("preload_the_plant() grow is null for xml file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                        if(ok)
                        {
                            QDomElement quantity = plantItem.firstChildElement("quantity");
                            if(!quantity.isNull())
                            {
                                if(quantity.isElement())
                                {
                                    float float_quantity=quantity.text().toFloat(&ok2);
                                    int integer_part=float_quantity;
                                    float random_part=float_quantity-integer_part;
                                    random_part*=RANDOM_FLOAT_PART_DIVIDER;
                                    plant.fix_quantity=integer_part;
                                    plant.random_quantity=random_part;
                                    if(!ok2)
                                    {
                                        ok=false;
                                        DebugClass::debugConsole(QString("preload_the_plant() quantity is not a number for xml file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                                    }
                                }
                                else
                                    DebugClass::debugConsole(QString("preload_the_plant() quantity is not element for xml file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                            }
                            else
                                DebugClass::debugConsole(QString("preload_the_plant() quantity is null for xml file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                        }
                        if(ok)
                        {
                            if(!GlobalServerData::serverPrivateVariables.items.contains(plant.itemUsed))
                            {
                                ok=false;
                                DebugClass::debugConsole(QString("preload_crafting_recipes() itemUsed is not into items list for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                            }
                        }
                        if(ok)
                            GlobalServerData::serverPrivateVariables.plants[id]=plant;
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
            }
            else
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the plant id: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber()));
        plantItem = plantItem.nextSiblingElement("plant");
    }

    DebugClass::debugConsole(QString("%1 plant(s) loaded").arg(GlobalServerData::serverPrivateVariables.plants.size()));
}

void BaseServerCrafting::preload_the_plant_on_map()
{
    int plant_on_the_map=0;
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT map,x,y,plant,player_id,plant_timestamps FROM plant");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT map,x,y,plant,player_id,plant_timestamps FROM plant");
        break;
    }
    QSqlQuery plantOnMapQuery;
    if(!plantOnMapQuery.exec(queryText))
        DebugClass::debugConsole(plantOnMapQuery.lastQuery()+": "+plantOnMapQuery.lastError().text());
    if(plantOnMapQuery.isValid())
        DebugClass::debugConsole(QString("SQL query is not valid: %1").arg(queryText));
    while(plantOnMapQuery.next())
    {
        bool ok;
        QString map=plantOnMapQuery.value(0).toString();
        if(!GlobalServerData::serverPrivateVariables.map_list.contains(map))
        {
            DebugClass::debugConsole(QString("Plant ignored because the map not exists: %1").arg(map));
            continue;
        }
        if(static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map])->plants.size()>=255)
        {
            DebugClass::debugConsole(QString("Plant ignored because the map have 255 or more plant: %1").arg(map));
            continue;
        }
        quint8 x=plantOnMapQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("Plant ignored because the x is not a number"));
            continue;
        }
        if(x>=GlobalServerData::serverPrivateVariables.map_list[map]->width)
        {
            DebugClass::debugConsole(QString("Plant ignored because the x>%1 for the map %2: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map]->width).arg(map).arg(x));
            continue;
        }
        quint8 y=plantOnMapQuery.value(2).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("Plant ignored because the y is not a number"));
            continue;
        }
        if(y>=GlobalServerData::serverPrivateVariables.map_list[map]->height)
        {
            DebugClass::debugConsole(QString("Plant ignored because the y>%1 for the map %2: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map]->height).arg(map).arg(y));
            continue;
        }
        quint8 plant=plantOnMapQuery.value(3).toUInt(&ok);
        if(!ok)
            continue;
        if(!GlobalServerData::serverPrivateVariables.plants.contains(plant))
        {
            DebugClass::debugConsole(QString("Plant dropped to not block the player, due to missing plant into the list: %1").arg(plant));
            remove_plant_on_map(map,x,y);
            continue;
        }
        quint32 player_id=plantOnMapQuery.value(4).toUInt(&ok);
        if(!ok)
            continue;
        if(!MoveOnTheMap::isDirt(*GlobalServerData::serverPrivateVariables.map_list[map],x,y))
        {
            DebugClass::debugConsole(QString("Plant ignored because coor is no dirt: %1 (%2,%3)").arg(map).arg(x).arg(y));
            continue;
        }
        quint64 plant_timestamps=plantOnMapQuery.value(5).toULongLong(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("Plant timestamps is not a number: %1 (%2,%3)").arg(map).arg(x).arg(y));
            continue;
        }

        //plant_timestamps
        MapServerCrafting::PlantOnMap plantOnMap;
        plantOnMap.x=x;
        plantOnMap.y=y;
        plantOnMap.plant=plant;
        plantOnMap.player_id=player_id;
        plantOnMap.mature_at=plant_timestamps+GlobalServerData::serverPrivateVariables.plants[plant].mature_seconds;
        plantOnMap.player_owned_expire_at=plant_timestamps+GlobalServerData::serverPrivateVariables.plants[plant].mature_seconds+60*60*24;
        static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map])->plants << plantOnMap;
        #ifdef DEBUG_MESSAGE_MAP_PLANTS
        DebugClass::debugConsole(QString("put on the map: %1 (%2,%3) the plant: %4, owned by played id: %5, mature at: %6 (%7+%8)")
                                 .arg(map).arg(x).arg(y)
                                 .arg(plant)
                                 .arg(player_id)
                                 .arg(plantOnMap.mature_at).arg(plant_timestamps).arg(GlobalServerData::serverPrivateVariables.plants[plant].mature_seconds)
                                 );
        #endif
        plant_on_the_map++;
    }

    DebugClass::debugConsole(QString("%1 plant(s) on the map loaded").arg(plant_on_the_map));
}

void BaseServerCrafting::preload_crafting_recipes()
{
    //open and quick check the file
    QFile craftingRecipesFile(GlobalServerData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_CRAFTING+"recipes.xml");
    QByteArray xmlContent;
    if(!craftingRecipesFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the crafting recipe file: %1, error: %2").arg(craftingRecipesFile.fileName()).arg(craftingRecipesFile.errorString()));
        return;
    }
    xmlContent=craftingRecipesFile.readAll();
    craftingRecipesFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(craftingRecipesFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="recipes")
    {
        DebugClass::debugConsole(QString("Unable to open the crafting recipe file: %1, \"recipes\" root balise not found for the xml file").arg(craftingRecipesFile.fileName()));
        return;
    }

    //load the content
    bool ok,ok2,ok3;
    QDomElement recipeItem = root.firstChildElement("recipe");
    while(!recipeItem.isNull())
    {
        if(recipeItem.isElement())
        {
            if(recipeItem.hasAttribute("id") && recipeItem.hasAttribute("itemToLearn") && recipeItem.hasAttribute("doItemId"))
            {
                quint8 success=100;
                if(recipeItem.hasAttribute("success"))
                {
                    quint8 tempShort=recipeItem.attribute("success").toUShort(&ok);
                    if(ok)
                    {
                        if(tempShort>100)
                            DebugClass::debugConsole(QString("preload_crafting_recipes() success can't be greater than 100 for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                        else
                            success=tempShort;
                    }
                    else
                        DebugClass::debugConsole(QString("preload_crafting_recipes() success in not an number for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                }
                quint16 quantity=1;
                if(recipeItem.hasAttribute("quantity"))
                {
                    quint32 tempShort=recipeItem.attribute("quantity").toUInt(&ok);
                    if(ok)
                    {
                        if(tempShort>65535)
                            DebugClass::debugConsole(QString("preload_crafting_recipes() quantity can't be greater than 65535 for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                        else
                            quantity=tempShort;
                    }
                    else
                        DebugClass::debugConsole(QString("preload_crafting_recipes() quantity in not an number for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                }

                quint32 id=recipeItem.attribute("id").toUInt(&ok);
                quint32 itemToLearn=recipeItem.attribute("itemToLearn").toULongLong(&ok2);
                quint32 doItemId=recipeItem.attribute("doItemId").toULongLong(&ok3);
                if(ok && ok2 && ok3)
                {
                    if(!GlobalServerData::serverPrivateVariables.crafingRecipes.contains(id))
                    {
                        ok=true;
                        CrafingRecipe recipe;
                        recipe.doItemId=doItemId;
                        recipe.itemToLearn=itemToLearn;
                        recipe.quantity=quantity;
                        recipe.success=success;
                        QDomElement material = recipeItem.firstChildElement("material");
                        while(!material.isNull() && ok)
                        {
                            if(material.isElement())
                            {
                                if(material.hasAttribute("itemId"))
                                {
                                    quint32 itemId=material.attribute("itemId").toUInt(&ok2);
                                    if(!ok2)
                                    {
                                        ok=false;
                                        DebugClass::debugConsole(QString("preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                                        break;
                                    }
                                    quint16 quantity=1;
                                    if(material.hasAttribute("quantity"))
                                    {
                                        quint32 tempShort=material.attribute("quantity").toUInt(&ok2);
                                        if(ok2)
                                        {
                                            if(tempShort>65535)
                                            {
                                                ok=false;
                                                DebugClass::debugConsole(QString("preload_crafting_recipes() material quantity can't be greater than 65535 for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                                                break;
                                            }
                                            else
                                                quantity=tempShort;
                                        }
                                        else
                                        {
                                            ok=false;
                                            DebugClass::debugConsole(QString("preload_crafting_recipes() material quantity in not an number for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                                            break;
                                        }
                                    }
                                    if(!GlobalServerData::serverPrivateVariables.items.contains(itemId))
                                    {
                                        ok=false;
                                        DebugClass::debugConsole(QString("preload_crafting_recipes() material itemId in not into items list for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                                        break;
                                    }
                                    CrafingRecipe::Material newMaterial;
                                    newMaterial.itemId=itemId;
                                    newMaterial.quantity=quantity;
                                    recipe.materials << newMaterial;
                                }
                                else
                                    DebugClass::debugConsole(QString("preload_crafting_recipes() material have not attribute itemId for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                            }
                            else
                                DebugClass::debugConsole(QString("preload_crafting_recipes() material is not an element for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                            material = material.nextSiblingElement("material");
                        }
                        if(ok)
                        {
                            if(!GlobalServerData::serverPrivateVariables.items.contains(recipe.itemToLearn))
                            {
                                ok=false;
                                DebugClass::debugConsole(QString("preload_crafting_recipes() itemToLearn is not into items list for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                            }
                        }
                        if(ok)
                        {
                            if(!GlobalServerData::serverPrivateVariables.items.contains(recipe.doItemId))
                            {
                                ok=false;
                                DebugClass::debugConsole(QString("preload_crafting_recipes() doItemId is not into items list for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                            }
                        }
                        if(ok)
                        {
                            GlobalServerData::serverPrivateVariables.crafingRecipes[id]=recipe;
                            GlobalServerData::serverPrivateVariables.itemToCrafingRecipes[recipe.itemToLearn]=id;
                        }
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QString("Unable to open the crafting recipe file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
            }
            else
                DebugClass::debugConsole(QString("Unable to open the crafting recipe file: %1, have not the crafting recipe id: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the crafting recipe file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()));
        recipeItem = recipeItem.nextSiblingElement("recipe");
    }

    DebugClass::debugConsole(QString("%1 crafting recipe(s) loaded").arg(GlobalServerData::serverPrivateVariables.crafingRecipes.size()));
}

void BaseServerCrafting::preload_shop()
{
    //open and quick check the file
    QFile shopFile(GlobalServerData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_SHOP+"shop.xml");
    QByteArray xmlContent;
    if(!shopFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the shops file: %1, error: %2").arg(shopFile.fileName()).arg(shopFile.errorString()));
        return;
    }
    xmlContent=shopFile.readAll();
    shopFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the shops file: %1, Parse error at line %2, column %3: %4").arg(shopFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="shops")
    {
        DebugClass::debugConsole(QString("Unable to open the shops file: %1, \"shops\" root balise not found for the xml file").arg(shopFile.fileName()));
        return;
    }

    //load the content
    bool ok;
    QDomElement shopItem = root.firstChildElement("shop");
    while(!shopItem.isNull())
    {
        if(shopItem.isElement())
        {
            if(shopItem.hasAttribute("id"))
            {
                quint32 id=shopItem.attribute("id").toUInt(&ok);
                if(ok)
                {
                    if(!GlobalServerData::serverPrivateVariables.shops.contains(id))
                    {
                        Shop shop;
                        QDomElement product = shopItem.firstChildElement("product");
                        while(!product.isNull())
                        {
                            if(product.isElement())
                            {
                                if(product.hasAttribute("itemId"))
                                {
                                    quint32 itemId=product.attribute("itemId").toUInt(&ok);
                                    if(!ok)
                                        DebugClass::debugConsole(QString("preload_shop() product attribute itemId is not a number for shops file: %1, child.tagName(): %2 (at line: %3)").arg(shopFile.fileName()).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                    else
                                    {
                                        if(!GlobalServerData::serverPrivateVariables.items.contains(itemId))
                                            DebugClass::debugConsole(QString("preload_shop() product itemId in not into items list for shops file: %1, child.tagName(): %2 (at line: %3)").arg(shopFile.fileName()).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                        else
                                            shop.items << itemId;
                                    }
                                }
                                else
                                    DebugClass::debugConsole(QString("preload_shop() material have not attribute itemId for shops file: %1, child.tagName(): %2 (at line: %3)").arg(shopFile.fileName()).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                            }
                            else
                                DebugClass::debugConsole(QString("preload_shop() material is not an element for shops file: %1, child.tagName(): %2 (at line: %3)").arg(shopFile.fileName()).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                            product = product.nextSiblingElement("product");
                        }
                        GlobalServerData::serverPrivateVariables.shops[id]=shop;
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the shops file: %1, child.tagName(): %2 (at line: %3)").arg(shopFile.fileName()).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QString("Unable to open the shops file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(shopFile.fileName()).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
            }
            else
                DebugClass::debugConsole(QString("Unable to open the shops file: %1, have not the shops id: child.tagName(): %2 (at line: %3)").arg(shopFile.fileName()).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the shops file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(shopFile.fileName()).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
        shopItem = shopItem.nextSiblingElement("shop");
    }

    DebugClass::debugConsole(QString("%1 shops(s) loaded").arg(GlobalServerData::serverPrivateVariables.shops.size()));
}

void BaseServerCrafting::remove_plant_on_map(const QString &map,const quint8 &x,const quint8 &y)
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("DELETE FROM plant WHERE map=\"%1\" AND x=%2 AND y=%3")
                .arg(SqlFunction::quoteSqlVariable(map))
                .arg(x)
                .arg(y);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        queryText=QString("DELETE FROM plant WHERE map=\"%1\" AND x=%2 AND y=%3")
            .arg(SqlFunction::quoteSqlVariable(map))
            .arg(x)
            .arg(y);
        break;
    }
    QSqlQuery removePlantOnMapQuery(queryText);
    Q_UNUSED(removePlantOnMapQuery);
}

void BaseServerCrafting::unload_the_plant()
{
    GlobalServerData::serverPrivateVariables.plants.clear();
}

void BaseServerCrafting::unload_the_plant_on_map()
{
}

void BaseServerCrafting::unload_crafting_recipes()
{
    GlobalServerData::serverPrivateVariables.crafingRecipes.clear();
    GlobalServerData::serverPrivateVariables.itemToCrafingRecipes.clear();
}

void BaseServerCrafting::unload_shop()
{
    GlobalServerData::serverPrivateVariables.shops.clear();
}
