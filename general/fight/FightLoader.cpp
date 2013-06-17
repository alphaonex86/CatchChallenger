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

bool operator<(const Monster::AttackToLearn &entry1, const Monster::AttackToLearn &entry2)
{
    if(entry1.learnAtLevel!=entry2.learnAtLevel)
        return entry1.learnAtLevel < entry2.learnAtLevel;
    else
        return entry1.learnSkill < entry2.learnSkill;
}

QHash<quint32,Monster> FightLoader::loadMonster(const QString &file, const QHash<quint32, Skill> &monsterSkills)
{
    QHash<quint32,Monster> monsters;
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
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
            if(item.hasAttribute("id") && item.hasAttribute("egg_step") && item.hasAttribute("xp_for_max_level") && item.hasAttribute("hp") && item.hasAttribute("attack") && item.hasAttribute("defense")
                    && item.hasAttribute("special_attack") && item.hasAttribute("special_defense") && item.hasAttribute("speed") && item.hasAttribute("give_sp") && item.hasAttribute("give_xp"))
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
                            monster.ratio_gender=ratio_gender.toUInt(&ok2);
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
                                        if(attack.hasAttribute("level") && attack.hasAttribute("skill"))
                                        {
                                            Monster::AttackToLearn attackVar;
                                            if(attack.hasAttribute("skill_level"))
                                            {
                                                attackVar.learnSkillLevel=attack.attribute("skill_level").toUShort(&ok);
                                                if(!ok)
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, skill_level is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                attackVar.learnSkillLevel=1;
                                            if(ok)
                                            {
                                                attackVar.learnAtLevel=attack.attribute("level").toUShort(&ok);
                                                if(!ok)
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, level is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            if(ok)
                                            {
                                                attackVar.learnSkill=attack.attribute("skill").toUShort(&ok);
                                                if(!ok)
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, skill is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            if(ok)
                                            {
                                                if(!monsterSkills.contains(attackVar.learnSkill))
                                                {
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack is not into attack loaded: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                    ok=false;
                                                }
                                            }
                                            if(ok)
                                            {
                                                if(attackVar.learnSkillLevel<=0 || attackVar.learnSkillLevel>(quint32)monsterSkills[attackVar.learnSkill].level.size())
                                                {
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack level is not in range 1-%5: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(monsterSkills[attackVar.learnSkill].level.size()));
                                                    ok=false;
                                                }
                                            }
                                            if(ok)
                                            {
                                                int index=0;
                                                while(index<monster.learn.size())
                                                {
                                                    if(monster.learn.at(index).learnSkillLevel==attackVar.learnSkillLevel && monster.learn.at(index).learnSkill==attackVar.learnSkill)
                                                    {
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, attack already do for this level: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
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
                                            if(ok)
                                                monster.learn<<attackVar;
                                            else
                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, one of information is wrong: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                        else
                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, missing arguements: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    }
                                    else
                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    attack = attack.nextSiblingElement("attack");
                                }
                                qSort(monster.learn);
                            }
                            else
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        else
                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not effet balise: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
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
    return monsters;
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
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
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
                                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(monster.tagName()).arg(monster.lineNumber()));
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
                                                break;
                                            }
                                            if(!monster.hasAttribute("level"))
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
                                    {
                                        botFightList[id]=botFight;
                                    }
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

QHash<quint32,Skill> FightLoader::loadMonsterSkill(const QString &file, const QHash<quint32, Buff> &monsterBuffs)
{
    QHash<quint32,Skill> monsterSkills;
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
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
                                                sp=0;
                                        }
                                        quint8 number;
                                        if(ok)
                                            number=level.attribute("number").toUShort(&ok);
                                        if(ok)
                                        {
                                            levelDef[number].sp=sp;
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
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                level = level.nextSiblingElement("level");
                            }
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
                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not effet balise: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
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
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
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
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                level = level.nextSiblingElement("level");
                            }
                            quint8 index=1;
                            while(levelDef.contains(index))
                            {
                                if(levelDef[index].fight.empty() && levelDef[index].walk.empty())
                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, no effect loaded for buff %4 at level %5, missing level to continue: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(id).arg(index));
                                monsterBuffs[id].level << levelDef[index];
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

