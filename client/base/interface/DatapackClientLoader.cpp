#include "../interface/DatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../LanguagesSelect.h"

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
        qDebug() << QString("already in progress");
        return;
    }
    inProgress=true;
    this->datapackPath=datapackPath;
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(":/images/inventory/unknow-object.png");
    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapackPath);
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
    inProgress=false;
    emit datapackParsed();
}

QString DatapackClientLoader::getDatapackPath()
{
    return datapackPath;
}

void DatapackClientLoader::parseItemsExtra()
{
    //open and quick check the file
    QFile itemsFile(datapackPath+DATAPACK_BASE_PATH_ITEM+"items.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        qDebug() << QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
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
                    if(!DatapackClientLoader::itemsExtra.contains(id))
                    {
                        //load the image
                        if(item.hasAttribute("image"))
                        {
                            QPixmap image(datapackPath+DATAPACK_BASE_PATH_ITEM+item.attribute("image"));
                            if(image.isNull())
                            {
                                qDebug() << QString("Unable to open the items image: %1: child.tagName(): %2 (at line: %3)").arg(datapackPath+DATAPACK_BASE_PATH_ITEM+item.attribute("image")).arg(item.tagName()).arg(item.lineNumber());
                                DatapackClientLoader::itemsExtra[id].image=*mDefaultInventoryImage;
                            }
                            else
                                DatapackClientLoader::itemsExtra[id].image=image;
                        }
                        else
                        {
                            qDebug() << QString("For parse item: Have not image attribute: child.tagName(): %1 (%2 at line: %3)").arg(item.tagName()).arg(itemsFile.fileName()).arg(item.lineNumber());
                            DatapackClientLoader::itemsExtra[id].image=*mDefaultInventoryImage;
                        }
                        // base size: 24x24
                        DatapackClientLoader::itemsExtra[id].image=DatapackClientLoader::itemsExtra[id].image.scaled(72,72);//then zoom: 3x

                        //load the name
                        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
                        bool name_found=false;
                        QDomElement name = item.firstChildElement("name");
                        if(!language.isEmpty() && language!="en")
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") || name.attribute("lang")==language)
                                    {
                                        DatapackClientLoader::itemsExtra[id].name=name.text();
                                        name_found=true;
                                        break;
                                    }
                                }
                                name = name.nextSiblingElement("name");
                            }
                        if(!name_found)
                        {
                            name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                    {
                                        DatapackClientLoader::itemsExtra[id].name=name.text();
                                        name_found=true;
                                        break;
                                    }
                                }
                                name = name.nextSiblingElement("name");
                            }
                        }
                        if(!name_found)
                        {
                            DatapackClientLoader::itemsExtra[id].name=tr("Unknown object");
                            qDebug() << QString("English name not found for the item with id: %1").arg(id);
                        }

                        //load the description
                        bool description_found=false;
                        QDomElement description = item.firstChildElement("description");
                        if(!language.isEmpty() && language!="en")
                            while(!description.isNull())
                            {
                                if(description.isElement())
                                {
                                    if(description.hasAttribute("lang") && name.attribute("lang")==language)
                                    {
                                        DatapackClientLoader::itemsExtra[id].description=description.text();
                                        description_found=true;
                                        break;
                                    }
                                }
                                description = description.nextSiblingElement("description");
                            }
                        if(!description_found)
                        {
                            description = item.firstChildElement("description");
                            while(!description.isNull())
                            {
                                if(description.isElement())
                                {
                                    if(!description.hasAttribute("lang") || name.attribute("lang")=="en")
                                    {
                                        DatapackClientLoader::itemsExtra[id].description=description.text();
                                        description_found=true;
                                        break;
                                    }
                                }
                                description = description.nextSiblingElement("description");
                            }
                        }
                        if(!description_found)
                        {
                            DatapackClientLoader::itemsExtra[id].description=tr("This object is not listed as know object. The information can't be found.");
                            qDebug() << QString("English description not found for the item with id: %1").arg(id);
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement("item");
    }

    qDebug() << QString("%1 item(s) extra loaded").arg(DatapackClientLoader::itemsExtra.size());
}

void DatapackClientLoader::parseMaps()
{
    QStringList returnList=CatchChallenger::FacilityLib::listFolder(datapackPath+DATAPACK_BASE_PATH_MAP);

    //load the map
    int size=returnList.size();
    int index=0;
    QRegularExpression mapFilter("\\.tmx$");
    while(index<size)
    {
        QString fileName=returnList.at(index);
        if(fileName.contains(mapFilter))
            maps << fileName;
        index++;
    }
    maps.sort();

    qDebug() << QString("%1 map(s) extra loaded").arg(maps.size());
}

void DatapackClientLoader::parseSkins()
{
    skins=CatchChallenger::FacilityLib::skinIdList(datapackPath+DATAPACK_BASE_PATH_SKIN);

    qDebug() << QString("%1 skin(s) loaded").arg(maps.size());
}

void DatapackClientLoader::resetAll()
{
    CatchChallenger::CommonDatapack::commonDatapack.unload();
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(":/images/inventory/unknow-object.png");
    datapackPath.clear();
    itemsExtra.clear();
    maps.clear();
    skins.clear();

    QHashIterator<quint8,PlantExtra> i(plantExtra);
     while (i.hasNext()) {
         i.next();
         delete i.value().tileset;
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
}

void DatapackClientLoader::parseQuestsExtra()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(datapackPath+DATAPACK_BASE_PATH_QUESTS).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    while(index<entryList.size())
    {
        if(!entryList.at(index).isDir())
        {
            index++;
            continue;
        }
        if(!QFile(entryList.at(index).absoluteFilePath()+"/definition.xml").exists())
        {
            index++;
            continue;
        }
        QFile itemsFile(entryList.at(index).absoluteFilePath()+"/definition.xml");
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
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
            qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!="quest")
        {
            qDebug() << QString("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(itemsFile.fileName());
            index++;
            continue;
        }

        //load the content
        bool ok;

        if(!root.hasAttribute("id"))
        {
            index++;
            continue;
        }
        DatapackClientLoader::QuestExtra quest;
        quint32 id=root.attribute("id").toUInt(&ok);
        if(!ok)
        {
            index++;
            continue;
        }

        //load name
        QDomElement name = root.firstChildElement("name");
        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        bool found=false;
        if(!language.isEmpty() && language!="en")
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(name.hasAttribute("lang") && name.attribute("lang")==language)
                    {
                        quest.name=name.text();
                        found=true;
                        break;
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(name.tagName()).arg(name.lineNumber());
                }
                name = name.nextSiblingElement("name");
            }
        if(!found)
        {
            name = root.firstChildElement("name");
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                    {
                        quest.name=name.text();
                        break;
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(name.tagName()).arg(name.lineNumber());
                }
                name = name.nextSiblingElement("name");
            }
        }

        QHash<quint32,QString> steps;
        //load step
        QDomElement step = root.firstChildElement("step");
        while(!step.isNull())
        {
            if(step.isElement())
            {
                if(step.hasAttribute("id"))
                {
                    quint32 id=step.attribute("id").toULongLong(&ok);
                    if(ok)
                    {
                        CatchChallenger::Quest::Step stepObject;
                        if(step.hasAttribute("bot"))
                        {
                            QStringList tempStringList=step.attribute("bot").split(";");
                            int index=0;
                            while(index<tempStringList.size())
                            {
                                quint32 tempInt=tempStringList.at(index).toUInt(&ok);
                                if(ok)
                                    stepObject.bots << tempInt;
                                index++;
                            }
                        }
                        QDomElement stepItem = step.firstChildElement("text");
                        bool found=false;
                        if(!language.isEmpty() && language!="en")
                        {
                            while(!stepItem.isNull())
                            {
                                if(stepItem.isElement())
                                {
                                    if(stepItem.hasAttribute("lang") || stepItem.attribute("lang")==language)
                                    {
                                        found=true;
                                        steps[id]=stepItem.text();
                                    }
                                }
                                else
                                    qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                                stepItem = stepItem.nextSiblingElement("text");
                            }
                        }
                        if(!found)
                        {
                            stepItem = step.firstChildElement("text");
                            while(!stepItem.isNull())
                            {
                                if(stepItem.isElement())
                                {
                                    if(!stepItem.hasAttribute("lang") || stepItem.attribute("lang")=="en")
                                        steps[id]=stepItem.text();
                                    else
                                        qDebug() << QString("Has attribute: %1, is not lang en: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                                }
                                else
                                    qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                                stepItem = stepItem.nextSiblingElement("text");
                            }
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                }
                else
                    qDebug() << QString("Has attribute: %1, have not id attribute: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
            step = step.nextSiblingElement("step");
        }

        //sort the step
        int indexLoop=1;
        while(indexLoop<(steps.size()+1))
        {
            if(!steps.contains(indexLoop))
                break;
            quest.steps << steps[indexLoop];
            indexLoop++;
        }
        if(indexLoop>=(steps.size()+1))
        {
            //add it, all seam ok
            questsExtra[id]=quest;
        }
        questsPathToId[entryList.at(index).absoluteFilePath()]=id;

        index++;
    }

    qDebug() << QString("%1 quest(s) extra loaded").arg(questsExtra.size());
}

void DatapackClientLoader::parseQuestsText()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(datapackPath+DATAPACK_BASE_PATH_QUESTS).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    while(index<entryList.size())
    {
        if(!entryList.at(index).isDir())
        {
            index++;
            continue;
        }
        if(!QFile(entryList.at(index).absoluteFilePath()+"/text.xml").exists())
        {
            index++;
            continue;
        }
        QFile itemsFile(entryList.at(index).absoluteFilePath()+"/text.xml");
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
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
            qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!="text")
        {
            qDebug() << QString("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(itemsFile.fileName());
            index++;
            continue;
        }

        //load the content
        bool ok;
        QHash<quint32,QString> client_logic_texts;
        //load text
        QDomElement client_logic = root.firstChildElement("client_logic");
        while(!client_logic.isNull())
        {
            if(client_logic.isElement())
            {
                if(client_logic.hasAttribute("id"))
                {
                    quint32 id=client_logic.attribute("id").toULongLong(&ok);
                    if(ok)
                    {
                        QDomElement text = client_logic.firstChildElement("text");
                        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
                        bool found=false;
                        if(!language.isEmpty() && language!="en")
                            while(!text.isNull())
                            {
                                if(text.isElement())
                                {
                                    if(text.hasAttribute("lang") && text.attribute("lang")==language)
                                    {
                                        client_logic_texts[id]=text.text();
                                        found=true;
                                    }
                                }
                                else
                                    qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                                text = text.nextSiblingElement("text");
                            }
                        if(!found)
                        {
                            text = client_logic.firstChildElement("text");
                            while(!text.isNull())
                            {
                                if(text.isElement())
                                {
                                    if(!text.hasAttribute("lang") || text.attribute("lang")=="en")
                                        client_logic_texts[id]=text.text();
                                    else
                                        qDebug() << QString("Has attribute: %1, is not lang en: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                                }
                                else
                                    qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                                text = text.nextSiblingElement("text");
                            }
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                }
                else
                    qDebug() << QString("Has attribute: %1, have not id attribute: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
            client_logic = client_logic.nextSiblingElement("client_logic");
        }
        #ifdef DEBUG_CLIENT_QUEST
        qDebug() << QString("%1 quest(s) text loaded for quest %2").arg(client_logic_texts.size()).arg(questsPathToId[entryList.at(index).absoluteFilePath()]);
        #endif
        questsText[questsPathToId[entryList.at(index).absoluteFilePath()]].text=client_logic_texts;
        index++;
    }

    qDebug() << QString("%1 quest(s) text loaded").arg(questsExtra.size());
}

void DatapackClientLoader::parseAudioAmbiance()
{
    //open and quick check the file
    QFile itemsFile(datapackPath+DATAPACK_BASE_PATH_MAP+"music.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        qDebug() << QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
    }

    //load the content
    QDomElement item = root.firstChildElement("map");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("type"))
            {
                QString type=item.attribute("type");
                if(!DatapackClientLoader::datapackLoader.audioAmbiance.contains(type))
                    audioAmbiance[type]=datapackPath+DATAPACK_BASE_PATH_MAP+item.text();
                else
                    qDebug() << QString("Unable to open the file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement("map");
    }

    qDebug() << QString("%1 audio ambiance(s) link loaded").arg(audioAmbiance.size());
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
    qDebug() << QString("%1 bot linked with quest(s) loaded").arg(botToQuestStart.size());
}

void DatapackClientLoader::parseZoneExtra()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(datapackPath+DATAPACK_BASE_PATH_ZONE).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    while(index<entryList.size())
    {
        if(!entryList.at(index).isFile())
        {
            index++;
            continue;
        }
        if(!entryList.at(index).fileName().contains(QRegularExpression("^[a-zA-Z0-9\\- _]+\\.xml$")))
        {
            qDebug() << QString("%1 the zone file name not match").arg(entryList.at(index).fileName());
            index++;
            continue;
        }
        QString zoneCodeName=entryList.at(index).fileName();
        zoneCodeName.remove(QRegularExpression("\\.xml$"));
        QFile itemsFile(entryList.at(index).absoluteFilePath());
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
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
            qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!="zone")
        {
            qDebug() << QString("Unable to open the file: %1, \"zone\" root balise not found for the xml file").arg(itemsFile.fileName());
            index++;
            continue;
        }

        //load the content
        bool haveName=false;
        DatapackClientLoader::ZoneExtra zone;

        //load name
        QDomElement name = root.firstChildElement("name");
        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        bool found=false;
        if(!language.isEmpty() && language!="en")
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                    {
                        haveName=true;
                        zone.name=name.text();
                        break;
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(name.tagName()).arg(name.lineNumber());
                }
                name = name.nextSiblingElement("name");
            }
        if(!found)
        {
            name = root.firstChildElement("name");
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                    {
                        haveName=true;
                        zone.name=name.text();
                        break;
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(name.tagName()).arg(name.lineNumber());
                }
                name = name.nextSiblingElement("name");
            }
        }
        if(haveName)
            zonesExtra[zoneCodeName]=zone;

        index++;
    }

    qDebug() << QString("%1 zone(s) extra loaded").arg(zonesExtra.size());
}
