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
    fightEngine.monsterBuffs=Pokecraft::FightLoader::loadMonsterBuff(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"buff.xml");

    qDebug() << QString("%1 monster buff(s) loaded").arg(fightEngine.monsterBuffs.size());
}

void DatapackClientLoader::parseSkills()
{
    fightEngine.monsterSkills=Pokecraft::FightLoader::loadMonsterSkill(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"skill.xml",fightEngine.monsterBuffs);

    qDebug() << QString("%1 monster skill(s) loaded").arg(fightEngine.monsterSkills.size());
}

void DatapackClientLoader::parseMonsters()
{
    fightEngine.monsters=Pokecraft::FightLoader::loadMonster(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml",fightEngine.monsterSkills);

    qDebug() << QString("%1 monster(s) loaded").arg(fightEngine.monsters.size());
}

void DatapackClientLoader::parseMonstersExtra()
{
    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        Pokecraft::DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        Pokecraft::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        Pokecraft::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
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
                    Pokecraft::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    if(!fightEngine.monsters.contains(id))
                        Pokecraft::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        Pokecraft::FightEngine::MonsterExtra monsterExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        Pokecraft::DebugClass::debugConsole(QString("monster extra loading: %1").arg(id));
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
                                Pokecraft::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
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
                                Pokecraft::DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
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
                        fightEngine.monsterExtra[id]=monsterExtraEntry;
                    }
                }
            }
            else
                Pokecraft::DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            Pokecraft::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("monster");
    }

    QHashIterator<quint32,Pokecraft::Monster> i(fightEngine.monsters);
    while(i.hasNext())
    {
        i.next();
        if(!fightEngine.monsterExtra.contains(i.key()))
        {
            Pokecraft::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            Pokecraft::FightEngine::MonsterExtra monsterExtraEntry;
            monsterExtraEntry.name=tr("Unknown");
            monsterExtraEntry.description=tr("Unknown");
            monsterExtraEntry.front=QPixmap(":/images/monsters/default/front.png");
            monsterExtraEntry.back=QPixmap(":/images/monsters/default/back.png");
            monsterExtraEntry.small=QPixmap(":/images/monsters/default/small.png");
            fightEngine.monsterExtra[i.key()]=monsterExtraEntry;
        }
    }

    qDebug() << QString("%1 monster(s) extra loaded").arg(fightEngine.monsterExtra.size());
}
