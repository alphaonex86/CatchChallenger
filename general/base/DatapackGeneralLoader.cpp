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

QHash<QString, QDomDocument> DatapackGeneralLoader::xmlLoadedFile;

QHash<QString, Reputation> DatapackGeneralLoader::loadReputation(const QString &file)
{
    QRegExp typeRegex(QStringLiteral("^[a-z]{1,32}$"));
    QDomDocument domDocument;
    QHash<QString, Reputation> reputation;
    if(xmlLoadedFile.contains(file))
        domDocument=xmlLoadedFile[file];
    else
    {
        //open and quick check the file
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            DebugClass::debugConsole(QString("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString()));
            return reputation;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            DebugClass::debugConsole(QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return reputation;
        }
        xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("list"))
    {
        DebugClass::debugConsole(QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file));
        return reputation;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("reputation");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QStringLiteral("type")))
            {
                QList<qint32> point_list_positive,point_list_negative;
                QStringList text_positive,text_negative;
                QDomElement level = item.firstChildElement(QStringLiteral("level"));
                ok=true;
                while(!level.isNull() && ok)
                {
                    if(level.isElement())
                    {
                        if(level.hasAttribute(QStringLiteral("point")))
                        {
                            qint32 point=level.attribute(QStringLiteral("point")).toInt(&ok);
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
                                            DebugClass::debugConsole(QString("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
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
                                            DebugClass::debugConsole(QString("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
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
                                DebugClass::debugConsole(QString("Unable to open the file: %1, point is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the file: %1, point attribute not found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    level = level.nextSiblingElement(QStringLiteral("level"));
                }
                if(ok)
                    if(point_list_positive.size()<2)
                    {
                        DebugClass::debugConsole(QString("Unable to open the file: %1, reputation have to few level: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_positive.contains(0))
                    {
                        DebugClass::debugConsole(QString("Unable to open the file: %1, no starting level for the positive: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_negative.empty() && !point_list_negative.contains(-1))
                    {
                        DebugClass::debugConsole(QString("Unable to open the file: %1, no starting level for the negative: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!item.attribute(QStringLiteral("type")).contains(typeRegex))
                    {
                        DebugClass::debugConsole(QString("Unable to open the file: %1, the type %4 don't match wiuth the regex: ^[a-z]{1,32}$: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute(QStringLiteral("type"))));
                        ok=false;
                    }
                if(ok)
                {
                    reputation[item.attribute(QStringLiteral("type"))].reputation_positive=point_list_positive;
                    reputation[item.attribute(QStringLiteral("type"))].reputation_negative=point_list_negative;
                }
            }
            else
                DebugClass::debugConsole(QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(QStringLiteral("reputation"));
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
        if(!QFile(entryList.at(index).absoluteFilePath()+QStringLiteral("/definition.xml")).exists())
        {
            index++;
            continue;
        }
        quint32 questId=entryList.at(index).fileName().toUInt(&ok);
        if(ok)
        {
            //add it, all seam ok
            QPair<bool,Quest> returnedQuest=loadSingleQuest(entryList.at(index).absoluteFilePath()+QStringLiteral("/definition.xml"));
            if(returnedQuest.first==true)
            {
                returnedQuest.second.id=questId;
                if(quests.contains(returnedQuest.second.id))
                    qDebug() << QString("The quest with id: %1 is already found, disable: %2").arg(returnedQuest.second.id).arg(entryList.at(index).absoluteFilePath()+"/definition.xml");
                else
                    quests[returnedQuest.second.id]=returnedQuest.second;
            }
        }
        else
            DebugClass::debugConsole(QString("Unable to open the folder: %1, because is folder name is not a number").arg(entryList.at(index).fileName()));
        index++;
    }
    return quests;
}

QPair<bool,Quest> DatapackGeneralLoader::loadSingleQuest(const QString &file)
{
    CatchChallenger::Quest quest;
    QDomDocument domDocument;
    if(xmlLoadedFile.contains(file))
        domDocument=xmlLoadedFile[file];
    else
    {
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return QPair<bool,Quest>(false,quest);
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return QPair<bool,Quest>(false,quest);
        }
        xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("quest"))
    {
        qDebug() << QString("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(file);
        return QPair<bool,Quest>(false,quest);
    }

    //load the content
    bool ok;
    QList<quint32> defaultBots;
    quest.id=0;
    quest.repeatable=false;
    if(root.hasAttribute(QStringLiteral("repeatable")))
        if(root.attribute(QStringLiteral("repeatable"))==QStringLiteral("yes") || root.attribute(QStringLiteral("repeatable"))==QStringLiteral("true"))
            quest.repeatable=true;
    if(root.hasAttribute(QStringLiteral("bot")))
    {
        QStringList tempStringList=root.attribute(QStringLiteral("bot")).split(QStringLiteral(";"));
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
    QDomElement requirements = root.firstChildElement(QStringLiteral("requirements"));
    while(!requirements.isNull())
    {
        if(requirements.isElement())
        {
            //load requirements reputation
            {
                QDomElement requirementsItem = requirements.firstChildElement(QStringLiteral("reputation"));
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute(QStringLiteral("type")) && requirementsItem.hasAttribute(QStringLiteral("level")))
                        {
                            QString stringLevel=requirementsItem.attribute(QStringLiteral("level"));
                            bool positif=!stringLevel.startsWith(QStringLiteral("-"));
                            if(!positif)
                                stringLevel.remove(0,1);
                            quint8 level=stringLevel.toUShort(&ok);
                            if(ok)
                            {
                                CatchChallenger::Quest::ReputationRequirements reputation;
                                reputation.level=level;
                                reputation.positif=positif;
                                reputation.type=requirementsItem.attribute(QStringLiteral("type"));
                                quest.requirements.reputation << reputation;
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, reputation is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber()).arg(stringLevel);
                        }
                        else
                            qDebug() << QString("Has attribute: %1, have not attribute type or level: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    requirementsItem = requirementsItem.nextSiblingElement(QStringLiteral("requirements"));
                }
            }
            //load requirements quest
            {
                QDomElement requirementsItem = requirements.firstChildElement(QStringLiteral("quest"));
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute(QStringLiteral("id")))
                        {
                            quint32 questId=requirementsItem.attribute(QStringLiteral("id")).toUInt(&ok);
                            if(ok)
                                quest.requirements.quests << questId;
                            else
                                qDebug() << QString("Unable to open the file: %1, requirement quest item id is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber()).arg(requirementsItem.attribute(QStringLiteral("id")));
                        }
                        else
                            qDebug() << QString("Has attribute: %1, requirement quest item have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    requirementsItem = requirementsItem.nextSiblingElement(QStringLiteral("quest"));
                }
            }
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(requirements.tagName()).arg(requirements.lineNumber());
        requirements = requirements.nextSiblingElement(QStringLiteral("requirements"));
    }

    //load rewards
    QDomElement rewards = root.firstChildElement(QStringLiteral("rewards"));
    while(!rewards.isNull())
    {
        if(rewards.isElement())
        {
            //load rewards reputation
            {
                QDomElement reputationItem = rewards.firstChildElement(QStringLiteral("reputation"));
                while(!reputationItem.isNull())
                {
                    if(reputationItem.isElement())
                    {
                        if(reputationItem.hasAttribute(QStringLiteral("type")) && reputationItem.hasAttribute(QStringLiteral("point")))
                        {
                            qint32 point=reputationItem.attribute(QStringLiteral("point")).toInt(&ok);
                            if(ok)
                            {
                                CatchChallenger::Quest::ReputationRewards reputation;
                                reputation.point=point;
                                reputation.type=reputationItem.attribute(QStringLiteral("type"));
                                quest.rewards.reputation << reputation;
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, quest rewards point is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                        }
                        else
                            qDebug() << QString("Has attribute: %1, quest rewards point have not type or point attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                    reputationItem = reputationItem.nextSiblingElement(QStringLiteral("rewards"));
                }
            }
            //load rewards item
            {
                QDomElement rewardsItem = rewards.firstChildElement(QStringLiteral("item"));
                while(!rewardsItem.isNull())
                {
                    if(rewardsItem.isElement())
                    {
                        if(rewardsItem.hasAttribute(QStringLiteral("id")))
                        {
                            CatchChallenger::Quest::Item item;
                            item.item=rewardsItem.attribute(QStringLiteral("id")).toUInt(&ok);
                            item.quantity=1;
                            if(ok)
                            {
                                if(!CommonDatapack::commonDatapack.items.item.contains(item.item))
                                {
                                    qDebug() << QStringLiteral("Unable to open the file: %1, rewards item id is not into the item list: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber()).arg(rewardsItem.attribute(QStringLiteral("id")));
                                    return QPair<bool,Quest>(false,quest);
                                }
                                if(rewardsItem.hasAttribute(QStringLiteral("quantity")))
                                {
                                    item.quantity=rewardsItem.attribute(QStringLiteral("quantity")).toUInt(&ok);
                                    if(!ok)
                                        item.quantity=1;
                                }
                                quest.rewards.items << item;
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, rewards item id is not a number: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber()).arg(rewardsItem.attribute(QStringLiteral("id")));
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, rewards item have not attribute id: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    rewardsItem = rewardsItem.nextSiblingElement(QStringLiteral("quest"));
                }
            }
            //load rewards allow
            {
                QDomElement allowItem = rewards.firstChildElement(QStringLiteral("allow"));
                while(!allowItem.isNull())
                {
                    if(allowItem.isElement())
                    {
                        if(allowItem.hasAttribute(QStringLiteral("type")))
                        {
                            if(allowItem.attribute(QStringLiteral("type"))==QStringLiteral("clan"))
                                quest.rewards.allow << CatchChallenger::ActionAllow_Clan;
                            else
                                qDebug() << QString("Unable to open the file: %1, allow type not understand: child.tagName(): %2 (at line: %3)").arg(file).arg(allowItem.tagName()).arg(allowItem.lineNumber()).arg(allowItem.attribute(QStringLiteral("id")));
                        }
                        else
                            qDebug() << QString("Has attribute: %1, rewards item have not attribute id: child.tagName(): %2 (at line: %3)").arg(file).arg(allowItem.tagName()).arg(allowItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(allowItem.tagName()).arg(allowItem.lineNumber());
                    allowItem = allowItem.nextSiblingElement(QStringLiteral("allow"));
                }
            }
            quest.rewards.allow.fromSet(quest.rewards.allow.toSet());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(rewards.tagName()).arg(rewards.lineNumber());
        rewards = rewards.nextSiblingElement("rewards");
    }

    QHash<quint8,CatchChallenger::Quest::Step> steps;
    //load step
    QDomElement step = root.firstChildElement(QStringLiteral("step"));
    while(!step.isNull())
    {
        if(step.isElement())
        {
            if(step.hasAttribute(QStringLiteral("id")))
            {
                quint32 id=step.attribute(QStringLiteral("id")).toUInt(&ok);
                if(ok)
                {
                    CatchChallenger::Quest::Step stepObject;
                    if(step.hasAttribute(QStringLiteral("bot")))
                    {
                        QStringList tempStringList=step.attribute(QStringLiteral("bot")).split(QStringLiteral(";"));
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
                        QDomElement stepItem = step.firstChildElement(QStringLiteral("item"));
                        while(!stepItem.isNull())
                        {
                            if(stepItem.isElement())
                            {
                                if(stepItem.hasAttribute(QStringLiteral("id")))
                                {
                                    CatchChallenger::Quest::Item item;
                                    item.item=stepItem.attribute(QStringLiteral("id")).toUInt(&ok);
                                    item.quantity=1;
                                    if(ok)
                                    {
                                        if(!CommonDatapack::commonDatapack.items.item.contains(item.item))
                                        {
                                            qDebug() << QString("Unable to open the file: %1, rewards item id is not into the item list: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(stepItem.tagName()).arg(stepItem.lineNumber()).arg(stepItem.attribute(QStringLiteral("id")));
                                            return QPair<bool,Quest>(false,quest);
                                        }
                                        if(stepItem.hasAttribute(QStringLiteral("quantity")))
                                        {
                                            item.quantity=stepItem.attribute(QStringLiteral("quantity")).toUInt(&ok);
                                            if(!ok)
                                                item.quantity=1;
                                        }
                                        stepObject.requirements.items << item;
                                        if(stepItem.hasAttribute(QStringLiteral("monster")) && stepItem.hasAttribute(QStringLiteral("rate")))
                                        {
                                            CatchChallenger::Quest::ItemMonster itemMonster;
                                            itemMonster.item=item.item;

                                            QStringList tempStringList=stepItem.attribute(QStringLiteral("monster")).split(QStringLiteral(";"));
                                            int index=0;
                                            while(index<tempStringList.size())
                                            {
                                                quint32 tempInt=tempStringList.at(index).toUInt(&ok);
                                                if(ok)
                                                    itemMonster.monsters << tempInt;
                                                index++;
                                            }

                                            QString rateString=stepItem.attribute(QStringLiteral("rate"));
                                            rateString.remove(QStringLiteral("%"));
                                            itemMonster.rate=rateString.toUShort(&ok);
                                            if(ok)
                                                stepObject.itemsMonster << itemMonster;
                                        }
                                    }
                                    else
                                        qDebug() << QString("Unable to open the file: %1, step id is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber()).arg(stepItem.attribute(QStringLiteral("id")));
                                }
                                else
                                    qDebug() << QString("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            stepItem = stepItem.nextSiblingElement(QStringLiteral("item"));
                        }
                    }
                    //do the fight
                    {
                        QDomElement fightItem = step.firstChildElement(QStringLiteral("fight"));
                        while(!fightItem.isNull())
                        {
                            if(fightItem.isElement())
                            {
                                if(fightItem.hasAttribute(QStringLiteral("id")))
                                {
                                    quint32 fightId=fightItem.attribute(QStringLiteral("id")).toUInt(&ok);
                                    if(ok)
                                        stepObject.requirements.fightId << fightId;
                                    else
                                        qDebug() << QString("Unable to open the file: %1, step id is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber()).arg(fightItem.attribute(QStringLiteral("id")));
                                }
                                else
                                    qDebug() << QString("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            fightItem = fightItem.nextSiblingElement(QStringLiteral("fight"));
                        }
                    }
                    steps[id]=stepObject;
                }
                else
                    qDebug() << QString("Unable to open the file: %1, step id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
            }
            else
                qDebug() << QString("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
        step = step.nextSiblingElement(QStringLiteral("step"));
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
    QDomDocument domDocument;
    //open and quick check the file
    if(xmlLoadedFile.contains(file))
        domDocument=xmlLoadedFile[file];
    else
    {
        QFile plantsFile(file);
        QByteArray xmlContent;
        if(!plantsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the plants file: %1, error: %2").arg(file).arg(plantsFile.errorString());
            return plants;
        }
        xmlContent=plantsFile.readAll();
        plantsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QString("Unable to open the plants file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return plants;
        }
        xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("plants"))
    {
        qDebug() << QString("Unable to open the plants file: %1, \"plants\" root balise not found for the xml file").arg(file);
        return plants;
    }

    //load the content
    bool ok,ok2;
    QDomElement plantItem = root.firstChildElement(QStringLiteral("plant"));
    while(!plantItem.isNull())
    {
        if(plantItem.isElement())
        {
            if(plantItem.hasAttribute(QStringLiteral("id")) && plantItem.hasAttribute(QStringLiteral("itemUsed")))
            {
                quint8 id=plantItem.attribute(QStringLiteral("id")).toUShort(&ok);
                quint32 itemUsed=plantItem.attribute(QStringLiteral("itemUsed")).toUInt(&ok2);
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
                        QDomElement quantity = plantItem.firstChildElement(QStringLiteral("quantity"));
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
                        QDomElement grow = plantItem.firstChildElement(QStringLiteral("grow"));
                        if(!grow.isNull())
                        {
                            if(grow.isElement())
                            {
                                QDomElement fruits = grow.firstChildElement(QStringLiteral("fruits"));
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
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(fruits.tagName()).arg(fruits.lineNumber()).arg(fruits.text());
                                            ok=false;
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        qDebug() << QString("Unable to parse the plants file: %1, fruits is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(fruits.tagName()).arg(fruits.lineNumber());
                                    }
                                }
                                else
                                {
                                    ok=false;
                                    qDebug() << QString("Unable to parse the plants file: %1, fruits is null: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                                }
                                QDomElement sprouted = grow.firstChildElement(QStringLiteral("sprouted"));
                                if(!sprouted.isNull())
                                {
                                    if(sprouted.isElement())
                                    {
                                        plant.sprouted_seconds=sprouted.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(sprouted.tagName()).arg(sprouted.lineNumber()).arg(sprouted.text());
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, sprouted is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(sprouted.tagName()).arg(sprouted.lineNumber());
                                }
                                QDomElement taller = grow.firstChildElement(QStringLiteral("taller"));
                                if(!taller.isNull())
                                {
                                    if(taller.isElement())
                                    {
                                        plant.taller_seconds=taller.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(taller.tagName()).arg(taller.lineNumber()).arg(taller.text());
                                            ok=false;
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, taller is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(taller.tagName()).arg(taller.lineNumber());
                                }
                                QDomElement flowering = grow.firstChildElement(QStringLiteral("flowering"));
                                if(!flowering.isNull())
                                {
                                    if(flowering.isElement())
                                    {
                                        plant.flowering_seconds=flowering.text().toUInt(&ok2)*60;
                                        if(!ok2)
                                        {
                                            ok=false;
                                            qDebug() << QString("Unable to parse the plants file: %1, sprouted is not a number: %4 child.tagName(): %2 (at line: %3)").arg(file).arg(flowering.tagName()).arg(flowering.lineNumber()).arg(flowering.text());
                                        }
                                        else
                                            intermediateTimeCount++;
                                    }
                                    else
                                        qDebug() << QString("Unable to parse the plants file: %1, flowering is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(flowering.tagName()).arg(flowering.lineNumber());
                                }
                            }
                            else
                                qDebug() << QString("Unable to parse the plants file: %1, grow is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                        }
                        else
                            qDebug() << QString("Unable to parse the plants file: %1, grow is null: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                        if(ok)
                        {
                            bool needIntermediateTimeFix=false;
                            if(plant.flowering_seconds>=plant.fruits_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    qDebug() << QString("Warning when parse the plants file: %1, flowering_seconds>=fruits_seconds: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                            }
                            if(plant.taller_seconds>=plant.flowering_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    qDebug() << QString("Warning when parse the plants file: %1, taller_seconds>=flowering_seconds: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                            }
                            if(plant.sprouted_seconds>=plant.taller_seconds)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    qDebug() << QString("Warning when parse the plants file: %1, sprouted_seconds>=taller_seconds: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
                            }
                            if(plant.sprouted_seconds<=0)
                            {
                                needIntermediateTimeFix=true;
                                if(intermediateTimeCount>=3)
                                    qDebug() << QString("Warning when parse the plants file: %1, sprouted_seconds<=0: child.tagName(): %2 (at line: %3)").arg(file).arg(grow.tagName()).arg(grow.lineNumber());
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
                        qDebug() << QString("Unable to open the plants file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(plantItem.tagName()).arg(plantItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the plants file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(plantItem.tagName()).arg(plantItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the plants file: %1, have not the plant id: child.tagName(): %2 (at line: %3)").arg(file).arg(plantItem.tagName()).arg(plantItem.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the plants file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(plantItem.tagName()).arg(plantItem.lineNumber());
        plantItem = plantItem.nextSiblingElement(QStringLiteral("plant"));
    }
    return plants;
}

QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> > DatapackGeneralLoader::loadCraftingRecipes(const QString &file,const QHash<quint32, Item> &items)
{
    QHash<quint32,CrafingRecipe> crafingRecipes;
    QHash<quint32,quint32> itemToCrafingRecipes;
    QDomDocument domDocument;
    //open and quick check the file
    if(xmlLoadedFile.contains(file))
        domDocument=xmlLoadedFile[file];
    else
    {
        QFile craftingRecipesFile(file);
        QByteArray xmlContent;
        if(!craftingRecipesFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the crafting recipe file: %1, error: %2").arg(file).arg(craftingRecipesFile.errorString());
            return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
        }
        xmlContent=craftingRecipesFile.readAll();
        craftingRecipesFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QString("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
        }
        xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("recipes"))
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, \"recipes\" root balise not found for the xml file").arg(file);
        return QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> >(crafingRecipes,itemToCrafingRecipes);
    }

    //load the content
    bool ok,ok2,ok3;
    QDomElement recipeItem = root.firstChildElement(QStringLiteral("recipe"));
    while(!recipeItem.isNull())
    {
        if(recipeItem.isElement())
        {
            if(recipeItem.hasAttribute(QStringLiteral("id")) && recipeItem.hasAttribute(QStringLiteral("itemToLearn")) && recipeItem.hasAttribute(QStringLiteral("doItemId")))
            {
                quint8 success=100;
                if(recipeItem.hasAttribute(QStringLiteral("success")))
                {
                    quint8 tempShort=recipeItem.attribute(QStringLiteral("success")).toUShort(&ok);
                    if(ok)
                    {
                        if(tempShort>100)
                            qDebug() << QString("preload_crafting_recipes() success can't be greater than 100 for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                        else
                            success=tempShort;
                    }
                    else
                        qDebug() << QString("preload_crafting_recipes() success in not an number for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }
                quint16 quantity=1;
                if(recipeItem.hasAttribute(QStringLiteral("quantity")))
                {
                    quint32 tempShort=recipeItem.attribute(QStringLiteral("quantity")).toUInt(&ok);
                    if(ok)
                    {
                        if(tempShort>65535)
                            qDebug() << QString("preload_crafting_recipes() quantity can't be greater than 65535 for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                        else
                            quantity=tempShort;
                    }
                    else
                        qDebug() << QString("preload_crafting_recipes() quantity in not an number for crafting recipe file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }

                quint32 id=recipeItem.attribute(QStringLiteral("id")).toUInt(&ok);
                quint32 itemToLearn=recipeItem.attribute(QStringLiteral("itemToLearn")).toUInt(&ok2);
                quint32 doItemId=recipeItem.attribute(QStringLiteral("doItemId")).toUInt(&ok3);
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
                        QDomElement material = recipeItem.firstChildElement(QStringLiteral("material"));
                        while(!material.isNull() && ok)
                        {
                            if(material.isElement())
                            {
                                if(material.hasAttribute(QStringLiteral("itemId")))
                                {
                                    quint32 itemId=material.attribute(QStringLiteral("itemId")).toUInt(&ok2);
                                    if(!ok2)
                                    {
                                        ok=false;
                                        qDebug() << QString("preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                        break;
                                    }
                                    quint16 quantity=1;
                                    if(material.hasAttribute(QStringLiteral("quantity")))
                                    {
                                        quint32 tempShort=material.attribute(QStringLiteral("quantity")).toUInt(&ok2);
                                        if(ok2)
                                        {
                                            if(tempShort>65535)
                                            {
                                                ok=false;
                                                qDebug() << QString("preload_crafting_recipes() material quantity can't be greater than 65535 for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                                break;
                                            }
                                            else
                                                quantity=tempShort;
                                        }
                                        else
                                        {
                                            ok=false;
                                            qDebug() << QString("preload_crafting_recipes() material quantity in not an number for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                            break;
                                        }
                                    }
                                    if(!items.contains(itemId))
                                    {
                                        ok=false;
                                        qDebug() << QString("preload_crafting_recipes() material itemId in not into items list for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
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
                                        qDebug() << QString("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                    }
                                    else
                                    {
                                        if(recipe.doItemId==newMaterial.item)
                                        {
                                            qDebug() << QString("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                            ok=false;
                                        }
                                        else
                                            recipe.materials << newMaterial;
                                    }
                                }
                                else
                                    qDebug() << QString("preload_crafting_recipes() material have not attribute itemId for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                            else
                                qDebug() << QString("preload_crafting_recipes() material is not an element for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            material = material.nextSiblingElement(QStringLiteral("material"));
                        }
                        if(ok)
                        {
                            if(!items.contains(recipe.itemToLearn))
                            {
                                ok=false;
                                qDebug() << QString("preload_crafting_recipes() itemToLearn is not into items list for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            if(!items.contains(recipe.doItemId))
                            {
                                ok=false;
                                qDebug() << QString("preload_crafting_recipes() doItemId is not into items list for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.contains(recipe.itemToLearn))
                            {
                                ok=false;
                                qDebug() << QString("preload_crafting_recipes() itemToLearn already used to learn another recipe: %4: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()).arg(itemToCrafingRecipes[recipe.itemToLearn]);
                            }
                        }
                        if(ok)
                        {
                            if(recipe.itemToLearn==recipe.doItemId)
                            {
                                ok=false;
                                qDebug() << QString("preload_crafting_recipes() the product of the recipe can't be them self: %4: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()).arg(id);
                            }
                        }
                        if(ok)
                        {
                            if(itemToCrafingRecipes.contains(recipe.doItemId))
                            {
                                ok=false;
                                qDebug() << QString("preload_crafting_recipes() the product of the recipe can't be a recipe: %4: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber()).arg(itemToCrafingRecipes[recipe.doItemId]);
                            }
                        }
                        if(ok)
                        {
                            crafingRecipes[id]=recipe;
                            itemToCrafingRecipes[recipe.itemToLearn]=id;
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the crafting recipe file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the crafting recipe file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the crafting recipe file: %1, have not the crafting recipe id: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the crafting recipe file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
        recipeItem = recipeItem.nextSiblingElement(QStringLiteral("recipe"));
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
        if(xmlLoadedFile.contains(file))
            domDocument=xmlLoadedFile[file];
        else
        {
            QFile industryFile(file);
            QByteArray xmlContent;
            if(!industryFile.open(QIODevice::ReadOnly))
            {
                qDebug() << QString("Unable to open the crafting recipe file: %1, error: %2").arg(file).arg(industryFile.errorString());
                file_index++;
                continue;
            }
            xmlContent=industryFile.readAll();
            industryFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << QString("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
                file_index++;
                continue;
            }
            xmlLoadedFile[file]=domDocument;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=QStringLiteral("industries"))
        {
            qDebug() << QString("Unable to open the crafting recipe file: %1, \"industries\" root balise not found for the xml file").arg(file);
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2,ok3;
        QDomElement industryItem = root.firstChildElement(QStringLiteral("industrialrecipe"));
        while(!industryItem.isNull())
        {
            if(industryItem.isElement())
            {
                if(industryItem.hasAttribute(QStringLiteral("id")) && industryItem.hasAttribute(QStringLiteral("time")) && industryItem.hasAttribute(QStringLiteral("cycletobefull")))
                {
                    Industry industry;
                    quint32 id=industryItem.attribute(QStringLiteral("id")).toUInt(&ok);
                    industry.time=industryItem.attribute(QStringLiteral("time")).toUInt(&ok2);
                    industry.cycletobefull=industryItem.attribute(QStringLiteral("cycletobefull")).toUInt(&ok3);
                    if(ok && ok2 && ok3)
                    {
                        if(!industries.contains(id))
                        {
                            if(industry.time<60*5)
                            {
                                qDebug() << QString("the time need be greater than 5*60 seconds to not slow down the server: %4, %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber()).arg(industry.time);
                                industry.time=60*5;
                            }
                            if(industry.cycletobefull<1)
                            {
                                qDebug() << QString("cycletobefull need be greater than 0: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                industry.cycletobefull=1;
                            }
                            else if(industry.cycletobefull>65535)
                            {
                                qDebug() << QString("cycletobefull need be lower to 10 to not slow down the server, use the quantity: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                industry.cycletobefull=10;
                            }
                            //resource
                            {
                                QDomElement resourceItem = industryItem.firstChildElement(QStringLiteral("resource"));
                                ok=true;
                                while(!resourceItem.isNull() && ok)
                                {
                                    if(resourceItem.isElement())
                                    {
                                        Industry::Resource resource;
                                        resource.quantity=1;
                                        if(resourceItem.hasAttribute(QStringLiteral("quantity")))
                                        {
                                            resource.quantity=resourceItem.attribute(QStringLiteral("quantity")).toUInt(&ok);
                                            if(!ok)
                                                qDebug() << QString("quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(resourceItem.hasAttribute(QStringLiteral("id")))
                                            {
                                                resource.item=resourceItem.attribute(QStringLiteral("id")).toUInt(&ok);
                                                if(!ok)
                                                    qDebug() << QString("id is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                else if(!items.contains(resource.item))
                                                {
                                                    ok=false;
                                                    qDebug() << QString("id is not into the item list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
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
                                                        qDebug() << QString("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
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
                                                            qDebug() << QString("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                        }
                                                        else
                                                            industry.resources << resource;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                qDebug() << QString("have not the id attribute: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        qDebug() << QString("is not a elements: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                    }
                                    resourceItem = resourceItem.nextSiblingElement(QStringLiteral("resource"));
                                }
                            }

                            //product
                            if(ok)
                            {
                                QDomElement productItem = industryItem.firstChildElement(QStringLiteral("product"));
                                ok=true;
                                while(!productItem.isNull() && ok)
                                {
                                    if(productItem.isElement())
                                    {
                                        Industry::Product product;
                                        product.quantity=1;
                                        if(productItem.hasAttribute(QStringLiteral("quantity")))
                                        {
                                            product.quantity=productItem.attribute(QStringLiteral("quantity")).toUInt(&ok);
                                            if(!ok)
                                                qDebug() << QString("quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(productItem.hasAttribute(QStringLiteral("id")))
                                            {
                                                product.item=productItem.attribute(QStringLiteral("id")).toUInt(&ok);
                                                if(!ok)
                                                    qDebug() << QString("id is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                else if(!items.contains(product.item))
                                                {
                                                    ok=false;
                                                    qDebug() << QString("id is not into the item list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
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
                                                        qDebug() << QString("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
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
                                                            qDebug() << QString("id of item already into resource or product list: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                                        }
                                                        else
                                                            industry.products << product;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ok=false;
                                                qDebug() << QString("have not the id attribute: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        qDebug() << QString("is not a elements: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                    }
                                    productItem = productItem.nextSiblingElement(QStringLiteral("product"));
                                }
                            }

                            //add
                            if(ok)
                            {
                                if(industry.products.isEmpty() || industry.resources.isEmpty())
                                    qDebug() << QString("product or resources is empty: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                else
                                    industries[id]=industry;
                            }
                        }
                        else
                            qDebug() << QString("Unable to open the industries file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the industries file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the industries file: %1, have not the id: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the industries file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
            industryItem = industryItem.nextSiblingElement(QStringLiteral("industrialrecipe"));
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
    if(xmlLoadedFile.contains(file))
        domDocument=xmlLoadedFile[file];
    else
    {
        QFile industriesLinkFile(file);
        QByteArray xmlContent;
        if(!industriesLinkFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the crafting recipe file: %1, error: %2").arg(file).arg(industriesLinkFile.errorString());
            return industriesLink;
        }
        xmlContent=industriesLinkFile.readAll();
        industriesLinkFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QString("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return industriesLink;
        }
        xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("industries"))
    {
        qDebug() << QString("Unable to open the crafting recipe file: %1, \"industries\" root balise not found for the xml file").arg(file);
        return industriesLink;
    }

    //load the content
    bool ok,ok2;
    QDomElement linkItem = root.firstChildElement(QStringLiteral("link"));
    while(!linkItem.isNull())
    {
        if(linkItem.isElement())
        {
            if(linkItem.hasAttribute(QStringLiteral("industrialrecipe")) && linkItem.hasAttribute(QStringLiteral("industry")))
            {
                quint32 industry_id=linkItem.attribute(QStringLiteral("industrialrecipe")).toUInt(&ok);
                quint32 factory_id=linkItem.attribute(QStringLiteral("industry")).toUInt(&ok2);
                if(ok && ok2)
                {
                    if(!industriesLink.contains(factory_id))
                    {
                        if(industries.contains(industry_id))
                            industriesLink[industry_id]=industry_id;
                        else
                            qDebug() << QString("Industry id for factory is not found: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Factory already found: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the industries link file: %1, the attribute is not a number, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the industries link file: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the industries link file: %1, is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(linkItem.tagName()).arg(linkItem.lineNumber());
        linkItem = linkItem.nextSiblingElement(QStringLiteral("link"));
    }
    return industriesLink;
}

ItemFull DatapackGeneralLoader::loadItems(const QString &folder,const QHash<quint32,Buff> &monsterBuffs)
{
    ItemFull items;
    QDomDocument domDocument;
    //open and quick check the file
    const QString &file=folder+QStringLiteral("items.xml");
    if(xmlLoadedFile.contains(file))
        domDocument=xmlLoadedFile[file];
    else
    {
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return items;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return items;
        }
        xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        qDebug() << QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file);
        return items;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(QStringLiteral("item"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QStringLiteral("id")))
            {
                quint32 id=item.attribute(QStringLiteral("id")).toUInt(&ok);
                if(ok)
                {
                    if(!items.item.contains(id))
                    {
                        //load the price
                        {
                            if(item.hasAttribute(QStringLiteral("price")))
                            {
                                bool ok;
                                items.item[id].price=item.attribute(QStringLiteral("price")).toUInt(&ok);
                                if(!ok)
                                {
                                    qDebug() << QString("price is not a number: child.tagName(): %1 (at line: %2)").arg(item.tagName()).arg(item.lineNumber());
                                    items.item[id].price=0;
                                }
                            }
                            else
                            {
                                /*if(!item.hasAttribute(QStringLiteral("quest")) || item.attribute(QStringLiteral("quest"))!=QStringLiteral("yes"))
                                    qDebug() << QString("For parse item: Price not found, default to 0 (not sellable): child.tagName(): %1 (%2 at line: %3)").arg(item.tagName()).arg(file).arg(item.lineNumber());*/
                                items.item[id].price=0;
                            }
                        }
                        //load the consumeAtUse
                        {
                            if(item.hasAttribute(QStringLiteral("consumeAtUse")))
                            {
                                if(item.attribute(QStringLiteral("consumeAtUse"))==QStringLiteral("false"))
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
                            QDomElement trapItem = item.firstChildElement(QStringLiteral("trap"));
                            if(!trapItem.isNull())
                            {
                                if(trapItem.isElement())
                                {
                                    Trap trap;
                                    trap.bonus_rate=1.0;
                                    if(trapItem.hasAttribute(QStringLiteral("bonus_rate")))
                                    {
                                        float bonus_rate=trapItem.attribute(QStringLiteral("bonus_rate")).toFloat(&ok);
                                        if(ok)
                                            trap.bonus_rate=bonus_rate;
                                        else
                                            qDebug() << QString("Unable to open the file: %1, bonus_rate is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(trapItem.tagName()).arg(trapItem.lineNumber());
                                    }
                                    else
                                        qDebug() << QString("Unable to open the file: %1, trap have not the attribute bonus_rate: child.tagName(): %2 (at line: %3)").arg(file).arg(trapItem.tagName()).arg(trapItem.lineNumber());
                                    items.trap[id]=trap;
                                    haveAnEffect=true;
                                }
                            }
                        }
                        //load the repel
                        if(!haveAnEffect)
                        {
                            QDomElement repelItem = item.firstChildElement(QStringLiteral("repel"));
                            if(!repelItem.isNull())
                            {
                                if(repelItem.isElement())
                                {
                                    if(repelItem.hasAttribute(QStringLiteral("step")))
                                    {
                                        quint32 step=repelItem.attribute(QStringLiteral("step")).toUInt(&ok);
                                        if(ok)
                                        {
                                            if(step>0)
                                            {
                                                items.repel[id]=step;
                                                haveAnEffect=true;
                                            }
                                            else
                                                qDebug() << QString("Unable to open the file: %1, step is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                        }
                                        else
                                            qDebug() << QString("Unable to open the file: %1, step is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                    }
                                    else
                                        qDebug() << QString("Unable to open the file: %1, trap have not the attribute min_level or max_level: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                }
                            }
                        }
                        //load the monster effect
                        if(!haveAnEffect)
                        {
                            {
                                QDomElement hpItem = item.firstChildElement(QStringLiteral("hp"));
                                while(!hpItem.isNull())
                                {
                                    if(hpItem.isElement())
                                    {
                                        if(hpItem.hasAttribute(QStringLiteral("add")))
                                        {
                                            if(hpItem.attribute(QStringLiteral("add"))==QStringLiteral("all"))
                                            {
                                                MonsterItemEffect monsterItemEffect;
                                                monsterItemEffect.type=MonsterItemEffectType_AddHp;
                                                monsterItemEffect.value=-1;
                                                items.monsterItemEffect.insert(id,monsterItemEffect);
                                            }
                                            else
                                            {
                                                qint32 add=hpItem.attribute(QStringLiteral("add")).toUInt(&ok);
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
                                                        qDebug() << QString("Unable to open the file: %1, step is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                                }
                                                else
                                                    qDebug() << QString("Unable to open the file: %1, step is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                            }
                                        }
                                        else
                                            qDebug() << QString("Unable to open the file: %1, trap have not the attribute min_level or max_level: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                    }
                                    hpItem = hpItem.nextSiblingElement(QStringLiteral("hp"));
                                }
                            }
                            {
                                QDomElement buffItem = item.firstChildElement(QStringLiteral("buff"));
                                while(!buffItem.isNull())
                                {
                                    if(buffItem.isElement())
                                    {
                                        if(buffItem.hasAttribute(QStringLiteral("remove")))
                                        {
                                            if(buffItem.attribute(QStringLiteral("remove"))==QStringLiteral("all"))
                                            {
                                                MonsterItemEffect monsterItemEffect;
                                                monsterItemEffect.type=MonsterItemEffectType_RemoveBuff;
                                                monsterItemEffect.value=-1;
                                                items.monsterItemEffect.insert(id,monsterItemEffect);
                                            }
                                            else
                                            {
                                                qint32 remove=buffItem.attribute(QStringLiteral("remove")).toUInt(&ok);
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
                                                            qDebug() << QString("Unable to open the file: %1, buff item to remove is not found: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                                    }
                                                    else
                                                        qDebug() << QString("Unable to open the file: %1, step is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                                }
                                                else
                                                    qDebug() << QString("Unable to open the file: %1, step is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                            }
                                        }
                                        else
                                            qDebug() << QString("Unable to open the file: %1, trap have not the attribute min_level or max_level: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());
                                    }
                                    buffItem = buffItem.nextSiblingElement(QStringLiteral("buff"));
                                }
                            }
                            if(items.monsterItemEffect.contains(id))
                                haveAnEffect=true;
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement(QStringLiteral("item"));
    }
    return items;
}

QPair<QList<QDomElement>, QList<Profile> > DatapackGeneralLoader::loadProfileList(const QString &datapackPath, const QString &file)
{
    QPair<QList<QDomElement>, QList<Profile> > returnVar;
    QDomDocument domDocument;
    //open and quick check the file
    if(xmlLoadedFile.contains(file))
        domDocument=xmlLoadedFile[file];
    else
    {
        QFile xmlFile(file);
        QByteArray xmlContent;
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file to have new profile: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return returnVar;
        }
        xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return returnVar;
        }
        xmlLoadedFile[file]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("list"))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return returnVar;
    }

    //load the content
    bool ok;
    QDomElement startItem = root.firstChildElement(QStringLiteral("start"));
    while(!startItem.isNull())
    {
        if(startItem.isElement())
        {
            Profile profile;
            QDomElement map = startItem.firstChildElement(QStringLiteral("map"));
            if(!map.isNull() && map.isElement() && map.hasAttribute(QStringLiteral("file")) && map.hasAttribute(QStringLiteral("x")) && map.hasAttribute(QStringLiteral("y")))
            {
                profile.map=map.attribute(QStringLiteral("file"));
                if(!QFile::exists(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+profile.map))
                {
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, map don't exists %2: child.tagName(): %3 (at line: %4)").arg(file).arg(profile.map).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(QStringLiteral("start"));
                    continue;
                }
                profile.x=map.attribute(QStringLiteral("x")).toUShort(&ok);
                if(!ok)
                {
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, map x is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(QStringLiteral("start"));
                    continue;
                }
                profile.y=map.attribute(QStringLiteral("y")).toUShort(&ok);
                if(!ok)
                {
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, map y is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(QStringLiteral("start"));
                    continue;
                }
            }
            else
            {
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, no correct map configuration: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement(QStringLiteral("start"));
                continue;
            }
            QDomElement forcedskin = startItem.firstChildElement(QStringLiteral("forcedskin"));
            if(!forcedskin.isNull() && forcedskin.isElement() && forcedskin.hasAttribute(QStringLiteral("value")))
            {
                profile.forcedskin=forcedskin.attribute(QStringLiteral("value")).split(QStringLiteral(";"));
                int index=0;
                while(index<profile.forcedskin.size())
                {
                    if(!QFile::exists(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_SKIN)+profile.forcedskin.at(index)))
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, skin %4 don't exists: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(profile.forcedskin.at(index)));
                        profile.forcedskin.removeAt(index);
                    }
                    else
                        index++;
                }
            }
            profile.cash=0;
            QDomElement cash = startItem.firstChildElement(QStringLiteral("cash"));
            if(!cash.isNull() && cash.isElement() && cash.hasAttribute(QStringLiteral("value")))
            {
                profile.cash=cash.attribute(QStringLiteral("value")).toULongLong(&ok);
                if(!ok)
                {
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, cash is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    profile.cash=0;
                }
            }
            QDomElement monstersElement = startItem.firstChildElement(QStringLiteral("monster"));
            while(!monstersElement.isNull())
            {
                Profile::Monster monster;
                if(monstersElement.isElement() && monstersElement.hasAttribute(QStringLiteral("id")) && monstersElement.hasAttribute(QStringLiteral("level")) && monstersElement.hasAttribute(QStringLiteral("captured_with")))
                {
                    monster.id=monstersElement.attribute(QStringLiteral("id")).toUInt(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, monster id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        monstersElement = monstersElement.nextSiblingElement(QStringLiteral("monster"));
                        continue;
                    }
                    monster.level=monstersElement.attribute(QStringLiteral("level")).toUShort(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, monster level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        monstersElement = monstersElement.nextSiblingElement(QStringLiteral("monster"));
                        continue;
                    }
                    if(monster.level==0 || monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, monster level is not into the range: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        monstersElement = monstersElement.nextSiblingElement(QStringLiteral("monster"));
                        continue;
                    }
                    monster.captured_with=monstersElement.attribute(QStringLiteral("captured_with")).toUInt(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, captured_with is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        monstersElement = monstersElement.nextSiblingElement(QStringLiteral("monster"));
                        continue;
                    }
                    profile.monsters << monster;
                }
                monstersElement = monstersElement.nextSiblingElement(QStringLiteral("monster"));
            }
            if(profile.monsters.empty())
            {
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, not monster to load: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement(QStringLiteral("start"));
                continue;
            }
            QDomElement reputationElement = startItem.firstChildElement(QStringLiteral("reputation"));
            while(!reputationElement.isNull())
            {
                Profile::Reputation reputationTemp;
                if(reputationElement.isElement() && reputationElement.hasAttribute(QStringLiteral("type")) && reputationElement.hasAttribute(QStringLiteral("level")))
                {
                    reputationTemp.type=reputationElement.attribute(QStringLiteral("type"));
                    reputationTemp.level=reputationElement.attribute(QStringLiteral("level")).toShort(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, reputation level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        reputationElement = reputationElement.nextSiblingElement(QStringLiteral("reputation"));
                        continue;
                    }
                    reputationTemp.point=0;
                    if(reputationElement.hasAttribute(QStringLiteral("point")))
                    {
                        reputationTemp.point=reputationElement.attribute(QStringLiteral("point")).toInt(&ok);
                        if(!ok)
                        {
                            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, reputation point is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                            reputationElement = reputationElement.nextSiblingElement(QStringLiteral("reputation"));
                            continue;
                        }
                        if((reputationTemp.point>0 && reputationTemp.level<0) || (reputationTemp.point<0 && reputationTemp.level>=0))
                        {
                            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, reputation point is not negative/positive like the level: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                            reputationElement = reputationElement.nextSiblingElement(QStringLiteral("reputation"));
                            continue;
                        }
                    }
                    profile.reputation << reputationTemp;
                }
                reputationElement = reputationElement.nextSiblingElement(QStringLiteral("reputation"));
            }
            QDomElement itemElement = startItem.firstChildElement(QStringLiteral("item"));
            while(!itemElement.isNull())
            {
                Profile::Item itemTemp;
                if(itemElement.isElement() && itemElement.hasAttribute(QStringLiteral("id")))
                {
                    itemTemp.id=itemElement.attribute(QStringLiteral("id")).toUInt(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, item id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        itemElement = itemElement.nextSiblingElement(QStringLiteral("item"));
                        continue;
                    }
                    itemTemp.quantity=0;
                    if(itemElement.hasAttribute(QStringLiteral("quantity")))
                    {
                        itemTemp.quantity=itemElement.attribute(QStringLiteral("quantity")).toUInt(&ok);
                        if(!ok)
                        {
                            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, item quantity is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                            itemElement = itemElement.nextSiblingElement(QStringLiteral("item"));
                            continue;
                        }
                        if(itemTemp.quantity==0)
                        {
                            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, item quantity is null: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                            itemElement = itemElement.nextSiblingElement(QStringLiteral("item"));
                            continue;
                        }
                    }
                    profile.items << itemTemp;
                }
                itemElement = itemElement.nextSiblingElement(QStringLiteral("item"));
            }
            returnVar.second << profile;
            returnVar.first << startItem;

        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
        startItem = startItem.nextSiblingElement(QStringLiteral("start"));
    }
    return returnVar;
}
