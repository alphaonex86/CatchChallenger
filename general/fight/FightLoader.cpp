#include "FightLoader.h"
#include "../base/DebugClass.h"
#include "../base/GeneralVariable.h"
#include "../base/CommonSettings.h"
#include "../base/CommonDatapack.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QtCore/qmath.h>
#include <QDir>

using namespace CatchChallenger;

QString FightLoader::text_type=QLatin1String("type");
QString FightLoader::text_name=QLatin1String("name");
QString FightLoader::text_id=QLatin1String("id");
QString FightLoader::text_multiplicator=QLatin1String("multiplicator");
QString FightLoader::text_number=QLatin1String("number");
QString FightLoader::text_to=QLatin1String("to");
QString FightLoader::text_dotcoma=QLatin1String(";");
QString FightLoader::text_list=QLatin1String("list");
QString FightLoader::text_monster=QLatin1String("monster");
QString FightLoader::text_monsters=QLatin1String("monsters");
QString FightLoader::text_dotxml=QLatin1String(".xml");
QString FightLoader::text_skills=QLatin1String("skills");
QString FightLoader::text_buffs=QLatin1String("buffs");
QString FightLoader::text_egg_step=QLatin1String("egg_step");
QString FightLoader::text_xp_for_max_level=QLatin1String("xp_for_max_level");
QString FightLoader::text_xp_max=QLatin1String("xp_max");
QString FightLoader::text_hp=QLatin1String("hp");
QString FightLoader::text_attack=QLatin1String("attack");
QString FightLoader::text_defense=QLatin1String("defense");
QString FightLoader::text_special_attack=QLatin1String("special_attack");
QString FightLoader::text_special_defense=QLatin1String("special_defense");
QString FightLoader::text_speed=QLatin1String("speed");
QString FightLoader::text_give_sp=QLatin1String("give_sp");
QString FightLoader::text_give_xp=QLatin1String("give_xp");
QString FightLoader::text_catch_rate=QLatin1String("catch_rate");
QString FightLoader::text_type2=QLatin1String("type2");
QString FightLoader::text_pow=QLatin1String("pow");
QString FightLoader::text_ratio_gender=QLatin1String("ratio_gender");
QString FightLoader::text_percent=QLatin1String("%");
QString FightLoader::text_attack_list=QLatin1String("attack_list");
QString FightLoader::text_skill=QLatin1String("skill");
QString FightLoader::text_skill_level=QLatin1String("skill_level");
QString FightLoader::text_attack_level=QLatin1String("attack_level");
QString FightLoader::text_level=QLatin1String("level");
QString FightLoader::text_byitem=QLatin1String("byitem");
QString FightLoader::text_evolution=QLatin1String("evolution");
QString FightLoader::text_evolutions=QLatin1String("evolutions");
QString FightLoader::text_trade=QLatin1String("trade");
QString FightLoader::text_evolveTo=QLatin1String("evolveTo");
QString FightLoader::text_item=QLatin1String("item");
QString FightLoader::text_fights=QLatin1String("fights");
QString FightLoader::text_fight=QLatin1String("fight");
QString FightLoader::text_gain=QLatin1String("gain");
QString FightLoader::text_cash=QLatin1String("cash");
QString FightLoader::text_sp=QLatin1String("sp");
QString FightLoader::text_effect=QLatin1String("effect");
QString FightLoader::text_endurance=QLatin1String("endurance");
QString FightLoader::text_life=QLatin1String("life");
QString FightLoader::text_applyOn=QLatin1String("applyOn");
QString FightLoader::text_aloneEnemy=QLatin1String("aloneEnemy");
QString FightLoader::text_themself=QLatin1String("themself");
QString FightLoader::text_allEnemy=QLatin1String("allEnemy");
QString FightLoader::text_allAlly=QLatin1String("allAlly");
QString FightLoader::text_nobody=QLatin1String("nobody");
QString FightLoader::text_quantity=QLatin1String("quantity");
QString FightLoader::text_more=QLatin1String("+");
QString FightLoader::text_success=QLatin1String("success");
QString FightLoader::text_buff=QLatin1String("buff");
QString FightLoader::text_capture_bonus=QLatin1String("capture_bonus");
QString FightLoader::text_duration=QLatin1String("duration");
QString FightLoader::text_Always=QLatin1String("Always");
QString FightLoader::text_NumberOfTurn=QLatin1String("NumberOfTurn");
QString FightLoader::text_durationNumberOfTurn=QLatin1String("durationNumberOfTurn");
QString FightLoader::text_ThisFight=QLatin1String("ThisFight");
QString FightLoader::text_inFight=QLatin1String("inFight");
QString FightLoader::text_inWalk=QLatin1String("inWalk");
QString FightLoader::text_steps=QLatin1String("steps");

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
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return types;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return types;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("types"))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"types\" root balise not found for the xml file").arg(file);
        return types;
    }

    //load the content
    bool ok;
    {
        QSet<QString> duplicate;
        QDomElement typeItem = root.firstChildElement(FightLoader::text_type);
        while(!typeItem.isNull())
        {
            if(typeItem.isElement())
            {
                if(typeItem.hasAttribute(FightLoader::text_name))
                {
                    QString name=typeItem.attribute(FightLoader::text_name);
                    if(!duplicate.contains(name))
                    {
                        duplicate << name;
                        Type type;
                        type.name=name;
                        nameToId[type.name]=types.size();
                        types << type;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, name is already set for type: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            typeItem = typeItem.nextSiblingElement(FightLoader::text_type);
        }
    }
    {
        QSet<QString> duplicate;
        QDomElement typeItem = root.firstChildElement(FightLoader::text_type);
        while(!typeItem.isNull())
        {
            if(typeItem.isElement())
            {
                if(typeItem.hasAttribute(FightLoader::text_name))
                {
                    QString name=typeItem.attribute(FightLoader::text_name);
                    if(!duplicate.contains(name))
                    {
                        duplicate << name;
                        QDomElement multiplicator = typeItem.firstChildElement(FightLoader::text_multiplicator);
                        while(!multiplicator.isNull())
                        {
                            if(multiplicator.isElement())
                            {
                                if(multiplicator.hasAttribute(FightLoader::text_number) && multiplicator.hasAttribute(FightLoader::text_to))
                                {
                                    float number=multiplicator.attribute(FightLoader::text_number).toFloat(&ok);
                                    QStringList to=multiplicator.attribute(FightLoader::text_to).split(FightLoader::text_dotcoma);
                                    if(ok && (number==2.0 || number==0.5 || number==0.0))
                                    {
                                        int index=0;
                                        while(index<to.size())
                                        {
                                            if(nameToId.contains(to.at(index)))
                                            {
                                                const QString &typeName=to.at(index);
                                                if(number==0)
                                                    types[nameToId.value(name)].multiplicator[nameToId.value(typeName)]=0;
                                                else if(number>1)
                                                    types[nameToId.value(name)].multiplicator[nameToId.value(typeName)]=number;
                                                else
                                                    types[nameToId.value(name)].multiplicator[nameToId.value(typeName)]=-(1.0/number);
                                            }
                                            else
                                                qDebug() << QStringLiteral("Unable to open the file: %1, name is not into list: %4 is not found: child.tagName(): %2 (at line: %3)").arg(file).arg(multiplicator.tagName()).arg(multiplicator.lineNumber()).arg(to.at(index));
                                            index++;
                                        }
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, name is already set for type: child.tagName(): %2 (at line: %3)").arg(file).arg(multiplicator.tagName()).arg(multiplicator.lineNumber());
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(multiplicator.tagName()).arg(multiplicator.lineNumber());
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(multiplicator.tagName()).arg(multiplicator.lineNumber());
                            multiplicator = multiplicator.nextSiblingElement(FightLoader::text_multiplicator);
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, name is already set for type: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            typeItem = typeItem.nextSiblingElement(FightLoader::text_type);
        }
    }
    return types;
}

QHash<quint16,Monster> FightLoader::loadMonster(const QString &folder, const QHash<quint16, Skill> &monsterSkills,const QList<Type> &types,const QHash<quint16, Item> &items)
{
    QHash<quint16,Monster> monsters;
    QDir dir(folder);
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const QString &file=fileList.at(file_index).absoluteFilePath();
        if(!file.endsWith(FightLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        QHash<QString,quint8> typeNameToId;
        int index=0;
        while(index<types.size())
        {
            typeNameToId[types.at(index).name]=index;
            index++;
        }
        QDomDocument domDocument;
        //open and quick check the file
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
            #endif
            QFile xmlFile(file);
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml monster file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
                file_index++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=FightLoader::text_monsters)
        {
            //DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2;
        QDomElement item = root.firstChildElement(FightLoader::text_monster);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                bool attributeIsOk=true;
                if(!item.hasAttribute(FightLoader::text_id))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"id\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(FightLoader::text_egg_step))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"egg_step\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(FightLoader::text_xp_for_max_level) && !item.hasAttribute(FightLoader::text_xp_max))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"xp_for_max_level\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                else
                {
                    if(!item.hasAttribute(FightLoader::text_xp_for_max_level))
                        item.setAttribute(FightLoader::text_xp_for_max_level,item.attribute(FightLoader::text_xp_max));
                }
                if(!item.hasAttribute(FightLoader::text_hp))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"hp\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(FightLoader::text_attack))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"attack\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(FightLoader::text_defense))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"defense\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(FightLoader::text_special_attack))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"special_attack\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(FightLoader::text_special_defense))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"special_defense\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(FightLoader::text_speed))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"speed\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(FightLoader::text_give_sp))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"give_sp\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(FightLoader::text_give_xp))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"give_xp\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    attributeIsOk=false;
                }
                if(attributeIsOk)
                {
                    Monster monster;
                    monster.catch_rate=100;
                    quint32 id=item.attribute(FightLoader::text_id).toUInt(&ok);
                    if(!ok)
                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else if(monsters.contains(id))
                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        DebugClass::debugConsole(QStringLiteral("monster loading: %1").arg(id));
                        #endif
                        if(item.hasAttribute(FightLoader::text_catch_rate))
                        {
                            bool ok2;
                            quint32 catch_rate=item.attribute(FightLoader::text_catch_rate).toUInt(&ok2);
                            if(ok2)
                            {
                                if(catch_rate<=255)
                                    monster.catch_rate=catch_rate;
                                else
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, catch_rate is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("catch_rate")));
                            }
                            else
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, catch_rate is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("catch_rate")));
                        }
                        if(item.hasAttribute(FightLoader::text_type))
                        {
                            const QStringList &typeList=item.attribute(FightLoader::text_type).split(FightLoader::text_dotcoma);
                            int index=0;
                            while(index<typeList.size())
                            {
                                if(typeNameToId.contains(typeList.at(index)))
                                    monster.type << typeNameToId.value(typeList.at(index));
                                else
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, type not found into the list: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute(FightLoader::text_type)));
                                index++;
                            }
                        }
                        if(item.hasAttribute(FightLoader::text_type2))
                        {
                            const QStringList &typeList=item.attribute(FightLoader::text_type2).split(FightLoader::text_dotcoma);
                            int index=0;
                            while(index<typeList.size())
                            {
                                if(typeNameToId.contains(typeList.at(index)))
                                    monster.type << typeNameToId.value(typeList.at(index));
                                else
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, type not found into the list: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute(FightLoader::text_type2)));
                                index++;
                            }
                        }
                        qreal pow=1.0;
                        if(ok)
                        {
                            if(item.hasAttribute(FightLoader::text_pow))
                            {
                                pow=item.attribute(FightLoader::text_pow).toDouble(&ok);
                                if(!ok)
                                {
                                    pow=1.0;
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, pow is not a double: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    ok=true;
                                }
                                if(pow<=1.0)
                                {
                                    pow=1.0;
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, pow is too low: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                }
                                if(pow>=10.0)
                                {
                                    pow=1.0;
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, pow is too hight: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                }
                            }
                        }
                        pow=qPow(pow,CommonSettings::commonSettings.rates_xp_pow);
                        if(ok)
                        {
                            monster.egg_step=item.attribute(FightLoader::text_egg_step).toUInt(&ok);
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, egg_step is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.xp_for_max_level=item.attribute(FightLoader::text_xp_for_max_level).toUInt(&ok);
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, xp_for_max_level is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.stat.hp=item.attribute(FightLoader::text_hp).toUInt(&ok);
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, hp is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.stat.attack=item.attribute(FightLoader::text_attack).toUInt(&ok);
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attack is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.stat.defense=item.attribute(FightLoader::text_defense).toUInt(&ok);
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, defense is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.stat.special_attack=item.attribute(FightLoader::text_special_attack).toUInt(&ok);
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, special_attack is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.stat.special_defense=item.attribute(FightLoader::text_special_defense).toUInt(&ok);
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, special_defense is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.stat.speed=item.attribute(FightLoader::text_speed).toUInt(&ok);
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, speed is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.give_xp=item.attribute(FightLoader::text_give_xp).toUInt(&ok)*CommonSettings::commonSettings.rates_xp;
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, give_xp is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.give_sp=item.attribute(FightLoader::text_give_sp).toUInt(&ok)*CommonSettings::commonSettings.rates_xp;
                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, give_sp is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            if(item.hasAttribute(FightLoader::text_ratio_gender))
                            {
                                QString ratio_gender=item.attribute(FightLoader::text_ratio_gender);
                                ratio_gender.remove(FightLoader::text_percent);
                                monster.ratio_gender=ratio_gender.toInt(&ok2);
                                if(!ok2)
                                {
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, ratio_gender is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    monster.ratio_gender=50;
                                }
                                if(monster.ratio_gender<-1 || monster.ratio_gender>100)
                                {
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, ratio_gender is not in range of -1, 100: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    monster.ratio_gender=50;
                                }
                            }
                            monster.ratio_gender=50;
                        }
                        if(ok)
                        {
                            {
                                QDomElement attack_list = item.firstChildElement(FightLoader::text_attack_list);
                                if(!attack_list.isNull())
                                {
                                    if(attack_list.isElement())
                                    {
                                        QDomElement attack = attack_list.firstChildElement(FightLoader::text_attack);
                                        while(!attack.isNull())
                                        {
                                            if(attack.isElement())
                                            {
                                                if(attack.hasAttribute(FightLoader::text_skill) || attack.hasAttribute(FightLoader::text_id))
                                                {
                                                    ok=true;
                                                    if(!attack.hasAttribute(FightLoader::text_skill))
                                                        attack.setAttribute(FightLoader::text_skill,attack.attribute(FightLoader::text_id));
                                                    Monster::AttackToLearn attackVar;
                                                    if(attack.hasAttribute(FightLoader::text_skill_level) || attack.hasAttribute(FightLoader::text_attack_level))
                                                    {
                                                        if(!attack.hasAttribute(FightLoader::text_skill_level))
                                                            attack.setAttribute(FightLoader::text_skill_level,attack.attribute(FightLoader::text_attack_level));
                                                        attackVar.learnSkillLevel=attack.attribute(FightLoader::text_skill_level).toUShort(&ok);
                                                        if(!ok)
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, skill_level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()));
                                                    }
                                                    else
                                                        attackVar.learnSkillLevel=1;
                                                    if(ok)
                                                    {
                                                        attackVar.learnSkill=attack.attribute(FightLoader::text_skill).toUShort(&ok);
                                                        if(!ok)
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, skill is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()));
                                                    }
                                                    if(ok)
                                                    {
                                                        if(!monsterSkills.contains(attackVar.learnSkill))
                                                        {
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attack is not into attack loaded: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()));
                                                            ok=false;
                                                        }
                                                    }
                                                    if(ok)
                                                    {
                                                        if(attackVar.learnSkillLevel<=0 || attackVar.learnSkillLevel>(quint32)monsterSkills.value(attackVar.learnSkill).level.size())
                                                        {
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attack level is not in range 1-%5: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()).arg(monsterSkills.value(attackVar.learnSkill).level.size()));
                                                            ok=false;
                                                        }
                                                    }
                                                    if(attack.hasAttribute(FightLoader::text_level))
                                                    {
                                                        if(ok)
                                                        {
                                                            attackVar.learnAtLevel=attack.attribute(FightLoader::text_level).toUShort(&ok);
                                                            if(ok)
                                                            {
                                                                int index;
                                                                //if it's the first lean don't need previous learn
                                                                if(attackVar.learnSkillLevel>1)
                                                                {
                                                                    index=0;
                                                                    while(index<monster.learn.size())
                                                                    {
                                                                        if(monster.learn.at(index).learnSkillLevel==(attackVar.learnSkillLevel-1) && monster.learn.at(index).learnSkill==attackVar.learnSkill)
                                                                            break;
                                                                        index++;
                                                                    }
                                                                    if(index==monster.learn.size())
                                                                    {
                                                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attack %4 with level %5 can't be added because not same attack with previous level: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()).arg(attackVar.learnSkill).arg(attackVar.learnSkillLevel));
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
                                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attack already do for this level for skill %4 at level %5 for monster %6: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()).arg(attackVar.learnSkill).arg(attackVar.learnSkillLevel).arg(id));
                                                                            ok=false;
                                                                            break;
                                                                        }
                                                                        if(monster.learn.at(index).learnSkill==attackVar.learnSkill && monster.learn.at(index).learnSkillLevel==attackVar.learnSkillLevel)
                                                                        {
                                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, this attack level is already found %4, level: %5 for attack: %6: child.tagName(): %2 (at line: %3)")
                                                                                                     .arg(file).arg(attack.tagName()).arg(attack.lineNumber())
                                                                                                     .arg(attackVar.learnSkill).arg(attackVar.learnSkillLevel)
                                                                                                     .arg(index)
                                                                                                     );
                                                                            ok=false;
                                                                            break;
                                                                        }
                                                                        index++;
                                                                    }
                                                                    if(ok)
                                                                        monster.learn<<attackVar;
                                                                }
                                                                else
                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, no way to learn %4, level: %5 for attack: %6: child.tagName(): %2 (at line: %3)")
                                                                                             .arg(file).arg(attack.tagName()).arg(attack.lineNumber())
                                                                                             .arg(attackVar.learnSkill).arg(attackVar.learnSkillLevel)
                                                                                             .arg(index)
                                                                                             );
                                                            }
                                                            else
                                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()));
                                                        }
                                                    }
                                                    else
                                                    {
                                                        if(attack.hasAttribute(FightLoader::text_byitem))
                                                        {
                                                            quint32 itemId;
                                                            if(ok)
                                                            {
                                                                itemId=attack.attribute(FightLoader::text_byitem).toUShort(&ok);
                                                                if(!ok)
                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, item to learn is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()).arg(attack.attribute(FightLoader::text_byitem)));
                                                            }
                                                            if(ok)
                                                            {
                                                                if(!items.contains(itemId))
                                                                {
                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, item to learn not found %4: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()).arg(itemId));
                                                                    ok=false;
                                                                }
                                                            }
                                                            if(ok)
                                                            {
                                                                if(monster.learnByItem.contains(itemId))
                                                                {
                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, item to learn is already used %4: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()).arg(itemId));
                                                                    ok=false;
                                                                }
                                                            }
                                                            if(ok)
                                                            {
                                                                Monster::AttackToLearnByItem tempEntry;
                                                                tempEntry.learnSkill=attackVar.learnSkill;
                                                                tempEntry.learnSkillLevel=attackVar.learnSkillLevel;
                                                                monster.learnByItem[itemId]=tempEntry;
                                                            }
                                                        }
                                                        else
                                                        {
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level and byitem is not found: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()));
                                                            ok=false;
                                                        }
                                                    }
                                                }
                                                else
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, missing arguements (level or skill): child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()));
                                            }
                                            else
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()));
                                            attack = attack.nextSiblingElement(FightLoader::text_attack);
                                        }
                                        qSort(monster.learn);
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                }
                                else
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not attack_list: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                            }
                            {
                                QDomElement evolutionsItem = item.firstChildElement(FightLoader::text_evolutions);
                                if(!evolutionsItem.isNull())
                                {
                                    if(evolutionsItem.isElement())
                                    {
                                        QDomElement evolutionItem = evolutionsItem.firstChildElement(FightLoader::text_evolution);
                                        while(!evolutionItem.isNull())
                                        {
                                            if(evolutionItem.isElement())
                                            {
                                                if(evolutionItem.hasAttribute(FightLoader::text_type) && (
                                                       evolutionItem.hasAttribute(FightLoader::text_level) ||
                                                       (evolutionItem.attribute(FightLoader::text_type)==FightLoader::text_trade && evolutionItem.hasAttribute(FightLoader::text_evolveTo)) ||
                                                       (evolutionItem.attribute(FightLoader::text_type)==FightLoader::text_item && evolutionItem.hasAttribute(FightLoader::text_item))
                                                       )
                                                   )
                                                {
                                                    ok=true;
                                                    Monster::Evolution evolutionVar;
                                                    const QString &typeText=evolutionItem.attribute(FightLoader::text_type);
                                                    if(typeText!=FightLoader::text_trade)
                                                    {
                                                        if(typeText==FightLoader::text_item)
                                                            evolutionVar.level=evolutionItem.attribute(FightLoader::text_item).toUInt(&ok);
                                                        else
                                                            evolutionVar.level=evolutionItem.attribute(FightLoader::text_level).toInt(&ok);
                                                        if(!ok)
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()));
                                                    }
                                                    else
                                                        evolutionVar.level=0;
                                                    if(ok)
                                                    {
                                                        evolutionVar.evolveTo=evolutionItem.attribute(FightLoader::text_evolveTo).toUInt(&ok);
                                                        if(!ok)
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, evolveTo is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()));
                                                    }
                                                    if(ok)
                                                    {
                                                        if(typeText==FightLoader::text_level)
                                                            evolutionVar.type=Monster::EvolutionType_Level;
                                                        else if(typeText==FightLoader::text_item)
                                                            evolutionVar.type=Monster::EvolutionType_Item;
                                                        else if(typeText==FightLoader::text_trade)
                                                            evolutionVar.type=Monster::EvolutionType_Trade;
                                                        else
                                                        {
                                                            ok=false;
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, unknown evolution type: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()).arg(typeText));
                                                        }
                                                    }
                                                    if(ok)
                                                    {
                                                        if(typeText==FightLoader::text_level && (evolutionVar.level<0 || evolutionVar.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX))
                                                        {
                                                            ok=false;
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level out of range: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()).arg(evolutionVar.level));
                                                        }
                                                    }
                                                    if(ok)
                                                    {
                                                        if(typeText==FightLoader::text_item)
                                                        {
                                                            if(!items.contains(evolutionVar.level))
                                                            {
                                                                ok=false;
                                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, unknown evolution item: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()).arg(evolutionVar.level));
                                                            }
                                                        }
                                                    }
                                                    if(ok)
                                                        monster.evolutions << evolutionVar;
                                                }
                                                else
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, missing arguements (level or skill): child.tagName(): %2 (at line: %3)").arg(file).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()));
                                            }
                                            else
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(evolutionItem.tagName()).arg(evolutionItem.lineNumber()));
                                            evolutionItem = evolutionItem.nextSiblingElement(FightLoader::text_evolution);
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                }
                            }
                            int index=0;
                            while(index<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                quint64 xp_for_this_level=qPow(index+1,pow);
                                quint64 xp_for_max_level=monster.xp_for_max_level;
                                quint64 max_xp=qPow(CATCHCHALLENGER_MONSTER_LEVEL_MAX,pow);
                                quint64 tempXp=xp_for_this_level*xp_for_max_level/max_xp;
                                if(tempXp<1)
                                    tempXp=1;
                                monster.level_to_xp << tempXp;
                                index++;
                            }
                            #ifdef DEBUG_MESSAGE_MONSTER_XP_LOAD
                            index=0;
                            while(index<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                int give_xp=(monster.give_xp*(index+1))/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                                DebugClass::debugConsole(QStringLiteral("monster: %1, xp %2 for the level: %3, monster for this level: %4").arg(id).arg(index+1).arg(monster.level_to_xp.at(index)).arg(monster.level_to_xp.at(index)/give_xp));
                                index++;
                            }
                            DebugClass::debugConsole(QStringLiteral("monster.level_to_xp.size(): %1").arg(monster.level_to_xp.size()));
                            #endif
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(monster.give_xp!=0)
                                if((monster.xp_for_max_level*CommonSettings::commonSettings.rates_xp/monster.give_xp)>150)
                                    DebugClass::debugConsole(QStringLiteral("Warning: you need more than %1 monster(s) to pass the last level, prefer do that's with the rate for the monster id: %2").arg(monster.xp_for_max_level/monster.give_xp).arg(id));
                            #endif
                            monsters[id]=monster;
                        }
                        else
                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, one of the attribute is wrong or is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    }
                }
                else
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the monster id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            item = item.nextSiblingElement(FightLoader::text_monster);
        }
        //check the evolveTo
        QHash<quint16,Monster>::iterator i = monsters.begin();
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
                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, the monster %4 have already evolution by level: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(i.key()));
                        i.value().evolutions.removeAt(index);
                        continue;
                    }
                    evolutionByLevel=true;
                }
                if(i.value().evolutions.at(index).type==Monster::EvolutionType_Trade)
                {
                    if(evolutionByTrade)
                    {
                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, the monster %4 have already evolution by trade: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(i.key()));
                        i.value().evolutions.removeAt(index);
                        continue;
                    }
                    evolutionByTrade=true;
                }
                if(i.value().evolutions.at(index).type==Monster::EvolutionType_Item)
                {
                    if(itemUse.contains(i.value().evolutions.at(index).level))
                    {
                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, the monster %4 have already evolution with this item: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(i.key()));
                        i.value().evolutions.removeAt(index);
                        continue;
                    }
                    itemUse << i.value().evolutions.at(index).level;
                }
                if(i.value().evolutions.at(index).evolveTo==i.key())
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, the monster %4 can't evolve into them self: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(i.key()));
                    i.value().evolutions.removeAt(index);
                    continue;
                }
                else if(!monsters.contains(i.value().evolutions.at(index).evolveTo))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, the monster %4 for the evolution of %5 can't be found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(i.value().evolutions.at(index).evolveTo).arg(i.key()));
                    i.value().evolutions.removeAt(index);
                    continue;
                }
                index++;
            }
            ++i;
        }
        file_index++;
    }
    return monsters;
}

QHash<quint16/*item*/, QHash<quint16/*monster*/,quint16/*evolveTo*/> > FightLoader::loadMonsterEvolutionItems(const QHash<quint16,Monster> &monsters)
{
    QHash<quint16, QHash<quint16,quint16> > evolutionItem;
    QHash<quint16,Monster>::const_iterator i = monsters.constBegin();
    QHash<quint16,Monster>::const_iterator iEnd = monsters.constEnd();
    while (i != iEnd) {
        int index=0;
        while(index<i.value().evolutions.size())
        {
            if(i.value().evolutions.at(index).type==Monster::EvolutionType_Item)
                evolutionItem[i.value().evolutions.at(index).level][i.key()]=i.value().evolutions.at(index).evolveTo;
            index++;
        }
        ++i;
    }
    return evolutionItem;
}

QHash<quint16/*item*/, QSet<quint16/*monster*/> > FightLoader::loadMonsterItemToLearn(const QHash<quint16,Monster> &monsters,const QHash<quint16/*item*/, QHash<quint16/*monster*/,quint16/*evolveTo*/> > &evolutionItem)
{
    QHash<quint16/*item*/, QSet<quint16/*monster*/> > learnItem;
    QHash<quint16,Monster>::const_iterator i = monsters.constBegin();
    QHash<quint16,Monster>::const_iterator iEnd = monsters.constEnd();
    while (i != iEnd) {
        QHash<quint16/*item*/,Monster::AttackToLearnByItem/*skill*/>::const_iterator j = i.value().learnByItem.constBegin();
        QHash<quint16/*item*/,Monster::AttackToLearnByItem/*skill*/>::const_iterator jEnd = i.value().learnByItem.constEnd();
        while (j != jEnd) {
            if(!evolutionItem.contains(j.key()))
                learnItem[j.key()] << i.key();
            else
                DebugClass::debugConsole(QStringLiteral("The item %1 can't be used to learn because already used to evolv").arg(j.key()));
            ++j;
        }
        ++i;
    }
    return learnItem;
}

QList<PlayerMonster::PlayerSkill> FightLoader::loadDefaultAttack(const quint16 &monsterId,const quint8 &level, const QHash<quint16,Monster> &monsters, const QHash<quint16, Skill> &monsterSkills)
{
    QList<CatchChallenger::PlayerMonster::PlayerSkill> skills;
    QList<CatchChallenger::Monster::AttackToLearn> attack=monsters.value(monsterId).learn;
    int index=0;
    while(index<attack.size())
    {
        if(attack.at(index).learnAtLevel<=level)
        {
            CatchChallenger::PlayerMonster::PlayerSkill temp;
            temp.level=attack.at(index).learnSkillLevel;
            temp.skill=attack.at(index).learnSkill;
            temp.endurance=0;
            if(monsterSkills.contains(temp.skill))
                if(temp.level<=monsterSkills.value(temp.skill).level.size() && temp.level>0)
                    temp.endurance=monsterSkills.value(temp.skill).level.at(temp.level-1).endurance;
            skills << temp;
        }
        index++;
    }
    while(skills.size()>4)
        skills.removeFirst();
    return skills;
}

QHash<quint16,BotFight> FightLoader::loadFight(const QString &folder, const QHash<quint16,Monster> &monsters, const QHash<quint16, Skill> &monsterSkills, const QHash<quint16, Item> &items)
{
    QHash<quint16,BotFight> botFightList;
    QDir dir(folder);
    QFileInfoList list=dir.entryInfoList(QStringList(),QDir::NoDotAndDotDot|QDir::Files);
    int index_file=0;
    while(index_file<list.size())
    {
        if(list.at(index_file).isFile())
        {
            const QString &file=list.at(index_file).absoluteFilePath();
            QDomDocument domDocument;
            //open and quick check the file
            #ifndef EPOLLCATCHCHALLENGERSERVER
            if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
                domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
            else
            {
                #endif
                QFile xmlFile(file);
                QByteArray xmlContent;
                if(!xmlFile.open(QIODevice::ReadOnly))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml fight file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
                    index_file++;
                    continue;
                }
                xmlContent=xmlFile.readAll();
                xmlFile.close();
                QString errorStr;
                int errorLine,errorColumn;
                if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
                {
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
                    index_file++;
                    continue;
                }
                #ifndef EPOLLCATCHCHALLENGERSERVER
                CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
            }
            #endif
            QDomElement root = domDocument.documentElement();
            if(root.tagName()!=FightLoader::text_fights)
            {
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"fights\" root balise not found for the xml file").arg(file));
                index_file++;
                continue;
            }

            //load the content
            bool ok;
            QDomElement item = root.firstChildElement(FightLoader::text_fight);
            while(!item.isNull())
            {
                if(item.isElement())
                {
                    if(item.hasAttribute(FightLoader::text_id))
                    {
                        quint32 id=item.attribute(FightLoader::text_id).toUInt(&ok);
                        if(ok)
                        {
                            bool entryValid=true;
                            CatchChallenger::BotFight botFight;
                            botFight.cash=0;
                            {
                                QDomElement monster = item.firstChildElement(FightLoader::text_monster);
                                while(entryValid && !monster.isNull())
                                {
                                    if(!monster.hasAttribute(FightLoader::text_id))
                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"id\": bot.tagName(): %1 (at line: %2)").arg(monster.tagName()).arg(monster.lineNumber()));
                                    else if(!monster.isElement())
                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(monster.tagName().arg(monster.attribute(FightLoader::text_type)).arg(monster.lineNumber())));
                                    else
                                    {
                                        CatchChallenger::BotFight::BotFightMonster botFightMonster;
                                        botFightMonster.level=1;
                                        botFightMonster.id=monster.attribute(FightLoader::text_id).toUInt(&ok);
                                        if(ok)
                                        {
                                            if(!monsters.contains(botFightMonster.id))
                                            {
                                                entryValid=false;
                                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Monster not found into the monster list: %1 into the file %2 (line %3)").arg(botFightMonster.id).arg(file).arg(monster.lineNumber()));
                                                break;
                                            }
                                            if(monster.hasAttribute(FightLoader::text_level))
                                            {
                                                botFightMonster.level=monster.attribute(FightLoader::text_level).toUShort(&ok);
                                                if(!ok)
                                                {
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("The level is not a number: bot.tagName(): %1, type: %2 (at line: %3)").arg(monster.tagName().arg(monster.attribute(FightLoader::text_type)).arg(monster.lineNumber())));
                                                    botFightMonster.level=1;
                                                }
                                                if(botFightMonster.level<1)
                                                {
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Can't be 0 or negative: bot.tagName(): %1, type: %2 (at line: %3)").arg(monster.tagName().arg(monster.attribute(FightLoader::text_type)).arg(monster.lineNumber())));
                                                    botFightMonster.level=1;
                                                }
                                            }
                                            QDomElement attack = monster.firstChildElement(FightLoader::text_attack);
                                            while(entryValid && !attack.isNull())
                                            {
                                                quint8 attackLevel=1;
                                                if(!attack.hasAttribute(FightLoader::text_id))
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(attack.tagName()).arg(attack.lineNumber()));
                                                else if(!attack.isElement())
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName().arg(attack.attribute(FightLoader::text_type)).arg(attack.lineNumber())));
                                                else
                                                {
                                                    quint32 attackId=attack.attribute(FightLoader::text_id).toUInt(&ok);
                                                    if(ok)
                                                    {
                                                        if(!monsterSkills.contains(attackId))
                                                        {
                                                            entryValid=false;
                                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Monster attack not found: %1 into the file %2 (line %3)").arg(attackId).arg(file).arg(monster.lineNumber()));
                                                            break;
                                                        }
                                                        if(attack.hasAttribute(FightLoader::text_level))
                                                        {
                                                            attackLevel=attack.attribute(FightLoader::text_level).toUShort(&ok);
                                                            if(!ok)
                                                            {
                                                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("The level is not a number: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName()).arg(attack.attribute(FightLoader::text_type)).arg(attack.lineNumber()));
                                                                entryValid=false;
                                                                break;
                                                            }
                                                            if(attackLevel<1)
                                                            {
                                                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Can't be 0 or negative: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName()).arg(attack.attribute(FightLoader::text_type)).arg(attack.lineNumber()));
                                                                entryValid=false;
                                                                break;
                                                            }
                                                        }
                                                        if(attackLevel>monsterSkills.value(attackId).level.size())
                                                        {
                                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Level out of range: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName()).arg(attack.attribute(FightLoader::text_type)).arg(attack.lineNumber()));
                                                            entryValid=false;
                                                            break;
                                                        }
                                                        CatchChallenger::PlayerMonster::PlayerSkill botFightAttack;
                                                        botFightAttack.skill=attackId;
                                                        botFightAttack.level=attackLevel;
                                                        botFightMonster.attacks << botFightAttack;
                                                    }
                                                }
                                                attack = attack.nextSiblingElement(FightLoader::text_attack);
                                            }
                                            if(botFightMonster.attacks.isEmpty())
                                                botFightMonster.attacks=loadDefaultAttack(botFightMonster.id,botFightMonster.level,monsters,monsterSkills);
                                            if(botFightMonster.attacks.isEmpty())
                                            {
                                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Empty attack list: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName()).arg(attack.attribute(FightLoader::text_type)).arg(attack.lineNumber()));
                                                entryValid=false;
                                                break;
                                            }
                                            botFight.monsters << botFightMonster;
                                        }
                                    }
                                    monster = monster.nextSiblingElement(FightLoader::text_monster);
                                }
                            }
                            {
                                QDomElement gain = item.firstChildElement(FightLoader::text_gain);
                                while(entryValid && !gain.isNull())
                                {
                                    if(gain.isElement())
                                    {
                                        if(gain.hasAttribute(FightLoader::text_cash))
                                        {
                                            const quint32 &cash=gain.attribute(FightLoader::text_cash).toUInt(&ok)*CommonSettings::commonSettings.rates_gold;
                                            if(ok)
                                                botFight.cash+=cash;
                                            else
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, unknow cash text: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                        else if(gain.hasAttribute(FightLoader::text_item))
                                        {
                                            BotFight::Item itemVar;
                                            itemVar.quantity=1;
                                            itemVar.id=gain.attribute(FightLoader::text_item).toUInt(&ok);
                                            if(ok)
                                            {
                                                if(items.contains(itemVar.id))
                                                {
                                                    if(gain.hasAttribute(FightLoader::text_quantity))
                                                    {
                                                        itemVar.quantity=gain.attribute(FightLoader::text_quantity).toUInt(&ok);
                                                        if(!ok || itemVar.quantity<1)
                                                        {
                                                            itemVar.quantity=1;
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, quantity value is wrong: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                        }
                                                    }
                                                    botFight.items << itemVar;
                                                }
                                                else
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, item not found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, unknow item id text: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                        else
                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("unknown fight gain: bot.tagName(): %1 tag %2 (at line: %3)").arg(file).arg(gain.tagName()).arg(gain.lineNumber()));
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(gain.tagName().arg(gain.attribute(FightLoader::text_type)).arg(gain.lineNumber())));
                                    gain = gain.nextSiblingElement(FightLoader::text_gain);
                                }
                            }
                            if(entryValid)
                            {
                                if(!botFightList.contains(id))
                                {
                                    if(!botFight.monsters.isEmpty())
                                        botFightList[id]=botFight;
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Monster list is empty to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                }
                                else
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                            }
                        }
                        else
                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    }
                }
                else
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                item = item.nextSiblingElement(FightLoader::text_fight);
            }
            index_file++;
        }
    }
    return botFightList;
}

QHash<quint16,Skill> FightLoader::loadMonsterSkill(const QString &folder, const QHash<quint8, Buff> &monsterBuffs, const QList<Type> &types)
{
    QHash<quint16,Skill> monsterSkills;
    QDir dir(folder);
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const QString &file=fileList.at(file_index).absoluteFilePath();
        if(!file.endsWith(FightLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        QHash<QString,quint8> typeNameToId;
        int index=0;
        while(index<types.size())
        {
            typeNameToId[types.at(index).name]=index;
            index++;
        }
        QDomDocument domDocument;
        //open and quick check the file
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
            #endif
            QFile xmlFile(file);
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml skill monster file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
                file_index++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=FightLoader::text_skills)
        {
            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2;
        QDomElement item = root.firstChildElement(FightLoader::text_skill);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(FightLoader::text_id))
                {
                    quint32 id=item.attribute(FightLoader::text_id).toUInt(&ok);
                    if(ok && monsterSkills.contains(id))
                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else if(ok)
                    {
                        QHash<quint8,Skill::SkillList> levelDef;
                        QDomElement effect = item.firstChildElement(FightLoader::text_effect);
                        if(!effect.isNull())
                        {
                            if(effect.isElement())
                            {
                                QDomElement level = effect.firstChildElement(FightLoader::text_level);
                                while(!level.isNull())
                                {
                                    if(level.isElement())
                                    {
                                        if(level.hasAttribute(FightLoader::text_number))
                                        {
                                            quint32 sp=0;
                                            if(level.hasAttribute(FightLoader::text_sp))
                                            {
                                                sp=level.attribute(FightLoader::text_sp).toUShort(&ok);
                                                if(!ok)
                                                {
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, sp is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(level.tagName()).arg(level.lineNumber()));
                                                    sp=0;
                                                }
                                            }
                                            quint8 endurance=40;
                                            if(level.hasAttribute(FightLoader::text_endurance))
                                            {
                                                endurance=level.attribute(FightLoader::text_endurance).toUShort(&ok);
                                                if(!ok)
                                                {
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, endurance is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(level.tagName()).arg(level.lineNumber()));
                                                    endurance=40;
                                                }
                                                if(endurance<1)
                                                {
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, endurance lower than 1: child.tagName(): %2 (at line: %3)").arg(file).arg(level.tagName()).arg(level.lineNumber()));
                                                    endurance=40;
                                                }
                                            }
                                            quint8 number;
                                            if(ok)
                                                number=level.attribute(FightLoader::text_number).toUShort(&ok);
                                            if(ok)
                                            {
                                                levelDef[number].sp_to_learn=sp;
                                                levelDef[number].endurance=endurance;
                                                if(number>0)
                                                {
                                                    {
                                                        QDomElement life = level.firstChildElement(FightLoader::text_life);
                                                        while(!life.isNull())
                                                        {
                                                            if(life.isElement())
                                                            {
                                                                Skill::Life effect;
                                                                if(life.hasAttribute(FightLoader::text_applyOn))
                                                                {
                                                                    if(life.attribute(FightLoader::text_applyOn)==FightLoader::text_aloneEnemy)
                                                                        effect.effect.on=ApplyOn_AloneEnemy;
                                                                    else if(life.attribute(FightLoader::text_applyOn)==FightLoader::text_themself)
                                                                        effect.effect.on=ApplyOn_Themself;
                                                                    else if(life.attribute(FightLoader::text_applyOn)==FightLoader::text_allEnemy)
                                                                        effect.effect.on=ApplyOn_AllEnemy;
                                                                    else if(life.attribute(FightLoader::text_applyOn)==FightLoader::text_allAlly)
                                                                        effect.effect.on=ApplyOn_AllAlly;
                                                                    else if(life.attribute(FightLoader::text_applyOn)==FightLoader::text_nobody)
                                                                        effect.effect.on=ApplyOn_Nobody;
                                                                    else
                                                                    {
                                                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, applyOn tag wrong %4: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(life.attribute("applyOn")));
                                                                        effect.effect.on=ApplyOn_AloneEnemy;
                                                                    }
                                                                }
                                                                else
                                                                    effect.effect.on=ApplyOn_AloneEnemy;
                                                                QString text;
                                                                if(life.hasAttribute(FightLoader::text_quantity))
                                                                    text=life.attribute(FightLoader::text_quantity);
                                                                if(text.endsWith(FightLoader::text_percent))
                                                                    effect.effect.type=QuantityType_Percent;
                                                                else
                                                                    effect.effect.type=QuantityType_Quantity;
                                                                text.remove(FightLoader::text_percent);
                                                                text.remove(FightLoader::text_more);
                                                                effect.effect.quantity=text.toInt(&ok);
                                                                effect.success=100;
                                                                if(life.hasAttribute(FightLoader::text_success))
                                                                {
                                                                    QString success=life.attribute(FightLoader::text_success);
                                                                    success.remove(FightLoader::text_percent);
                                                                    effect.success=success.toUShort(&ok2);
                                                                    if(!ok2)
                                                                    {
                                                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, success wrong corrected to 100%: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                                        effect.success=100;
                                                                    }
                                                                }
                                                                if(ok)
                                                                {
                                                                    if(effect.effect.quantity!=0)
                                                                        levelDef[number].life << effect;
                                                                }
                                                                else
                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, %4 is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(text));
                                                            }
                                                            life = life.nextSiblingElement(FightLoader::text_life);
                                                        }
                                                    }
                                                    {
                                                        QDomElement buff = level.firstChildElement(FightLoader::text_buff);
                                                        while(!buff.isNull())
                                                        {
                                                            if(buff.isElement())
                                                            {
                                                                if(buff.hasAttribute(FightLoader::text_id))
                                                                {
                                                                    quint32 idBuff=buff.attribute(FightLoader::text_id).toUInt(&ok);
                                                                    if(ok)
                                                                    {
                                                                        Skill::Buff effect;
                                                                        if(buff.hasAttribute(FightLoader::text_applyOn))
                                                                        {
                                                                            if(buff.attribute(FightLoader::text_applyOn)==FightLoader::text_aloneEnemy)
                                                                                effect.effect.on=ApplyOn_AloneEnemy;
                                                                            else if(buff.attribute(FightLoader::text_applyOn)==FightLoader::text_themself)
                                                                                effect.effect.on=ApplyOn_Themself;
                                                                            else if(buff.attribute(FightLoader::text_applyOn)==FightLoader::text_allEnemy)
                                                                                effect.effect.on=ApplyOn_AllEnemy;
                                                                            else if(buff.attribute(FightLoader::text_applyOn)==FightLoader::text_allAlly)
                                                                                effect.effect.on=ApplyOn_AllAlly;
                                                                            else if(buff.attribute(FightLoader::text_applyOn)==FightLoader::text_nobody)
                                                                                effect.effect.on=ApplyOn_Nobody;
                                                                            else
                                                                            {
                                                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, applyOn tag wrong %4: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(buff.attribute("applyOn")));
                                                                                effect.effect.on=ApplyOn_AloneEnemy;
                                                                            }
                                                                        }
                                                                        else
                                                                            effect.effect.on=ApplyOn_AloneEnemy;
                                                                        if(!monsterBuffs.contains(idBuff))
                                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, this buff id is not found: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).at(idBuff));
                                                                        else
                                                                        {
                                                                            effect.effect.level=1;
                                                                            ok2=true;
                                                                            if(buff.hasAttribute(FightLoader::text_level))
                                                                            {
                                                                                QString level=buff.attribute(FightLoader::text_level);
                                                                                effect.effect.level=level.toUShort(&ok2);
                                                                                if(!ok2)
                                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level wrong: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(buff.attribute("level")));
                                                                                if(level<=0)
                                                                                {
                                                                                    ok2=false;
                                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level need be egal or greater than 1: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                                                }
                                                                            }
                                                                            if(ok2)
                                                                            {
                                                                                if(monsterBuffs.value(idBuff).level.size()<effect.effect.level)
                                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level needed: %4, level max found: %5: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(effect.effect.level).arg(monsterBuffs.value(idBuff).level.size()));
                                                                                else
                                                                                {
                                                                                    effect.effect.buff=idBuff;
                                                                                    effect.success=100;
                                                                                    if(buff.hasAttribute(FightLoader::text_success))
                                                                                    {
                                                                                        QString success=buff.attribute(FightLoader::text_success);
                                                                                        success.remove(FightLoader::text_percent);
                                                                                        effect.success=success.toUShort(&ok2);
                                                                                        if(!ok2)
                                                                                        {
                                                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, success wrong corrected to 100%: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                                                            effect.success=100;
                                                                                        }
                                                                                    }
                                                                                    levelDef[number].buff << effect;
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                    else
                                                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not tag id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                                }
                                                                else
                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not tag id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                            }
                                                            buff = buff.nextSiblingElement(FightLoader::text_buff);
                                                        }
                                                    }
                                                }
                                                else
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level need be egal or greater than 1: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, number tag is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    level = level.nextSiblingElement(FightLoader::text_level);
                                }
                                if(levelDef.size()==0)
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, 0 level found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                else
                                {
                                    monsterSkills[id].type=255;
                                    if(item.hasAttribute(FightLoader::text_type))
                                    {
                                        if(typeNameToId.contains(item.attribute(FightLoader::text_type)))
                                            monsterSkills[id].type=typeNameToId.value(item.attribute(FightLoader::text_type));
                                        else
                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, type not found: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute(FightLoader::text_type)));
                                    }
                                }
                                //order by level to learn
                                quint8 index=1;
                                while(levelDef.contains(index))
                                {
                                    /*if(levelDef.value(index).buff.empty() && levelDef.value(index).life.empty())
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, no effect loaded for skill %4 at level %5, missing level to continue: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(id).arg(index));*/
                                    monsterSkills[id].level << levelDef.value(index);
                                    levelDef.remove(index);
                                    #ifdef DEBUG_MESSAGE_SKILL_LOAD
                                    DebugClass::debugConsole(QStringLiteral("for the level %1 of skill %2 have %3 effect(s) in buff and %4 effect(s) in life").arg(index).arg(id).arg(GlobalData::serverPrivateVariables.monsterSkills.value(id).level.at(index-1).buff.size()).arg(GlobalData::serverPrivateVariables.monsterSkills.value(id).level.at(index-1).life.size()));
                                    #endif
                                    index++;
                                }
                                if(levelDef.size()>0)
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level up to %4 loaded, missing level to continue: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(index));
                                #ifdef DEBUG_MESSAGE_SKILL_LOAD
                                else
                                    DebugClass::debugConsole(QStringLiteral("%1 level(s) loaded for skill %2").arg(index-1).arg(id));
                                #endif
                            }
                            else
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        /*else
                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not effect balise: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));*/
                    }
                    else
                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the skill id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            item = item.nextSiblingElement(FightLoader::text_skill);
        }
        //check the default attack
        if(!monsterSkills.contains(0))
            DebugClass::debugConsole(QStringLiteral("Warning: no default monster attack if no more attack"));
        else if(monsterSkills.value(0).level.isEmpty())
        {
            monsterSkills.remove(0);
            DebugClass::debugConsole(QStringLiteral("Warning: no level for default monster attack if no more attack"));
        }
        else
        {
            if(monsterSkills.value(0).level.first().life.isEmpty())
            {
                monsterSkills.remove(0);
                DebugClass::debugConsole(QStringLiteral("Warning: no life effect for the default attack"));
            }
            else
            {
                const int &list_size=monsterSkills.value(0).level.first().life.size();
                int index=0;
                while(index<list_size)
                {
                    const Skill::Life &life=monsterSkills.value(0).level.first().life.at(index);
                    if(life.success==100 && life.effect.on==ApplyOn_AloneEnemy && life.effect.quantity<0)
                        break;
                    index++;
                }
                if(index==list_size)
                {
                    const Skill::Life &life=monsterSkills.value(0).level.first().life.first();
                    monsterSkills.remove(0);
                    DebugClass::debugConsole(QStringLiteral("Warning: no valid life effect for the default attack (id: 0): success=100%: %1, on=ApplyOn_AloneEnemy: %2, quantity<0: %3 for skill").arg(life.success).arg(life.effect.on).arg(life.effect.quantity));
                }
            }
        }
        file_index++;
    }
    return monsterSkills;
}

QHash<quint8,Buff> FightLoader::loadMonsterBuff(const QString &folder)
{
    QHash<quint8,Buff> monsterBuffs;
    QDir dir(folder);
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const QString &file=fileList.at(file_index).absoluteFilePath();
        if(!file.endsWith(FightLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        QDomDocument domDocument;
        //open and quick check the file
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
            #endif
            QFile xmlFile(file);
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml buff monster file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
                file_index++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=FightLoader::text_buffs)
        {
            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        QDomElement item = root.firstChildElement(FightLoader::text_buff);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(FightLoader::text_id))
                {
                    quint32 id=item.attribute(FightLoader::text_id).toUInt(&ok);
                    if(ok && monsterBuffs.contains(id))
                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id already found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else if(ok)
                    {
                        Buff::Duration general_duration=Buff::Duration_ThisFight;
                        quint8 general_durationNumberOfTurn=0;
                        float general_capture_bonus=1.0;
                        if(item.hasAttribute(FightLoader::text_capture_bonus))
                        {
                           general_capture_bonus=item.attribute(FightLoader::text_capture_bonus).toFloat(&ok);
                            if(!ok)
                            {
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, capture_bonus is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                general_capture_bonus=1.0;
                            }
                        }
                        if(item.hasAttribute(FightLoader::text_duration))
                        {
                            if(item.attribute(FightLoader::text_duration)==FightLoader::text_Always)
                                general_duration=Buff::Duration_Always;
                            else if(item.attribute(FightLoader::text_duration)==FightLoader::text_NumberOfTurn)
                            {
                                if(item.hasAttribute(FightLoader::text_durationNumberOfTurn))
                                {
                                    general_durationNumberOfTurn=item.attribute(FightLoader::text_durationNumberOfTurn).toUShort(&ok);
                                    if(!ok)
                                    {
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, durationNumberOfTurn is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        general_durationNumberOfTurn=3;
                                    }
                                    if(general_durationNumberOfTurn<=0)
                                    {
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, durationNumberOfTurn is egal to 0: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        general_durationNumberOfTurn=3;
                                    }
                                }
                                else
                                    general_durationNumberOfTurn=3;
                                general_duration=Buff::Duration_NumberOfTurn;
                            }
                            else if(item.attribute(FightLoader::text_duration)==FightLoader::text_ThisFight)
                                general_duration=Buff::Duration_ThisFight;
                            else
                            {
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attribute duration have wrong value \"%4\" is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("duration")));
                                general_duration=Buff::Duration_ThisFight;
                            }
                        }
                        QHash<quint8,Buff::GeneralEffect> levelDef;
                        QDomElement effect = item.firstChildElement(FightLoader::text_effect);
                        if(!effect.isNull())
                        {
                            if(effect.isElement())
                            {
                                QDomElement level = effect.firstChildElement(FightLoader::text_level);
                                while(!level.isNull())
                                {
                                    if(level.isElement())
                                    {
                                        if(level.hasAttribute(FightLoader::text_number))
                                        {
                                            quint8 number=level.attribute(FightLoader::text_number).toUShort(&ok);
                                            if(ok)
                                            {
                                                if(number>0)
                                                {
                                                    Buff::Duration duration=general_duration;
                                                    quint8 durationNumberOfTurn=general_durationNumberOfTurn;
                                                    float capture_bonus=general_capture_bonus;
                                                    if(item.hasAttribute(FightLoader::text_capture_bonus))
                                                    {
                                                       capture_bonus=item.attribute(FightLoader::text_capture_bonus).toFloat(&ok);
                                                        if(!ok)
                                                        {
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, capture_bonus is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                            capture_bonus=general_capture_bonus;
                                                        }
                                                    }
                                                    if(item.hasAttribute(FightLoader::text_duration))
                                                    {
                                                        if(item.attribute(FightLoader::text_duration)==FightLoader::text_Always)
                                                            duration=Buff::Duration_Always;
                                                        else if(item.attribute(FightLoader::text_duration)==FightLoader::text_NumberOfTurn)
                                                        {
                                                            if(item.hasAttribute(FightLoader::text_durationNumberOfTurn))
                                                            {
                                                                durationNumberOfTurn=item.attribute(FightLoader::text_durationNumberOfTurn).toUShort(&ok);
                                                                if(!ok)
                                                                {
                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, durationNumberOfTurn is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                                    durationNumberOfTurn=general_durationNumberOfTurn;
                                                                }
                                                            }
                                                            else
                                                                durationNumberOfTurn=general_durationNumberOfTurn;
                                                            duration=Buff::Duration_NumberOfTurn;
                                                        }
                                                        else if(item.attribute(FightLoader::text_duration)==FightLoader::text_ThisFight)
                                                            duration=Buff::Duration_ThisFight;
                                                        else
                                                        {
                                                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, attribute duration have wrong value \"%4\" is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("duration")));
                                                            duration=general_duration;
                                                        }
                                                    }
                                                    levelDef[number].duration=duration;
                                                    levelDef[number].durationNumberOfTurn=durationNumberOfTurn;
                                                    levelDef[number].capture_bonus=capture_bonus;



                                                    QDomElement inFight = level.firstChildElement(FightLoader::text_inFight);
                                                    while(!inFight.isNull())
                                                    {
                                                        if(inFight.isElement())
                                                        {
                                                            ok=true;
                                                            Buff::Effect effect;
                                                            QString text;
                                                            if(inFight.hasAttribute(FightLoader::text_hp))
                                                            {
                                                                text=inFight.attribute(FightLoader::text_hp);
                                                                effect.on=Buff::Effect::EffectOn_HP;
                                                            }
                                                            else if(inFight.hasAttribute(FightLoader::text_defense))
                                                            {
                                                                text=inFight.attribute(FightLoader::text_defense);
                                                                effect.on=Buff::Effect::EffectOn_Defense;
                                                            }
                                                            else if(inFight.hasAttribute(FightLoader::text_attack))
                                                            {
                                                                text=inFight.attribute(FightLoader::text_attack);
                                                                effect.on=Buff::Effect::EffectOn_Attack;
                                                            }
                                                            else
                                                            {
                                                                ok=false;
                                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, not know attribute balise: child.tagName(): %2 (at line: %3)").arg(file).arg(inFight.tagName()).arg(inFight.lineNumber()));
                                                            }
                                                            if(ok)
                                                            {
                                                                if(text.endsWith(FightLoader::text_percent))
                                                                    effect.type=QuantityType_Percent;
                                                                else
                                                                    effect.type=QuantityType_Quantity;
                                                                text.remove(FightLoader::text_percent);
                                                                text.remove(FightLoader::text_more);
                                                                effect.quantity=text.toInt(&ok);
                                                                if(ok)
                                                                    levelDef[number].fight << effect;
                                                                else
                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"%4\" something is wrong, or is not a number, or not into hp or defense balise: child.tagName(): %2 (at line: %3)").arg(file).arg(inFight.tagName()).arg(inFight.lineNumber()).arg(text));
                                                            }
                                                        }
                                                        inFight = inFight.nextSiblingElement(FightLoader::text_inFight);
                                                    }
                                                    QDomElement inWalk = level.firstChildElement(FightLoader::text_inWalk);
                                                    while(!inWalk.isNull())
                                                    {
                                                        if(inWalk.isElement())
                                                        {
                                                            if(inWalk.hasAttribute(FightLoader::text_steps))
                                                            {
                                                                quint32 steps=inWalk.attribute(FightLoader::text_steps).toUInt(&ok);
                                                                if(ok)
                                                                {
                                                                    Buff::EffectInWalk effect;
                                                                    effect.steps=steps;
                                                                    QString text;
                                                                    if(inWalk.hasAttribute(FightLoader::text_hp))
                                                                    {
                                                                        text=inWalk.attribute(FightLoader::text_hp);
                                                                        effect.effect.on=Buff::Effect::EffectOn_HP;
                                                                    }
                                                                    else if(inWalk.hasAttribute(FightLoader::text_defense))
                                                                    {
                                                                        text=inWalk.attribute(FightLoader::text_defense);
                                                                        effect.effect.on=Buff::Effect::EffectOn_Defense;
                                                                    }
                                                                    else
                                                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, not action found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                                    if(text.endsWith(FightLoader::text_percent))
                                                                        effect.effect.type=QuantityType_Percent;
                                                                    else
                                                                        effect.effect.type=QuantityType_Quantity;
                                                                    text.remove(FightLoader::text_percent);
                                                                    text.remove(FightLoader::text_more);
                                                                    effect.effect.quantity=text.toInt(&ok);
                                                                    if(ok)
                                                                        levelDef[number].walk << effect;
                                                                    else
                                                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, %4 is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(text));
                                                                }
                                                                else
                                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not tag steps: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                            }
                                                            else
                                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not tag steps: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                        }
                                                        inWalk = inWalk.nextSiblingElement(FightLoader::text_inWalk);
                                                    }
                                                }
                                                else
                                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level need be egal or greater than 1: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                            }
                                            else
                                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, number tag is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    level = level.nextSiblingElement(FightLoader::text_level);
                                }
                                quint8 index=1;
                                while(levelDef.contains(index))
                                {
                                    /*if(levelDef.value(index).fight.empty() && levelDef.value(index).walk.empty())
                                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, no effect loaded for buff %4 at level %5, missing level to continue: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(id).arg(index));*/
                                    monsterBuffs[id].level << levelDef.value(index);
                                    levelDef.remove(index);
                                    #ifdef DEBUG_MESSAGE_BUFF_LOAD
                                    DebugClass::debugConsole(QStringLiteral("for the level %1 of buff %2 have %3 effect(s) in fight and %4 effect(s) in walk").arg(index).arg(id).arg(GlobalData::serverPrivateVariables.monsterBuffs.value(id).level.at(index-1).fight.size()).arg(GlobalData::serverPrivateVariables.monsterBuffs.value(id).level.at(index-1).walk.size()));
                                    #endif
                                    index++;
                                }
                                if(levelDef.size()>0)
                                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, level up to %4 loaded, missing level to continue: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(index));
                                #ifdef DEBUG_MESSAGE_BUFF_LOAD
                                else
                                    DebugClass::debugConsole(QStringLiteral("%1 level(s) loaded for buff %2").arg(index-1).arg(id));
                                #endif
                            }
                            else
                                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, effect balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        else
                            DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not effet balise: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    }
                    else
                        DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, have not the buff id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            item = item.nextSiblingElement(FightLoader::text_buff);
        }
        file_index++;
    }
    return monsterBuffs;
}

