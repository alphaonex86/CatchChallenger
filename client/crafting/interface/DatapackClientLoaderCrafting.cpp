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
                            if(grow.isElement())
                            {
                                QDomElement sprouted = grow.firstChildElement("sprouted");
                                if(!sprouted.isNull())
                                    if(sprouted.isElement())
                                    {
                                        plant.sprouted_seconds=sprouted.text().toULongLong(&ok2)*60;
                                        if(!ok2)
                                            ok=false;
                                    }
                                QDomElement taller = grow.firstChildElement("taller");
                                if(!taller.isNull())
                                    if(taller.isElement())
                                    {
                                        plant.taller_seconds=taller.text().toULongLong(&ok2)*60;
                                        if(ok2)
                                            ok=false;
                                    }
                                QDomElement flowering = grow.firstChildElement("flowering");
                                if(!flowering.isNull())
                                    if(flowering.isElement())
                                    {
                                        plant.flowering_seconds=flowering.text().toULongLong(&ok2)*60;
                                        if(ok2)
                                            ok=false;
                                    }
                                QDomElement fruits = grow.firstChildElement("fruits");
                                if(!fruits.isNull())
                                    if(fruits.isElement())
                                    {
                                        plant.fruits_seconds=fruits.text().toULongLong(&ok2)*60;
                                        if(ok2)
                                            ok=false;
                                    }
                            }
                        if(ok)
                        {
                            //try load the tileset
                            plant.tileset = new Tiled::Tileset("plant",22,32);
                            if(!plant.tileset->loadFromImage(QImage(datapackPath+DATAPACK_BASE_PATH_PLANTS+QString::number(id)+".png"),datapackPath+DATAPACK_BASE_PATH_PLANTS+QString::number(id)+".png"))
                                if(!plant.tileset->loadFromImage(QImage(":/images/plant/unknow_plant.png"),":/images/plant/unknow_plant.png"))
                                    qDebug() << "Unable the load the default plant tileset";
                        }
                        if(ok)
                            plants[id]=plant;
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
