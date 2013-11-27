#include "FightLoader.h"
#include "../base/DebugClass.h"
#include "../base/GeneralVariable.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QtCore/qmath.h>
#include <QDir>

using namespace CatchChallenger;

bool CatchChallenger::operator<(const Monster::AttackToLearn &entry1, const Monster::AttackToLearn &entry2)
{
    if(entry1.learnAtLevel!=entry2.learnAtLevel)
        return entry1.learnAtLevel < entry2.learnAtLevel;
    else
        return entry1.learnSkill < entry2.learnSkill;
}

QList<Type> FightLoader::loadTypes(const QString &file)
{
    QHash<QString,quint8> nameToId;
    QList<Type> types;
    //open and quick check the file
    QFile itemsFile(file);
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return types;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return types;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="types")
    {
        qDebug() << QString("Unable to open the file: %1, \"list\" root balise not found for the xml file").arg(itemsFile.fileName());
        return types;
    }

    //load the content
    bool ok;
    {
        QSet<QString> duplicate;
        QDomElement typeItem = root.firstChildElement("type");
        while(!typeItem.isNull())
        {
            if(typeItem.isElement())
            {
                if(typeItem.hasAttribute("name"))
                {
                    QString name=typeItem.attribute("name");
                    if(!duplicate.contains(name))
                    {
                        duplicate << name;
                        Type type;
                        type.name=name;
                        nameToId[type.name]=types.size();
                        types << type;
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, name is already set for type: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            typeItem = typeItem.nextSiblingElement("type");
        }
    }
    {
        QSet<QString> duplicate;
        QDomElement typeItem = root.firstChildElement("type");
        while(!typeItem.isNull())
        {
            if(typeItem.isElement())
            {
                if(typeItem.hasAttribute("name"))
                {
                    QString name=typeItem.attribute("name");
                    if(!duplicate.contains(name))
                    {
                        duplicate << name;
                        QDomElement multiplicator = typeItem.firstChildElement("multiplicator");
                        while(!multiplicator.isNull())
                        {
                            if(multiplicator.isElement())
                            {
                                if(multiplicator.hasAttribute("number") && multiplicator.hasAttribute("to"))
                                {
                                    float number=multiplicator.attribute("number").toFloat(&ok);
                                    QStringList to=multiplicator.attribute("to").split(";");
                                    if(ok && (number==2.0 || number==0.5 || number==0.0))
                                    {
                                        int index=0;
                                        while(index<to.size())
                                        {
                                            if(nameToId.contains(to.at(index)))
                                            {
                                                if(number==0)
                                                    types[nameToId[to.at(index)]].multiplicator[nameToId[to.at(index)]]=0;
                                                else if(number>1)
                                                    types[nameToId[to.at(index)]].multiplicator[nameToId[to.at(index)]]=number;
                                                else
                                                    types[nameToId[to.at(index)]].multiplicator[nameToId[to.at(index)]]=-(1.0/number);
                                            }
                                            else
                                                qDebug() << QString("Unable to open the file: %1, name is not into list: %4 is not found: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber()).arg(to.at(index));
                                            index++;
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to open the file: %1, name is already set for type: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                                }
                                else
                                    qDebug() << QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                            multiplicator = multiplicator.nextSiblingElement("multiplicator");
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, name is already set for type: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            typeItem = typeItem.nextSiblingElement("type");
        }
    }
    return types;
}

QHash<quint32,Monster> FightLoader::loadMonster(const QString &file, const QHash<quint32, Skill> &monsterSkills,const QList<Type> &types,const QHash<quint32, Item> &items)
{
    QHash<QString,quint8> typeNameToId;
    int index=0;
    while(index<types.size())
    {
        typeNameToId[types.at(index).name]=index;
        index++;
    }
    QHash<quint32,Monster> monsters;
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml monster file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return monsters;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return monsters;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return monsters;
    }

    //load the content
    bool ok,ok2;
    QDomElement item = root.firstChildElement("monster");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            bool attributeIsOk=true;
            if(!item.hasAttribute("id"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"id\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(!item.hasAttribute("egg_step"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"egg_step\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(!item.hasAttribute("xp_for_max_level") && !item.hasAttribute("xp_max"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"xp_for_max_level\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            else
            {
                if(!item.hasAttribute("xp_for_max_level"))
                    item.setAttribute("xp_for_max_level",item.attribute("xp_max"));
            }
            if(!item.hasAttribute("hp"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"hp\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(!item.hasAttribute("attack"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"attack\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(!item.hasAttribute("defense"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"defense\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(!item.hasAttribute("special_attack"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"special_attack\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(!item.hasAttribute("special_defense"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"special_defense\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(!item.hasAttribute("speed"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"speed\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(!item.hasAttribute("give_sp"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"give_sp\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(!item.hasAttribute("give_xp"))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster attribute \"give_xp\": child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(attributeIsOk)
            {
                Monster monster;
                quint32 id=item.attribute("id").toUInt(&ok);
                if(!ok)
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else if(monsters.contains(id))
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                    DebugClass::debugConsole(QString("monster loading: %1").arg(id));
                    #endif
                    if(item.hasAttribute("type"))
                    {
                        if(typeNameToId.contains(item.attribute("type")))
                            monster.type << typeNameToId[item.attribute("type")];
                        else
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, type not found into the list: %4 child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("type")));
                    }
                    if(item.hasAttribute("type2"))
                    {
                        if(typeNameToId.contains(item.attribute("type2")))
                            monster.type << typeNameToId[item.attribute("type2")];
                        else
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, type not found into the list: %4 child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("type")));
                    }
                    qreal pow=3;
                    if(ok)
                    {
                        if(item.hasAttribute("pow"))
                        {
                            pow=item.attribute("pow").toDouble(&ok);
                            if(!ok)
                            {
                                pow=3;
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, pow is not a double: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                ok=true;
                            }
                            if(pow<=1)
                            {
                                pow=3;
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, pow is too low: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            }
                            if(pow>=5)
                            {
                                pow=3;
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, pow is too hight: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            }
                        }
                    }
                    if(ok)
                    {
                        monster.egg_step=item.attribute("egg_step").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, egg_step is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.xp_for_max_level=item.attribute("xp_for_max_level").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, xp_for_max_level is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.stat.hp=item.attribute("hp").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, hp is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.stat.attack=item.attribute("attack").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.stat.defense=item.attribute("defense").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, defense is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.stat.special_attack=item.attribute("special_attack").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, special_attack is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.stat.special_defense=item.attribute("special_defense").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, special_defense is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.stat.speed=item.attribute("speed").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, speed is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.give_xp=item.attribute("give_xp").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, give_xp is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.give_sp=item.attribute("give_sp").toUInt(&ok);
                        if(!ok)
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, give_sp is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    if(ok)
                    {
                        if(item.hasAttribute("ratio_gender"))
                        {
                            QString ratio_gender=item.attribute("ratio_gender");
                            ratio_gender.remove("%");
                            monster.ratio_gender=ratio_gender.toInt(&ok2);
                            if(!ok2)
                            {
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, ratio_gender is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                monster.ratio_gender=50;
                            }
                            if(monster.ratio_gender<-1 || monster.ratio_gender>100)
                            {
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, ratio_gender is not in range of -1, 100: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                monster.ratio_gender=50;
                            }
                        }
                        monster.ratio_gender=50;
                    }
                    if(ok)
                    {
                        {
                            QDomElement attack_list = item.firstChildElement("attack_list");
                            if(!attack_list.isNull())
                            {
                                if(attack_list.isElement())
                                {
                                    QDomElement attack = attack_list.firstChildElement("attack");
                                    while(!attack.isNull())
                                    {
                                        if(attack.isElement())
                                        {
                                            if(attack.hasAttribute("level") && (attack.hasAttribute("skill") || attack.hasAttribute("id")))
                                            {
                                                ok=true;
                                                if(!attack.hasAttribute("skill"))
                                                    attack.setAttribute("skill",attack.attribute("id"));
                                                Monster::AttackToLearn attackVar;
                                                if(attack.hasAttribute("skill_level") || attack.hasAttribute("attack_level"))
                                                {
                                                    if(!attack.hasAttribute("skill_level"))
                                                        attack.setAttribute("skill_level",attack.attribute("attack_level"));
                                                    attackVar.learnSkillLevel=attack.attribute("skill_level").toUShort(&ok);
                                                    if(!ok)
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, skill_level is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()));
                                                }
                                                else
                                                    attackVar.learnSkillLevel=1;
                                                if(ok)
                                                {
                                                    attackVar.learnAtLevel=attack.attribute("level").toUShort(&ok);
                                                    if(!ok)
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, level is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()));
                                                }
                                                if(ok)
                                                {
                                                    attackVar.learnSkill=attack.attribute("skill").toUShort(&ok);
                                                    if(!ok)
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, skill is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()));
                                                }
                                                if(ok)
                                                {
                                                    if(!monsterSkills.contains(attackVar.learnSkill))
                                                    {
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack is not into attack loaded: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()));
                                                        ok=false;
                                                    }
                                                }
                                                if(ok)
                                                {
                                                    if(attackVar.learnSkillLevel<=0 || attackVar.learnSkillLevel>(quint32)monsterSkills[attackVar.learnSkill].level.size())
                                                    {
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack level is not in range 1-%5: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()).arg(monsterSkills[attackVar.learnSkill].level.size()));
                                                        ok=false;
                                                    }
                                                }
                                                if(ok)
                                                {
                                                    int index;
                                                    //if it's the first lean don't need previous learn
                                                    if(attackVar.learnSkillLevel>1)
                                                    {
                                                        index=0;
                                                        while(index<monster.learn.size())
                                                        {
                                                            if((monster.learn.at(index).learnSkillLevel-1)==attackVar.learnSkillLevel)
                                                                break;
                                                            index++;
                                                        }
                                                        if(index==monster.learn.size())
                                                        {
                                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack %4 with level %5 can't be added because not same attack with previous level: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()).arg(attackVar.learnSkill).arg(attackVar.learnSkillLevel));
                                                            ok=false;
                                                        }
                                                    }
                                                    //check if can learn
                                                    if(ok)
                                                    {
                                                        index=0;
                                                        while(index<monster.learn.size())
                                                        {
                                                            if(attackVar.learnSkillLevel>1 && (monster.learn.at(index).learnSkillLevel-1)==attackVar.learnSkillLevel)
                                                                ok=true;
                                                            if(monster.learn.at(index).learnSkillLevel==attackVar.learnSkillLevel && monster.learn.at(index).learnSkill==attackVar.learnSkill)
                                                            {
                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack already do for this level for skill %4 at level %5 for monster %6: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()).arg(attackVar.learnSkill).arg(attackVar.learnSkillLevel).arg(id));
                                                                ok=false;
                                                                break;
                                                            }
                                                            if(monster.learn.at(index).learnSkill==attackVar.learnSkill && monster.learn.at(index).learnSkillLevel==attackVar.learnSkillLevel)
                                                            {
                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, this attack level is already found %4, level: %5 for attack: %6: child.tagName(): %2 (at line: %3)")
                                                                                         .arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber())
                                                                                         .arg(attackVar.learnSkill).arg(attackVar.learnSkillLevel)
                                                                                         .arg(index)
                                                                                         );
                                                                ok=false;
                                                                break;
                                                            }
                                                            index++;
                                                        }
                                                    }
                                                }
                                                if(ok)
                                                    monster.learn<<attackVar;
                                                /*else
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, one of information is wrong: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()));*/
                                            }
                                            else
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, missing arguements (level or skill): child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()));
                                        }
                                        else
                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(attack.tagName()).arg(attack.lineNumber()));
                                        attack = attack.nextSiblingElement("attack");
                                    }
                                    qSort(monster.learn);
                                }
                                else
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            }
                            else
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not attack_list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        {
                            QDomElement evolutionsItem = item.firstChildElement("evolutions");
                            if(!evolutionsItem.isNull())
                            {
                                if(evolutionsItem.isElement())
                                {
                                    QDomElement evolutionItem = evolutionsItem.firstChildElement("evolution");
                                    while(!evolutionItem.isNull())
                                    {
                                        if(evolutionItem.isElement())
                                        {
                                            if(evolutionItem.hasAttribute("type") && (evolutionItem.hasAttribute("level") || evolutionItem.attribute("type")=="trade") && evolutionItem.hasAttribute("evolveTo"))
                                            {
                                                ok=true;
                                                Monster::Evolution evolutionVar;
                                                const QString &typeText=evolutionItem.attribute("type");
                                                if(typeText!="trade")
                                                {
                                                    if(typeText=="item")
                                                        evolutionVar.level=evolutionItem.attribute("level").toUInt(&ok);
                                                    else
                                                        evolutionVar.level=evolutionItem.attribute("level").toInt(&ok);
                                                    if(!ok)
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, level is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()));
                                                }
                                                else
                                                    evolutionVar.level=0;
                                                if(ok)
                                                {
                                                    evolutionVar.evolveTo=evolutionItem.attribute("evolveTo").toUInt(&ok);
                                                    if(!ok)
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, evolveTo is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()));
                                                }
                                                if(ok)
                                                {
                                                    if(typeText=="level")
                                                        evolutionVar.type=Monster::EvolutionType_Level;
                                                    else if(typeText=="item")
                                                        evolutionVar.type=Monster::EvolutionType_Item;
                                                    else if(typeText=="trade")
                                                        evolutionVar.type=Monster::EvolutionType_Trade;
                                                    else
                                                    {
                                                        ok=false;
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, unknown evolution type: %4 child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()).arg(typeText));
                                                    }
                                                }
                                                if(ok)
                                                {
                                                    if(typeText=="level" && (evolutionVar.level<0 || evolutionVar.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX))
                                                    {
                                                        ok=false;
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, level out of range: %4 child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()).arg(evolutionVar.level));
                                                    }
                                                }
                                                if(ok)
                                                {
                                                    if(typeText=="item")
                                                    {
                                                        if(!items.contains(evolutionVar.level))
                                                        {
                                                            ok=false;
                                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, unknown evolution item: %4 child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()).arg(evolutionVar.level));
                                                        }
                                                    }
                                                }
                                                if(ok)
                                                    monster.evolutions << evolutionVar;
                                            }
                                            else
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, missing arguements (level or skill): child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()));
                                        }
                                        else
                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()));
                                        evolutionItem = evolutionItem.nextSiblingElement("evolution");
                                    }
                                }
                                else
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            }
                        }
                        int index=0;
                        while(index<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                        {
                            quint64 xp_for_this_level=qPow(index+1,pow);
                            quint64 xp_for_max_level=monster.xp_for_max_level;
                            quint64 max_xp=qPow(CATCHCHALLENGER_MONSTER_LEVEL_MAX,pow);
                            monster.level_to_xp << xp_for_this_level*xp_for_max_level/max_xp;
                            index++;
                        }
                        #ifdef DEBUG_MESSAGE_MONSTER_XP_LOAD
                        index=0;
                        while(index<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                        {
                            int give_xp=(monster.give_xp*(index+1))/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                            DebugClass::debugConsole(QString("monster: %1, xp %2 for the level: %3, monster for this level: %4").arg(id).arg(index+1).arg(monster.level_to_xp.at(index)).arg(monster.level_to_xp.at(index)/give_xp));
                            index++;
                        }
                        DebugClass::debugConsole(QString("monster.level_to_xp.size(): %1").arg(monster.level_to_xp.size()));
                        #endif
                        if((monster.xp_for_max_level/monster.give_xp)>100)
                            DebugClass::debugConsole(QString("Warning: you need more than %1 monster(s) to pass the last level, prefer do that's with the rate for the monster id: %2").arg(monster.xp_for_max_level/monster.give_xp).arg(id));
                        monsters[id]=monster;
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, one of the attribute is wrong or is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                }
            }
            else
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("monster");
    }
    //check the evolveTo
    QHash<quint32,Monster>::iterator i = monsters.begin();
    while (i != monsters.end()) {
        int index=0;
        bool evolutionByLevel=false,evolutionByTrade=false;
        while(index<i.value().evolutions.size())
        {
            QSet<quint32> itemUse;
            if(i.value().evolutions.at(index).type==Monster::EvolutionType_Level)
            {
                if(evolutionByLevel)
                {
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, the monster %4 have already evolution by level: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(i.key()));
                    i.value().evolutions.removeAt(index);
                    continue;
                }
                evolutionByLevel=true;
            }
            if(i.value().evolutions.at(index).type==Monster::EvolutionType_Trade)
            {
                if(evolutionByTrade)
                {
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, the monster %4 have already evolution by trade: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(i.key()));
                    i.value().evolutions.removeAt(index);
                    continue;
                }
                evolutionByTrade=true;
            }
            if(i.value().evolutions.at(index).type==Monster::EvolutionType_Item)
            {
                if(itemUse.contains(i.value().evolutions.at(index).level))
                {
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, the monster %4 have already evolution with this item: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(i.key()));
                    i.value().evolutions.removeAt(index);
                    continue;
                }
                itemUse << i.value().evolutions.at(index).level;
            }
            if(i.value().evolutions.at(index).evolveTo==i.key())
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, the monster %4 can't evolve into them self: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(i.key()));
                i.value().evolutions.removeAt(index);
                continue;
            }
            else if(!monsters.contains(i.value().evolutions.at(index).evolveTo))
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, the monster %4 for the evolution of %5 can't be found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(i.value().evolutions.at(index).evolveTo).arg(i.key()));
                i.value().evolutions.removeAt(index);
                continue;
            }
            index++;
        }
        ++i;
    }
    return monsters;
}

QHash<quint32, QSet<quint32> > FightLoader::loadMonsterEvolutionItems(const QHash<quint32,Monster> &monsters)
{
    QHash<quint32, QSet<quint32> > evolutionItem;
    QHash<quint32,Monster>::const_iterator i = monsters.constBegin();
    while (i != monsters.constEnd()) {
        int index=0;
        while(index<i.value().evolutions.size())
        {
            if(i.value().evolutions.at(index).type==Monster::EvolutionType_Item)
                evolutionItem[i.value().evolutions.at(index).level] << i.key();
            index++;
        }
        ++i;
    }
    return evolutionItem;
}

QList<PlayerMonster::PlayerSkill> FightLoader::loadDefaultAttack(const quint32 &monsterId,const quint8 &level, const QHash<quint32,Monster> &monsters, const QHash<quint32, Skill> &monsterSkills)
{
    QList<CatchChallenger::PlayerMonster::PlayerSkill> skills;
    QList<CatchChallenger::Monster::AttackToLearn> attack=monsters[monsterId].learn;
    int index=0;
    while(index<attack.size())
    {
        if(attack[index].learnAtLevel<=level)
        {
            CatchChallenger::PlayerMonster::PlayerSkill temp;
            temp.level=attack[index].learnSkillLevel;
            temp.skill=attack[index].learnSkill;
            temp.endurance=0;
            if(monsterSkills.contains(temp.skill))
                if(temp.level<=monsterSkills[temp.skill].level.size() && temp.level>0)
                    temp.endurance=monsterSkills[temp.skill].level.at(temp.level-1).endurance;
            skills << temp;
        }
        index++;
    }
    while(skills.size()>4)
        skills.removeFirst();
    return skills;
}

QHash<quint32,BotFight> FightLoader::loadFight(const QString &folder, const QHash<quint32,Monster> &monsters, const QHash<quint32, Skill> &monsterSkills)
{
    QHash<quint32,BotFight> botFightList;
    QDir dir(folder);
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
                DebugClass::debugConsole(QString("Unable to open the xml fight file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
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
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
                index_file++;
                continue;
            }
            QDomElement root = domDocument.documentElement();
            if(root.tagName()!="fights")
            {
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"fights\" root balise not found for the xml file").arg(xmlFile.fileName()));
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
                        if(ok)
                        {
                            bool entryValid=true;
                            CatchChallenger::BotFight botFight;
                            botFight.cash=0;
                            {
                                QDomElement monster = item.firstChildElement("monster");
                                while(entryValid && !monster.isNull())
                                {
                                    if(!monster.hasAttribute("id"))
                                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"id\": bot.tagName(): %1 (at line: %2)").arg(monster.tagName()).arg(monster.lineNumber()));
                                    else if(!monster.isElement())
                                        CatchChallenger::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(monster.tagName().arg(monster.attribute("type")).arg(monster.lineNumber())));
                                    else
                                    {
                                        CatchChallenger::BotFight::BotFightMonster botFightMonster;
                                        botFightMonster.level=1;
                                        botFightMonster.id=monster.attribute("id").toUInt(&ok);
                                        if(ok)
                                        {
                                            if(!monsters.contains(botFightMonster.id))
                                            {
                                                entryValid=false;
                                                CatchChallenger::DebugClass::debugConsole(QString("Monster not found into the monster list: %1 into the file %2 (line %3)").arg(botFightMonster.id).arg(xmlFile.fileName()).arg(monster.lineNumber()));
                                                break;
                                            }
                                            if(monster.hasAttribute("level"))
                                            {
                                                botFightMonster.level=monster.attribute("level").toUShort(&ok);
                                                if(!ok)
                                                {
                                                    CatchChallenger::DebugClass::debugConsole(QString("The level is not a number: bot.tagName(): %1, type: %2 (at line: %3)").arg(monster.tagName().arg(monster.attribute("type")).arg(monster.lineNumber())));
                                                    botFightMonster.level=1;
                                                }
                                                if(botFightMonster.level<1)
                                                {
                                                    CatchChallenger::DebugClass::debugConsole(QString("Can't be 0 or negative: bot.tagName(): %1, type: %2 (at line: %3)").arg(monster.tagName().arg(monster.attribute("type")).arg(monster.lineNumber())));
                                                    botFightMonster.level=1;
                                                }
                                            }
                                            QDomElement attack = monster.firstChildElement("attack");
                                            while(entryValid && !attack.isNull())
                                            {
                                                quint8 attackLevel=1;
                                                if(!attack.hasAttribute("id"))
                                                    CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(attack.tagName()).arg(attack.lineNumber()));
                                                else if(!attack.isElement())
                                                    CatchChallenger::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName().arg(attack.attribute("type")).arg(attack.lineNumber())));
                                                else
                                                {
                                                    quint32 attackId=attack.attribute("id").toUInt(&ok);
                                                    if(ok)
                                                    {
                                                        if(!monsterSkills.contains(attackId))
                                                        {
                                                            entryValid=false;
                                                            CatchChallenger::DebugClass::debugConsole(QString("Monster attack not found: %1 into the file %2 (line %3)").arg(attackId).arg(xmlFile.fileName()).arg(monster.lineNumber()));
                                                            break;
                                                        }
                                                        if(attack.hasAttribute("level"))
                                                        {
                                                            attackLevel=attack.attribute("level").toUShort(&ok);
                                                            if(!ok)
                                                            {
                                                                CatchChallenger::DebugClass::debugConsole(QString("The level is not a number: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName()).arg(attack.attribute("type")).arg(attack.lineNumber()));
                                                                entryValid=false;
                                                                break;
                                                            }
                                                            if(attackLevel<1)
                                                            {
                                                                CatchChallenger::DebugClass::debugConsole(QString("Can't be 0 or negative: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName()).arg(attack.attribute("type")).arg(attack.lineNumber()));
                                                                entryValid=false;
                                                                break;
                                                            }
                                                        }
                                                        if(attackLevel>monsterSkills[attackId].level.size())
                                                        {
                                                            CatchChallenger::DebugClass::debugConsole(QString("Level out of range: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName()).arg(attack.attribute("type")).arg(attack.lineNumber()));
                                                            entryValid=false;
                                                            break;
                                                        }
                                                        CatchChallenger::PlayerMonster::PlayerSkill botFightAttack;
                                                        botFightAttack.skill=attackId;
                                                        botFightAttack.level=attackLevel;
                                                        botFightMonster.attacks << botFightAttack;
                                                    }
                                                }
                                                attack = attack.nextSiblingElement("attack");
                                            }
                                            if(botFightMonster.attacks.isEmpty())
                                                botFightMonster.attacks=loadDefaultAttack(botFightMonster.id,botFightMonster.level,monsters,monsterSkills);
                                            if(botFightMonster.attacks.isEmpty())
                                            {
                                                CatchChallenger::DebugClass::debugConsole(QString("Empty attack list: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName()).arg(attack.attribute("type")).arg(attack.lineNumber()));
                                                entryValid=false;
                                                break;
                                            }
                                            botFight.monsters << botFightMonster;
                                        }
                                    }
                                    monster = monster.nextSiblingElement("monster");
                                }
                            }
                            {
                                QDomElement gain = item.firstChildElement("gain");
                                while(entryValid && !gain.isNull())
                                {
                                    if(!gain.hasAttribute("cash"))
                                        CatchChallenger::DebugClass::debugConsole(QString("unknown fight gain: bot.tagName(): %1 (at line: %2)").arg(gain.tagName()).arg(gain.lineNumber()));
                                    else if(!gain.isElement())
                                        CatchChallenger::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(gain.tagName().arg(gain.attribute("type")).arg(gain.lineNumber())));
                                    else
                                    {
                                        quint32 cash=gain.attribute("cash").toUInt(&ok);
                                        if(ok)
                                            botFight.cash+=cash;
                                        else
                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, unknow cash text: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    }
                                    gain = gain.nextSiblingElement("gain");
                                }
                            }
                            if(entryValid)
                            {
                                if(!botFightList.contains(id))
                                {
                                    if(!botFight.monsters.isEmpty())
                                        botFightList[id]=botFight;
                                    else
                                        DebugClass::debugConsole(QString("Monster list is empty to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                }
                                else
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            }
                        }
                        else
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                }
                else
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                item = item.nextSiblingElement("fight");
            }
            index_file++;
        }
    }
    return botFightList;
}

QHash<quint32,Skill> FightLoader::loadMonsterSkill(const QString &file, const QHash<quint32, Buff> &monsterBuffs, const QList<Type> &types)
{
    QHash<QString,quint8> typeNameToId;
    int index=0;
    while(index<types.size())
    {
        typeNameToId[types.at(index).name]=index;
        index++;
    }
    QHash<quint32,Skill> monsterSkills;
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml skill monster file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return monsterSkills;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return monsterSkills;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return monsterSkills;
    }

    //load the content
    bool ok,ok2;
    QDomElement item = root.firstChildElement("skill");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toUInt(&ok);
                if(ok && monsterSkills.contains(id))
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else if(ok)
                {
                    QHash<quint8,Skill::SkillList> levelDef;
                    QDomElement effect = item.firstChildElement("effect");
                    if(!effect.isNull())
                    {
                        if(effect.isElement())
                        {
                            QDomElement level = effect.firstChildElement("level");
                            while(!level.isNull())
                            {
                                if(level.isElement())
                                {
                                    if(level.hasAttribute("number"))
                                    {
                                        quint32 sp=0;
                                        if(level.hasAttribute("sp"))
                                        {
                                            sp=level.attribute("sp").toUShort(&ok);
                                            if(!ok)
                                            {
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, sp is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(level.tagName()).arg(level.lineNumber()));
                                                sp=0;
                                            }
                                        }
                                        quint8 endurance=40;
                                        if(level.hasAttribute("endurance"))
                                        {
                                            endurance=level.attribute("endurance").toUShort(&ok);
                                            if(!ok)
                                            {
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, endurance is not number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(level.tagName()).arg(level.lineNumber()));
                                                endurance=40;
                                            }
                                            if(endurance<1)
                                            {
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, endurance lower than 1: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(level.tagName()).arg(level.lineNumber()));
                                                endurance=40;
                                            }
                                        }
                                        quint8 number;
                                        if(ok)
                                            number=level.attribute("number").toUShort(&ok);
                                        if(ok)
                                        {
                                            levelDef[number].sp_to_learn=sp;
                                            levelDef[number].endurance=endurance;
                                            if(number>0)
                                            {
                                                {
                                                    QDomElement life = level.firstChildElement("life");
                                                    while(!life.isNull())
                                                    {
                                                        if(life.isElement())
                                                        {
                                                            Skill::Life effect;
                                                            if(life.hasAttribute("applyOn"))
                                                            {
                                                                if(life.attribute("applyOn")=="aloneEnemy")
                                                                    effect.effect.on=ApplyOn_AloneEnemy;
                                                                else if(life.attribute("applyOn")=="themself")
                                                                    effect.effect.on=ApplyOn_Themself;
                                                                else if(life.attribute("applyOn")=="allEnemy")
                                                                    effect.effect.on=ApplyOn_AllEnemy;
                                                                else if(life.attribute("applyOn")=="allAlly")
                                                                    effect.effect.on=ApplyOn_AllAlly;
                                                                else if(life.attribute("applyOn")=="nobody")
                                                                    effect.effect.on=ApplyOn_Nobody;
                                                                else
                                                                {
                                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, applyOn tag wrong %4: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(life.attribute("applyOn")));
                                                                    effect.effect.on=ApplyOn_AloneEnemy;
                                                                }
                                                            }
                                                            else
                                                                effect.effect.on=ApplyOn_AloneEnemy;
                                                            QString text;
                                                            if(life.hasAttribute("quantity"))
                                                                text=life.attribute("quantity");
                                                            if(text.endsWith("%"))
                                                                effect.effect.type=QuantityType_Percent;
                                                            else
                                                                effect.effect.type=QuantityType_Quantity;
                                                            text.remove("%");
                                                            text.remove("+");
                                                            effect.effect.quantity=text.toInt(&ok);
                                                            effect.success=100;
                                                            if(life.hasAttribute("success"))
                                                            {
                                                                QString success=life.attribute("success");
                                                                success.remove("%");
                                                                effect.success=success.toUShort(&ok2);
                                                                if(!ok2)
                                                                {
                                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, success wrong corrected to 100%: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                                    effect.success=100;
                                                                }
                                                            }
                                                            if(ok)
                                                                levelDef[number].life << effect;
                                                            else
                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, %4 is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(text));
                                                        }
                                                        life = life.nextSiblingElement("life");
                                                    }
                                                }
                                                {
                                                    QDomElement buff = level.firstChildElement("buff");
                                                    while(!buff.isNull())
                                                    {
                                                        if(buff.isElement())
                                                        {
                                                            if(buff.hasAttribute("id"))
                                                            {
                                                                quint32 idBuff=buff.attribute("id").toUInt(&ok);
                                                                if(ok)
                                                                {
                                                                    Skill::Buff effect;
                                                                    if(buff.hasAttribute("applyOn"))
                                                                    {
                                                                        if(buff.attribute("applyOn")=="aloneEnemy")
                                                                            effect.effect.on=ApplyOn_AloneEnemy;
                                                                        else if(buff.attribute("applyOn")=="themself")
                                                                            effect.effect.on=ApplyOn_Themself;
                                                                        else if(buff.attribute("applyOn")=="allEnemy")
                                                                            effect.effect.on=ApplyOn_AllEnemy;
                                                                        else if(buff.attribute("applyOn")=="allAlly")
                                                                            effect.effect.on=ApplyOn_AllAlly;
                                                                        else if(buff.attribute("applyOn")=="nobody")
                                                                            effect.effect.on=ApplyOn_Nobody;
                                                                        else
                                                                        {
                                                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, applyOn tag wrong %4: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(buff.attribute("applyOn")));
                                                                            effect.effect.on=ApplyOn_AloneEnemy;
                                                                        }
                                                                    }
                                                                    else
                                                                        effect.effect.on=ApplyOn_AloneEnemy;
                                                                    if(!monsterBuffs.contains(idBuff))
                                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, this buff id is not found: %4: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).at(idBuff));
                                                                    else
                                                                    {
                                                                        effect.effect.level=1;
                                                                        ok2=true;
                                                                        if(buff.hasAttribute("level"))
                                                                        {
                                                                            QString level=buff.attribute("level");
                                                                            effect.effect.level=level.toUShort(&ok2);
                                                                            if(!ok2)
                                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, level wrong: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(buff.attribute("level")));
                                                                            if(level<=0)
                                                                            {
                                                                                ok2=false;
                                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, level need be egal or greater than 1: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                                            }
                                                                        }
                                                                        if(ok2)
                                                                        {
                                                                            if(monsterBuffs[idBuff].level.size()<effect.effect.level)
                                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, level needed: %4, level max found: %5: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(effect.effect.level).arg(monsterBuffs[idBuff].level.size()));
                                                                            else
                                                                            {
                                                                                effect.effect.buff=idBuff;
                                                                                effect.success=100;
                                                                                if(buff.hasAttribute("success"))
                                                                                {
                                                                                    QString success=buff.attribute("success");
                                                                                    success.remove("%");
                                                                                    effect.success=success.toUShort(&ok2);
                                                                                    if(!ok2)
                                                                                    {
                                                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, success wrong corrected to 100%: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                                                        effect.success=100;
                                                                                    }
                                                                                }
                                                                                levelDef[number].buff << effect;
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                                else
                                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not tag id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                            }
                                                            else
                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not tag id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                        }
                                                        buff = buff.nextSiblingElement("buff");
                                                    }
                                                }
                                            }
                                            else
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, level need be egal or greater than 1: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                        else
                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, number tag is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    }
                                }
                                else
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, level balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                level = level.nextSiblingElement("level");
                            }
                            if(levelDef.size()==0)
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, 0 level found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            else
                            {
                                monsterSkills[id].type=255;
                                if(item.hasAttribute("type"))
                                {
                                    if(typeNameToId.contains(item.attribute("type")))
                                        monsterSkills[id].type=typeNameToId[item.attribute("type")];
                                    else
                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, type not found: %4: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("type")));
                                }
                            }
                            //order by level to learn
                            quint8 index=1;
                            while(levelDef.contains(index))
                            {
                                if(levelDef[index].buff.empty() && levelDef[index].life.empty())
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, no effect loaded for skill %4 at level %5, missing level to continue: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(id).arg(index));
                                monsterSkills[id].level << levelDef[index];
                                levelDef.remove(index);
                                #ifdef DEBUG_MESSAGE_SKILL_LOAD
                                DebugClass::debugConsole(QString("for the level %1 of skill %2 have %3 effect(s) in buff and %4 effect(s) in life").arg(index).arg(id).arg(GlobalData::serverPrivateVariables.monsterSkills[id].level.at(index-1).buff.size()).arg(GlobalData::serverPrivateVariables.monsterSkills[id].level.at(index-1).life.size()));
                                #endif
                                index++;
                            }
                            if(levelDef.size()>0)
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, level up to %4 loaded, missing level to continue: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(index));
                            #ifdef DEBUG_MESSAGE_SKILL_LOAD
                            else
                                DebugClass::debugConsole(QString("%1 level(s) loaded for skill %2").arg(index-1).arg(id));
                            #endif
                        }
                        else
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not effect balise: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the skill id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("skill");
    }
    //check the default attack
    if(!monsterSkills.contains(0))
        DebugClass::debugConsole(QString("Warning: no default monster attack if no more attack"));
    else if(monsterSkills[0].level.isEmpty())
    {
        monsterSkills.remove(0);
        DebugClass::debugConsole(QString("Warning: no level for default monster attack if no more attack"));
    }
    else
    {
        if(monsterSkills[0].level.first().life.isEmpty())
        {
            monsterSkills.remove(0);
            DebugClass::debugConsole(QString("Warning: no life effect for the default attack"));
        }
        else
        {
            int index=0;
            while(index<monsterSkills[0].level.first().life.size())
            {
                const Skill::Life &life=monsterSkills[0].level.first().life.at(index);
                if(life.success==100 && life.effect.on==ApplyOn_AloneEnemy && life.effect.quantity<0)
                    break;
                index++;
            }
            if(index==monsterSkills[0].level.first().life.size())
            {
                monsterSkills.remove(0);
                DebugClass::debugConsole(QString("Warning: no valid life effect for the default attack: success=100%, on=ApplyOn_AloneEnemy, quantity<0"));
            }
        }
    }
    return monsterSkills;
}

QHash<quint32,Buff> FightLoader::loadMonsterBuff(const QString &file)
{
    QHash<quint32,Buff> monsterBuffs;
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml buff monster file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return monsterBuffs;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return monsterBuffs;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return monsterBuffs;
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
                if(ok && monsterBuffs.contains(id))
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else if(ok)
                {
                    Buff::Duration duration=Buff::Duration_ThisFight;
                    quint8 durationNumberOfTurn=0;
                    if(item.hasAttribute("duration"))
                    {
                        if(item.attribute("duration")=="Always")
                            duration=Buff::Duration_Always;
                        else if(item.attribute("duration")=="NumberOfTurn")
                        {
                            if(item.hasAttribute("durationNumberOfTurn"))
                            {
                                durationNumberOfTurn=item.attribute("durationNumberOfTurn").toUShort(&ok);
                                if(!ok)
                                {
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, durationNumberOfTurn is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    durationNumberOfTurn=3;
                                }
                            }
                            else
                                durationNumberOfTurn=3;
                            duration=Buff::Duration_NumberOfTurn;
                        }
                        else if(item.attribute("duration")=="ThisFight")
                            duration=Buff::Duration_ThisFight;
                        else
                        {
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, attribute duration have wrong value \"%4\" is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("duration")));
                            duration=Buff::Duration_ThisFight;
                        }
                    }
                    QHash<quint8,Buff::GeneralEffect> levelDef;
                    QDomElement effect = item.firstChildElement("effect");
                    if(!effect.isNull())
                    {
                        if(effect.isElement())
                        {
                            QDomElement level = effect.firstChildElement("level");
                            while(!level.isNull())
                            {
                                if(level.isElement())
                                {
                                    if(level.hasAttribute("number"))
                                    {
                                        quint8 number=level.attribute("number").toUShort(&ok);
                                        if(ok)
                                        {
                                            if(number>0)
                                            {
                                                QDomElement inFight = level.firstChildElement("inFight");
                                                while(!inFight.isNull())
                                                {
                                                    if(inFight.isElement())
                                                    {
                                                        Buff::Effect effect;
                                                        QString text;
                                                        if(inFight.hasAttribute("hp"))
                                                        {
                                                            text=inFight.attribute("hp");
                                                            effect.on=Buff::Effect::EffectOn_HP;
                                                        }
                                                        else if(inFight.hasAttribute("defense"))
                                                        {
                                                            text=inFight.attribute("defense");
                                                            effect.on=Buff::Effect::EffectOn_Defense;
                                                        }
                                                        if(text.endsWith("%"))
                                                            effect.type=QuantityType_Percent;
                                                        else
                                                            effect.type=QuantityType_Quantity;
                                                        text.remove("%");
                                                        text.remove("+");
                                                        effect.quantity=text.toInt(&ok);
                                                        if(ok)
                                                            levelDef[number].fight << effect;
                                                        else
                                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, %4 is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(text));
                                                    }
                                                    inFight = inFight.nextSiblingElement("inFight");
                                                }
                                                QDomElement inWalk = level.firstChildElement("inWalk");
                                                while(!inWalk.isNull())
                                                {
                                                    if(inWalk.isElement())
                                                    {
                                                        if(inWalk.hasAttribute("steps"))
                                                        {
                                                            quint32 steps=inWalk.attribute("steps").toUInt(&ok);
                                                            if(ok)
                                                            {
                                                                Buff::EffectInWalk effect;
                                                                effect.steps=steps;
                                                                QString text;
                                                                if(inWalk.hasAttribute("hp"))
                                                                {
                                                                    text=inWalk.attribute("hp");
                                                                    effect.effect.on=Buff::Effect::EffectOn_HP;
                                                                }
                                                                else if(inWalk.hasAttribute("defense"))
                                                                {
                                                                    text=inWalk.attribute("defense");
                                                                    effect.effect.on=Buff::Effect::EffectOn_Defense;
                                                                }
                                                                else
                                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, not action found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                                if(text.endsWith("%"))
                                                                    effect.effect.type=QuantityType_Percent;
                                                                else
                                                                    effect.effect.type=QuantityType_Quantity;
                                                                text.remove("%");
                                                                text.remove("+");
                                                                effect.effect.quantity=text.toInt(&ok);
                                                                if(ok)
                                                                    levelDef[number].walk << effect;
                                                                else
                                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, %4 is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(text));
                                                            }
                                                            else
                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not tag steps: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                        }
                                                        else
                                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not tag steps: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                    }
                                                    inWalk = inWalk.nextSiblingElement("inWalk");
                                                }
                                            }
                                            else
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, level need be egal or greater than 1: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                        else
                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, number tag is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    }
                                }
                                else
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, level balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                level = level.nextSiblingElement("level");
                            }
                            quint8 index=1;
                            while(levelDef.contains(index))
                            {
                                if(levelDef[index].fight.empty() && levelDef[index].walk.empty())
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, no effect loaded for buff %4 at level %5, missing level to continue: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(id).arg(index));
                                monsterBuffs[id].level << levelDef[index];
                                monsterBuffs[id].duration=duration;
                                monsterBuffs[id].durationNumberOfTurn=durationNumberOfTurn;
                                levelDef.remove(index);
                                #ifdef DEBUG_MESSAGE_BUFF_LOAD
                                DebugClass::debugConsole(QString("for the level %1 of buff %2 have %3 effect(s) in fight and %4 effect(s) in walk").arg(index).arg(id).arg(GlobalData::serverPrivateVariables.monsterBuffs[id].level.at(index-1).fight.size()).arg(GlobalData::serverPrivateVariables.monsterBuffs[id].level.at(index-1).walk.size()));
                                #endif
                                index++;
                            }
                            if(levelDef.size()>0)
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, level up to %4 loaded, missing level to continue: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(index));
                            #ifdef DEBUG_MESSAGE_BUFF_LOAD
                            else
                                DebugClass::debugConsole(QString("%1 level(s) loaded for buff %2").arg(index-1).arg(id));
                            #endif
                        }
                        else
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not effet balise: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the buff id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("buff");
    }
    return monsterBuffs;
}

