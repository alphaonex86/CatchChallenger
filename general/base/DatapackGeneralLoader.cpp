#include "DatapackGeneralLoader.h"
#include "DebugClass.h"
#include "GeneralVariable.h"
#include "CommonDatapack.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfoList>
#include <QDir>

using namespace CatchChallenger;

QHash<QString, Reputation> DatapackGeneralLoader::loadReputation(const QString &file)
{
    QHash<QString, Reputation> reputation;
    //open and quick check the file
    QFile itemsFile(file);
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString()));
        return reputation;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return reputation;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        DebugClass::debugConsole(QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName()));
        return reputation;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("reputation");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("type"))
            {
                QList<qint32> point_list_positive,point_list_negative;
                QStringList text_positive,text_negative;
                QDomElement level = item.firstChildElement("level");
                ok=true;
                while(!level.isNull() && ok)
                {
                    if(level.isElement())
                    {
                        if(level.hasAttribute("point"))
                        {
                            qint32 point=level.attribute("point").toInt(&ok);
                            QString text_val;
                            if(ok)
                            {
                                QDomElement text = level.firstChildElement("text");
                                ok=true;
                                while(!text.isNull() && ok)
                                {
                                    if(text.isElement())
                                    {
                                        if(text.hasAttribute("lang"))
                                            if(text.attribute("lang")=="en" && !text.text().isEmpty())
                                            {
                                                text_val=text.text();
                                                break;
                                            }
                                    }
                                    else
                                        DebugClass::debugConsole(QString("Unable to open the file: %1, point attribute not found: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    text = text.nextSiblingElement("level");
                                }
                                if(text_val.isEmpty())
                                    ok=false;
                                if(ok)
                                {
                                    bool found=false;
                                    int index=0;
                                    if(point>=0)
                                    {
                                        while(index<point_list_positive.size())
                                        {
                                            if(point_list_positive.at(index)==point)
                                            {
                                                DebugClass::debugConsole(QString("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                found=true;
                                                ok=false;
                                                break;
                                            }
                                            if(point_list_positive.at(index)>point)
                                            {
                                                point_list_positive.insert(index,point);
                                                text_positive.insert(index,text_val);
                                                found=true;
                                                break;
                                            }
                                            index++;
                                        }
                                        if(!found)
                                        {
                                            point_list_positive << point;
                                            text_positive << text_val;
                                        }
                                    }
                                    else
                                    {
                                        while(index<point_list_negative.size())
                                        {
                                            if(point_list_negative.at(index)==point)
                                            {
                                                DebugClass::debugConsole(QString("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                found=true;
                                                ok=false;
                                                break;
                                            }
                                            if(point_list_negative.at(index)<point)
                                            {
                                                point_list_negative.insert(index,point);
                                                text_negative.insert(index,text_val);
                                                found=true;
                                                break;
                                            }
                                            index++;
                                        }
                                        if(!found)
                                        {
                                            point_list_negative << point;
                                            text_negative << text_val;
                                        }
                                    }
                                }
                            }
                            else
                                DebugClass::debugConsole(QString("Unable to open the file: %1, point is not number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        }
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the file: %1, point attribute not found: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    level = level.nextSiblingElement("level");
                }
                if(ok)
                    if(point_list_positive.size()<2)
                    {
                        DebugClass::debugConsole(QString("Unable to open the file: %1, reputation have to few level: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_positive.contains(0))
                    {
                        DebugClass::debugConsole(QString("Unable to open the file: %1, no starting level for the positive: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_negative.empty() && !point_list_negative.contains(-1))
                    {
                        DebugClass::debugConsole(QString("Unable to open the file: %1, no starting level for the negative: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!item.attribute("type").contains(QRegExp("^[a-z]{1,32}$")))
                    {
                        DebugClass::debugConsole(QString("Unable to open the file: %1, the type %4 don't match wiuth the regex: ^[a-z]{1,32}$: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("type")));
                        ok=false;
                    }
                if(ok)
                {
                    reputation[item.attribute("type")].reputation_positive=point_list_positive;
                    reputation[item.attribute("type")].reputation_negative=point_list_negative;
                    reputation[item.attribute("type")].text_positive=text_positive;
                    reputation[item.attribute("type")].text_negative=text_negative;
                }
            }
            else
                DebugClass::debugConsole(QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("reputation");
    }

    return reputation;
}

QHash<quint32, Quest> DatapackGeneralLoader::loadQuests(const QString &folder)
{
    QHash<quint32, Quest> quests;
    //open and quick check the file
    QFileInfoList entryList=QDir(folder).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
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
        //add it, all seam ok
        QPair<bool,Quest> returnedQuest=loadSingleQuest(entryList.at(index).absoluteFilePath()+"/definition.xml");
        if(returnedQuest.first==true)
        {
            if(quests.contains(returnedQuest.second.id))
                qDebug() << QString("The quest with id: %1 is already found, disable: %2").arg(returnedQuest.second.id).arg(entryList.at(index).absoluteFilePath()+"/definition.xml");
            else
                quests[returnedQuest.second.id]=returnedQuest.second;
        }

        index++;
    }
    return quests;
}

QPair<bool,Quest> DatapackGeneralLoader::loadSingleQuest(const QString &file)
{
    CatchChallenger::Quest quest;
    QFile itemsFile(file);
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return QPair<bool,Quest>(false,quest);
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return QPair<bool,Quest>(false,quest);
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="quest")
    {
        qDebug() << QString("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(itemsFile.fileName());
        return QPair<bool,Quest>(false,quest);
    }

    //load the content
    bool ok;
    if(!root.hasAttribute("id"))
    {
        qDebug() << QString("Unable to open the file: %1, \"quest\" root balise have not the id attribute").arg(itemsFile.fileName());
        return QPair<bool,Quest>(false,quest);
    }
    QList<quint32> defaultBots;
    quest.id=root.attribute("id").toUInt(&ok);
    if(!ok)
    {
        qDebug() << QString("Unable to open the file: %1, \"quest\" root balise id attribute is not a number").arg(itemsFile.fileName());
        return QPair<bool,Quest>(false,quest);
    }
    quest.repeatable=false;
    if(root.hasAttribute("repeatable"))
        if(root.attribute("repeatable")=="yes" || root.attribute("repeatable")=="true")
            quest.repeatable=true;
    if(root.hasAttribute("bot"))
    {
        QStringList tempStringList=root.attribute("bot").split(";");
        int index=0;
        while(index<tempStringList.size())
        {
            quint32 tempInt=tempStringList.at(index).toUInt(&ok);
            if(ok)
                defaultBots << tempInt;
            index++;
        }
    }

    //load requirements
    QDomElement requirements = root.firstChildElement("requirements");
    while(!requirements.isNull())
    {
        if(requirements.isElement())
        {
            //load requirements reputation
            {
                QDomElement requirementsItem = requirements.firstChildElement("reputation");
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute("type") && requirementsItem.hasAttribute("level"))
                        {
                            QString stringLevel=requirementsItem.attribute("level");
                            bool positif=!stringLevel.startsWith("-");
                            if(!positif)
                                stringLevel.remove(0,1);
                            quint8 level=stringLevel.toUShort(&ok);
                            if(ok)
                            {
                                CatchChallenger::Quest::ReputationRequirements reputation;
                                reputation.level=level;
                                reputation.positif=positif;
                                reputation.type=requirementsItem.attribute("type");
                                quest.requirements.reputation << reputation;
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, reputation is not a number %4: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber()).arg(stringLevel);
                        }
                        else
                            qDebug() << QString("Has attribute: %1, have not attribute type or level: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    requirementsItem = requirementsItem.nextSiblingElement("requirements");
                }
            }
            //load requirements quest
            {
                QDomElement requirementsItem = requirements.firstChildElement("quest");
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute("id"))
                        {
                            quint32 questId=requirementsItem.attribute("id").toUInt(&ok);
                            if(ok)
                                quest.requirements.quests << questId;
                            else
                                qDebug() << QString("Unable to open the file: %1, requirement quest item id is not a number %4: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber()).arg(requirementsItem.attribute("id"));
                        }
                        else
                            qDebug() << QString("Has attribute: %1, requirement quest item have not id attribute: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    requirementsItem = requirementsItem.nextSiblingElement("quest");
                }
            }
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(requirements.tagName()).arg(requirements.lineNumber());
        requirements = requirements.nextSiblingElement("requirements");
    }

    //load rewards
    QDomElement rewards = root.firstChildElement("rewards");
    while(!rewards.isNull())
    {
        if(rewards.isElement())
        {
            //load rewards reputation
            {
                QDomElement reputationItem = rewards.firstChildElement("reputation");
                while(!reputationItem.isNull())
                {
                    if(reputationItem.isElement())
                    {
                        if(reputationItem.hasAttribute("type") && reputationItem.hasAttribute("point"))
                        {
                            qint32 point=reputationItem.attribute("point").toInt(&ok);
                            if(ok)
                            {
                                CatchChallenger::Quest::ReputationRewards reputation;
                                reputation.point=point;
                                reputation.type=reputationItem.attribute("type");
                                quest.rewards.reputation << reputation;
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, quest rewards point is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                        }
                        else
                            qDebug() << QString("Has attribute: %1, quest rewards point have not type or point attribute: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                    reputationItem = reputationItem.nextSiblingElement("rewards");
                }
            }
            //load rewards item
            {
                QDomElement rewardsItem = rewards.firstChildElement("item");
                while(!rewardsItem.isNull())
                {
                    if(rewardsItem.isElement())
                    {
                        if(rewardsItem.hasAttribute("id"))
                        {
                            CatchChallenger::Quest::Item item;
                            item.item=rewardsItem.attribute("id").toUInt(&ok);
                            item.quantity=1;
                            if(ok)
                            {
                                if(!CommonDatapack::commonDatapack.items.item.contains(item.item))
                                {
                                    qDebug() << QString("Unable to open the file: %1, rewards item id is not into the item list: %4: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber()).arg(rewardsItem.attribute("id"));
                                    return QPair<bool,Quest>(false,quest);
                                }
                                if(rewardsItem.hasAttribute("quantity"))
                                {
                                    item.quantity=rewardsItem.attribute("quantity").toUInt(&ok);
                                    if(!ok)
                                        item.quantity=1;
                                }
                                quest.rewards.items << item;
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, rewards item id is not a number: %4: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber()).arg(rewardsItem.attribute("id"));
                        }
                        else
                            qDebug() << QString("Has attribute: %1, rewards item have not attribute id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    rewardsItem = rewardsItem.nextSiblingElement("quest");
                }
            }
            //load rewards allow
            {
                QDomElement allowItem = rewards.firstChildElement("allow");
                while(!allowItem.isNull())
                {
                    if(allowItem.isElement())
                    {
                        if(allowItem.hasAttribute("type"))
                        {
                            if(allowItem.attribute("type")=="clan")
                                quest.rewards.allow << CatchChallenger::ActionAllow_Clan;
                            else
                                qDebug() << QString("Unable to open the file: %1, allow type not understand: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(allowItem.tagName()).arg(allowItem.lineNumber()).arg(allowItem.attribute("id"));
                        }
                        else
                            qDebug() << QString("Has attribute: %1, rewards item have not attribute id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(allowItem.tagName()).arg(allowItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(allowItem.tagName()).arg(allowItem.lineNumber());
                    allowItem = allowItem.nextSiblingElement("allow");
                }
            }
            quest.rewards.allow.fromSet(quest.rewards.allow.toSet());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(rewards.tagName()).arg(rewards.lineNumber());
        rewards = rewards.nextSiblingElement("rewards");
    }

    QHash<quint8,CatchChallenger::Quest::Step> steps;
    //load step
    QDomElement step = root.firstChildElement("step");
    while(!step.isNull())
    {
        if(step.isElement())
        {
            if(step.hasAttribute("id"))
            {
                quint32 id=step.attribute("id").toUInt(&ok);
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
                    else
                        stepObject.bots=defaultBots;
                    //do the item
                    {
                        QDomElement stepItem = step.firstChildElement("item");
                        while(!stepItem.isNull())
                        {
                            if(stepItem.isElement())
                            {
                                if(stepItem.hasAttribute("id"))
                                {
                                    CatchChallenger::Quest::Item item;
                                    item.item=stepItem.attribute("id").toUInt(&ok);
                                    item.quantity=1;
                                    if(ok)
                                    {
                                        if(!CommonDatapack::commonDatapack.items.item.contains(item.item))
                                        {
                                            qDebug() << QString("Unable to open the file: %1, rewards item id is not into the item list: %4: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(stepItem.tagName()).arg(stepItem.lineNumber()).arg(stepItem.attribute("id"));
                                            return QPair<bool,Quest>(false,quest);
                                        }
                                        if(stepItem.hasAttribute("quantity"))
                                        {
                                            item.quantity=stepItem.attribute("quantity").toUInt(&ok);
                                            if(!ok)
                                                item.quantity=1;
                                        }
                                        stepObject.requirements.items << item;
                                        if(stepItem.hasAttribute("monster") && stepItem.hasAttribute("rate"))
                                        {
                                            CatchChallenger::Quest::ItemMonster itemMonster;
                                            itemMonster.item=item.item;

                                            QStringList tempStringList=stepItem.attribute("monster").split(";");
                                            int index=0;
                                            while(index<tempStringList.size())
                                            {
                                                quint32 tempInt=tempStringList.at(index).toUInt(&ok);
                                                if(ok)
                                                    itemMonster.monsters << tempInt;
                                                index++;
                                            }

                                            QString rateString=stepItem.attribute("rate");
                                            rateString.remove("%");
                                            itemMonster.rate=rateString.toUShort(&ok);
                                            if(ok)
                                                stepObject.itemsMonster << itemMonster;
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to open the file: %1, step id is not a number %4: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber()).arg(stepItem.attribute("id"));
                                }
                                else
                                    qDebug() << QString("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                            stepItem = stepItem.nextSiblingElement("item");
                        }
                    }
                    //do the fight
                    {
                        QDomElement fightItem = step.firstChildElement("fight");
                        while(!fightItem.isNull())
                        {
                            if(fightItem.isElement())
                            {
                                if(fightItem.hasAttribute("id"))
                                {
                                    quint32 fightId=fightItem.attribute("id").toUInt(&ok);
                                    if(ok)
                                        stepObject.requirements.fightId << fightId;
                                    else
                                        qDebug() << QString("Unable to open the file: %1, step id is not a number %4: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber()).arg(fightItem.attribute("id"));
                                }
                                else
                                    qDebug() << QString("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
                            fightItem = fightItem.nextSiblingElement("fight");
                        }
                    }
                    steps[id]=stepObject;
                }
                else
                    qDebug() << QString("Unable to open the file: %1, step id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
            }
            else
                qDebug() << QString("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(step.tagName()).arg(step.lineNumber());
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
    if(indexLoop<(steps.size()+1))
        return QPair<bool,Quest>(false,quest);
    return QPair<bool,Quest>(true,quest);
}

QHash<quint8, Plant> DatapackGeneralLoader::loadPlants(const QString &file)
{
    QHash<quint8, Plant> plants;
    //open and quick check the file
    QFile plantsFile(file);
    QByteArray xmlContent;
    if(!plantsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the plants file: %1, error: %2").arg(plantsFile.fileName()).arg(plantsFile.errorString());
        return plants;
    }
    xmlContent=plantsFile.readAll();
    plantsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the plants file: %1, Parse error at line %2, column %3: %4").arg(plantsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return plants;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="plants")
    {
        qDebug() << QString("Unable to open the plants file: %1, \"plants\" root balise not found for the xml file").arg(plantsFile.fileName());
        return plants;
    }

    //load the content
    bool ok,ok2;
    QDomElement plantItem = root.firstChildElement("plant");
    while(!plantItem.isNull())
    {
        if(plantItem.isElement())
        {
            if(plantItem.hasAttribute("id") && plantItem.hasAttribute("itemUsed"))
            {
                quint8 id=plantItem.attribute("id").toUShort(&ok);
                quint32 itemUsed=plantItem.attribute("itemUsed").toUInt(&ok2);
                if(ok && ok2)
                {
                    if(!plants.contains(id))
                    {
                        Plant plant;
                        plant.itemUsed=itemUsed;
                        ok=false;
                        QDomElement quantity = plantItem.firstChildElement("quantity");
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
                                ok=ok2;
                            }
                        }
                        QDomElement grow = plantItem.firstChildElement("grow");
                        if(!grow.isNull())
                        {
                            if(grow.isElement())
                            {
                                QDomElement sprouted = grow.firstChildElement("sprouted");
                                if(!sprouted.isNull())
                                    if(sprouted.isElement())
                                    {
                                        plant.sprouted_seconds=sprouted.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(sprouted.tagName()).arg(sprouted.lineNumber()).arg(sprouted.text());
                                            ok=false;
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, sprouted is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(sprouted.tagName()).arg(sprouted.lineNumber());
                                else
                                    qDebug() << QString("Unable to parse the plants file: %1, sprouted is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(sprouted.tagName()).arg(sprouted.lineNumber());
                                QDomElement taller = grow.firstChildElement("taller");
                                if(!taller.isNull())
                                    if(taller.isElement())
                                    {
                                        plant.taller_seconds=taller.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(taller.tagName()).arg(taller.lineNumber()).arg(taller.text());
                                            ok=false;
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, taller is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(taller.tagName()).arg(taller.lineNumber());
                                else
                                    qDebug() << QString("Unable to parse the plants file: %1, taller is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(taller.tagName()).arg(taller.lineNumber());
                                QDomElement flowering = grow.firstChildElement("flowering");
                                if(!flowering.isNull())
                                    if(flowering.isElement())
                                    {
                                        plant.flowering_seconds=flowering.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            ok=false;
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(flowering.tagName()).arg(flowering.lineNumber()).arg(flowering.text());
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, flowering is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(flowering.tagName()).arg(flowering.lineNumber());
                                else
                                    qDebug() << QString("Unable to parse the plants file: %1, flowering is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(flowering.tagName()).arg(flowering.lineNumber());
                                QDomElement fruits = grow.firstChildElement("fruits");
                                if(!fruits.isNull())
                                    if(fruits.isElement())
                                    {
                                        plant.fruits_seconds=fruits.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(fruits.tagName()).arg(fruits.lineNumber()).arg(fruits.text());
                                            ok=false;
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, fruits is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(fruits.tagName()).arg(fruits.lineNumber());
                                else
                                    qDebug() << QString("Unable to parse the plants file: %1, fruits is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(fruits.tagName()).arg(fruits.lineNumber());

                            }
                            else
                                qDebug() << QString("Unable to parse the plants file: %1, grow is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(grow.tagName()).arg(grow.lineNumber());
                        }
                        else
                            qDebug() << QString("Unable to parse the plants file: %1, grow is null: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(grow.tagName()).arg(grow.lineNumber());
                        if(ok)
                            plants[id]=plant;
                    }
                    else
                        qDebug() << QString("Unable to open the plants file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the plants file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the plants file: %1, have not the plant id: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the plants file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(plantsFile.fileName()).arg(plantItem.tagName()).arg(plantItem.lineNumber());
        plantItem = plantItem.nextSiblingElement("plant");
    }
    return plants;
}

QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> > DatapackGeneralLoader::loadCraftingRecipes(const QString &file,const QHash<quint32, Item> &items)
{
    QHash<quint32,CrafingRecipe> crafingRecipes;
    QHash<quint32,quint32> itemToCrafingRecipes;
    //open and quick check the file
    QFile craftingRecipesFile(file);
    QByteArray xmlContent;
    if(!craftingRecipesFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, error: %2").arg(craftingRecipesFile.fileName()).arg(craftingRecipesFile.errorString());
        return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
    }
    xmlContent=craftingRecipesFile.readAll();
    craftingRecipesFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(craftingRecipesFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="recipes")
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, \"recipes\" root balise not found for the xml file").arg(craftingRecipesFile.fileName());
        return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
    }

    //load the content
    bool ok,ok2,ok3;
    QDomElement recipeItem = root.firstChildElement("recipe");
    while(!recipeItem.isNull())
    {
        if(recipeItem.isElement())
        {
            if(recipeItem.hasAttribute("id") && recipeItem.hasAttribute("itemToLearn") && recipeItem.hasAttribute("doItemId"))
            {
                quint8 success=100;
                if(recipeItem.hasAttribute("success"))
                {
                    quint8 tempShort=recipeItem.attribute("success").toUShort(&ok);
                    if(ok)
                    {
                        if(tempShort>100)
                            qDebug() << QString("preload_crafting_recipes() success can't be greater than 100 for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                        else
                            success=tempShort;
                    }
                    else
                        qDebug() << QString("preload_crafting_recipes() success in not an number for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }
                quint16 quantity=1;
                if(recipeItem.hasAttribute("quantity"))
                {
                    quint32 tempShort=recipeItem.attribute("quantity").toUInt(&ok);
                    if(ok)
                    {
                        if(tempShort>65535)
                            qDebug() << QString("preload_crafting_recipes() quantity can't be greater than 65535 for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                        else
                            quantity=tempShort;
                    }
                    else
                        qDebug() << QString("preload_crafting_recipes() quantity in not an number for crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }

                quint32 id=recipeItem.attribute("id").toUInt(&ok);
                quint32 itemToLearn=recipeItem.attribute("itemToLearn").toUInt(&ok2);
                quint32 doItemId=recipeItem.attribute("doItemId").toUInt(&ok3);
                if(ok && ok2 && ok3)
                {
                    if(!crafingRecipes.contains(id))
                    {
                        ok=true;
                        CatchChallenger::CrafingRecipe recipe;
                        recipe.doItemId=doItemId;
                        recipe.itemToLearn=itemToLearn;
                        recipe.quantity=quantity;
                        recipe.success=success;
                        QDomElement material = recipeItem.firstChildElement("material");
                        while(!material.isNull() && ok)
                        {
                            if(material.isElement())
                            {
                                if(material.hasAttribute("itemId"))
                                {
                                    quint32 itemId=material.attribute("itemId").toUInt(&ok2);
                                    if(!ok2)
                                    {
                                        ok=false;
                                        qDebug() << QString("preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                        break;
                                    }
                                    quint16 quantity=1;
                                    if(material.hasAttribute("quantity"))
                                    {
                                        quint32 tempShort=material.attribute("quantity").toUInt(&ok2);
                                        if(ok2)
                                        {
                                            if(tempShort>65535)
                                            {
                                                ok=false;
                                                qDebug() << QString("preload_crafting_recipes() material quantity can't be greater than 65535 for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                                break;
                                            }
                                            else
                                                quantity=tempShort;
                                        }
                                        else
                                        {
                                            ok=false;
                                            qDebug() << QString("preload_crafting_recipes() material quantity in not an number for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                            break;
                                        }
                                    }
                                    if(!items.contains(itemId))
                                    {
                                        ok=false;
                                        qDebug() << QString("preload_crafting_recipes() material itemId in not into items list for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                        break;
                                    }
                                    CatchChallenger::CrafingRecipe::Material newMaterial;
                                    newMaterial.itemId=itemId;
                                    newMaterial.quantity=quantity;
                                    recipe.materials << newMaterial;
                                }
                                else
                                    qDebug() << QString("preload_crafting_recipes() material have not attribute itemId for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                            else
                                qDebug() << QString("preload_crafting_recipes() material is not an element for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            material = material.nextSiblingElement("material");
                        }
                        if(ok)
                        {
                            if(!items.contains(recipe.itemToLearn))
                            {
                                ok=false;
                                qDebug() << QString("preload_crafting_recipes() itemToLearn is not into items list for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            if(!items.contains(recipe.doItemId))
                            {
                                ok=false;
                                qDebug() << QString("preload_crafting_recipes() doItemId is not into items list for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            crafingRecipes[id]=recipe;
                            itemToCrafingRecipes[recipe.itemToLearn]=id;
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the crafting recipe file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the crafting recipe file: %1, have not the crafting recipe id: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the crafting recipe file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(craftingRecipesFile.fileName()).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
        recipeItem = recipeItem.nextSiblingElement("recipe");
    }
    return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
}

QHash<quint32,Industry> DatapackGeneralLoader::loadIndustries(const QString &folder,const QHash<quint32, Item> &items)
{
    QHash<quint32,Industry> industries;
    QDir dir(folder);
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(fileList.at(file_index).fileName()=="list.xml")
        {
            file_index++;
            continue;
        }
        //open and quick check the file
        QFile industryFile(fileList.at(file_index).absoluteFilePath());
        QByteArray xmlContent;
        if(!industryFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the crafting recipe file: %1, error: %2").arg(industryFile.fileName()).arg(industryFile.errorString());
            file_index++;
            continue;
        }
        xmlContent=industryFile.readAll();
        industryFile.close();
        QDomDocument domDocument;
        QString errorStr;
        int errorLine,errorColumn;
        if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QString("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(industryFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            file_index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!="industries")
        {
            qDebug() << QString("Unable to open the crafting recipe file: %1, \"industries\" root balise not found for the xml file").arg(industryFile.fileName());
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2,ok3;
        QDomElement industryItem = root.firstChildElement("industry");
        while(!industryItem.isNull())
        {
            if(industryItem.isElement())
            {
                if(industryItem.hasAttribute("id") && industryItem.hasAttribute("time") && industryItem.hasAttribute("cycletobefull"))
                {
                    quint32 id=industryItem.attribute("id").toUInt(&ok);
                    quint32 time=industryItem.attribute("time").toUInt(&ok2);
                    quint8 cycletobefull=industryItem.attribute("cycletobefull").toUShort(&ok3);
                    if(ok && ok2 && ok3)
                    {
                        if(time<60*5)
                            qDebug() << QString("the time need be greater than 5*60 seconds to not slow down the server: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                        else if(cycletobefull<1)
                            qDebug() << QString("cycletobefull need be greater than 0: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                        else if(!industries.contains(id))
                        {
                            Industry industry;
                            //ressource
                            {
                                QDomElement ressourceItem = industryItem.firstChildElement("ressource");
                                ok=true;
                                while(!ressourceItem.isNull() && ok)
                                {
                                    if(ressourceItem.isElement())
                                    {
                                        Industry::Resource ressource;
                                        ressource.quantity=1;
                                        if(ressourceItem.hasAttribute("quantity"))
                                        {
                                            ressource.quantity=ressourceItem.attribute("quantity").toUInt(&ok);
                                            if(!ok)
                                                qDebug() << QString("quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(ressourceItem.hasAttribute("id"))
                                            {
                                                ressource.item=ressourceItem.attribute("id").toUInt(&ok);
                                                if(!ok)
                                                    qDebug() << QString("id is not a number: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                else if(!items.contains(ressource.item))
                                                {
                                                    ok=false;
                                                    qDebug() << QString("id is not into the item list: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                }
                                                else
                                                    industry.ressources << ressource;
                                            }
                                            else
                                            {
                                                ok=false;
                                                qDebug() << QString("have not the id attribute: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        qDebug() << QString("is not a elements: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                    }
                                    ressourceItem = ressourceItem.nextSiblingElement("ressource");
                                }
                            }

                            //product
                            if(ok)
                            {
                                QDomElement productItem = industryItem.firstChildElement("product");
                                ok=true;
                                while(!productItem.isNull() && ok)
                                {
                                    if(productItem.isElement())
                                    {
                                        Industry::Product product;
                                        product.quantity=1;
                                        if(productItem.hasAttribute("quantity"))
                                        {
                                            product.quantity=productItem.attribute("quantity").toUInt(&ok);
                                            if(!ok)
                                                qDebug() << QString("quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(productItem.hasAttribute("id"))
                                            {
                                                product.item=productItem.attribute("id").toUInt(&ok);
                                                if(!ok)
                                                    qDebug() << QString("id is not a number: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                else if(!items.contains(product.item))
                                                {
                                                    ok=false;
                                                    qDebug() << QString("id is not into the item list: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                }
                                                else
                                                    industry.products << product;
                                            }
                                            else
                                            {
                                                ok=false;
                                                qDebug() << QString("have not the id attribute: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        qDebug() << QString("is not a elements: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                    }
                                    productItem = productItem.nextSiblingElement("product");
                                }
                            }

                            //add
                            if(ok)
                            {
                                if(industry.products.isEmpty() || industry.ressources.isEmpty())
                                    qDebug() << QString("product or ressources is empty: %1: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                else
                                    industries[id]=industry;
                            }
                        }
                        else
                            qDebug() << QString("Unable to open the insdutries file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the insdutries file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the insdutries file: %1, have not the id: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the insdutries file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(industryFile.fileName()).arg(industryItem.tagName()).arg(industryItem.lineNumber());
            industryItem = industryItem.nextSiblingElement("industry");
        }
        file_index++;
    }
    return industries;
}

QHash<quint32,quint32> DatapackGeneralLoader::loadIndustriesLink(const QString &file,const QHash<quint32,Industry> &industries)
{
    QHash<quint32,quint32> industriesLink;
    //open and quick check the file
    QFile industriesLinkFile(file);
    QByteArray xmlContent;
    if(!industriesLinkFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, error: %2").arg(industriesLinkFile.fileName()).arg(industriesLinkFile.errorString());
        return industriesLink;
    }
    xmlContent=industriesLinkFile.readAll();
    industriesLinkFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(industriesLinkFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return industriesLink;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="industries")
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, \"industries\" root balise not found for the xml file").arg(industriesLinkFile.fileName());
        return industriesLink;
    }

    //load the content
    bool ok,ok2;
    QDomElement linkItem = root.firstChildElement("link");
    while(!linkItem.isNull())
    {
        if(linkItem.isElement())
        {
            if(linkItem.hasAttribute("industry_id") && linkItem.hasAttribute("factory_id"))
            {
                quint32 industry_id=linkItem.attribute("industry_id").toUInt(&ok);
                quint32 factory_id=linkItem.attribute("factory_id").toUInt(&ok2);
                if(ok && ok2)
                {
                    if(!industriesLink.contains(factory_id))
                    {
                        if(industries.contains(factory_id))
                            industriesLink[factory_id]=industry_id;
                        else
                            qDebug() << QString("Industry id for factory is not found: %1, child.tagName(): %2 (at line: %3)").arg(industriesLinkFile.fileName()).arg(linkItem.tagName()).arg(linkItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Factory already found: %1, child.tagName(): %2 (at line: %3)").arg(industriesLinkFile.fileName()).arg(linkItem.tagName()).arg(linkItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the insdutries link file: %1, the attribute is not a number, child.tagName(): %2 (at line: %3)").arg(industriesLinkFile.fileName()).arg(linkItem.tagName()).arg(linkItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the insdutries link file: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(industriesLinkFile.fileName()).arg(linkItem.tagName()).arg(linkItem.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the insdutries link file: %1, is not a element, child.tagName(): %2 (at line: %3)").arg(industriesLinkFile.fileName()).arg(linkItem.tagName()).arg(linkItem.lineNumber());
        linkItem = linkItem.nextSiblingElement("link");
    }
    return industriesLink;
}

ItemFull DatapackGeneralLoader::loadItems(const QString &folder)
{
    ItemFull items;
    //open and quick check the file
    QFile itemsFile(folder+"items.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return items;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return items;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        qDebug() << QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return items;
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
                quint32 id=item.attribute("id").toUInt(&ok);
                if(ok)
                {
                    if(!items.item.contains(id))
                    {
                        //load the price
                        {
                            if(item.hasAttribute("price"))
                            {
                                bool ok;
                                items.item[id].price=item.attribute("price").toUInt(&ok);
                                if(!ok)
                                {
                                    qDebug() << QString("price is not a number: child.tagName(): %1 (at line: %2)").arg(item.tagName()).arg(item.lineNumber());
                                    items.item[id].price=0;
                                }
                            }
                            else
                            {
                                if(!item.hasAttribute("quest") || item.attribute("quest")!="yes")
                                    qDebug() << QString("For parse item: Price not found, default to 0 (not sellable): child.tagName(): %1 (%2 at line: %3)").arg(item.tagName()).arg(itemsFile.fileName()).arg(item.lineNumber());
                                items.item[id].price=0;
                            }
                        }
                        //load the trap
                        {
                            QDomElement trap = item.firstChildElement("trap");
                            if(!trap.isNull())
                            {
                                if(trap.isElement())
                                {
                                    if(trap.hasAttribute("min_level") && trap.hasAttribute("max_level"))
                                    {
                                        quint8 min_level=trap.attribute("min_level").toUShort(&ok);
                                        if(ok)
                                        {
                                            quint8 max_level=trap.attribute("max_level").toUShort(&ok);
                                            if(ok)
                                            {
                                                Trap trap;
                                                trap.min_level=min_level;
                                                trap.max_level=max_level;
                                                items.trap[id]=trap;
                                            }
                                            else
                                                qDebug() << QString("Unable to open the file: %1, max_level is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(trap.tagName()).arg(trap.lineNumber());
                                        }
                                        else
                                            qDebug() << QString("Unable to open the file: %1, min_level is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(trap.tagName()).arg(trap.lineNumber());
                                    }
                                    else
                                        qDebug() << QString("Unable to open the file: %1, trap have not the attribute min_level or max_level: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(trap.tagName()).arg(trap.lineNumber());
                                }
                            }
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
    return items;
}
