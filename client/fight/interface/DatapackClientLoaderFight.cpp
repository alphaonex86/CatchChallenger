#include "../../base/interface/DatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/DebugClass.h"
#include "../../general/fight/FightLoader.h"

#include <QDomElement>
#include <QDomDocument>
#include <QFile>
#include <QByteArray>
#include <QDebug>

void DatapackClientLoader::parseBuff()
{
    CatchChallenger::FightEngine::fightEngine.monsterBuffs=CatchChallenger::FightLoader::loadMonsterBuff(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"buff.xml");

    qDebug() << QString("%1 monster buff(s) loaded").arg(CatchChallenger::FightEngine::fightEngine.monsterBuffs.size());
}

void DatapackClientLoader::parseSkills()
{
    CatchChallenger::FightEngine::fightEngine.monsterSkills=CatchChallenger::FightLoader::loadMonsterSkill(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"skill.xml",CatchChallenger::FightEngine::fightEngine.monsterBuffs);

    qDebug() << QString("%1 monster skill(s) loaded").arg(CatchChallenger::FightEngine::fightEngine.monsterSkills.size());
}

void DatapackClientLoader::parseMonsters()
{
    CatchChallenger::FightEngine::fightEngine.monsters=CatchChallenger::FightLoader::loadMonster(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml",CatchChallenger::FightEngine::fightEngine.monsterSkills);

    qDebug() << QString("%1 monster(s) loaded").arg(CatchChallenger::FightEngine::fightEngine.monsters.size());
}

void DatapackClientLoader::parseMonstersExtra()
{
    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml");
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
                    if(!CatchChallenger::FightEngine::fightEngine.monsters.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        CatchChallenger::FightEngine::MonsterExtra monsterExtraEntry;
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
                        monsterExtraEntry.small=QPixmap(QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("small.png"));
                        if(monsterExtraEntry.small.isNull())
                            monsterExtraEntry.small=QPixmap(":/images/monsters/default/small.png");
                        CatchChallenger::FightEngine::fightEngine.monsterExtra[id]=monsterExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("monster");
    }

    QHashIterator<quint32,CatchChallenger::Monster> i(CatchChallenger::FightEngine::fightEngine.monsters);
    while(i.hasNext())
    {
        i.next();
        if(!CatchChallenger::FightEngine::fightEngine.monsterExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            CatchChallenger::FightEngine::MonsterExtra monsterExtraEntry;
            monsterExtraEntry.name=tr("Unknown");
            monsterExtraEntry.description=tr("Unknown");
            monsterExtraEntry.front=QPixmap(":/images/monsters/default/front.png");
            monsterExtraEntry.back=QPixmap(":/images/monsters/default/back.png");
            monsterExtraEntry.small=QPixmap(":/images/monsters/default/small.png");
            CatchChallenger::FightEngine::fightEngine.monsterExtra[i.key()]=monsterExtraEntry;
        }
    }

    qDebug() << QString("%1 monster(s) extra loaded").arg(CatchChallenger::FightEngine::fightEngine.monsterExtra.size());
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
                    if(!CatchChallenger::FightEngine::fightEngine.monsterBuffs.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        CatchChallenger::FightEngine::MonsterExtra::Buff monsterBuffExtraEntry;
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
                        CatchChallenger::FightEngine::fightEngine.monsterBuffsExtra[id]=monsterBuffExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("buff");
    }

    QHashIterator<quint32,CatchChallenger::Monster::Buff> i(CatchChallenger::FightEngine::fightEngine.monsterBuffs);
    while(i.hasNext())
    {
        i.next();
        if(!CatchChallenger::FightEngine::fightEngine.monsterBuffsExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            CatchChallenger::FightEngine::MonsterExtra::Buff monsterBuffExtraEntry;
            monsterBuffExtraEntry.name=tr("Unknown");
            monsterBuffExtraEntry.description=tr("Unknown");
            CatchChallenger::FightEngine::fightEngine.monsterBuffsExtra[i.key()]=monsterBuffExtraEntry;
        }
    }

    qDebug() << QString("%1 buff(s) extra loaded").arg(CatchChallenger::FightEngine::fightEngine.monsterBuffsExtra.size());
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
                    if(!CatchChallenger::FightEngine::fightEngine.monsterSkills.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        CatchChallenger::FightEngine::MonsterExtra::Skill monsterSkillExtraEntry;
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
                        CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra[id]=monsterSkillExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("skill");
    }

    QHashIterator<quint32,CatchChallenger::Monster::Skill> i(CatchChallenger::FightEngine::fightEngine.monsterSkills);
    while(i.hasNext())
    {
        i.next();
        if(!CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            CatchChallenger::FightEngine::MonsterExtra::Skill monsterSkillExtraEntry;
            monsterSkillExtraEntry.name=tr("Unknown");
            monsterSkillExtraEntry.description=tr("Unknown");
            CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra[i.key()]=monsterSkillExtraEntry;
        }
    }

    qDebug() << QString("%1 skill(s) extra loaded").arg(CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra.size());
}
