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
    QRegExp typeRegex(QLatin1String("^[a-z]{1,32}$"));
    QDomDocument domDocument;
    QHash<QString, Reputation> reputation;
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        //open and quick check the file
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString()));
            return reputation;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return reputation;
        }
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("list"))
    {
        DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file));
        return reputation;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(QLatin1String("reputation"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QLatin1String("type")))
            {
                QList<qint32> point_list_positive,point_list_negative;
                QStringList text_positive,text_negative;
                QDomElement level = item.firstChildElement(QLatin1String("level"));
                ok=true;
                while(!level.isNull() && ok)
                {
                    if(level.isElement())
                    {
                        if(level.hasAttribute(QLatin1String("point")))
                        {
                            qint32 point=level.attribute(QLatin1String("point")).toInt(&ok);
                            QString text_val;
                            if(ok)
                            {
                                ok=true;
                                bool found=false;
                                int index=0;
                                if(point>=0)
                                {
                                    while(index<point_list_positive.size())
                                    {
                                        if(point_list_positive.at(index)==point)
                                        {
                                            DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
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
                                            DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
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
                            else
                                DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, point is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                    }
                    else
                        DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, point attribute not found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    level = level.nextSiblingElement(QLatin1String("level"));
                }
                if(ok)
                    if(point_list_positive.size()<2)
                    {
                        DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, reputation have to few level: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_positive.contains(0))
                    {
                        DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, no starting level for the positive: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_negative.empty() && !point_list_negative.contains(-1))
                    {
                        DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, no starting level for the negative: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!item.attribute(QLatin1String("type")).contains(typeRegex))
                    {
                        DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, the type %4 don't match wiuth the regex: ^[a-z]{1,32}$: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute(QLatin1String("type"))));
                        ok=false;
                    }
                if(ok)
                {
                    reputation[item.attribute(QLatin1String("type"))].reputation_positive=point_list_positive;
                    reputation[item.attribute(QLatin1String("type"))].reputation_negative=point_list_negative;
                }
            }
            else
                DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(QLatin1String("reputation"));
    }

    return reputation;
}

QHash<quint32, Quest> DatapackGeneralLoader::loadQuests(const QString &folder)
{
    bool ok;
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
        if(!QFile(entryList.at(index).absoluteFilePath()+QLatin1String("/definition.xml")).exists())
        {
            index++;
            continue;
        }
        quint32 questId=entryList.at(index).fileName().toUInt(&ok);
        if(ok)
        {
            //add it, all seam ok
            QPair<bool,Quest> returnedQuest=loadSingleQuest(entryList.at(index).absoluteFilePath()+QLatin1String("/definition.xml"));
            if(returnedQuest.first==true)
            {
                returnedQuest.second.id=questId;
                if(quests.contains(returnedQuest.second.id))
                    qDebug() << QStringLiteral("The quest with id: %1 is already found, disable: %2").arg(returnedQuest.second.id).arg(entryList.at(index).absoluteFilePath()+"/definition.xml");
                else
                    quests[returnedQuest.second.id]=returnedQuest.second;
            }
        }
        else
            DebugClass::debugConsole(QStringLiteral("Unable to open the folder: %1, because is folder name is not a number").arg(entryList.at(index).fileName()));
        index++;
    }
    return quests;
}

QPair<bool,Quest> DatapackGeneralLoader::loadSingleQuest(const QString &file)
{
    CatchChallenger::Quest quest;
    quest.id=0;
    QDomDocument domDocument;
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return QPair<bool,Quest>(false,quest);
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return QPair<bool,Quest>(false,quest);
        }
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("quest"))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(file);
        return QPair<bool,Quest>(false,quest);
    }

    //load the content
    bool ok;
    QList<quint32> defaultBots;
    quest.id=0;
    quest.repeatable=false;
    if(root.hasAttribute(QLatin1String("repeatable")))
        if(root.attribute(QLatin1String("repeatable"))==QLatin1String("yes") || root.attribute(QLatin1String("repeatable"))==QLatin1String("true"))
            quest.repeatable=true;
    if(root.hasAttribute(QLatin1String("bot")))
    {
        QStringList tempStringList=root.attribute(QLatin1String("bot")).split(QLatin1String(";"));
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
    QDomElement requirements = root.firstChildElement(QLatin1String("requirements"));
    while(!requirements.isNull())
    {
        if(requirements.isElement())
        {
            //load requirements reputation
            {
                QDomElement requirementsItem = requirements.firstChildElement(QLatin1String("reputation"));
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute(QLatin1String("type")) && requirementsItem.hasAttribute(QLatin1String("level")))
                        {
                            QString stringLevel=requirementsItem.attribute(QLatin1String("level"));
                            bool positif=!stringLevel.startsWith(QLatin1String("-"));
                            if(!positif)
                                stringLevel.remove(0,1);
                            quint8 level=stringLevel.toUShort(&ok);
                            if(ok)
                            {
                                CatchChallenger::Quest::ReputationRequirements reputation;
                                reputation.level=level;
                                reputation.positif=positif;
                                reputation.type=requirementsItem.attribute(QLatin1String("type"));
                                quest.requirements.reputation << reputation;
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, reputation is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber()).arg(stringLevel);
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, have not attribute type or level: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    requirementsItem = requirementsItem.nextSiblingElement(QLatin1String("requirements"));
                }
            }
            //load requirements quest
            {
                QDomElement requirementsItem = requirements.firstChildElement(QLatin1String("quest"));
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute(QLatin1String("id")))
                        {
                            quint32 questId=requirementsItem.attribute(QLatin1String("id")).toUInt(&ok);
                            if(ok)
                                quest.requirements.quests << questId;
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, requirement quest item id is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber()).arg(requirementsItem.attribute(QLatin1String("id")));
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, requirement quest item have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    requirementsItem = requirementsItem.nextSiblingElement(QLatin1String("quest"));
                }
            }
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(requirements.tagName()).arg(requirements.lineNumber());
        requirements = requirements.nextSiblingElement(QLatin1String("requirements"));
    }

    //load rewards
    QDomElement rewards = root.firstChildElement(QLatin1String("rewards"));
    while(!rewards.isNull())
    {
        if(rewards.isElement())
        {
            //load rewards reputation
            {
                QDomElement reputationItem = rewards.firstChildElement(QLatin1String("reputation"));
                while(!reputationItem.isNull())
                {
                    if(reputationItem.isElement())
                    {
                        if(reputationItem.hasAttribute(QLatin1String("type")) && reputationItem.hasAttribute(QLatin1String("point")))
                        {
                            qint32 point=reputationItem.attribute(QLatin1String("point")).toInt(&ok);
                            if(ok)
                            {
                                CatchChallenger::Quest::ReputationRewards reputation;
                                reputation.point=point;
                                reputation.type=reputationItem.attribute(QLatin1String("type"));
                                quest.rewards.reputation << reputation;
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, quest rewards point is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, quest rewards point have not type or point attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                    reputationItem = reputationItem.nextSiblingElement(QLatin1String("rewards"));
                }
            }
            //load rewards item
            {
                QDomElement rewardsItem = rewards.firstChildElement(QLatin1String("item"));
                while(!rewardsItem.isNull())
                {
                    if(rewardsItem.isElement())
                    {
                        if(rewardsItem.hasAttribute(QLatin1String("id")))
                        {
                            CatchChallenger::Quest::Item item;
                            item.item=rewardsItem.attribute(QLatin1String("id")).toUInt(&ok);
                            item.quantity=1;
                            if(ok)
                            {
                                if(!CommonDatapack::commonDatapack.items.item.contains(item.item))
                                {
                                    qDebug() << QStringLiteral("Unable to open the file: %1, rewards item id is not into the item list: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber()).arg(rewardsItem.attribute(QLatin1String("id")));
                                    return QPair<bool,Quest>(false,quest);
                                }
                                if(rewardsItem.hasAttribute(QLatin1String("quantity")))
                                {
                                    item.quantity=rewardsItem.attribute(QLatin1String("quantity")).toUInt(&ok);
                                    if(!ok)
                                        item.quantity=1;
                                }
                                quest.rewards.items << item;
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, rewards item id is not a number: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber()).arg(rewardsItem.attribute(QLatin1String("id")));
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, rewards item have not attribute id: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    rewardsItem = rewardsItem.nextSiblingElement(QLatin1String("quest"));
                }
            }
            //load rewards allow
            {
                QDomElement allowItem = rewards.firstChildElement(QLatin1String("allow"));
                while(!allowItem.isNull())
                {
                    if(allowItem.isElement())
                    {
                        if(allowItem.hasAttribute(QLatin1String("type")))
                        {
                            if(allowItem.attribute(QLatin1String("type"))==QLatin1String("clan"))
                                quest.rewards.allow << CatchChallenger::ActionAllow_Clan;
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, allow type not understand: child.tagName(): %2 (at line: %3)").arg(file).arg(allowItem.tagName()).arg(allowItem.lineNumber()).arg(allowItem.attribute(QLatin1String("id")));
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, rewards item have not attribute id: child.tagName(): %2 (at line: %3)").arg(file).arg(allowItem.tagName()).arg(allowItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(allowItem.tagName()).arg(allowItem.lineNumber());
                    allowItem = allowItem.nextSiblingElement(QLatin1String("allow"));
                }
            }
            quest.rewards.allow.fromSet(quest.rewards.allow.toSet());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(rewards.tagName()).arg(rewards.lineNumber());
        rewards = rewards.nextSiblingElement("rewards");
    }

    QHash<quint8,CatchChallenger::Quest::Step> steps;
    //load step
    QDomElement step = root.firstChildElement(QLatin1String("step"));
    while(!step.isNull())
    {
        if(step.isElement())
        {
            if(step.hasAttribute(QLatin1String("id")))
            {
                quint32 id=step.attribute(QLatin1String("id")).toUInt(&ok);
                if(ok)
                {
                    CatchChallenger::Quest::Step stepObject;
                    if(step.hasAttribute(QLatin1String("bot")))
                    {
                        QStringList tempStringList=step.attribute(QLatin1String("bot")).split(QLatin1String(";"));
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
                        QDomElement stepItem = step.firstChildElement(QLatin1String("item"));
                        while(!stepItem.isNull())
                        {
                            if(stepItem.isElement())
                            {
                                if(stepItem.hasAttribute(QLatin1String("id")))
                                {
                                    CatchChallenger::Quest::Item item;
                                    item.item=stepItem.attribute(QLatin1String("id")).toUInt(&ok);
                                    item.quantity=1;
                                    if(ok)
                                    {
                                        if(!CommonDatapack::commonDatapack.items.item.contains(item.item))
                                        {
                                            qDebug() << QStringLiteral("Unable to open the file: %1, rewards item id is not into the item list: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(stepItem.tagName()).arg(stepItem.lineNumber()).arg(stepItem.attribute(QLatin1String("id")));
                                            return QPair<bool,Quest>(false,quest);
                                        }
                                        if(stepItem.hasAttribute(QLatin1String("quantity")))
                                        {
                                            item.quantity=stepItem.attribute(QLatin1String("quantity")).toUInt(&ok);
                                            if(!ok)
                                                item.quantity=1;
                                        }
                                        stepObject.requirements.items << item;
                                        if(stepItem.hasAttribute(QLatin1String("monster")) && stepItem.hasAttribute(QLatin1String("rate")))
                                        {
                                            CatchChallenger::Quest::ItemMonster itemMonster;
                                            itemMonster.item=item.item;

                                            QStringList tempStringList=stepItem.attribute(QLatin1String("monster")).split(QLatin1String(";"));
                                            int index=0;
                                            while(index<tempStringList.size())
                                            {
                                                quint32 tempInt=tempStringList.at(index).toUInt(&ok);
                                                if(ok)
                                                    itemMonster.monsters << tempInt;
                                                index++;
                                            }

                                            QString rateString=stepItem.attribute(QLatin1String("rate"));
                                            rateString.remove(QLatin1String("%"));
                                            itemMonster.rate=rateString.toUShort(&ok);
                                            if(ok)
                                                stepObject.itemsMonster << itemMonster;
                                        }
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, step id is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber()).arg(stepItem.attribute(QLatin1String("id")));
                                }
                                else
                                    qDebug() << QStringLiteral("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            stepItem = stepItem.nextSiblingElement(QLatin1String("item"));
                        }
                    }
                    //do the fight
                    {
                        QDomElement fightItem = step.firstChildElement(QLatin1String("fight"));
                        while(!fightItem.isNull())
                        {
                            if(fightItem.isElement())
                            {
                                if(fightItem.hasAttribute(QLatin1String("id")))
                                {
                                    quint32 fightId=fightItem.attribute(QLatin1String("id")).toUInt(&ok);
                                    if(ok)
                                        stepObject.requirements.fightId << fightId;
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, step id is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber()).arg(fightItem.attribute(QLatin1String("id")));
                                }
                                else
                                    qDebug() << QStringLiteral("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            fightItem = fightItem.nextSiblingElement(QLatin1String("fight"));
                        }
                    }
                    steps[id]=stepObject;
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, step id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
        step = step.nextSiblingElement(QLatin1String("step"));
    }

    //sort the step
    int indexLoop=1;
    while(indexLoop<(steps.size()+1))
    {
        if(!steps.contains(indexLoop))
            break;
        quest.steps << steps.value(indexLoop);
        indexLoop++;
    }
    if(indexLoop<(steps.size()+1))
        return QPair<bool,Quest>(false,quest);
    return QPair<bool,Quest>(true,quest);
}

QHash<quint8, Plant> DatapackGeneralLoader::loadPlants(const QString &file)
{
    QHash<quint8, Plant> plants;
    QDomDocument domDocument;
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile plantsFile(file);
        QByteArray xmlContent;
        if(!plantsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the plants file: %1, error: %2").arg(file).arg(plantsFile.errorString());
            return plants;
        }
        xmlContent=plantsFile.readAll();
        plantsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the plants file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return plants;
        }
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("plants"))
    {
        qDebug() << QStringLiteral("Unable to open the plants file: %1, \"plants\" root balise not found for the xml file").arg(file);
        return plants;
    }

    //load the content
    bool ok,ok2;
    QDomElement plantItem = root.firstChildElement(QLatin1String("plant"));
    while(!plantItem.isNull())
    {
        if(plantItem.isElement())
        {
            if(plantItem.hasAttribute(QLatin1String("id")) && plantItem.hasAttribute(QLatin1String("itemUsed")))
            {
                quint8 id=plantItem.attribute(QLatin1String("id")).toUShort(&ok);
                quint32 itemUsed=plantItem.attribute(QLatin1String("itemUsed")).toUInt(&ok2);
                if(ok && ok2)
                {
                    if(!plants.contains(id))
                    {
                        Plant plant;
                        plant.fruits_seconds=0;
                        plant.sprouted_seconds=0;
                        plant.taller_seconds=0;
                        plant.flowering_seconds=0;
                        plant.itemUsed=itemUsed;
                        ok=false;
                        QDomElement quantity = plantItem.firstChildElement(QLatin1String("quantity"));
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
                        int intermediateTimeCount=0;
                        QDomElement grow = plantItem.firstChildElement(QLatin1String("grow"));
                        if(!grow.isNull())
                        {
                            if(grow.isElement())
                            {
                                QDomElement fruits = grow.firstChildElement(QLatin1String("fruits"));
                                if(!fruits.isNull())
                                {
                                    if(fruits.isElement())
                                    {
                                        plant.fruits_seconds=fruits.text().toUInt(&ok2)*60;
                                        plant.sprouted_seconds=plant.fruits_seconds;
                                        plant.taller_seconds=plant.fruits_seconds;
                                        plant.flowering_seconds=plant.fruits_seconds;
                                        if(!ok2)
                                        {
                                            qDebug() << QStringLiteral("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(fruits.tagName()).arg(fruits.lineNumber()).arg(fruits.text());
                                            ok=false;
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        qDebug() << QStringLiteral("Unable to parse the plants file: %1, fruits is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(fruits.tagName()).arg(fruits.lineNumber());
                                    }
                                }
                                else
                                {
                                    ok=false;
                                    qDebug() << QStringLiteral("Unable to parse the plants file: %1, fruits is null: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                                }
                                QDomElement sprouted = grow.firstChildElement(QLatin1String("sprouted"));
                                if(!sprouted.isNull())
                                {
                                    if(sprouted.isElement())
                                    {
                                        plant.sprouted_seconds=sprouted.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QStringLiteral("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(sprouted.tagName()).arg(sprouted.lineNumber()).arg(sprouted.text());
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to parse the plants file: %1, sprouted is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(sprouted.tagName()).arg(sprouted.lineNumber());
                                }
                                QDomElement taller = grow.firstChildElement(QLatin1String("taller"));
                                if(!taller.isNull())
                                {
                                    if(taller.isElement())
                                    {
                                        plant.taller_seconds=taller.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QStringLiteral("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(taller.tagName()).arg(taller.lineNumber()).arg(taller.text());
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to parse the plants file: %1, taller is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(taller.tagName()).arg(taller.lineNumber());
                                }
                                QDomElement flowering = grow.firstChildElement(QLatin1String("flowering"));
                                if(!flowering.isNull())
                                {
                                    if(flowering.isElement())
                                    {
                                        plant.flowering_seconds=flowering.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            ok=false;
                                            qDebug() << QStringLiteral("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(flowering.tagName()).arg(flowering.lineNumber()).arg(flowering.text());
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to parse the plants file: %1, flowering is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(flowering.tagName()).arg(flowering.lineNumber());
                                }
                            }
                            else
                                qDebug() << QStringLiteral("Unable to parse the plants file: %1, grow is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                        }
                        else
                            qDebug() << QStringLiteral("Unable to parse the plants file: %1, grow is null: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                        if(ok)
                        {
                            bool needIntermediateTimeFix=false;
                            if(plant.flowering_seconds>=plant.fruits_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    qDebug() << QStringLiteral("Warning when parse the plants file: %1, flowering_seconds>=fruits_seconds: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                            }
                            if(plant.taller_seconds>=plant.flowering_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    qDebug() << QStringLiteral("Warning when parse the plants file: %1, taller_seconds>=flowering_seconds: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                            }
                            if(plant.sprouted_seconds>=plant.taller_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    qDebug() << QStringLiteral("Warning when parse the plants file: %1, sprouted_seconds>=taller_seconds: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                            }
                            if(plant.sprouted_seconds<=0)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    qDebug() << QStringLiteral("Warning when parse the plants file: %1, sprouted_seconds<=0: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                            }
                            if(needIntermediateTimeFix)
                            {
                                plant.flowering_seconds=plant.fruits_seconds*3/4;
                                plant.taller_seconds=plant.fruits_seconds*2/4;
                                plant.sprouted_seconds=plant.fruits_seconds*1/4;
                            }
                            plants[id]=plant;
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the plants file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(plantItem.tagName()).arg(plantItem.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the plants file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(plantItem.tagName()).arg(plantItem.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the plants file: %1, have not the plant id: child.tagName(): %2 (at line: %3)").arg(file).arg(plantItem.tagName()).arg(plantItem.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the plants file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(plantItem.tagName()).arg(plantItem.lineNumber());
        plantItem = plantItem.nextSiblingElement(QLatin1String("plant"));
    }
    return plants;
}

QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> > DatapackGeneralLoader::loadCraftingRecipes(const QString &file,const QHash<quint32, Item> &items)
{
    QHash<quint32,CrafingRecipe> crafingRecipes;
    QHash<quint32,quint32> itemToCrafingRecipes;
    QDomDocument domDocument;
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile craftingRecipesFile(file);
        QByteArray xmlContent;
        if(!craftingRecipesFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, error: %2").arg(file).arg(craftingRecipesFile.errorString());
            return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
        }
        xmlContent=craftingRecipesFile.readAll();
        craftingRecipesFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
        }
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("recipes"))
    {
        qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, \"recipes\" root balise not found for the xml file").arg(file);
        return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
    }

    //load the content
    bool ok,ok2,ok3;
    QDomElement recipeItem = root.firstChildElement(QLatin1String("recipe"));
    while(!recipeItem.isNull())
    {
        if(recipeItem.isElement())
        {
            if(recipeItem.hasAttribute(QLatin1String("id")) && recipeItem.hasAttribute(QLatin1String("itemToLearn")) && recipeItem.hasAttribute(QLatin1String("doItemId")))
            {
                quint8 success=100;
                if(recipeItem.hasAttribute(QLatin1String("success")))
                {
                    quint8 tempShort=recipeItem.attribute(QLatin1String("success")).toUShort(&ok);
                    if(ok)
                    {
                        if(tempShort>100)
                            qDebug() << QStringLiteral("preload_crafting_recipes() success can't be greater than 100 for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                        else
                            success=tempShort;
                    }
                    else
                        qDebug() << QStringLiteral("preload_crafting_recipes() success in not an number for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }
                quint16 quantity=1;
                if(recipeItem.hasAttribute(QLatin1String("quantity")))
                {
                    quint32 tempShort=recipeItem.attribute(QLatin1String("quantity")).toUInt(&ok);
                    if(ok)
                    {
                        if(tempShort>65535)
                            qDebug() << QStringLiteral("preload_crafting_recipes() quantity can't be greater than 65535 for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                        else
                            quantity=tempShort;
                    }
                    else
                        qDebug() << QStringLiteral("preload_crafting_recipes() quantity in not an number for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }

                quint32 id=recipeItem.attribute(QLatin1String("id")).toUInt(&ok);
                quint32 itemToLearn=recipeItem.attribute(QLatin1String("itemToLearn")).toUInt(&ok2);
                quint32 doItemId=recipeItem.attribute(QLatin1String("doItemId")).toUInt(&ok3);
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
                        QDomElement material = recipeItem.firstChildElement(QLatin1String("material"));
                        while(!material.isNull() && ok)
                        {
                            if(material.isElement())
                            {
                                if(material.hasAttribute(QLatin1String("itemId")))
                                {
                                    quint32 itemId=material.attribute(QLatin1String("itemId")).toUInt(&ok2);
                                    if(!ok2)
                                    {
                                        ok=false;
                                        qDebug() << QStringLiteral("preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                        break;
                                    }
                                    quint16 quantity=1;
                                    if(material.hasAttribute(QLatin1String("quantity")))
                                    {
                                        quint32 tempShort=material.attribute(QLatin1String("quantity")).toUInt(&ok2);
                                        if(ok2)
                                        {
                                            if(tempShort>65535)
                                            {
                                                ok=false;
                                                qDebug() << QStringLiteral("preload_crafting_recipes() material quantity can't be greater than 65535 for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                                break;
                                            }
                                            else
                                                quantity=tempShort;
                                        }
                                        else
                                        {
                                            ok=false;
                                            qDebug() << QStringLiteral("preload_crafting_recipes() material quantity in not an number for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                            break;
                                        }
                                    }
                                    if(!items.contains(itemId))
                                    {
                                        ok=false;
                                        qDebug() << QStringLiteral("preload_crafting_recipes() material itemId in not into items list for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                        break;
                                    }
                                    CatchChallenger::CrafingRecipe::Material newMaterial;
                                    newMaterial.item=itemId;
                                    newMaterial.quantity=quantity;
                                    int index=0;
                                    while(index<recipe.materials.size())
                                    {
                                        if(recipe.materials.at(index).item==newMaterial.item)
                                            break;
                                        index++;
                                    }
                                    if(index<recipe.materials.size())
                                    {
                                        ok=false;
                                        qDebug() << QStringLiteral("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                    }
                                    else
                                    {
                                        if(recipe.doItemId==newMaterial.item)
                                        {
                                            qDebug() << QStringLiteral("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                            ok=false;
                                        }
                                        else
                                            recipe.materials << newMaterial;
                                    }
                                }
                                else
                                    qDebug() << QStringLiteral("preload_crafting_recipes() material have not attribute itemId for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                            else
                                qDebug() << QStringLiteral("preload_crafting_recipes() material is not an element for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            material = material.nextSiblingElement(QLatin1String("material"));
                        }
                        if(ok)
                        {
                            if(!items.contains(recipe.itemToLearn))
                            {
                                ok=false;
                                qDebug() << QStringLiteral("preload_crafting_recipes() itemToLearn is not into items list for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            if(!items.contains(recipe.doItemId))
                            {
                                ok=false;
                                qDebug() << QStringLiteral("preload_crafting_recipes() doItemId is not into items list for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.contains(recipe.itemToLearn))
                            {
                                ok=false;
                                qDebug() << QStringLiteral("preload_crafting_recipes() itemToLearn already used to learn another recipe: %4: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()).arg(itemToCrafingRecipes.value(recipe.itemToLearn));
                            }
                        }
                        if(ok)
                        {
                            if(recipe.itemToLearn==recipe.doItemId)
                            {
                                ok=false;
                                qDebug() << QStringLiteral("preload_crafting_recipes() the product of the recipe can't be them self: %4: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()).arg(id);
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.contains(recipe.doItemId))
                            {
                                ok=false;
                                qDebug() << QStringLiteral("preload_crafting_recipes() the product of the recipe can't be a recipe: %4: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()).arg(itemToCrafingRecipes.value(recipe.doItemId));
                            }
                        }
                        if(ok)
                        {
                            crafingRecipes[id]=recipe;
                            itemToCrafingRecipes[recipe.itemToLearn]=id;
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, have not the crafting recipe id: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
        recipeItem = recipeItem.nextSiblingElement(QLatin1String("recipe"));
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
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        QDomDocument domDocument;
        const QString &file=fileList.at(file_index).absoluteFilePath();
        //open and quick check the file
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
            QFile industryFile(file);
            QByteArray xmlContent;
            if(!industryFile.open(QIODevice::ReadOnly))
            {
                qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, error: %2").arg(file).arg(industryFile.errorString());
                file_index++;
                continue;
            }
            xmlContent=industryFile.readAll();
            industryFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
                file_index++;
                continue;
            }
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=QLatin1String("industries"))
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, \"industries\" root balise not found for the xml file").arg(file);
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2,ok3;
        QDomElement industryItem = root.firstChildElement(QLatin1String("industrialrecipe"));
        while(!industryItem.isNull())
        {
            if(industryItem.isElement())
            {
                if(industryItem.hasAttribute(QLatin1String("id")) && industryItem.hasAttribute(QLatin1String("time")) && industryItem.hasAttribute(QLatin1String("cycletobefull")))
                {
                    Industry industry;
                    quint32 id=industryItem.attribute(QLatin1String("id")).toUInt(&ok);
                    industry.time=industryItem.attribute(QLatin1String("time")).toUInt(&ok2);
                    industry.cycletobefull=industryItem.attribute(QLatin1String("cycletobefull")).toUInt(&ok3);
                    if(ok && ok2 && ok3)
                    {
                        if(!industries.contains(id))
                        {
                            if(industry.time<60*5)
                            {
                                qDebug() << QStringLiteral("the time need be greater than 5*60 seconds to not slow down the server: %4, %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber()).arg(industry.time);
                                industry.time=60*5;
                            }
                            if(industry.cycletobefull<1)
                            {
                                qDebug() << QStringLiteral("cycletobefull need be greater than 0: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                industry.cycletobefull=1;
                            }
                            else if(industry.cycletobefull>65535)
                            {
                                qDebug() << QStringLiteral("cycletobefull need be lower to 10 to not slow down the server, use the quantity: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                industry.cycletobefull=10;
                            }
                            //resource
                            {
                                QDomElement resourceItem = industryItem.firstChildElement(QLatin1String("resource"));
                                ok=true;
                                while(!resourceItem.isNull() && ok)
                                {
                                    if(resourceItem.isElement())
                                    {
                                        Industry::Resource resource;
                                        resource.quantity=1;
                                        if(resourceItem.hasAttribute(QLatin1String("quantity")))
                                        {
                                            resource.quantity=resourceItem.attribute(QLatin1String("quantity")).toUInt(&ok);
                                            if(!ok)
                                                qDebug() << QStringLiteral("quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(resourceItem.hasAttribute(QLatin1String("id")))
                                            {
                                                resource.item=resourceItem.attribute(QLatin1String("id")).toUInt(&ok);
                                                if(!ok)
                                                    qDebug() << QStringLiteral("id is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                else if(!items.contains(resource.item))
                                                {
                                                    ok=false;
                                                    qDebug() << QStringLiteral("id is not into the item list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                }
                                                else
                                                {
                                                    int index=0;
                                                    while(index<industry.resources.size())
                                                    {
                                                        if(industry.resources.at(index).item==resource.item)
                                                            break;
                                                        index++;
                                                    }
                                                    if(index<industry.resources.size())
                                                    {
                                                        ok=false;
                                                        qDebug() << QStringLiteral("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                    }
                                                    else
                                                    {
                                                        index=0;
                                                        while(index<industry.products.size())
                                                        {
                                                            if(industry.products.at(index).item==resource.item)
                                                                break;
                                                            index++;
                                                        }
                                                        if(index<industry.products.size())
                                                        {
                                                            ok=false;
                                                            qDebug() << QStringLiteral("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                        }
                                                        else
                                                            industry.resources << resource;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                qDebug() << QStringLiteral("have not the id attribute: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        qDebug() << QStringLiteral("is not a elements: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                    }
                                    resourceItem = resourceItem.nextSiblingElement(QLatin1String("resource"));
                                }
                            }

                            //product
                            if(ok)
                            {
                                QDomElement productItem = industryItem.firstChildElement(QLatin1String("product"));
                                ok=true;
                                while(!productItem.isNull() && ok)
                                {
                                    if(productItem.isElement())
                                    {
                                        Industry::Product product;
                                        product.quantity=1;
                                        if(productItem.hasAttribute(QLatin1String("quantity")))
                                        {
                                            product.quantity=productItem.attribute(QLatin1String("quantity")).toUInt(&ok);
                                            if(!ok)
                                                qDebug() << QStringLiteral("quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(productItem.hasAttribute(QLatin1String("id")))
                                            {
                                                product.item=productItem.attribute(QLatin1String("id")).toUInt(&ok);
                                                if(!ok)
                                                    qDebug() << QStringLiteral("id is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                else if(!items.contains(product.item))
                                                {
                                                    ok=false;
                                                    qDebug() << QStringLiteral("id is not into the item list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                }
                                                else
                                                {
                                                    int index=0;
                                                    while(index<industry.resources.size())
                                                    {
                                                        if(industry.resources.at(index).item==product.item)
                                                            break;
                                                        index++;
                                                    }
                                                    if(index<industry.resources.size())
                                                    {
                                                        ok=false;
                                                        qDebug() << QStringLiteral("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                    }
                                                    else
                                                    {
                                                        index=0;
                                                        while(index<industry.products.size())
                                                        {
                                                            if(industry.products.at(index).item==product.item)
                                                                break;
                                                            index++;
                                                        }
                                                        if(index<industry.products.size())
                                                        {
                                                            ok=false;
                                                            qDebug() << QStringLiteral("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                        }
                                                        else
                                                            industry.products << product;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                qDebug() << QStringLiteral("have not the id attribute: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        qDebug() << QStringLiteral("is not a elements: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                    }
                                    productItem = productItem.nextSiblingElement(QLatin1String("product"));
                                }
                            }

                            //add
                            if(ok)
                            {
                                if(industry.products.isEmpty() || industry.resources.isEmpty())
                                    qDebug() << QStringLiteral("product or resources is empty: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                else
                                    industries[id]=industry;
                            }
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the industries file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the industries file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the industries file: %1, have not the id: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the industries file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
            industryItem = industryItem.nextSiblingElement(QLatin1String("industrialrecipe"));
        }
        file_index++;
    }
    return industries;
}

QHash<quint32,quint32> DatapackGeneralLoader::loadIndustriesLink(const QString &file,const QHash<quint32,Industry> &industries)
{
    QHash<quint32,quint32> industriesLink;
    QDomDocument domDocument;
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile industriesLinkFile(file);
        QByteArray xmlContent;
        if(!industriesLinkFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, error: %2").arg(file).arg(industriesLinkFile.errorString());
            return industriesLink;
        }
        xmlContent=industriesLinkFile.readAll();
        industriesLinkFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return industriesLink;
        }
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("industries"))
    {
        qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, \"industries\" root balise not found for the xml file").arg(file);
        return industriesLink;
    }

    //load the content
    bool ok,ok2;
    QDomElement linkItem = root.firstChildElement(QLatin1String("link"));
    while(!linkItem.isNull())
    {
        if(linkItem.isElement())
        {
            if(linkItem.hasAttribute(QLatin1String("industrialrecipe")) && linkItem.hasAttribute(QLatin1String("industry")))
            {
                quint32 industry_id=linkItem.attribute(QLatin1String("industrialrecipe")).toUInt(&ok);
                quint32 factory_id=linkItem.attribute(QLatin1String("industry")).toUInt(&ok2);
                if(ok && ok2)
                {
                    if(!industriesLink.contains(factory_id))
                    {
                        if(industries.contains(industry_id))
                            industriesLink[industry_id]=industry_id;
                        else
                            qDebug() << QStringLiteral("Industry id for factory is not found: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Factory already found: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the industries link file: %1, the attribute is not a number, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the industries link file: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the industries link file: %1, is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
        linkItem = linkItem.nextSiblingElement(QLatin1String("link"));
    }
    return industriesLink;
}

ItemFull DatapackGeneralLoader::loadItems(const QString &folder,const QHash<quint32,Buff> &monsterBuffs)
{
    ItemFull items;
    QDomDocument domDocument;
    //open and quick check the file
    const QString &file=folder+QLatin1String("items.xml");
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return items;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return items;
        }
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("items"))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file);
        return items;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(QLatin1String("item"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QLatin1String("id")))
            {
                quint32 id=item.attribute(QLatin1String("id")).toUInt(&ok);
                if(ok)
                {
                    if(!items.item.contains(id))
                    {
                        //load the price
                        {
                            if(item.hasAttribute(QLatin1String("price")))
                            {
                                bool ok;
                                items.item[id].price=item.attribute(QLatin1String("price")).toUInt(&ok);
                                if(!ok)
                                {
                                    qDebug() << QStringLiteral("price is not a number: child.tagName(): %1 (at line: %2)").arg(item.tagName()).arg(item.lineNumber());
                                    items.item[id].price=0;
                                }
                            }
                            else
                            {
                                /*if(!item.hasAttribute(QLatin1String("quest")) || item.attribute(QLatin1String("quest"))!=QLatin1String("yes"))
                                    qDebug() << QStringLiteral("For parse item: Price not found, default to 0 (not sellable): child.tagName(): %1 (%2 at line: %3)").arg(item.tagName()).arg(file).arg(item.lineNumber());*/
                                items.item[id].price=0;
                            }
                        }
                        //load the consumeAtUse
                        {
                            if(item.hasAttribute(QLatin1String("consumeAtUse")))
                            {
                                if(item.attribute(QLatin1String("consumeAtUse"))==QLatin1String("false"))
                                    items.item[id].consumeAtUse=false;
                                else
                                    items.item[id].consumeAtUse=true;
                            }
                            else
                                items.item[id].consumeAtUse=true;
                        }
                        bool haveAnEffect=false;
                        //load the trap
                        if(!haveAnEffect)
                        {
                            QDomElement trapItem = item.firstChildElement(QLatin1String("trap"));
                            if(!trapItem.isNull())
                            {
                                if(trapItem.isElement())
                                {
                                    Trap trap;
                                    trap.bonus_rate=1.0;
                                    if(trapItem.hasAttribute(QLatin1String("bonus_rate")))
                                    {
                                        float bonus_rate=trapItem.attribute(QLatin1String("bonus_rate")).toFloat(&ok);
                                        if(ok)
                                            trap.bonus_rate=bonus_rate;
                                        else
                                            qDebug() << QStringLiteral("Unable to open the file: %1, bonus_rate is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(trapItem.tagName()).arg(trapItem.lineNumber());
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, trap have not the attribute bonus_rate: child.tagName(): %2 (at line: %3)").arg(file).arg(trapItem.tagName()).arg(trapItem.lineNumber());
                                    items.trap[id]=trap;
                                    haveAnEffect=true;
                                }
                            }
                        }
                        //load the repel
                        if(!haveAnEffect)
                        {
                            QDomElement repelItem = item.firstChildElement(QLatin1String("repel"));
                            if(!repelItem.isNull())
                            {
                                if(repelItem.isElement())
                                {
                                    if(repelItem.hasAttribute(QLatin1String("step")))
                                    {
                                        quint32 step=repelItem.attribute(QLatin1String("step")).toUInt(&ok);
                                        if(ok)
                                        {
                                            if(step>0)
                                            {
                                                items.repel[id]=step;
                                                haveAnEffect=true;
                                            }
                                            else
                                                qDebug() << QStringLiteral("Unable to open the file: %1, step is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                        }
                                        else
                                            qDebug() << QStringLiteral("Unable to open the file: %1, step is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, trap have not the attribute min_level or max_level: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                }
                            }
                        }
                        //load the monster effect
                        if(!haveAnEffect)
                        {
                            {
                                QDomElement hpItem = item.firstChildElement(QLatin1String("hp"));
                                while(!hpItem.isNull())
                                {
                                    if(hpItem.isElement())
                                    {
                                        if(hpItem.hasAttribute(QLatin1String("add")))
                                        {
                                            if(hpItem.attribute(QLatin1String("add"))==QLatin1String("all"))
                                            {
                                                MonsterItemEffect monsterItemEffect;
                                                monsterItemEffect.type=MonsterItemEffectType_AddHp;
                                                monsterItemEffect.value=-1;
                                                items.monsterItemEffect.insert(id,monsterItemEffect);
                                            }
                                            else
                                            {
                                                qint32 add=hpItem.attribute(QLatin1String("add")).toUInt(&ok);
                                                if(ok)
                                                {
                                                    if(add>0)
                                                    {
                                                        MonsterItemEffect monsterItemEffect;
                                                        monsterItemEffect.type=MonsterItemEffectType_AddHp;
                                                        monsterItemEffect.value=add;
                                                        items.monsterItemEffect.insert(id,monsterItemEffect);
                                                    }
                                                    else
                                                        qDebug() << QStringLiteral("Unable to open the file: %1, step is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                                }
                                                else
                                                    qDebug() << QStringLiteral("Unable to open the file: %1, step is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                            }
                                        }
                                        else
                                            qDebug() << QStringLiteral("Unable to open the file: %1, trap have not the attribute min_level or max_level: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                    }
                                    hpItem = hpItem.nextSiblingElement(QLatin1String("hp"));
                                }
                            }
                            {
                                QDomElement buffItem = item.firstChildElement(QLatin1String("buff"));
                                while(!buffItem.isNull())
                                {
                                    if(buffItem.isElement())
                                    {
                                        if(buffItem.hasAttribute(QLatin1String("remove")))
                                        {
                                            if(buffItem.attribute(QLatin1String("remove"))==QLatin1String("all"))
                                            {
                                                MonsterItemEffect monsterItemEffect;
                                                monsterItemEffect.type=MonsterItemEffectType_RemoveBuff;
                                                monsterItemEffect.value=-1;
                                                items.monsterItemEffect.insert(id,monsterItemEffect);
                                            }
                                            else
                                            {
                                                qint32 remove=buffItem.attribute(QLatin1String("remove")).toUInt(&ok);
                                                if(ok)
                                                {
                                                    if(remove>0)
                                                    {
                                                        if(monsterBuffs.contains(remove))
                                                        {
                                                            MonsterItemEffect monsterItemEffect;
                                                            monsterItemEffect.type=MonsterItemEffectType_RemoveBuff;
                                                            monsterItemEffect.value=remove;
                                                            items.monsterItemEffect.insert(id,monsterItemEffect);
                                                        }
                                                        else
                                                            qDebug() << QStringLiteral("Unable to open the file: %1, buff item to remove is not found: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                                    }
                                                    else
                                                        qDebug() << QStringLiteral("Unable to open the file: %1, step is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                                }
                                                else
                                                    qDebug() << QStringLiteral("Unable to open the file: %1, step is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                            }
                                        }
                                        else
                                            qDebug() << QStringLiteral("Unable to open the file: %1, trap have not the attribute min_level or max_level: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                    }
                                    buffItem = buffItem.nextSiblingElement(QLatin1String("buff"));
                                }
                            }
                            if(items.monsterItemEffect.contains(id))
                                haveAnEffect=true;
                        }
                        //load the monster offline effect
                        if(!haveAnEffect)
                        {
                            QDomElement levelItem = item.firstChildElement(QLatin1String("level"));
                            while(!levelItem.isNull())
                            {
                                if(levelItem.isElement())
                                {
                                    if(levelItem.hasAttribute(QLatin1String("up")))
                                    {
                                        const quint32 &levelUp=levelItem.attribute(QLatin1String("up")).toUInt(&ok);
                                        if(!ok)
                                            qDebug() << QStringLiteral("Unable to open the file: %1, level up is not possitive number: child.tagName(): %2 (at line: %3)").arg(file).arg(levelItem.tagName()).arg(levelItem.lineNumber());
                                        else if(levelUp<=0)
                                            qDebug() << QStringLiteral("Unable to open the file: %1, level up is greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(levelItem.tagName()).arg(levelItem.lineNumber());
                                        else
                                        {
                                            MonsterItemEffectOutOfFight monsterItemEffectOutOfFight;
                                            monsterItemEffectOutOfFight.type=MonsterItemEffectTypeOutOfFight_AddLevel;
                                            monsterItemEffectOutOfFight.value=levelUp;
                                            items.monsterItemEffectOutOfFight.insert(id,monsterItemEffectOutOfFight);
                                        }
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, level have not the attribute up: child.tagName(): %2 (at line: %3)").arg(file).arg(levelItem.tagName()).arg(levelItem.lineNumber());
                                }
                                levelItem = levelItem.nextSiblingElement(QLatin1String("level"));
                            }
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement(QLatin1String("item"));
    }
    return items;
}

QPair<QList<QDomElement>, QList<Profile> > DatapackGeneralLoader::loadProfileList(const QString &datapackPath, const QString &file,const QHash<quint32, Item> &items,const QHash<quint32,Monster> &monsters,const QHash<QString, Reputation> &reputations)
{
    QPair<QList<QDomElement>, QList<Profile> > returnVar;
    QDomDocument domDocument;
    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile xmlFile(file);
        QByteArray xmlContent;
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file to have new profile: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return returnVar;
        }
        xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return returnVar;
        }
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("list"))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return returnVar;
    }

    //load the content
    bool ok;
    QDomElement startItem = root.firstChildElement(QLatin1String("start"));
    while(!startItem.isNull())
    {
        if(startItem.isElement())
        {
            Profile profile;
            QDomElement map = startItem.firstChildElement(QLatin1String("map"));
            if(!map.isNull() && map.isElement() && map.hasAttribute(QLatin1String("file")) && map.hasAttribute(QLatin1String("x")) && map.hasAttribute(QLatin1String("y")))
            {
                profile.map=map.attribute(QLatin1String("file"));
                if(!QFile::exists(datapackPath+QLatin1String(DATAPACK_BASE_PATH_MAP)+profile.map))
                {
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, map don't exists %2: child.tagName(): %3 (at line: %4)").arg(file).arg(profile.map).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(QLatin1String("start"));
                    continue;
                }
                profile.x=map.attribute(QLatin1String("x")).toUShort(&ok);
                if(!ok)
                {
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, map x is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(QLatin1String("start"));
                    continue;
                }
                profile.y=map.attribute(QLatin1String("y")).toUShort(&ok);
                if(!ok)
                {
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, map y is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(QLatin1String("start"));
                    continue;
                }
            }
            else
            {
                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, no correct map configuration: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement(QLatin1String("start"));
                continue;
            }
            QDomElement forcedskin = startItem.firstChildElement(QLatin1String("forcedskin"));
            if(!forcedskin.isNull() && forcedskin.isElement() && forcedskin.hasAttribute(QLatin1String("value")))
            {
                profile.forcedskin=forcedskin.attribute(QLatin1String("value")).split(QLatin1String(";"));
                int index=0;
                while(index<profile.forcedskin.size())
                {
                    if(!QFile::exists(datapackPath+QLatin1String(DATAPACK_BASE_PATH_SKIN)+profile.forcedskin.at(index)))
                    {
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, skin %4 don't exists: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(profile.forcedskin.at(index)));
                        profile.forcedskin.removeAt(index);
                    }
                    else
                        index++;
                }
            }
            profile.cash=0;
            QDomElement cash = startItem.firstChildElement(QLatin1String("cash"));
            if(!cash.isNull() && cash.isElement() && cash.hasAttribute(QLatin1String("value")))
            {
                profile.cash=cash.attribute(QLatin1String("value")).toULongLong(&ok);
                if(!ok)
                {
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, cash is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    profile.cash=0;
                }
            }
            QDomElement monstersElement = startItem.firstChildElement(QLatin1String("monster"));
            while(!monstersElement.isNull())
            {
                Profile::Monster monster;
                if(monstersElement.isElement() && monstersElement.hasAttribute(QLatin1String("id")) && monstersElement.hasAttribute(QLatin1String("level")) && monstersElement.hasAttribute(QLatin1String("captured_with")))
                {
                    monster.id=monstersElement.attribute(QLatin1String("id")).toUInt(&ok);
                    if(!ok)
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, monster id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    if(ok)
                    {
                        monster.level=monstersElement.attribute(QLatin1String("level")).toUShort(&ok);
                        if(!ok)
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, monster level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    }
                    if(ok)
                    {
                        if(monster.level==0 || monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, monster level is not into the range: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    }
                    if(ok)
                    {
                        monster.captured_with=monstersElement.attribute(QLatin1String("captured_with")).toUInt(&ok);
                        if(!ok)
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, captured_with is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    }
                    if(ok)
                    {
                        if(!monsters.contains(monster.id))
                        {
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, starter don't found the monster %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(monster.id));
                            ok=false;
                        }
                    }
                    if(ok)
                    {
                        if(!items.contains(monster.captured_with))
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, starter don't found the monster capture item %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(monster.id));
                    }
                    if(ok)
                        profile.monsters << monster;
                }
                monstersElement = monstersElement.nextSiblingElement(QLatin1String("monster"));
            }
            if(profile.monsters.empty())
            {
                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, not monster to load: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement(QLatin1String("start"));
                continue;
            }
            QDomElement reputationElement = startItem.firstChildElement(QLatin1String("reputation"));
            while(!reputationElement.isNull())
            {
                Profile::Reputation reputationTemp;
                if(reputationElement.isElement() && reputationElement.hasAttribute(QLatin1String("type")) && reputationElement.hasAttribute(QLatin1String("level")))
                {
                    reputationTemp.type=reputationElement.attribute(QLatin1String("type"));
                    reputationTemp.level=reputationElement.attribute(QLatin1String("level")).toShort(&ok);
                    if(!ok)
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, reputation level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    if(ok)
                    {
                        if(!reputations.contains(reputationTemp.type))
                        {
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, reputation type not found %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(reputationTemp.type));
                            ok=false;
                        }
                        if(ok)
                        {
                            if(reputationTemp.level==0)
                            {
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, reputation level is useless if level 0: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                ok=false;
                            }
                            else if(reputationTemp.level<0)
                            {
                                if((-reputationTemp.level)>reputations.value(reputationTemp.type).reputation_negative.size())
                                {
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, reputation level is lower than minimal level for %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(reputationTemp.type));
                                    ok=false;
                                }
                            }
                            else// if(reputationTemp.level>0)
                            {
                                if((reputationTemp.level)>=reputations.value(reputationTemp.type).reputation_positive.size())
                                {
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, reputation level is higther than maximal level for %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(reputationTemp.type));
                                    ok=false;
                                }
                            }
                        }
                        if(ok)
                        {
                            reputationTemp.point=0;
                            if(reputationElement.hasAttribute(QLatin1String("point")))
                            {
                                reputationTemp.point=reputationElement.attribute(QLatin1String("point")).toInt(&ok);
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, reputation point is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                if(ok)
                                {
                                    if((reputationTemp.point>0 && reputationTemp.level<0) || (reputationTemp.point<0 && reputationTemp.level>=0))
                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, reputation point is not negative/positive like the level: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                }
                            }
                        }
                    }
                    if(ok)
                        profile.reputation << reputationTemp;
                }
                reputationElement = reputationElement.nextSiblingElement(QLatin1String("reputation"));
            }
            QDomElement itemElement = startItem.firstChildElement(QLatin1String("item"));
            while(!itemElement.isNull())
            {
                Profile::Item itemTemp;
                if(itemElement.isElement() && itemElement.hasAttribute(QLatin1String("id")))
                {
                    itemTemp.id=itemElement.attribute(QLatin1String("id")).toUInt(&ok);
                    if(!ok)
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, item id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    if(ok)
                    {
                        itemTemp.quantity=0;
                        if(itemElement.hasAttribute(QLatin1String("quantity")))
                        {
                            itemTemp.quantity=itemElement.attribute(QLatin1String("quantity")).toUInt(&ok);
                            if(!ok)
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, item quantity is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                            if(ok)
                            {
                                if(itemTemp.quantity==0)
                                {
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, item quantity is null: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                    ok=false;
                                }
                            }
                            if(ok)
                            {
                                if(!items.contains(itemTemp.id))
                                {
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, item not found as know item %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(itemTemp.id));
                                    ok=false;
                                }
                            }
                        }
                    }
                    if(ok)
                        profile.items << itemTemp;
                }
                itemElement = itemElement.nextSiblingElement(QLatin1String("item"));
            }
            returnVar.second << profile;
            returnVar.first << startItem;

        }
        else
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
        startItem = startItem.nextSiblingElement(QLatin1String("start"));
    }
    return returnVar;
}
