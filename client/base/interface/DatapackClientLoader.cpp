#include "../interface/DatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../LanguagesSelect.h"
#include "../../tiled/tiled_tileset.h"
#include "../../tiled/tiled_mapreader.h"

#include <QDebug>
#include <QFile>
#include <QDomElement>
#include <QDomDocument>
#include <QByteArray>
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>

DatapackClientLoader DatapackClientLoader::datapackLoader;

DatapackClientLoader::DatapackClientLoader()
{
    mDefaultInventoryImage=NULL;
    inProgress=false;
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

bool DatapackClientLoader::isParsingDatapack()
{
    return inProgress;
}

void DatapackClientLoader::parseDatapack(const QString &datapackPath)
{
    if(inProgress)
    {
        qDebug() << QStringLiteral("already in progress");
        return;
    }
    inProgress=true;
    this->datapackPath=datapackPath;
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/images/inventory/unknow-object.png"));
    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapackPath);
    language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    parseTypesExtra();
    parseItemsExtra();
    parseMaps();
    parseSkins();
    parseMonstersExtra();
    parseBuffExtra();
    parseSkillsExtra();
    parseQuestsLink();
    parseQuestsExtra();
    parseQuestsText();
    parseBotFightsExtra();
    parsePlantsExtra();
    parseAudioAmbiance();
    parseZoneExtra();
    parseTileset();
    parseReputationExtra();
    inProgress=false;
    emit datapackParsed();
}

QString DatapackClientLoader::getDatapackPath()
{
    return datapackPath;
}

void DatapackClientLoader::parseReputationExtra()
{
    QDomDocument domDocument;
    //open and quick check the file
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLAYER)+QStringLiteral("reputation.xml");
    if(CatchChallenger::DatapackGeneralLoader::xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::DatapackGeneralLoader::xmlLoadedFile.value(file);
    else
    {
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString()));
            return;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return;
        }
        qDebug() << (QStringLiteral("Xml not already loaded: %1").arg(file));
        CatchChallenger::DatapackGeneralLoader::xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("list"))
    {
        qDebug() << (QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file));
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(QStringLiteral("reputation"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QStringLiteral("type")))
            {
                ok=true;

                //load the name
                {
                    bool name_found=false;
                    QDomElement name = item.firstChildElement(QStringLiteral("name"));
                    if(!language.isEmpty() && language!=QStringLiteral("en"))
                        while(!name.isNull())
                        {
                            if(name.isElement())
                            {
                                if(name.hasAttribute(QStringLiteral("lang")) && name.attribute(QStringLiteral("lang"))==language)
                                {
                                    reputationExtra[item.attribute(QStringLiteral("type"))].name=name.text();
                                    name_found=true;
                                    break;
                                }
                            }
                            name = name.nextSiblingElement(QStringLiteral("name"));
                        }
                    if(!name_found)
                    {
                        name = item.firstChildElement(QStringLiteral("name"));
                        while(!name.isNull())
                        {
                            if(name.isElement())
                            {
                                if(!name.hasAttribute(QStringLiteral("lang")) || name.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                {
                                    reputationExtra[item.attribute(QStringLiteral("type"))].name=name.text();
                                    name_found=true;
                                    break;
                                }
                            }
                            name = name.nextSiblingElement(QStringLiteral("name"));
                        }
                    }
                    if(!name_found)
                    {
                        reputationExtra[item.attribute(QStringLiteral("type"))].name=tr("Unknown");
                        qDebug() << QStringLiteral("English name not found for the reputation with id: %1").arg(item.attribute(QStringLiteral("type")));
                    }
                }

                QList<qint32> point_list_positive,point_list_negative;
                QStringList text_positive,text_negative;
                QDomElement level = item.firstChildElement(QStringLiteral("level"));
                ok=true;
                while(!level.isNull() && ok)
                {
                    if(level.isElement())
                    {
                        if(level.hasAttribute(QStringLiteral("point")))
                        {
                            qint32 point=level.attribute(QStringLiteral("point")).toInt(&ok);
                            //QString text_val;
                            if(ok)
                            {
                                ok=true;

                                QString text;
                                //load the name
                                {
                                    bool name_found=false;
                                    QDomElement name = level.firstChildElement(QStringLiteral("text"));
                                    if(!language.isEmpty() && language!=QStringLiteral("en"))
                                        while(!name.isNull())
                                        {
                                            if(name.isElement())
                                            {
                                                if(name.hasAttribute(QStringLiteral("lang")) && name.attribute(QStringLiteral("lang"))==language)
                                                {
                                                    text=name.text();
                                                    name_found=true;
                                                    break;
                                                }
                                            }
                                            name = name.nextSiblingElement(QStringLiteral("text"));
                                        }
                                    if(!name_found)
                                    {
                                        name = level.firstChildElement(QStringLiteral("text"));
                                        while(!name.isNull())
                                        {
                                            if(name.isElement())
                                            {
                                                if(!name.hasAttribute(QStringLiteral("lang")) || name.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                                {
                                                    text=name.text();
                                                    name_found=true;
                                                    break;
                                                }
                                            }
                                            name = name.nextSiblingElement(QStringLiteral("text"));
                                        }
                                    }
                                    if(!name_found)
                                    {
                                        text=tr("Unknown");
                                        qDebug() << QStringLiteral("English name not found for the reputation with id: %1").arg(item.attribute("type"));
                                    }
                                }

                                bool found=false;
                                int index=0;
                                if(point>=0)
                                {
                                    while(index<point_list_positive.size())
                                    {
                                        if(point_list_positive.at(index)==point)
                                        {
                                            qDebug() << (QStringLiteral("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            found=true;
                                            ok=false;
                                            break;
                                        }
                                        if(point_list_positive.at(index)>point)
                                        {
                                            point_list_positive.insert(index,point);
                                            text_positive.insert(index,text);
                                            found=true;
                                            break;
                                        }
                                        index++;
                                    }
                                    if(!found)
                                    {
                                        point_list_positive << point;
                                        text_positive << text;
                                    }
                                }
                                else
                                {
                                    while(index<point_list_negative.size())
                                    {
                                        if(point_list_negative.at(index)==point)
                                        {
                                            qDebug() << (QStringLiteral("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            found=true;
                                            ok=false;
                                            break;
                                        }
                                        if(point_list_negative.at(index)<point)
                                        {
                                            point_list_negative.insert(index,point);
                                            text_negative.insert(index,text);
                                            found=true;
                                            break;
                                        }
                                        index++;
                                    }
                                    if(!found)
                                    {
                                        point_list_negative << point;
                                        text_negative << text;
                                    }
                                }
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the file: %1, point is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                    }
                    else
                        qDebug() << (QStringLiteral("Unable to open the file: %1, point attribute not found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    level = level.nextSiblingElement(QStringLiteral("level"));
                }
                if(ok)
                    if(point_list_positive.size()<2)
                    {
                        qDebug() << (QStringLiteral("Unable to open the file: %1, reputation have to few level: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_positive.contains(0))
                    {
                        qDebug() << (QStringLiteral("Unable to open the file: %1, no starting level for the positive: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_negative.empty() && !point_list_negative.contains(-1))
                    {
                        qDebug() << (QStringLiteral("Unable to open the file: %1, no starting level for the negative: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!item.attribute("type").contains(QRegExp("^[a-z]{1,32}$")))
                    {
                        qDebug() << (QStringLiteral("Unable to open the file: %1, the type %4 don't match wiuth the regex: ^[a-z]{1,32}$: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("type")));
                        ok=false;
                    }
                if(ok)
                {
                    reputationExtra[item.attribute(QStringLiteral("type"))].reputation_positive=text_positive;
                    reputationExtra[item.attribute(QStringLiteral("type"))].reputation_negative=text_negative;
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            qDebug() << (QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(QStringLiteral("reputation"));
    }
    QHash<QString, CatchChallenger::Reputation>::const_iterator i = CatchChallenger::CommonDatapack::commonDatapack.reputation.constBegin();
    while (i != CatchChallenger::CommonDatapack::commonDatapack.reputation.constEnd()) {
        if(!reputationExtra.contains(i.key()))
            reputationExtra[i.key()].name=tr("Unknown");
        while(reputationExtra[i.key()].reputation_negative.size()<i.value().reputation_negative.size())
            reputationExtra[i.key()].reputation_negative << tr("Unknown");
        while(reputationExtra[i.key()].reputation_positive.size()<i.value().reputation_positive.size())
            reputationExtra[i.key()].reputation_positive << tr("Unknown");
        ++i;
    }

    qDebug() << QStringLiteral("%1 reputation(s) extra loaded").arg(reputationExtra.size());
}

void DatapackClientLoader::parseItemsExtra()
{
    QDomDocument domDocument;
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM)+QStringLiteral("items.xml");
    //open and quick check the file
    if(CatchChallenger::DatapackGeneralLoader::xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::DatapackGeneralLoader::xmlLoadedFile.value(file);
    else
    {
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return;
        }
        qDebug() << (QStringLiteral("Xml not already loaded: %1").arg(file));
        CatchChallenger::DatapackGeneralLoader::xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("items"))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file);
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(QStringLiteral("item"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QStringLiteral("id")))
            {
                quint32 id=item.attribute(QStringLiteral("id")).toULongLong(&ok);
                if(ok)
                {
                    if(!DatapackClientLoader::itemsExtra.contains(id))
                    {
                        //load the image
                        if(item.hasAttribute(QStringLiteral("image")))
                        {
                            QPixmap image(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM)+item.attribute("image"));
                            if(image.isNull())
                            {
                                qDebug() << QStringLiteral("Unable to open the items image: %1: child.tagName(): %2 (at line: %3)").arg(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM)+item.attribute(QStringLiteral("image"))).arg(item.tagName()).arg(item.lineNumber());
                                DatapackClientLoader::itemsExtra[id].image=*mDefaultInventoryImage;
                                DatapackClientLoader::itemsExtra[id].imagePath=QStringLiteral(":/images/inventory/unknow-object.png");
                            }
                            else
                            {
                                DatapackClientLoader::itemsExtra[id].imagePath=QFileInfo(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM)+item.attribute(QStringLiteral("image"))).absoluteFilePath();
                                DatapackClientLoader::itemsExtra[id].image=image;
                            }
                        }
                        else
                        {
                            qDebug() << QStringLiteral("For parse item: Have not image attribute: child.tagName(): %1 (%2 at line: %3)").arg(item.tagName()).arg(file).arg(item.lineNumber());
                            DatapackClientLoader::itemsExtra[id].image=*mDefaultInventoryImage;
                            DatapackClientLoader::itemsExtra[id].imagePath=QStringLiteral(":/images/inventory/unknow-object.png");
                        }
                        // base size: 24x24
                        DatapackClientLoader::itemsExtra[id].image=DatapackClientLoader::itemsExtra.value(id).image.scaled(72,72);//then zoom: 3x

                        //load the name
                        {
                            bool name_found=false;
                            QDomElement name = item.firstChildElement(QStringLiteral("name"));
                            if(!language.isEmpty() && language!=QStringLiteral("en"))
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(name.hasAttribute(QStringLiteral("lang")) && name.attribute(QStringLiteral("lang"))==language)
                                        {
                                            DatapackClientLoader::itemsExtra[id].name=name.text();
                                            name_found=true;
                                            break;
                                        }
                                    }
                                    name = name.nextSiblingElement(QStringLiteral("name"));
                                }
                            if(!name_found)
                            {
                                name = item.firstChildElement(QStringLiteral("name"));
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(!name.hasAttribute(QStringLiteral("lang")) || name.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                        {
                                            DatapackClientLoader::itemsExtra[id].name=name.text();
                                            name_found=true;
                                            break;
                                        }
                                    }
                                    name = name.nextSiblingElement(QStringLiteral("name"));
                                }
                            }
                            if(!name_found)
                            {
                                DatapackClientLoader::itemsExtra[id].name=tr("Unknown object");
                                qDebug() << QStringLiteral("English name not found for the item with id: %1").arg(id);
                            }
                        }

                        //load the description
                        {
                            bool description_found=false;
                            QDomElement description = item.firstChildElement(QStringLiteral("description"));
                            if(!language.isEmpty() && language!=QStringLiteral("en"))
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(description.hasAttribute(QStringLiteral("lang")) && description.attribute(QStringLiteral("lang"))==language)
                                        {
                                            DatapackClientLoader::itemsExtra[id].description=description.text();
                                            description_found=true;
                                            break;
                                        }
                                    }
                                    description = description.nextSiblingElement(QStringLiteral("description"));
                                }
                            if(!description_found)
                            {
                                description = item.firstChildElement(QStringLiteral("description"));
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(!description.hasAttribute(QStringLiteral("lang")) || description.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                        {
                                            DatapackClientLoader::itemsExtra[id].description=description.text();
                                            description_found=true;
                                            break;
                                        }
                                    }
                                    description = description.nextSiblingElement(QStringLiteral("description"));
                                }
                            }
                            if(!description_found)
                            {
                                DatapackClientLoader::itemsExtra[id].description=tr("This object is not listed as know object. The information can't be found.");
                                //qDebug() << QStringLiteral("English description not found for the item with id: %1").arg(id);
                            }
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement(QStringLiteral("item"));
    }

    qDebug() << QStringLiteral("%1 item(s) extra loaded").arg(DatapackClientLoader::itemsExtra.size());
}

void DatapackClientLoader::parseMaps()
{
    QStringList returnList=CatchChallenger::FacilityLib::listFolder(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP));

    //load the map
    int size=returnList.size();
    int index=0;
    QRegularExpression mapFilter(QStringLiteral("\\.tmx$"));
    while(index<size)
    {
        QString fileName=returnList.at(index);
        if(fileName.contains(mapFilter))
            maps << fileName;
        index++;
    }
    maps.sort();

    qDebug() << QStringLiteral("%1 map(s) extra loaded").arg(maps.size());
}

void DatapackClientLoader::parseSkins()
{
    skins=CatchChallenger::FacilityLib::skinIdList(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_SKIN));

    qDebug() << QStringLiteral("%1 skin(s) loaded").arg(skins.size());
}

void DatapackClientLoader::resetAll()
{
    CatchChallenger::CommonDatapack::commonDatapack.unload();
    language.clear();
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/images/inventory/unknow-object.png"));
    datapackPath.clear();
    itemsExtra.clear();
    maps.clear();
    skins.clear();

    {
        QHashIterator<quint8,PlantExtra> i(plantExtra);
        while (i.hasNext()) {
            i.next();
            delete i.value().tileset;
        }
    }
    plantExtra.clear();
    itemToPlants.clear();
    questsExtra.clear();
    questsText.clear();
    botToQuestStart.clear();
    botFightsExtra.clear();
    monsterExtra.clear();
    monsterBuffsExtra.clear();
    monsterSkillsExtra.clear();
    audioAmbiance.clear();
    zonesExtra.clear();
    typeExtra.clear();
    {
         QHashIterator<QString,Tiled::Tileset *> i(Tiled::Tileset::preloadedTileset);
         while (i.hasNext()) {
             i.next();
             delete i.value();
         }
         Tiled::Tileset::preloadedTileset.clear();
    }
}

void DatapackClientLoader::parseQuestsExtra()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_QUESTS)).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    while(index<entryList.size())
    {
        bool showRewards=false;
        if(!entryList.at(index).isDir())
        {
            index++;
            continue;
        }
        bool ok;
        quint32 id=entryList.at(index).fileName().toUInt(&ok);
        if(!ok)
        {
            qDebug() << QStringLiteral("Unable to open the folder: %1, because is folder name is not a number").arg(entryList.at(index).fileName());
            index++;
            continue;
        }
        QDomDocument domDocument;
        const QString &file=entryList.at(index).absoluteFilePath()+QStringLiteral("/definition.xml");
        if(CatchChallenger::DatapackGeneralLoader::xmlLoadedFile.contains(file))
            domDocument=CatchChallenger::DatapackGeneralLoader::xmlLoadedFile.value(file);
        else
        {
            QFile itemsFile(file);
            QByteArray xmlContent;
            if(!itemsFile.open(QIODevice::ReadOnly))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
                index++;
                continue;
            }
            xmlContent=itemsFile.readAll();
            itemsFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
                index++;
                continue;
            }
            qDebug() << (QStringLiteral("Xml not already loaded: %1").arg(file));
            CatchChallenger::DatapackGeneralLoader::xmlLoadedFile[file]=domDocument;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=QStringLiteral("quest"))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(file);
            index++;
            continue;
        }

        DatapackClientLoader::QuestExtra quest;

        //load name
        {
            QDomElement name = root.firstChildElement(QStringLiteral("name"));
            bool found=false;
            if(!language.isEmpty() && language!=QStringLiteral("en"))
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(name.hasAttribute(QStringLiteral("lang")) && name.attribute(QStringLiteral("lang"))==language)
                        {
                            quest.name=name.text();
                            found=true;
                            break;
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(name.tagName()).arg(name.lineNumber());
                    }
                    name = name.nextSiblingElement(QStringLiteral("name"));
                }
            if(!found)
            {
                name = root.firstChildElement(QStringLiteral("name"));
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(!name.hasAttribute(QStringLiteral("lang")) || name.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                        {
                            quest.name=name.text();
                            break;
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(name.tagName()).arg(name.lineNumber());
                    }
                    name = name.nextSiblingElement(QStringLiteral("name"));
                }
            }
        }

        //load showRewards
        {
            QDomElement rewards = root.firstChildElement(QStringLiteral("rewards"));
            if(!rewards.isNull() && rewards.isElement())
            {
                if(rewards.hasAttribute(QStringLiteral("show")))
                    if(rewards.attribute(QStringLiteral("show"))==QStringLiteral("true"))
                        showRewards=true;
            }
        }

        QHash<quint32,QString> steps;
        {
            //load step
            QDomElement step = root.firstChildElement(QStringLiteral("step"));
            while(!step.isNull())
            {
                if(step.isElement())
                {
                    if(step.hasAttribute(QStringLiteral("id")))
                    {
                        quint32 id=step.attribute(QStringLiteral("id")).toULongLong(&ok);
                        if(ok)
                        {
                            CatchChallenger::Quest::Step stepObject;
                            if(step.hasAttribute(QStringLiteral("bot")))
                            {
                                QStringList tempStringList=step.attribute(QStringLiteral("bot")).split(QStringLiteral(";"));
                                int index=0;
                                while(index<tempStringList.size())
                                {
                                    quint32 tempInt=tempStringList.at(index).toUInt(&ok);
                                    if(ok)
                                        stepObject.bots << tempInt;
                                    index++;
                                }
                            }
                            QDomElement stepItem = step.firstChildElement(QStringLiteral("text"));
                            bool found=false;
                            if(!language.isEmpty() && language!=QStringLiteral("en"))
                            {
                                while(!stepItem.isNull())
                                {
                                    if(stepItem.isElement())
                                    {
                                        if(stepItem.hasAttribute(QStringLiteral("lang")) || stepItem.attribute(QStringLiteral("lang"))==language)
                                        {
                                            found=true;
                                            steps[id]=stepItem.text();
                                        }
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                                    stepItem = stepItem.nextSiblingElement(QStringLiteral("text"));
                                }
                            }
                            if(!found)
                            {
                                stepItem = step.firstChildElement(QStringLiteral("text"));
                                while(!stepItem.isNull())
                                {
                                    if(stepItem.isElement())
                                    {
                                        if(!stepItem.hasAttribute(QStringLiteral("lang")) || stepItem.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                            steps[id]=stepItem.text();
                                        else
                                            qDebug() << QStringLiteral("Has attribute: %1, is not lang en: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                                    stepItem = stepItem.nextSiblingElement(QStringLiteral("text"));
                                }
                            }
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Has attribute: %1, have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                step = step.nextSiblingElement(QStringLiteral("step"));
            }
        }

        //sort the step
        int indexLoop=1;
        while(indexLoop<(steps.size()+1))
        {
            if(!steps.contains(indexLoop))
                break;
            quest.steps << steps.value(indexLoop);
            indexLoop++;
        }
        if(indexLoop>=(steps.size()+1))
        {
            //add it, all seam ok
            questsExtra[id]=quest;
            questsExtra[id].showRewards=showRewards;
        }
        questsPathToId[entryList.at(index).absoluteFilePath()]=id;

        index++;
    }

    qDebug() << QStringLiteral("%1 quest(s) extra loaded").arg(questsExtra.size());
}

void DatapackClientLoader::parseQuestsText()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_QUESTS)).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    while(index<entryList.size())
    {
        if(!entryList.at(index).isDir())
        {
            index++;
            continue;
        }
        const QString &file=entryList.at(index).absoluteFilePath()+QStringLiteral("/text.xml");
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            index++;
            continue;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QDomDocument domDocument;
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=QStringLiteral("text"))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(file);
            index++;
            continue;
        }

        //load the content
        bool ok;
        QHash<quint32,QString> client_logic_texts;
        //load text
        QDomElement client_logic = root.firstChildElement(QStringLiteral("client_logic"));
        while(!client_logic.isNull())
        {
            if(client_logic.isElement())
            {
                if(client_logic.hasAttribute(QStringLiteral("id")))
                {
                    quint32 id=client_logic.attribute(QStringLiteral("id")).toULongLong(&ok);
                    if(ok)
                    {
                        QDomElement text = client_logic.firstChildElement(QStringLiteral("text"));
                        bool found=false;
                        if(!language.isEmpty() && language!=QStringLiteral("en"))
                            while(!text.isNull())
                            {
                                if(text.isElement())
                                {
                                    if(text.hasAttribute(QStringLiteral("lang")) && text.attribute(QStringLiteral("lang"))==language)
                                    {
                                        client_logic_texts[id]=text.text();
                                        found=true;
                                    }
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                                text = text.nextSiblingElement(QStringLiteral("text"));
                            }
                        if(!found)
                        {
                            text = client_logic.firstChildElement(QStringLiteral("text"));
                            while(!text.isNull())
                            {
                                if(text.isElement())
                                {
                                    if(!text.hasAttribute(QStringLiteral("lang")) || text.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                        client_logic_texts[id]=text.text();
                                    else
                                        qDebug() << QStringLiteral("Has attribute: %1, is not lang en: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                                text = text.nextSiblingElement(QStringLiteral("text"));
                            }
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Has attribute: %1, have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
            client_logic = client_logic.nextSiblingElement(QStringLiteral("client_logic"));
        }
        #ifdef DEBUG_CLIENT_QUEST
        qDebug() << QStringLiteral("%1 quest(s) text loaded for quest %2").arg(client_logic_texts.size()).arg(questsPathToId.value(entryList.at(index).absoluteFilePath()));
        #endif
        questsText[questsPathToId.value(entryList.at(index).absoluteFilePath())].text=client_logic_texts;
        index++;
    }

    qDebug() << QStringLiteral("%1 quest(s) text loaded").arg(questsExtra.size());
}

void DatapackClientLoader::parseAudioAmbiance()
{
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+QStringLiteral("music.xml");
    QDomDocument domDocument;
    //open and quick check the file
    if(CatchChallenger::DatapackGeneralLoader::xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::DatapackGeneralLoader::xmlLoadedFile.value(file);
    else
    {
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return;
        }
        qDebug() << (QStringLiteral("Xml not already loaded: %1").arg(file));
        CatchChallenger::DatapackGeneralLoader::xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("list"))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file);
        return;
    }

    //load the content
    QDomElement item = root.firstChildElement(QStringLiteral("map"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QStringLiteral("type")))
            {
                QString type=item.attribute(QStringLiteral("type"));
                if(!DatapackClientLoader::datapackLoader.audioAmbiance.contains(type))
                    audioAmbiance[type]=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+item.text();
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement(QStringLiteral("map"));
    }

    qDebug() << QStringLiteral("%1 audio ambiance(s) link loaded").arg(audioAmbiance.size());
}

void DatapackClientLoader::parseQuestsLink()
{
    QHashIterator<quint32,CatchChallenger::Quest> i(CatchChallenger::CommonDatapack::commonDatapack.quests);
    while(i.hasNext()) {
        i.next();
        QList<quint32> bots=i.value().steps.first().bots;
        int index=0;
        while(index<bots.size())
        {
            botToQuestStart.insert(bots.at(index),i.key());
            index++;
        }
    }
    qDebug() << QStringLiteral("%1 bot linked with quest(s) loaded").arg(botToQuestStart.size());
}

void DatapackClientLoader::parseZoneExtra()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ZONE)).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    QRegularExpression xmlFilter(QStringLiteral("^[a-zA-Z0-9\\- _]+\\.xml$"));
    QRegularExpression removeXml(QStringLiteral("\\.xml$"));
    while(index<entryList.size())
    {
        if(!entryList.at(index).isFile())
        {
            index++;
            continue;
        }
        if(!entryList.at(index).fileName().contains(xmlFilter))
        {
            qDebug() << QStringLiteral("%1 the zone file name not match").arg(entryList.at(index).fileName());
            index++;
            continue;
        }
        QString zoneCodeName=entryList.at(index).fileName();
        const QString &file=entryList.at(index).absoluteFilePath();
        zoneCodeName.remove(removeXml);
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            index++;
            continue;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QDomDocument domDocument;
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=QStringLiteral("zone"))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"zone\" root balise not found for the xml file").arg(file);
            index++;
            continue;
        }

        //load the content
        bool haveName=false;
        DatapackClientLoader::ZoneExtra zone;

        //load name
        QDomElement name = root.firstChildElement(QStringLiteral("name"));
        bool found=false;
        if(!language.isEmpty() && language!=QStringLiteral("en"))
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute(QStringLiteral("lang")) || name.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                    {
                        haveName=true;
                        zone.name=name.text();
                        break;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(name.tagName()).arg(name.lineNumber());
                }
                name = name.nextSiblingElement(QStringLiteral("name"));
            }
        if(!found)
        {
            name = root.firstChildElement(QStringLiteral("name"));
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute(QStringLiteral("lang")) || name.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                    {
                        haveName=true;
                        zone.name=name.text();
                        break;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(name.tagName()).arg(name.lineNumber());
                }
                name = name.nextSiblingElement(QStringLiteral("name"));
            }
        }
        if(haveName)
            zonesExtra[zoneCodeName]=zone;

        index++;
    }

    qDebug() << QStringLiteral("%1 zone(s) extra loaded").arg(zonesExtra.size());
}

void DatapackClientLoader::parseTileset()
{
    QStringList fileList=CatchChallenger::FacilityLib::listFolder(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP));
    int index=0;
    while(index<fileList.size())
    {
        const QString &filePath=fileList.at(index);
        if(filePath.endsWith(QStringLiteral(".tsx")))
        {
            const QString &source=QFileInfo(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+filePath).absoluteFilePath();
            QFile file(source);
            if(file.open(QIODevice::ReadOnly))
            {
                Tiled::MapReader mapReader;
                Tiled::Tileset *tileset = mapReader.readTileset(&file, source);
                if (tileset)
                {
                    tileset->setFileName(source);
                    Tiled::Tileset::preloadedTileset[source]=tileset;
                }
                file.close();
            }
            else
                qDebug() << QStringLiteral("Tileset: %1 can't be open: %2").arg(source).arg(file.errorString());
        }
        index++;
    }

    qDebug() << QStringLiteral("%1 tileset(s) loaded").arg(Tiled::Tileset::preloadedTileset.size());
}
