#include "DatapackGeneralLoader.h"
#include "DebugClass.h"

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
        QFile itemsFile(entryList.at(index).absoluteFilePath()+"/definition.xml");
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
            index++;
            continue;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QDomDocument domDocument;
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!="quest")
        {
            qDebug() << QString("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(itemsFile.fileName());
            index++;
            continue;
        }

        //load the content
        bool ok;

        if(!root.hasAttribute("id"))
        {
            index++;
            continue;
        }
        QList<quint32> defaultBots;
        CatchChallenger::Quest quest;
        quest.id=root.attribute("id").toUInt(&ok);
        if(!ok)
        {
            index++;
            continue;
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
                QDomElement requirementsItem;
                //load requirements reputation
                requirementsItem = requirements.firstChildElement("reputation");
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
                //load requirements quest
                requirementsItem = requirements.firstChildElement("quest");
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
                QDomElement rewardsItem;
                //load rewards reputation
                rewardsItem = rewards.firstChildElement("reputation");
                while(!rewardsItem.isNull())
                {
                    if(rewardsItem.isElement())
                    {
                        if(rewardsItem.hasAttribute("type") && rewardsItem.hasAttribute("point"))
                        {
                            qint32 point=rewardsItem.attribute("point").toInt(&ok);
                            if(ok)
                            {
                                CatchChallenger::Quest::ReputationRewards reputation;
                                reputation.point=point;
                                reputation.type=rewardsItem.attribute("type");
                                quest.rewards.reputation << reputation;
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, quest rewards point is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                        }
                        else
                            qDebug() << QString("Has attribute: %1, quest rewards point have not type or point attribute: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    rewardsItem = rewardsItem.nextSiblingElement("rewards");
                }
                //load rewards item
                rewardsItem = rewards.firstChildElement("item");
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
        if(indexLoop>=(steps.size()+1))
        {
            //add it, all seam ok
            quests[quest.id]=quest;
        }

        index++;
    }
    return quests;
}

