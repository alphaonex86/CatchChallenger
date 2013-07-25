#include "../../base/interface/DatapackClientLoader.h"
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
    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml monster extra file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("monster");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toUInt(&ok);
                if(!ok)
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    if(!CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        DatapackClientLoader::MonsterExtra monsterExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        CatchChallenger::DebugClass::debugConsole(QString("monster extra loading: %1").arg(id));
                        #endif
                        QDomElement name = item.firstChildElement("name");
                        while(!name.isNull())
                        {
                            if(name.isElement() && name.hasAttribute("lang"))
                            {
                                if(name.attribute("lang")=="en")
                                    monsterExtraEntry.name=name.text();
                            }
                            else
                                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            name = name.nextSiblingElement("name");
                        }
                        QDomElement description = item.firstChildElement("description");
                        while(!description.isNull())
                        {
                            if(description.isElement() && description.hasAttribute("lang"))
                            {
                                if(description.attribute("lang")=="en")
                                    monsterExtraEntry.description=description.text();
                            }
                            else
                                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            description = description.nextSiblingElement("description");
                        }
                        if(monsterExtraEntry.name.isEmpty())
                            monsterExtraEntry.name=tr("Unknown");
                        if(monsterExtraEntry.description.isEmpty())
                            monsterExtraEntry.description=tr("Unknown");
                        monsterExtraEntry.front=QPixmap(QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("front.png"));
                        if(monsterExtraEntry.front.isNull())
                            monsterExtraEntry.front=QPixmap(":/images/monsters/default/front.png");
                        monsterExtraEntry.back=QPixmap(QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("back.png"));
                        if(monsterExtraEntry.back.isNull())
                            monsterExtraEntry.back=QPixmap(":/images/monsters/default/back.png");
                        monsterExtraEntry.thumb=QPixmap(QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("small.png"));
                        if(monsterExtraEntry.thumb.isNull())
                            monsterExtraEntry.thumb=QPixmap(":/images/monsters/default/small.png");
                        DatapackClientLoader::datapackLoader.monsterExtra[id]=monsterExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster id at monster extra: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("monster");
    }

    QHashIterator<quint32,CatchChallenger::Monster> i(CatchChallenger::CommonDatapack::commonDatapack.monsters);
    while(i.hasNext())
    {
        i.next();
        if(!DatapackClientLoader::datapackLoader.monsterExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            DatapackClientLoader::MonsterExtra monsterExtraEntry;
            monsterExtraEntry.name=tr("Unknown");
            monsterExtraEntry.description=tr("Unknown");
            monsterExtraEntry.front=QPixmap(":/images/monsters/default/front.png");
            monsterExtraEntry.back=QPixmap(":/images/monsters/default/back.png");
            monsterExtraEntry.thumb=QPixmap(":/images/monsters/default/small.png");
            DatapackClientLoader::datapackLoader.monsterExtra[i.key()]=monsterExtraEntry;
        }
    }

    qDebug() << QString("%1 monster(s) extra loaded").arg(DatapackClientLoader::datapackLoader.monsterExtra.size());
}

void DatapackClientLoader::parseBuffExtra()
{
    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"buff.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("buff");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toUInt(&ok);
                if(!ok)
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    if(!CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        DatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        CatchChallenger::DebugClass::debugConsole(QString("monster extra loading: %1").arg(id));
                        #endif
                        QDomElement name = item.firstChildElement("name");
                        while(!name.isNull())
                        {
                            if(name.isElement() && name.hasAttribute("lang"))
                            {
                                if(name.attribute("lang")=="en")
                                    monsterBuffExtraEntry.name=name.text();
                            }
                            else
                                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            name = name.nextSiblingElement("name");
                        }
                        QDomElement description = item.firstChildElement("description");
                        while(!description.isNull())
                        {
                            if(description.isElement() && description.hasAttribute("lang"))
                            {
                                if(description.attribute("lang")=="en")
                                    monsterBuffExtraEntry.description=description.text();
                            }
                            else
                                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            description = description.nextSiblingElement("description");
                        }
                        if(monsterBuffExtraEntry.name.isEmpty())
                            monsterBuffExtraEntry.name=tr("Unknown");
                        if(monsterBuffExtraEntry.description.isEmpty())
                            monsterBuffExtraEntry.description=tr("Unknown");
                        DatapackClientLoader::datapackLoader.monsterBuffsExtra[id]=monsterBuffExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the buff id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("buff");
    }

    QHashIterator<quint32,CatchChallenger::Buff> i(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs);
    while(i.hasNext())
    {
        i.next();
        if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            DatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
            monsterBuffExtraEntry.name=tr("Unknown");
            monsterBuffExtraEntry.description=tr("Unknown");
            DatapackClientLoader::datapackLoader.monsterBuffsExtra[i.key()]=monsterBuffExtraEntry;
        }
    }

    qDebug() << QString("%1 buff(s) extra loaded").arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.size());
}

void DatapackClientLoader::parseSkillsExtra()
{
    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"skill.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("skill");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toUInt(&ok);
                if(!ok)
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    if(!CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        DatapackClientLoader::MonsterExtra::Skill monsterSkillExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        CatchChallenger::DebugClass::debugConsole(QString("monster extra loading: %1").arg(id));
                        #endif
                        QDomElement name = item.firstChildElement("name");
                        while(!name.isNull())
                        {
                            if(name.isElement() && name.hasAttribute("lang"))
                            {
                                if(name.attribute("lang")=="en")
                                    monsterSkillExtraEntry.name=name.text();
                            }
                            else
                                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            name = name.nextSiblingElement("name");
                        }
                        QDomElement description = item.firstChildElement("description");
                        while(!description.isNull())
                        {
                            if(description.isElement() && description.hasAttribute("lang"))
                            {
                                if(description.attribute("lang")=="en")
                                    monsterSkillExtraEntry.description=description.text();
                            }
                            else
                                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            description = description.nextSiblingElement("description");
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
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the skill id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("skill");
    }

    QHashIterator<quint32,CatchChallenger::Skill> i(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills);
    while(i.hasNext())
    {
        i.next();
        if(!monsterSkillsExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            DatapackClientLoader::MonsterExtra::Skill monsterSkillExtraEntry;
            monsterSkillExtraEntry.name=tr("Unknown");
            monsterSkillExtraEntry.description=tr("Unknown");
            monsterSkillsExtra[i.key()]=monsterSkillExtraEntry;
        }
    }

    qDebug() << QString("%1 skill(s) extra loaded").arg(monsterSkillsExtra.size());
}

void DatapackClientLoader::parseBotFightsExtra()
{
    QDir dir(datapackPath+DATAPACK_BASE_PATH_FIGHT);
    QFileInfoList list=dir.entryInfoList(QStringList(),QDir::NoDotAndDotDot|QDir::Files);
    int index_file=0;
    while(index_file<list.size())
    {
        if(list.at(index_file).isFile())
        {
            //open and quick check the file
            QFile xmlFile(list.at(index_file).absoluteFilePath());
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
                index_file++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QDomDocument domDocument;
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
                index_file++;
                continue;
            }
            QDomElement root = domDocument.documentElement();
            if(root.tagName()!="fights")
            {
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"fights\" root balise not found for the xml file").arg(xmlFile.fileName()));
                index_file++;
                continue;
            }

            //load the content
            bool ok;
            QDomElement item = root.firstChildElement("fight");
            while(!item.isNull())
            {
                if(item.isElement())
                {
                    if(item.hasAttribute("id"))
                    {
                        quint32 id=item.attribute("id").toUInt(&ok);
                        if(ok && CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(id) && !botFightsExtra.contains(id))
                        {
                            BotFightExtra botFightExtra;
                            botFightExtra.start=tr("Ready for the fight?");
                            botFightExtra.win=tr("You are so strong for me!");
                            {
                                QDomElement start = item.firstChildElement("start");
                                while(!start.isNull())
                                {
                                    if(start.isElement() && start.hasAttribute("lang"))
                                    {
                                        if(start.attribute("lang")=="en")
                                            botFightExtra.start=start.text();
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    start = start.nextSiblingElement("start");
                                }
                                QDomElement win = item.firstChildElement("win");
                                while(!win.isNull())
                                {
                                    if(win.isElement() && win.hasAttribute("lang"))
                                    {
                                        if(win.attribute("lang")=="en")
                                            botFightExtra.win=win.text();
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    win = win.nextSiblingElement("win");
                                }
                            }
                            botFightsExtra[id]=botFightExtra;
                        }
                        else
                            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id is not a number, not into bot fight or already into the extra text: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                }
                else
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                item = item.nextSiblingElement("fight");
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
            CatchChallenger::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            BotFightExtra botFightExtra;
            botFightExtra.start=tr("Ready for the fight?");
            botFightExtra.win=tr("You are so strong for me!");
            botFightsExtra[i.key()]=botFightExtra;
        }
    }

    qDebug() << QString("%1 bot fight extra(s) loaded").arg(maps.size());
}
