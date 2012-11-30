#include "DatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"

#include <QDomElement>
#include <QDomDocument>
#include <QFile>
#include <QByteArray>
#include <QDebug>

DatapackClientLoader DatapackClientLoader::datapackLoader;

DatapackClientLoader::DatapackClientLoader()
{
    mDefaultInventoryImage=NULL;
    start();
}

DatapackClientLoader::~DatapackClientLoader()
{
    if(mDefaultInventoryImage==NULL)
        delete mDefaultInventoryImage;
    quit();
    wait();
}

QPixmap DatapackClientLoader::defaultInventoryImage()
{
    return *mDefaultInventoryImage;
}

void DatapackClientLoader::run()
{
    exec();
}

void DatapackClientLoader::parseDatapack(const QString &datapackPath)
{
    this->datapackPath=datapackPath;
    parseItems();
    parseMaps();
    parsePlants();

    emit datapackParsed();
}

void DatapackClientLoader::parseItems()
{
    //open and quick check the file
    QFile itemsFile(datapackPath+DATAPACK_BASE_PATH_ITEM+"items.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the items file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the items file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        qDebug() << QString("Unable to open the items file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("item");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toULongLong(&ok);
                if(ok)
                {
                    if(!DatapackClientLoader::items.contains(id))
                    {
                        //load the image
                        if(item.hasAttribute("image"))
                        {
                            QPixmap image(datapackPath+DATAPACK_BASE_PATH_ITEM+item.attribute("image"));
                            if(image.isNull())
                            {
                                qDebug() << QString("Unable to open the items image: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(datapackPath+DATAPACK_BASE_PATH_ITEM+item.attribute("image")).arg(item.tagName()).arg(item.lineNumber());
                                DatapackClientLoader::items[id].image=*mDefaultInventoryImage;
                            }
                            else
                                DatapackClientLoader::items[id].image=image;
                        }
                        else
                        {
                            qDebug() << QString("No image, load the default, id number already set: child.tagName(): %1 (at line: %2)").arg(item.tagName()).arg(item.lineNumber());
                            DatapackClientLoader::items[id].image=*mDefaultInventoryImage;
                        }
                        DatapackClientLoader::items[id].image=DatapackClientLoader::items[id].image.scaled(64,64);

                        //load the name
                        DatapackClientLoader::items[id].name=tr("Unknow object");
                        QDomElement name = item.firstChildElement("name");
                        while(!name.isNull())
                        {
                            if(name.isElement())
                            {
                                if(!name.hasAttribute("lang"))
                                {
                                    DatapackClientLoader::items[id].name=name.text();
                                    break;
                                }
                            }
                            name = name.nextSiblingElement("name");
                        }

                        //load the description
                        DatapackClientLoader::items[id].description=tr("This object is not listed as know object. The information can't be found.");
                        QDomElement description = item.firstChildElement("description");
                        while(!description.isNull())
                        {
                            if(description.isElement())
                            {
                                if(!description.hasAttribute("lang"))
                                {
                                    DatapackClientLoader::items[id].description=description.text();
                                    break;
                                }
                            }
                            description = description.nextSiblingElement("description");
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the items file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the items file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the items file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the items file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement("item");
    }

    qDebug() << QString("%1 item(s) loaded").arg(DatapackClientLoader::items.size());
}

void DatapackClientLoader::parseMaps()
{
    QStringList returnList=Pokecraft::FacilityLib::listFolder(datapackPath+DATAPACK_BASE_PATH_MAP);

    //load the map
    int size=returnList.size();
    int index=0;
    QRegExp mapFilter("\\.tmx$");
    while(index<size)
    {
        QString fileName=returnList.at(index);
        if(fileName.contains(mapFilter))
            maps << fileName;
        index++;
    }
    maps.sort();

    qDebug() << QString("%1 map(s) loaded").arg(maps.size());
}

void DatapackClientLoader::resetAll()
{
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(":/images/inventory/unknow-object.png");
    datapackPath.clear();
    items.clear();
    maps.clear();

    QHashIterator<quint8,Plant> i(plants);
     while (i.hasNext()) {
         i.next();
         delete i.value().tileset;
     }
     plants.clear();
     itemToplants.clear();
}
