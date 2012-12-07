#include "../base/interface/DatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"

#include <QDomElement>
#include <QDomDocument>
#include <QFile>
#include <QByteArray>
#include <QDebug>

void DatapackClientLoader::parsePlants()
{
    //open and quick check the file
    QFile plantsFile(datapackPath+DATAPACK_BASE_PATH_PLANTS+"plants.xml");
    QByteArray xmlContent;
    if(!plantsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the plants file: %1, error: %2").arg(plantsFile.fileName()).arg(plantsFile.errorString());
        return;
    }
    xmlContent=plantsFile.readAll();
    plantsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the plants file: %1, Parse error at line %2, column %3: %4").arg(plantsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="plants")
    {
        qDebug() << QString("Unable to open the plants file: %1, \"plants\" root balise not found for the xml file").arg(plantsFile.fileName());
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
                quint8 id=plantItem.attribute("id").toULongLong(&ok);
                quint32 itemUsed=plantItem.attribute("itemUsed").toULongLong(&ok2);
                if(ok && ok2)
                {
                    if(!plants.contains(id))
                    {
                        ok=true;
                        Plant plant;
                        plant.itemUsed=itemUsed;
                        QDomElement grow = plantItem.firstChildElement("grow");
                        if(!grow.isNull())
                        {
                            if(grow.isElement())
                            {
                                QDomElement sprouted = grow.firstChildElement("sprouted");
                                if(!sprouted.isNull())
                                    if(sprouted.isElement())
                                    {
                                        plant.sprouted_seconds=sprouted.text().toULongLong(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(sprouted.tagName()).arg(sprouted.lineNumber()).arg(sprouted.text());
                                            ok=false;
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, sprouted is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(sprouted.tagName()).arg(sprouted.lineNumber());
                                else
                                    qDebug() << QString("Unable to parse the plants file: %1, sprouted is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(sprouted.tagName()).arg(sprouted.lineNumber());
                                QDomElement taller = grow.firstChildElement("taller");
                                if(!taller.isNull())
                                    if(taller.isElement())
                                    {
                                        plant.taller_seconds=taller.text().toULongLong(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(taller.tagName()).arg(taller.lineNumber()).arg(taller.text());
                                            ok=false;
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, taller is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(taller.tagName()).arg(taller.lineNumber());
                                else
                                    qDebug() << QString("Unable to parse the plants file: %1, taller is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(taller.tagName()).arg(taller.lineNumber());
                                QDomElement flowering = grow.firstChildElement("flowering");
                                if(!flowering.isNull())
                                    if(flowering.isElement())
                                    {
                                        plant.flowering_seconds=flowering.text().toULongLong(&ok2)*60;
                                        if(!ok2)
                                        {
                                            ok=false;
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(flowering.tagName()).arg(flowering.lineNumber()).arg(flowering.text());
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, flowering is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(flowering.tagName()).arg(flowering.lineNumber());
                                else
                                    qDebug() << QString("Unable to parse the plants file: %1, flowering is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(flowering.tagName()).arg(flowering.lineNumber());
                                QDomElement fruits = grow.firstChildElement("fruits");
                                if(!fruits.isNull())
                                    if(fruits.isElement())
                                    {
                                        plant.fruits_seconds=fruits.text().toULongLong(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(fruits.tagName()).arg(fruits.lineNumber()).arg(fruits.text());
                                            ok=false;
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, fruits is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(fruits.tagName()).arg(fruits.lineNumber());
                                else
                                    qDebug() << QString("Unable to parse the plants file: %1, fruits is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(fruits.tagName()).arg(fruits.lineNumber());

                            }
                            else
                                qDebug() << QString("Unable to parse the plants file: %1, grow is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(grow.tagName()).arg(grow.lineNumber());
                        }
                        else
                            qDebug() << QString("Unable to parse the plants file: %1, grow is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(grow.tagName()).arg(grow.lineNumber());
                        if(ok)
                        {
                            //try load the tileset
                            plant.tileset = new Tiled::Tileset("plant",16,32);
                            if(!plant.tileset->loadFromImage(QImage(datapackPath+DATAPACK_BASE_PATH_PLANTS+QString::number(id)+".png"),datapackPath+DATAPACK_BASE_PATH_PLANTS+QString::number(id)+".png"))
                                if(!plant.tileset->loadFromImage(QImage(":/images/plant/unknow_plant.png"),":/images/plant/unknow_plant.png"))
                                    qDebug() << "Unable the load the default plant tileset";
                        }
                        if(ok)
                        {
                            plants[id]=plant;
                            itemToPlants[plant.itemUsed]=id;
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the plants file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the plants file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the plants file: %1, have not the plant id: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the plants file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber());
        plantItem = plantItem.nextSiblingElement("plant");
    }

    qDebug() << QString("%1 plant(s) loaded").arg(plants.size());
}

void DatapackClientLoader::parseCraftingRecipes()
{
    //open and quick check the file
    QFile craftingRecipesFile(datapackPath+DATAPACK_BASE_PATH_CRAFTING+"recipes.xml");
    QByteArray xmlContent;
    if(!craftingRecipesFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, error: %2").arg(craftingRecipesFile.fileName()).arg(craftingRecipesFile.errorString());
        return;
    }
    xmlContent=craftingRecipesFile.readAll();
    craftingRecipesFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(craftingRecipesFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="recipes")
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, \"recipes\" root balise not found for the xml file").arg(craftingRecipesFile.fileName());
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
                            qDebug() << QString("preload_crafting_recipes() success can't be greater than 100 for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                        else
                            success=tempShort;
                    }
                    else
                        qDebug() << QString("preload_crafting_recipes() success in not an number for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }
                quint16 quantity=1;
                if(recipeItem.hasAttribute("quantity"))
                {
                    quint32 tempShort=recipeItem.attribute("quantity").toUInt(&ok);
                    if(ok)
                    {
                        if(tempShort>65535)
                            qDebug() << QString("preload_crafting_recipes() quantity can't be greater than 65535 for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                        else
                            quantity=tempShort;
                    }
                    else
                        qDebug() << QString("preload_crafting_recipes() quantity in not an number for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }

                quint32 id=recipeItem.attribute("id").toUInt(&ok);
                quint32 itemToLearn=recipeItem.attribute("itemToLearn").toULongLong(&ok2);
                quint32 doItemId=recipeItem.attribute("doItemId").toULongLong(&ok3);
                if(ok && ok2 && ok3)
                {
                    if(!crafingRecipes.contains(id))
                    {
                        ok=true;
                        Pokecraft::CrafingRecipe recipe;
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
                                        qDebug() << QString("preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
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
                                                qDebug() << QString("preload_crafting_recipes() material quantity can't be greater than 65535 for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                                break;
                                            }
                                            else
                                                quantity=tempShort;
                                        }
                                        else
                                        {
                                            ok=false;
                                            qDebug() << QString("preload_crafting_recipes() material quantity in not an number for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                            break;
                                        }
                                    }
                                    if(!items.contains(itemId))
                                    {
                                        ok=false;
                                        qDebug() << QString("preload_crafting_recipes() material itemId in not into items list for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                        break;
                                    }
                                    Pokecraft::CrafingRecipe::Material newMaterial;
                                    newMaterial.itemId=itemId;
                                    newMaterial.quantity=quantity;
                                    recipe.materials << newMaterial;
                                }
                                else
                                    qDebug() << QString("preload_crafting_recipes() material have not attribute itemId for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                            else
                                qDebug() << QString("preload_crafting_recipes() material is not an element for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            material = material.nextSiblingElement("material");
                        }
                        if(ok)
                        {
                            if(!items.contains(recipe.itemToLearn))
                            {
                                ok=false;
                                qDebug() << QString("preload_crafting_recipes() itemToLearn is not into items list for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            if(!items.contains(recipe.doItemId))
                            {
                                ok=false;
                                qDebug() << QString("preload_crafting_recipes() doItemId is not into items list for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            crafingRecipes[id]=recipe;
                            //itemToCrafingRecipes[recipe.];
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the crafting recipe file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the crafting recipe file: %1, have not the crafting recipe id: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the crafting recipe file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
        recipeItem = recipeItem.nextSiblingElement("recipe");
    }

    qDebug() << QString("%1 crafting recipe(s) loaded").arg(crafingRecipes.size());
}
