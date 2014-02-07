#include "../../base/interface/DatapackClientLoader.h"
#include "../../base/LanguagesSelect.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/DebugClass.h"
#include "../../general/fight/FightLoader.h"
#include "../../general/base/CommonDatapack.h"
#include "../fight/interface/ClientFightEngine.h"

#include <QDomElement>
#include <QDomDocument>
#include <QFile>
#include <QByteArray>
#include <QDebug>

void DatapackClientLoader::parseMonstersExtra()
{
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("monster.xml");
    QDomDocument domDocument;
    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile xmlFile(file);
        QByteArray xmlContent;
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml monster extra file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return;
        }
        xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return;
        }
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("list"))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return;
    }

    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
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
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    if(!CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        DatapackClientLoader::MonsterExtra monsterExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("monster extra loading: %1").arg(id));
                        #endif
                        {
                            bool found=false;
                            QDomElement name = item.firstChildElement(QStringLiteral("name"));
                            if(!language.isEmpty() && language!=QStringLiteral("en"))
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(name.hasAttribute(QStringLiteral("lang")) && name.attribute(QStringLiteral("lang"))==language)
                                        {
                                            monsterExtraEntry.name=name.text();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    name = name.nextSiblingElement(QStringLiteral("name"));
                                }
                            if(!found)
                            {
                                name = item.firstChildElement(QStringLiteral("name"));
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(!name.hasAttribute(QStringLiteral("lang")) || name.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                        {
                                            monsterExtraEntry.name=name.text();
                                            break;
                                        }
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    name = name.nextSiblingElement(QStringLiteral("name"));
                                }
                            }
                        }
                        {
                            bool found=false;
                            QDomElement description = item.firstChildElement(QStringLiteral("description"));
                            if(!language.isEmpty() && language!=QStringLiteral("en"))
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(description.hasAttribute(QStringLiteral("lang")) && description.attribute(QStringLiteral("lang"))==language)
                                        {
                                                monsterExtraEntry.description=description.text();
                                                found=true;
                                                break;
                                        }
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    description = description.nextSiblingElement(QStringLiteral("description"));
                                }
                            if(!found)
                            {
                                description = item.firstChildElement(QStringLiteral("description"));
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(!description.hasAttribute(QStringLiteral("lang")) || description.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                        {
                                                monsterExtraEntry.description=description.text();
                                                break;
                                        }
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    description = description.nextSiblingElement(QStringLiteral("description"));
                                }
                            }
                        }
                        //load the kind
                        {
                            bool kind_found=false;
                            QDomElement kind = item.firstChildElement(QStringLiteral("kind"));
                            if(!language.isEmpty() && language!=QStringLiteral("en"))
                                while(!kind.isNull())
                                {
                                    if(kind.isElement())
                                    {
                                        if(kind.hasAttribute(QStringLiteral("lang")) && kind.attribute(QStringLiteral("lang"))==language)
                                        {
                                            monsterExtraEntry.kind=kind.text();
                                            kind_found=true;
                                            break;
                                        }
                                    }
                                    kind = kind.nextSiblingElement(QStringLiteral("kind"));
                                }
                            if(!kind_found)
                            {
                                kind = item.firstChildElement(QStringLiteral("kind"));
                                while(!kind.isNull())
                                {
                                    if(kind.isElement())
                                    {
                                        if(!kind.hasAttribute(QStringLiteral("lang")) || kind.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                        {
                                            monsterExtraEntry.kind=kind.text();
                                            kind_found=true;
                                            break;
                                        }
                                    }
                                    kind = kind.nextSiblingElement(QStringLiteral("kind"));
                                }
                            }
                        }

                        //load the habitat
                        {
                            bool habitat_found=false;
                            QDomElement habitat = item.firstChildElement(QStringLiteral("habitat"));
                            if(!language.isEmpty() && language!=QStringLiteral("en"))
                                while(!habitat.isNull())
                                {
                                    if(habitat.isElement())
                                    {
                                        if(habitat.hasAttribute(QStringLiteral("lang")) && habitat.attribute(QStringLiteral("lang"))==language)
                                        {
                                            monsterExtraEntry.habitat=habitat.text();
                                            habitat_found=true;
                                            break;
                                        }
                                    }
                                    habitat = habitat.nextSiblingElement(QStringLiteral("habitat"));
                                }
                            if(!habitat_found)
                            {
                                habitat = item.firstChildElement(QStringLiteral("habitat"));
                                while(!habitat.isNull())
                                {
                                    if(habitat.isElement())
                                    {
                                        if(!habitat.hasAttribute(QStringLiteral("lang")) || habitat.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                        {
                                            monsterExtraEntry.habitat=habitat.text();
                                            habitat_found=true;
                                            break;
                                        }
                                    }
                                    habitat = habitat.nextSiblingElement(QStringLiteral("habitat"));
                                }
                            }
                        }
                        if(monsterExtraEntry.name.isEmpty())
                            monsterExtraEntry.name=tr("Unknown");
                        if(monsterExtraEntry.description.isEmpty())
                            monsterExtraEntry.description=tr("Unknown");
                        monsterExtraEntry.frontPath=datapackPath+QStringLiteral("/")+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QString::number(id)+QStringLiteral("/")+QStringLiteral("front.png");
                        monsterExtraEntry.front=QPixmap(monsterExtraEntry.frontPath);
                        if(monsterExtraEntry.front.isNull())
                        {
                            monsterExtraEntry.frontPath=datapackPath+QStringLiteral("/")+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QString::number(id)+QStringLiteral("/")+QStringLiteral("front.gif");
                            monsterExtraEntry.front=QPixmap(monsterExtraEntry.frontPath);
                            if(monsterExtraEntry.front.isNull())
                            {
                                monsterExtraEntry.frontPath=QStringLiteral(":/images/monsters/default/front.png");
                                monsterExtraEntry.front=QPixmap(monsterExtraEntry.frontPath);
                            }
                        }
                        monsterExtraEntry.backPath=datapackPath+QStringLiteral("/")+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QString::number(id)+QStringLiteral("/")+QStringLiteral("back.png");
                        monsterExtraEntry.back=QPixmap(monsterExtraEntry.backPath);
                        if(monsterExtraEntry.back.isNull())
                        {
                            monsterExtraEntry.backPath=datapackPath+QStringLiteral("/")+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QString::number(id)+QStringLiteral("/")+QStringLiteral("back.gif");
                            monsterExtraEntry.back=QPixmap(monsterExtraEntry.backPath);
                            if(monsterExtraEntry.back.isNull())
                            {
                                monsterExtraEntry.backPath=QStringLiteral(":/images/monsters/default/back.png");
                                monsterExtraEntry.back=QPixmap(monsterExtraEntry.backPath);
                            }
                        }
                        monsterExtraEntry.thumb=datapackPath+QStringLiteral("/")+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QString::number(id)+QStringLiteral("/")+QStringLiteral("small.png");
                        if(monsterExtraEntry.thumb.isNull())
                        {
                            monsterExtraEntry.thumb=datapackPath+QStringLiteral("/")+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QString::number(id)+QStringLiteral("/")+QStringLiteral("small.gif");
                            if(monsterExtraEntry.thumb.isNull())
                                monsterExtraEntry.thumb=QPixmap(QStringLiteral(":/images/monsters/default/small.png"));
                        }
                        monsterExtraEntry.thumb=monsterExtraEntry.thumb.scaled(64,64);
                        DatapackClientLoader::datapackLoader.monsterExtra[id]=monsterExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster id at monster extra: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(QStringLiteral("monster"));
    }

    QHashIterator<quint32,CatchChallenger::Monster> i(CatchChallenger::CommonDatapack::commonDatapack.monsters);
    while(i.hasNext())
    {
        i.next();
        if(!DatapackClientLoader::datapackLoader.monsterExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            DatapackClientLoader::MonsterExtra monsterExtraEntry;
            monsterExtraEntry.name=tr("Unknown");
            monsterExtraEntry.description=tr("Unknown");
            monsterExtraEntry.front=QPixmap(QStringLiteral(":/images/monsters/default/front.png"));
            monsterExtraEntry.back=QPixmap(QStringLiteral(":/images/monsters/default/back.png"));
            monsterExtraEntry.thumb=QPixmap(QStringLiteral(":/images/monsters/default/small.png"));
            monsterExtraEntry.thumb=monsterExtraEntry.thumb.scaled(64,64);
            DatapackClientLoader::datapackLoader.monsterExtra[i.key()]=monsterExtraEntry;
        }
    }

    qDebug() << QStringLiteral("%1 monster(s) extra loaded").arg(DatapackClientLoader::datapackLoader.monsterExtra.size());
}

void DatapackClientLoader::parseTypesExtra()
{
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("type.xml");
    QDomDocument domDocument;
    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
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
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("types"))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"list\" root balise not found for the xml file").arg(file);
        return;
    }

    //load the content
    {
        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        QSet<QString> duplicate;
        QDomElement typeItem = root.firstChildElement(QStringLiteral("type"));
        while(!typeItem.isNull())
        {
            if(typeItem.isElement())
            {
                if(typeItem.hasAttribute(QStringLiteral("name")))
                {
                    QString name=typeItem.attribute(QStringLiteral("name"));
                    if(!duplicate.contains(name))
                    {
                        duplicate << name;
                        TypeText type;
                        bool found=false;
                        QDomElement nameItem = typeItem.firstChildElement(QStringLiteral("name"));
                        if(!language.isEmpty() && language!=QStringLiteral("en"))
                            while(!nameItem.isNull())
                            {
                                if(nameItem.isElement())
                                {
                                    if(nameItem.hasAttribute(QStringLiteral("lang")) && nameItem.attribute(QStringLiteral("lang"))==language)
                                    {
                                        type.name=nameItem.text();
                                        found=true;
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(nameItem.tagName()).arg(nameItem.lineNumber()));
                                nameItem = nameItem.nextSiblingElement(QStringLiteral("name"));
                            }
                        if(!found)
                        {
                            nameItem = typeItem.firstChildElement(QStringLiteral("name"));
                            while(!nameItem.isNull())
                            {
                                if(nameItem.isElement())
                                {
                                    if(!nameItem.hasAttribute(QStringLiteral("lang")) || nameItem.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                    {
                                        type.name=nameItem.text();
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(nameItem.tagName()).arg(nameItem.lineNumber()));
                                nameItem = nameItem.nextSiblingElement(QStringLiteral("name"));
                            }
                        }
                        DatapackClientLoader::datapackLoader.typeExtra[DatapackClientLoader::datapackLoader.typeExtra.size()]=type;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, name is already set for type: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            typeItem = typeItem.nextSiblingElement(QStringLiteral("type"));
        }
    }

    qDebug() << QStringLiteral("%1 type(s) extra loaded").arg(DatapackClientLoader::datapackLoader.typeExtra.size());
}

void DatapackClientLoader::parseBuffExtra()
{
    QDomDocument domDocument;
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("buff.xml");
    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile xmlFile(file);
        QByteArray xmlContent;
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return;
        }
        xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return;
        }
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("list"))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return;
    }

    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(QStringLiteral("buff"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QStringLiteral("id")))
            {
                quint32 id=item.attribute(QStringLiteral("id")).toUInt(&ok);
                if(!ok)
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    if(!CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        DatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("monster extra loading: %1").arg(id));
                        #endif
                        bool found=false;
                        QDomElement name = item.firstChildElement(QStringLiteral("name"));
                        if(!language.isEmpty() && language!=QStringLiteral("en"))
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute(QStringLiteral("lang")) && name.attribute(QStringLiteral("lang"))==language)
                                    {
                                        monsterBuffExtraEntry.name=name.text();
                                        found=true;
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                name = name.nextSiblingElement(QStringLiteral("name"));
                            }
                        if(!found)
                        {
                            name = item.firstChildElement(QStringLiteral("name"));
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute(QStringLiteral("lang")) || name.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                    {
                                        monsterBuffExtraEntry.name=name.text();
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                name = name.nextSiblingElement(QStringLiteral("name"));
                            }
                        }
                        found=false;
                        QDomElement description = item.firstChildElement(QStringLiteral("description"));
                        if(!language.isEmpty() && language!=QStringLiteral("en"))
                            while(!description.isNull())
                            {
                                if(description.isElement())
                                {
                                    if(description.hasAttribute(QStringLiteral("lang")) && description.attribute(QStringLiteral("lang"))==language)
                                    {
                                        monsterBuffExtraEntry.description=description.text();
                                        found=true;
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                description = description.nextSiblingElement(QStringLiteral("description"));
                            }
                        if(!found)
                        {
                            description = item.firstChildElement(QStringLiteral("description"));
                            while(!description.isNull())
                            {
                                if(description.isElement())
                                {
                                    if(!description.hasAttribute(QStringLiteral("lang")) || description.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                    {
                                        monsterBuffExtraEntry.description=description.text();
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                description = description.nextSiblingElement(QStringLiteral("description"));
                            }
                        }
                        if(monsterBuffExtraEntry.name.isEmpty())
                            monsterBuffExtraEntry.name=tr("Unknown");
                        if(monsterBuffExtraEntry.description.isEmpty())
                            monsterBuffExtraEntry.description=tr("Unknown");
                        const QString &pngFile=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_BUFFICON)+QString::number(id)+QStringLiteral(".png");
                        const QString &gifFile=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_BUFFICON)+QString::number(id)+QStringLiteral(".gif");
                        if(QFile(pngFile).exists())
                            monsterBuffExtraEntry.icon=QIcon(pngFile);
                        else if(QFile(gifFile).exists())
                            monsterBuffExtraEntry.icon=QIcon(gifFile);
                        QList<QSize> availableSizes=monsterBuffExtraEntry.icon.availableSizes();
                        if(monsterBuffExtraEntry.icon.isNull() || availableSizes.isEmpty())
                            monsterBuffExtraEntry.icon=QIcon(QStringLiteral(":/images/interface/buff.png"));
                        DatapackClientLoader::datapackLoader.monsterBuffsExtra[id]=monsterBuffExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the buff id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(QStringLiteral("buff"));
    }

    QHashIterator<quint32,CatchChallenger::Buff> i(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs);
    while(i.hasNext())
    {
        i.next();
        if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            DatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
            monsterBuffExtraEntry.name=tr("Unknown");
            monsterBuffExtraEntry.description=tr("Unknown");
            monsterBuffExtraEntry.icon=QIcon(QStringLiteral(":/images/interface/buff.png"));
            DatapackClientLoader::datapackLoader.monsterBuffsExtra[i.key()]=monsterBuffExtraEntry;
        }
    }

    qDebug() << QStringLiteral("%1 buff(s) extra loaded").arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.size());
}

void DatapackClientLoader::parseSkillsExtra()
{
    //open and quick check the file
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("skill.xml");
    QDomDocument domDocument;
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile xmlFile(file);
        QByteArray xmlContent;
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return;
        }
        xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return;
        }
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("list"))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return;
    }

    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    bool found;
    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("skill");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QStringLiteral("id")))
            {
                quint32 id=item.attribute(QStringLiteral("id")).toUInt(&ok);
                if(!ok)
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    if(!CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        DatapackClientLoader::MonsterExtra::Skill monsterSkillExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("monster extra loading: %1").arg(id));
                        #endif
                        found=false;
                        QDomElement name = item.firstChildElement(QStringLiteral("name"));
                        if(!language.isEmpty() && language!=QStringLiteral("en"))
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute(QStringLiteral("lang")) && name.attribute(QStringLiteral("lang"))==language)
                                    {
                                        monsterSkillExtraEntry.name=name.text();
                                        found=true;
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                name = name.nextSiblingElement(QStringLiteral("name"));
                            }
                        if(!found)
                        {
                            name = item.firstChildElement(QStringLiteral("name"));
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute(QStringLiteral("lang")) || name.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                    {
                                        monsterSkillExtraEntry.name=name.text();
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                name = name.nextSiblingElement(QStringLiteral("name"));
                            }
                        }
                        found=false;
                        QDomElement description = item.firstChildElement(QStringLiteral("description"));
                        if(!language.isEmpty() && language!=QStringLiteral("en"))
                            while(!description.isNull())
                            {
                                if(description.isElement())
                                {
                                    if(description.hasAttribute(QStringLiteral("lang")) && description.attribute(QStringLiteral("lang"))==language)
                                    {
                                        monsterSkillExtraEntry.description=description.text();
                                        found=true;
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                description = description.nextSiblingElement(QStringLiteral("description"));
                            }
                        if(!found)
                        {
                            description = item.firstChildElement(QStringLiteral("description"));
                            while(!description.isNull())
                            {
                                if(description.isElement())
                                {
                                    if(!description.hasAttribute(QStringLiteral("lang")) || description.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                    {
                                        monsterSkillExtraEntry.description=description.text();
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                description = description.nextSiblingElement(QStringLiteral("description"));
                            }
                        }
                        if(monsterSkillExtraEntry.name.isEmpty())
                            monsterSkillExtraEntry.name=tr("Unknown");
                        if(monsterSkillExtraEntry.description.isEmpty())
                            monsterSkillExtraEntry.description=tr("Unknown");
                        monsterSkillsExtra[id]=monsterSkillExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the skill id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(QStringLiteral("skill"));
    }

    QHashIterator<quint32,CatchChallenger::Skill> i(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills);
    while(i.hasNext())
    {
        i.next();
        if(!monsterSkillsExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            DatapackClientLoader::MonsterExtra::Skill monsterSkillExtraEntry;
            monsterSkillExtraEntry.name=tr("Unknown");
            monsterSkillExtraEntry.description=tr("Unknown");
            monsterSkillsExtra[i.key()]=monsterSkillExtraEntry;
        }
    }

    qDebug() << QStringLiteral("%1 skill(s) extra loaded").arg(monsterSkillsExtra.size());
}

void DatapackClientLoader::parseBotFightsExtra()
{
    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    bool found;
    QDir dir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_FIGHT));
    QFileInfoList list=dir.entryInfoList(QStringList(),QDir::NoDotAndDotDot|QDir::Files);
    int index_file=0;
    while(index_file<list.size())
    {
        if(list.at(index_file).isFile())
        {
            const QString &file=list.at(index_file).absoluteFilePath();
            QDomDocument domDocument;
            //open and quick check the file
            if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
                domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
            else
            {
                QFile xmlFile(file);
                QByteArray xmlContent;
                if(!xmlFile.open(QIODevice::ReadOnly))
                {
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
                    index_file++;
                    continue;
                }
                xmlContent=xmlFile.readAll();
                xmlFile.close();
                QString errorStr;
                int errorLine,errorColumn;
                if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
                {
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
                    index_file++;
                    continue;
                }
            }
            QDomElement root = domDocument.documentElement();
            if(root.tagName()!=QStringLiteral("fights"))
            {
                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"fights\" root balise not found for the xml file").arg(file));
                index_file++;
                continue;
            }

            //load the content
            bool ok;
            QDomElement item = root.firstChildElement(QStringLiteral("fight"));
            while(!item.isNull())
            {
                if(item.isElement())
                {
                    if(item.hasAttribute(QStringLiteral("id")))
                    {
                        quint32 id=item.attribute(QStringLiteral("id")).toUInt(&ok);
                        if(ok)
                        {
                            if(CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(id))
                            {
                                if(!botFightsExtra.contains(id))
                                {
                                    BotFightExtra botFightExtra;
                                    botFightExtra.start=tr("Ready for the fight?");
                                    botFightExtra.win=tr("You are so strong for me!");
                                    {
                                        found=false;
                                        QDomElement start = item.firstChildElement(QStringLiteral("start"));
                                        if(!language.isEmpty() && language!=QStringLiteral("en"))
                                            while(!start.isNull())
                                            {
                                                if(start.isElement())
                                                {
                                                    if(start.hasAttribute(QStringLiteral("lang")) && start.attribute(QStringLiteral("lang"))==language)
                                                    {
                                                        botFightExtra.start=start.text();
                                                        found=true;
                                                        break;
                                                    }
                                                }
                                                else
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, start balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                start = start.nextSiblingElement(QStringLiteral("start"));
                                            }
                                        if(!found)
                                        {
                                            start = item.firstChildElement(QStringLiteral("start"));
                                            while(!start.isNull())
                                            {
                                                if(start.isElement())
                                                {
                                                    if(!start.hasAttribute(QStringLiteral("lang")) || start.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                                    {
                                                        botFightExtra.start=start.text();
                                                        break;
                                                    }
                                                }
                                                else
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, start balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                start = start.nextSiblingElement(QStringLiteral("start"));
                                            }
                                        }
                                        found=false;
                                        QDomElement win = item.firstChildElement(QStringLiteral("win"));
                                        if(!language.isEmpty() && language!=QStringLiteral("en"))
                                            while(!win.isNull())
                                            {
                                                if(win.isElement())
                                                {
                                                    if(win.hasAttribute(QStringLiteral("lang")) && win.attribute(QStringLiteral("lang"))==language)
                                                    {
                                                        botFightExtra.win=win.text();
                                                        found=true;
                                                        break;
                                                    }
                                                }
                                                else
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, win balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                win = win.nextSiblingElement(QStringLiteral("win"));
                                            }
                                        if(!found)
                                        {
                                            win = item.firstChildElement(QStringLiteral("win"));
                                            while(!win.isNull())
                                            {
                                                if(win.isElement())
                                                {
                                                    if(!win.hasAttribute(QStringLiteral("lang")) || win.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                                    {
                                                        botFightExtra.win=win.text();
                                                        break;
                                                    }
                                                }
                                                else
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, win balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                win = win.nextSiblingElement(QStringLiteral("win"));
                                            }
                                        }
                                    }
                                    botFightsExtra[id]=botFightExtra;
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id is already into the botFight extra, child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                            }
                            else
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, bot fights have not the id %4, child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(id));
                        }
                        else
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id is not a number to parse bot fight extra, child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    }
                }
                else
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                item = item.nextSiblingElement(QStringLiteral("fight"));
            }
            index_file++;
        }
    }

    QHashIterator<quint32,CatchChallenger::BotFight> i(CatchChallenger::CommonDatapack::commonDatapack.botFights);
    while(i.hasNext())
    {
        i.next();
        if(!botFightsExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            BotFightExtra botFightExtra;
            botFightExtra.start=tr("Ready for the fight?");
            botFightExtra.win=tr("You are so strong for me!");
            botFightsExtra[i.key()]=botFightExtra;
        }
    }

    qDebug() << QStringLiteral("%1 bot fight extra(s) loaded").arg(maps.size());
}
