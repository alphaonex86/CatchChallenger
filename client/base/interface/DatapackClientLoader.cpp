#include "../interface/DatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/DatapackGeneralLoader.h"

#include <QDebug>
#include <QFile>
#include <QDomElement>
#include <QDomDocument>
#include <QByteArray>
#include <QDir>
#include <QFileInfoList>

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
    parseItems();
    parseMaps();
    parsePlants();
    parseCraftingRecipes();
    parseBuff();
    parseSkills();
    parseMonsters();
    parseReputation();
    parseMonstersExtra();
    parseBuffExtra();
    parseSkillsExtra();
    parseQuests();
    parseQuestsLink();
    parseQuestsExtra();
    parseQuestsText();
    inProgress=false;
    emit datapackParsed();
}

void DatapackClientLoader::parseItems()
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
                        //load the price
                        if(item.hasAttribute("price"))
                        {
                            bool ok;
                            DatapackClientLoader::items[id].price=item.attribute("price").toUInt(&ok);
                            if(!ok)
                            {
                                qDebug() << QString("price is not a number: child.tagName(): %1 (at line: %2)").arg(item.tagName()).arg(item.lineNumber());
                                DatapackClientLoader::items[id].price=0;
                            }
                        }
                        else
                        {
                            qDebug() << QString("No image, load the default, id number already set: child.tagName(): %1 (at line: %2)").arg(item.tagName()).arg(item.lineNumber());
                            DatapackClientLoader::items[id].image=*mDefaultInventoryImage;
                        }
                        // base size: 24x24
                        DatapackClientLoader::items[id].image=DatapackClientLoader::items[id].image.scaled(72,72);//then zoom: 3x

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

    qDebug() << QString("%1 item(s) loaded").arg(DatapackClientLoader::items.size());
}

void DatapackClientLoader::parseQuests()
{
    quests=CatchChallenger::DatapackGeneralLoader::loadQuests(datapackPath+DATAPACK_BASE_PATH_QUESTS);
    qDebug() << QString("%1 quest(s) loaded").arg(quests.size());
}

void DatapackClientLoader::parseReputation()
{
    reputation=CatchChallenger::DatapackGeneralLoader::loadReputation(datapackPath+DATAPACK_BASE_PATH_PLAYER+"reputation.xml");
    qDebug() << QString("%1 reputation(s) loaded").arg(reputation.size());
}

void DatapackClientLoader::parseMaps()
{
    QStringList returnList=CatchChallenger::FacilityLib::listFolder(datapackPath+DATAPACK_BASE_PATH_MAP);

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
     itemToPlants.clear();
     itemToCrafingRecipes.clear();
     crafingRecipes.clear();
     quests.clear();
     questsExtra.clear();
     questsText.clear();
     botToQuestStart.clear();
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
        if(root.tagName()!="quest")
        {
            qDebug() << QString("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(itemsFile.fileName());
            return;
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
        while(!name.isNull())
        {
            if(name.isElement() && name.hasAttribute("lang") && name.attribute("lang")=="en")
                quest.name=name.text();
            else
                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(name.tagName()).arg(name.lineNumber());
            name = name.nextSiblingElement("name");
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
                        while(!stepItem.isNull())
                        {
                            if(stepItem.isElement())
                            {
                                if(stepItem.hasAttribute("lang") && stepItem.attribute("lang")=="en")
                                    steps[id]=stepItem.text();
                                else
                                    qDebug() << QString("Has attribute: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                            stepItem = stepItem.nextSiblingElement("text");
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                }
                else
                    qDebug() << QString("Has attribute: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
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
        if(root.tagName()!="text")
        {
            qDebug() << QString("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(itemsFile.fileName());
            return;
        }

        //load the content
        bool ok;

        if(!root.hasAttribute("id"))
        {
            index++;
            continue;
        }
        DatapackClientLoader::QuestExtra quest;
        if(!ok)
        {
            index++;
            continue;
        }

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
                        while(!text.isNull())
                        {
                            if(text.isElement())
                            {
                                if(text.hasAttribute("lang") && text.attribute("lang")=="en")
                                    client_logic_texts[id]=text.text();
                                else
                                    qDebug() << QString("Has attribute: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                            text = text.nextSiblingElement("text");
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                }
                else
                    qDebug() << QString("Has attribute: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(client_logic.tagName()).arg(client_logic.lineNumber());
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

void DatapackClientLoader::parseQuestsLink()
{
    QHashIterator<quint32,CatchChallenger::Quest> i(quests);
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
