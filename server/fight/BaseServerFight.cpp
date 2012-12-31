#include "BaseServerFight.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DebugClass.h"
#include "../VariableServer.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>

using namespace Pokecraft;

void BaseServerFight::preload_monsters()
{
/*    //open and quick check the file
    QFile xmlFile(GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return;
    }

    //load the content
    bool ok,ok2;
    QDomElement item = root.firstChildElement("monster");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id") && item.hasAttribute("ratio_gender") && item.hasAttribute("catch_rate") && item.hasAttribute("xp_max")
                     && item.hasAttribute("hp") && item.hasAttribute("attack") && item.hasAttribute("defense") && item.hasAttribute("special_attack") && item.hasAttribute("special_defense") && item.hasAttribute("speed"))
            {
                quint8 id=item.attribute("id").toUShort(&ok);
                quint32 itemUsed=item.attribute("itemUsed").toULongLong(&ok2);
                if(ok && ok2)
                {
                    if(!GlobalData::serverPrivateVariables.plants.contains(id))
                    {
                        ok=true;
                        Plant plant;
                        plant.itemUsed=itemUsed;
                        QDomElement grow = item.firstChildElement("grow");
                        if(!grow.isNull())
                        {
                            if(grow.isElement())
                            {
                                QDomElement fruits = grow.firstChildElement("fruits");
                                if(!fruits.isNull())
                                {
                                    if(fruits.isElement())
                                    {
                                        plant.mature_seconds=fruits.text().toULongLong(&ok2)*60;
                                        if(!ok2)
                                        {
                                            ok=false;
                                            DebugClass::debugConsole(QString("preload_the_plant() fruits in not an number for xml file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(QString("preload_the_plant() fruit is not element for xml file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                }
                                else
                                    DebugClass::debugConsole(QString("preload_the_plant() fruit is null for xml file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            }
                            else
                                DebugClass::debugConsole(QString("preload_the_plant() grow is not an element for xml file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        else
                            DebugClass::debugConsole(QString("preload_the_plant() grow is null for xml file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        if(ok)
                        {
                            QDomElement quantity = item.firstChildElement("quantity");
                            if(!quantity.isNull())
                            {
                                if(quantity.isElement())
                                {
                                    float float_quantity=quantity.text().toFloat(&ok2);
                                    int integer_part=float_quantity;
                                    float random_part=float_quantity-integer_part;
                                    random_part*=RANDOM_FLOAT_PART_DIVIDER;
                                    plant.fix_quantity=integer_part;
                                    plant.random_quantity=random_part;
                                    if(!ok2)
                                    {
                                        ok=false;
                                        DebugClass::debugConsole(QString("preload_the_plant() quantity is not a number for xml file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    }
                                }
                                else
                                    DebugClass::debugConsole(QString("preload_the_plant() quantity is not element for xml file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            }
                            else
                                DebugClass::debugConsole(QString("preload_the_plant() quantity is null for xml file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        if(ok)
                        {
                            if(!GlobalData::serverPrivateVariables.items.contains(plant.itemUsed))
                            {
                                ok=false;
                                DebugClass::debugConsole(QString("preload_crafting_recipes() itemUsed is not into items list for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                            }
                        }
                        if(ok)
                            GlobalData::serverPrivateVariables.plants[id]=plant;
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the plant id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("monster");
    }

    DebugClass::debugConsole(QString("%1 monster(s) loaded").arg(GlobalData::serverPrivateVariables.plants.size()));*/
}

void BaseServerFight::preload_skills()
{
    //open and quick check the file
    QFile xmlFile(GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS+"skills.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return;
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
                quint8 id=item.attribute("id").toUShort(&ok);
                if(ok)
                {
                    QHash<quint8,MonsterSkill::MonsterSkillList> levelDef;
                    QDomElement effect = item.firstChildElement("effect");
                    if(!effect.isNull())
                    {
                        if(effect.isElement())
                        {
                            QDomElement level = effect.firstChildElement("level");
                            if(!level.isNull())
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
                                                QDomElement life = level.firstChildElement("life");
                                                while(!life.isNull())
                                                {
                                                    if(life.isElement())
                                                    {
                                                        MonsterSkill::MonsterSkillLife effect;
                                                        if(life.hasAttribute("applyOn"))
                                                        {
                                                            if(life.attribute("applyOn")=="aloneEnemy")
                                                                effect.on=MonsterSkill::ApplyOn_AloneEnemy;
                                                            else if(life.attribute("applyOn")=="themself")
                                                                effect.on=MonsterSkill::ApplyOn_Themself;
                                                            else if(life.attribute("applyOn")=="allEnemy")
                                                                effect.on=MonsterSkill::ApplyOn_AllEnemy;
                                                            else if(life.attribute("applyOn")=="allAlly")
                                                                effect.on=MonsterSkill::ApplyOn_AllAlly;
                                                            else
                                                            {
                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, applyOn tag wrong %4: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(life.attribute("applyOn")));
                                                                effect.on=MonsterSkill::ApplyOn_AloneEnemy;
                                                            }
                                                        }
                                                        else
                                                            effect.on=MonsterSkill::ApplyOn_AloneEnemy;
                                                        QString text;
                                                        if(life.hasAttribute("quantity"))
                                                            text=life.attribute("quantity");
                                                        if(text.endsWith("%"))
                                                            effect.type=QuantityType_Percent;
                                                        else
                                                            effect.type=QuantityType_Quantity;
                                                        text.remove("%");
                                                        effect.quantity=text.toInt(&ok);
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
                                                    }
                                                }
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
                                                                MonsterSkill::MonsterSkillBuff effect;
                                                                if(life.hasAttribute("applyOn"))
                                                                {
                                                                    if(life.attribute("applyOn")=="aloneEnemy")
                                                                        effect.on=MonsterSkill::ApplyOn_AloneEnemy;
                                                                    else if(life.attribute("applyOn")=="themself")
                                                                        effect.on=MonsterSkill::ApplyOn_Themself;
                                                                    else if(life.attribute("applyOn")=="allEnemy")
                                                                        effect.on=MonsterSkill::ApplyOn_AllEnemy;
                                                                    else if(life.attribute("applyOn")=="allAlly")
                                                                        effect.on=MonsterSkill::ApplyOn_AllAlly;
                                                                    else
                                                                    {
                                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, applyOn tag wrong %4: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(life.attribute("applyOn")));
                                                                        effect.on=MonsterSkill::ApplyOn_AloneEnemy;
                                                                    }
                                                                }
                                                                else
                                                                    effect.on=MonsterSkill::ApplyOn_AloneEnemy;
                                                                if(!GlobalData::serverPrivateVariables.monsterBuffs.contains(idBuff))
                                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, this buff id is not found: %4: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).at(idBuff));
                                                                else
                                                                {
                                                                    effect.level=1;
                                                                    ok2=true;
                                                                    if(life.hasAttribute("level"))
                                                                    {
                                                                        QString level=life.attribute("level");
                                                                        effect.level=level.toUShort(&ok2);
                                                                        if(!ok2)
                                                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, level wrong: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(life.attribute("level")));
                                                                        if(level<=0)
                                                                        {
                                                                            ok2=false;
                                                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, level need be egal or greater than 1: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                                        }
                                                                    }
                                                                    if(ok2)
                                                                    {
                                                                        if(GlobalData::serverPrivateVariables.monsterBuffs[idBuff].level.size()<effect.level)
                                                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, level needed: %4, level max found: %5: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(effect.level).arg(GlobalData::serverPrivateVariables.monsterBuffs[idBuff].level.size()));
                                                                        else
                                                                        {
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
                                                }
                                                quint8 index=1;
                                                while(levelDef.contains(index))
                                                {
                                                    if(levelDef[index].buff.empty() && levelDef[index].life.empty())
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, no effect loaded for skill %4 at level %5, missing level to continue: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(id).arg(index));
                                                    GlobalData::serverPrivateVariables.monsterSkills[id].level << levelDef[index];
                                                    levelDef.remove(index);
                                                    index++;
                                                }
                                                if(levelDef.size()>0)
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, level up to %4 loaded, missing level to continue: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(index));
                                                #ifdef DEBUG_MESSAGE_BUFF_LOAD
                                                else
                                                    DebugClass::debugConsole(QString("%1 level(s) loaded for buff %2").arg(index).arg(id));
                                                #endif
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
                            }
                            else
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not effet balise: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
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
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the plant id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("skill");
    }

    DebugClass::debugConsole(QString("%1 skill(s) loaded").arg(GlobalData::serverPrivateVariables.monsterBuffs.size()));
}

void BaseServerFight::preload_buff()
{
    //open and quick check the file
    QFile xmlFile(GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS+"buff.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
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
                quint8 id=item.attribute("id").toUShort(&ok);
                if(ok)
                {
                    QHash<quint8,MonsterBuff::GeneralEffect> levelDef;
                    QDomElement effect = item.firstChildElement("effect");
                    if(!effect.isNull())
                    {
                        if(effect.isElement())
                        {
                            QDomElement level = effect.firstChildElement("level");
                            if(!level.isNull())
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
                                                        MonsterBuff::Effect effect;
                                                        QString text;
                                                        if(inFight.hasAttribute("hp"))
                                                        {
                                                            text=inFight.attribute("hp");
                                                            effect.on=MonsterBuff::Effect::EffectOn_HP;
                                                        }
                                                        else if(inFight.hasAttribute("defense"))
                                                        {
                                                            text=inFight.attribute("defense");
                                                            effect.on=MonsterBuff::Effect::EffectOn_Defense;
                                                        }
                                                        if(text.endsWith("%"))
                                                            effect.type=QuantityType_Percent;
                                                        else
                                                            effect.type=QuantityType_Quantity;
                                                        text.remove("%");
                                                        effect.quantity=text.toInt(&ok);
                                                        if(ok)
                                                            levelDef[number].fight << effect;
                                                    }
                                                }
                                                QDomElement inWalk = level.firstChildElement("inWalk");
                                                while(!inWalk.isNull())
                                                {
                                                    if(inWalk.isElement())
                                                    {
                                                        if(inWalk.hasAttribute("afterStep"))
                                                        {
                                                            quint32 afterStep=inWalk.attribute("afterStep").toUInt(&ok);
                                                            if(ok)
                                                            {
                                                                MonsterBuff::EffectInWalk effect;
                                                                effect.afterStep=afterStep;
                                                                QString text;
                                                                if(inWalk.hasAttribute("hp"))
                                                                {
                                                                    text=inWalk.attribute("hp");
                                                                    effect.effect.on=MonsterBuff::Effect::EffectOn_HP;
                                                                }
                                                                else if(inWalk.hasAttribute("defense"))
                                                                {
                                                                    text=inWalk.attribute("defense");
                                                                    effect.effect.on=MonsterBuff::Effect::EffectOn_Defense;
                                                                }
                                                                if(text.endsWith("%"))
                                                                    effect.effect.type=QuantityType_Percent;
                                                                else
                                                                    effect.effect.type=QuantityType_Quantity;
                                                                text.remove("%");
                                                                effect.effect.quantity=text.toUShort(&ok);
                                                                if(ok)
                                                                    levelDef[number].walk << effect;
                                                            }
                                                            else
                                                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not tag afterStep: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                        }
                                                        else
                                                            DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not tag afterStep: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                    }
                                                }
                                                quint8 index=1;
                                                while(levelDef.contains(index))
                                                {
                                                    if(levelDef[index].fight.empty() && levelDef[index].walk.empty())
                                                        DebugClass::debugConsole(QString("Unable to open the xml file: %1, no effect loaded for buff %4 at level %5, missing level to continue: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(id).arg(index));
                                                    GlobalData::serverPrivateVariables.monsterBuffs[id].level << levelDef[index];
                                                    levelDef.remove(index);
                                                    index++;
                                                }
                                                if(levelDef.size()>0)
                                                    DebugClass::debugConsole(QString("Unable to open the xml file: %1, level up to %4 loaded, missing level to continue: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(index));
                                                #ifdef DEBUG_MESSAGE_BUFF_LOAD
                                                else
                                                    DebugClass::debugConsole(QString("%1 level(s) loaded for buff %2").arg(index).arg(id));
                                                #endif
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
                            }
                            else
                                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not effet balise: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
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
                DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the plant id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("buff");
    }

    DebugClass::debugConsole(QString("%1 buff(s) loaded").arg(GlobalData::serverPrivateVariables.monsterBuffs.size()));
}

void BaseServerFight::unload_buff()
{
}

void BaseServerFight::unload_skills()
{
}

void BaseServerFight::unload_monsters()
{
}
