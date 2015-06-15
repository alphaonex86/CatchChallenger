#include "DatapackGeneralLoader.h"
#include "DebugClass.h"
#include "GeneralVariable.h"
#include "CommonDatapack.h"
#include "FacilityLib.h"
#include "FacilityLibGeneral.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfoList>
#include <QDir>

using namespace CatchChallenger;

const QString DatapackGeneralLoader::text_list=QLatin1String("list");
const QString DatapackGeneralLoader::text_dotxml=QLatin1String(".xml");
const QString DatapackGeneralLoader::text_dottmx=QLatin1String(".tmx");
const QString DatapackGeneralLoader::text_reputation=QLatin1String("reputation");
const QString DatapackGeneralLoader::text_type=QLatin1String("type");
const QString DatapackGeneralLoader::text_level=QLatin1String("level");
const QString DatapackGeneralLoader::text_point=QLatin1String("point");
const QString DatapackGeneralLoader::text_slashdefinitionxml=QLatin1String("/definition.xml");
const QString DatapackGeneralLoader::text_quest=QLatin1String("quest");
const QString DatapackGeneralLoader::text_repeatable=QLatin1String("repeatable");
const QString DatapackGeneralLoader::text_yes=QLatin1String("yes");
const QString DatapackGeneralLoader::text_true=QLatin1String("true");
const QString DatapackGeneralLoader::text_bot=QLatin1String("bot");
const QString DatapackGeneralLoader::text_dotcomma=QLatin1String(";");
const QString DatapackGeneralLoader::text_requirements=QLatin1String("requirements");
const QString DatapackGeneralLoader::text_less=QLatin1String("-");
const QString DatapackGeneralLoader::text_id=QLatin1String("id");
const QString DatapackGeneralLoader::text_rewards=QLatin1String("rewards");
const QString DatapackGeneralLoader::text_allow=QLatin1String("allow");
const QString DatapackGeneralLoader::text_clan=QLatin1String("clan");
const QString DatapackGeneralLoader::text_step=QLatin1String("step");
const QString DatapackGeneralLoader::text_item=QLatin1String("item");
const QString DatapackGeneralLoader::text_quantity=QLatin1String("quantity");
const QString DatapackGeneralLoader::text_monster=QLatin1String("monster");
const QString DatapackGeneralLoader::text_rate=QLatin1String("rate");
const QString DatapackGeneralLoader::text_percent=QLatin1String("%");
const QString DatapackGeneralLoader::text_fight=QLatin1String("fight");
const QString DatapackGeneralLoader::text_plants=QLatin1String("plants");
const QString DatapackGeneralLoader::text_plant=QLatin1String("plant");
const QString DatapackGeneralLoader::text_itemUsed=QLatin1String("itemUsed");
const QString DatapackGeneralLoader::text_grow=QLatin1String("grow");
const QString DatapackGeneralLoader::text_fruits=QLatin1String("fruits");
const QString DatapackGeneralLoader::text_sprouted=QLatin1String("sprouted");
const QString DatapackGeneralLoader::text_taller=QLatin1String("taller");
const QString DatapackGeneralLoader::text_flowering=QLatin1String("flowering");
const QString DatapackGeneralLoader::text_recipes=QLatin1String("recipes");
const QString DatapackGeneralLoader::text_recipe=QLatin1String("recipe");
const QString DatapackGeneralLoader::text_itemToLearn=QLatin1String("itemToLearn");
const QString DatapackGeneralLoader::text_doItemId=QLatin1String("doItemId");
const QString DatapackGeneralLoader::text_success=QLatin1String("success");
const QString DatapackGeneralLoader::text_material=QLatin1String("material");
const QString DatapackGeneralLoader::text_industries=QLatin1String("industries");
const QString DatapackGeneralLoader::text_industrialrecipe=QLatin1String("industrialrecipe");
const QString DatapackGeneralLoader::text_time=QLatin1String("time");
const QString DatapackGeneralLoader::text_cycletobefull=QLatin1String("cycletobefull");
const QString DatapackGeneralLoader::text_resource=QLatin1String("resource");
const QString DatapackGeneralLoader::text_product=QLatin1String("product");
const QString DatapackGeneralLoader::text_link=QLatin1String("link");
const QString DatapackGeneralLoader::text_price=QLatin1String("price");
const QString DatapackGeneralLoader::text_consumeAtUse=QLatin1String("consumeAtUse");
const QString DatapackGeneralLoader::text_false=QLatin1String("false");
const QString DatapackGeneralLoader::text_trap=QLatin1String("trap");
const QString DatapackGeneralLoader::text_bonus_rate=QLatin1String("bonus_rate");
const QString DatapackGeneralLoader::text_repel=QLatin1String("repel");
const QString DatapackGeneralLoader::text_hp=QLatin1String("hp");
const QString DatapackGeneralLoader::text_add=QLatin1String("add");
const QString DatapackGeneralLoader::text_all=QLatin1String("all");
const QString DatapackGeneralLoader::text_buff=QLatin1String("buff");
const QString DatapackGeneralLoader::text_remove=QLatin1String("remove");
const QString DatapackGeneralLoader::text_up=QLatin1String("up");
const QString DatapackGeneralLoader::text_start=QLatin1String("start");
const QString DatapackGeneralLoader::text_map=QLatin1String("map");
const QString DatapackGeneralLoader::text_file=QLatin1String("file");
const QString DatapackGeneralLoader::text_x=QLatin1String("x");
const QString DatapackGeneralLoader::text_y=QLatin1String("y");
const QString DatapackGeneralLoader::text_forcedskin=QLatin1String("forcedskin");
const QString DatapackGeneralLoader::text_cash=QLatin1String("cash");
const QString DatapackGeneralLoader::text_itemId=QLatin1String("itemId");
const QString DatapackGeneralLoader::text_industry=QLatin1String("industry");
const QString DatapackGeneralLoader::text_items=QLatin1String("items");
const QString DatapackGeneralLoader::text_value=QLatin1String("value");
const QString DatapackGeneralLoader::text_captured_with=QLatin1String("captured_with");
const QString DatapackGeneralLoader::text_monstersCollision=QLatin1String("monstersCollision");
const QString DatapackGeneralLoader::text_monsterType=QLatin1String("monsterType");
const QString DatapackGeneralLoader::text_walkOn=QLatin1String("walkOn");
const QString DatapackGeneralLoader::text_actionOn=QLatin1String("actionOn");
const QString DatapackGeneralLoader::text_layer=QLatin1String("layer");
const QString DatapackGeneralLoader::text_tile=QLatin1String("tile");
const QString DatapackGeneralLoader::text_background=QLatin1String("background");
const QString DatapackGeneralLoader::text_slash=QLatin1String("/");
const QString DatapackGeneralLoader::text_layers=QLatin1String("layers");
const QString DatapackGeneralLoader::text_events=QLatin1String("events");
const QString DatapackGeneralLoader::text_event=QLatin1String("event");
const QString DatapackGeneralLoader::text_shop=QLatin1String("shop");
const QString DatapackGeneralLoader::text_shops=QLatin1String("shops");
const QString DatapackGeneralLoader::text_overridePrice=QLatin1String("overridePrice");
const QString DatapackGeneralLoader::text_inverse=QLatin1String("inverse");

QList<Reputation> DatapackGeneralLoader::loadReputation(const QString &file)
{
    QRegExp excludeFilterRegex(QLatin1String("[\"']"));
    QRegExp typeRegex(QLatin1String("^[a-z]{1,32}$"));
    QDomDocument domDocument;
    QList<Reputation> reputation;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        //open and quick check the file
        QFile itemsFile(file);
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString()));
            return reputation;
        }
        const QByteArray &xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return reputation;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_list)
    {
        qDebug() << (QStringLiteral("Unable to open the file: %1, \"list\" root balise not found for reputation of the xml file").arg(file));
        return reputation;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(DatapackGeneralLoader::text_reputation);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(DatapackGeneralLoader::text_type))
            {
                QList<qint32> point_list_positive,point_list_negative;
                QStringList text_positive,text_negative;
                QDomElement level = item.firstChildElement(DatapackGeneralLoader::text_level);
                ok=true;
                while(!level.isNull() && ok)
                {
                    if(level.isElement())
                    {
                        if(level.hasAttribute(DatapackGeneralLoader::text_point))
                        {
                            const qint32 &point=level.attribute(DatapackGeneralLoader::text_point).toInt(&ok);
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
                                            qDebug() << (QStringLiteral("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
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
                                            qDebug() << (QStringLiteral("Unable to open the file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
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
                                qDebug() << (QStringLiteral("Unable to open the file: %1, point is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                    }
                    else
                        qDebug() << (QStringLiteral("Unable to open the file: %1, point attribute not found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    level = level.nextSiblingElement(DatapackGeneralLoader::text_level);
                }
                qSort(point_list_positive);
                qSort(point_list_negative.end(),point_list_negative.begin());
                if(ok)
                    if(point_list_positive.size()<2)
                    {
                        qDebug() << (QStringLiteral("Unable to open the file: %1, reputation have to few level: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_positive.contains(0))
                    {
                        qDebug() << (QStringLiteral("Unable to open the file: %1, no starting level for the positive: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_negative.empty() && !point_list_negative.contains(-1))
                    {
                        //qDebug() << (QStringLiteral("Unable to open the file: %1, no starting level for the negative, first level need start with -1, fix by change range: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        QList<qint32> point_list_negative_new;
                        int lastValue=-1;
                        int index=0;
                        while(index<point_list_negative.size())
                        {
                            point_list_negative_new << lastValue;
                            lastValue=point_list_negative.at(index);//(1 less to negative value)
                            index++;
                        }
                        point_list_negative=point_list_negative_new;
                    }
                if(ok)
                    if(!item.attribute(DatapackGeneralLoader::text_type).contains(typeRegex))
                    {
                        qDebug() << (QStringLiteral("Unable to open the file: %1, the type %4 don't match wiuth the regex: ^[a-z]{1,32}$: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute(DatapackGeneralLoader::text_type)));
                        ok=false;
                    }
                if(ok)
                {
                    const QString &type=item.attribute(DatapackGeneralLoader::text_type);
                    if(!type.contains(excludeFilterRegex))
                    {
                        Reputation reputationTemp;
                        reputationTemp.name=type;
                        reputationTemp.reputation_positive=point_list_positive;
                        reputationTemp.reputation_negative=point_list_negative;
                        reputation << reputationTemp;
                    }
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            qDebug() << (QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(DatapackGeneralLoader::text_reputation);
    }

    return reputation;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
QHash<quint16, Quest> DatapackGeneralLoader::loadQuests(const QString &folder)
{
    bool ok;
    QHash<quint16, Quest> quests;
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
        if(!QFile(entryList.at(index).absoluteFilePath()+DatapackGeneralLoader::text_slashdefinitionxml).exists())
        {
            index++;
            continue;
        }
        const quint32 &questId=entryList.at(index).fileName().toUInt(&ok);
        if(ok)
        {
            //add it, all seam ok
            QPair<bool,Quest> returnedQuest=loadSingleQuest(entryList.at(index).absoluteFilePath()+DatapackGeneralLoader::text_slashdefinitionxml);
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
            qDebug() << (QStringLiteral("Unable to open the folder: %1, because is folder name is not a number").arg(entryList.at(index).fileName()));
        index++;
    }
    return quests;
}

QPair<bool,Quest> DatapackGeneralLoader::loadSingleQuest(const QString &file)
{
    QHash<QString,int> reputationNameToId;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    CatchChallenger::Quest quest;
    quest.id=0;
    QDomDocument domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        #endif
        QFile itemsFile(file);
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return QPair<bool,Quest>(false,quest);
        }
        const QByteArray &xmlContent=itemsFile.readAll();
        itemsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return QPair<bool,Quest>(false,quest);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_quest)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(file);
        return QPair<bool,Quest>(false,quest);
    }

    //load the content
    bool ok;
    QList<quint16> defaultBots;
    quest.id=0;
    quest.repeatable=false;
    if(root.hasAttribute(DatapackGeneralLoader::text_repeatable))
        if(root.attribute(DatapackGeneralLoader::text_repeatable)==DatapackGeneralLoader::text_yes || root.attribute(DatapackGeneralLoader::text_repeatable)==DatapackGeneralLoader::text_true)
            quest.repeatable=true;
    if(root.hasAttribute(DatapackGeneralLoader::text_bot))
    {
        const QStringList &tempStringList=root.attribute(DatapackGeneralLoader::text_bot).split(DatapackGeneralLoader::text_dotcomma);
        int index=0;
        while(index<tempStringList.size())
        {
            quint16 tempInt=tempStringList.at(index).toUShort(&ok);
            if(ok)
                defaultBots << tempInt;
            index++;
        }
    }

    //load requirements
    QDomElement requirements = root.firstChildElement(DatapackGeneralLoader::text_requirements);
    while(!requirements.isNull())
    {
        if(requirements.isElement())
        {
            //load requirements reputation
            {
                QDomElement requirementsItem = requirements.firstChildElement(DatapackGeneralLoader::text_reputation);
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute(DatapackGeneralLoader::text_type) && requirementsItem.hasAttribute(DatapackGeneralLoader::text_level))
                        {
                            if(reputationNameToId.contains(requirementsItem.attribute(DatapackGeneralLoader::text_type)))
                            {
                                QString stringLevel=requirementsItem.attribute(DatapackGeneralLoader::text_level);
                                bool positif=!stringLevel.startsWith(DatapackGeneralLoader::text_less);
                                if(!positif)
                                    stringLevel.remove(0,1);
                                quint8 level=stringLevel.toUShort(&ok);
                                if(ok)
                                {
                                    CatchChallenger::ReputationRequirements reputation;
                                    reputation.level=level;
                                    reputation.positif=positif;
                                    reputation.reputationId=reputationNameToId.value(requirementsItem.attribute(DatapackGeneralLoader::text_type));
                                    quest.requirements.reputation << reputation;
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, reputation is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber()).arg(stringLevel);
                            }
                            else
                                qDebug() << QStringLiteral("Has attribute: %1, reputation not found: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, have not attribute type or level: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    requirementsItem = requirementsItem.nextSiblingElement(DatapackGeneralLoader::text_reputation);
                }
            }
            //load requirements quest
            {
                QDomElement requirementsItem = requirements.firstChildElement(DatapackGeneralLoader::text_quest);
                while(!requirementsItem.isNull())
                {
                    if(requirementsItem.isElement())
                    {
                        if(requirementsItem.hasAttribute(DatapackGeneralLoader::text_id))
                        {
                            const quint32 &questId=requirementsItem.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                            if(ok)
                            {
                                QuestRequirements questNewEntry;
                                questNewEntry.quest=questId;
                                questNewEntry.inverse=false;
                                if(requirementsItem.hasAttribute(DatapackGeneralLoader::text_inverse))
                                    if(requirementsItem.attribute(DatapackGeneralLoader::text_inverse)==DatapackGeneralLoader::text_true)
                                        questNewEntry.inverse=true;
                                quest.requirements.quests << questNewEntry;
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, requirement quest item id is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber()).arg(requirementsItem.attribute(DatapackGeneralLoader::text_id));
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, requirement quest item have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(requirementsItem.tagName()).arg(requirementsItem.lineNumber());
                    requirementsItem = requirementsItem.nextSiblingElement(DatapackGeneralLoader::text_quest);
                }
            }
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(requirements.tagName()).arg(requirements.lineNumber());
        requirements = requirements.nextSiblingElement(DatapackGeneralLoader::text_requirements);
    }

    //load rewards
    QDomElement rewards = root.firstChildElement(DatapackGeneralLoader::text_rewards);
    while(!rewards.isNull())
    {
        if(rewards.isElement())
        {
            //load rewards reputation
            {
                QDomElement reputationItem = rewards.firstChildElement(DatapackGeneralLoader::text_reputation);
                while(!reputationItem.isNull())
                {
                    if(reputationItem.isElement())
                    {
                        if(reputationItem.hasAttribute(DatapackGeneralLoader::text_type) && reputationItem.hasAttribute(DatapackGeneralLoader::text_point))
                        {
                            if(reputationNameToId.contains(reputationItem.attribute(DatapackGeneralLoader::text_type)))
                            {
                                const qint32 &point=reputationItem.attribute(DatapackGeneralLoader::text_point).toInt(&ok);
                                if(ok)
                                {
                                    CatchChallenger::ReputationRewards reputation;
                                    reputation.reputationId=reputationNameToId.value(reputationItem.attribute(DatapackGeneralLoader::text_type));
                                    reputation.point=point;
                                    quest.rewards.reputation << reputation;
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, quest rewards point is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, quest rewards point is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, quest rewards point have not type or point attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                    reputationItem = reputationItem.nextSiblingElement(DatapackGeneralLoader::text_reputation);
                }
            }
            //load rewards item
            {
                QDomElement rewardsItem = rewards.firstChildElement(DatapackGeneralLoader::text_item);
                while(!rewardsItem.isNull())
                {
                    if(rewardsItem.isElement())
                    {
                        if(rewardsItem.hasAttribute(DatapackGeneralLoader::text_id))
                        {
                            CatchChallenger::Quest::Item item;
                            item.item=rewardsItem.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                            item.quantity=1;
                            if(ok)
                            {
                                if(!CommonDatapack::commonDatapack.items.item.contains(item.item))
                                {
                                    qDebug() << QStringLiteral("Unable to open the file: %1, rewards item id is not into the item list: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber()).arg(rewardsItem.attribute(DatapackGeneralLoader::text_id));
                                    return QPair<bool,Quest>(false,quest);
                                }
                                if(rewardsItem.hasAttribute(DatapackGeneralLoader::text_quantity))
                                {
                                    item.quantity=rewardsItem.attribute(DatapackGeneralLoader::text_quantity).toUInt(&ok);
                                    if(!ok)
                                        item.quantity=1;
                                }
                                quest.rewards.items << item;
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, rewards item id is not a number: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber()).arg(rewardsItem.attribute(DatapackGeneralLoader::text_id));
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, rewards item have not attribute id: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(rewardsItem.tagName()).arg(rewardsItem.lineNumber());
                    rewardsItem = rewardsItem.nextSiblingElement(DatapackGeneralLoader::text_item);
                }
            }
            //load rewards allow
            {
                QDomElement allowItem = rewards.firstChildElement(DatapackGeneralLoader::text_allow);
                while(!allowItem.isNull())
                {
                    if(allowItem.isElement())
                    {
                        if(allowItem.hasAttribute(DatapackGeneralLoader::text_type))
                        {
                            if(allowItem.attribute(DatapackGeneralLoader::text_type)==DatapackGeneralLoader::text_clan)
                                quest.rewards.allow << CatchChallenger::ActionAllow_Clan;
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, allow type not understand: child.tagName(): %2 (at line: %3)").arg(file).arg(allowItem.tagName()).arg(allowItem.lineNumber()).arg(allowItem.attribute(DatapackGeneralLoader::text_id));
                        }
                        else
                            qDebug() << QStringLiteral("Has attribute: %1, rewards item have not attribute id: child.tagName(): %2 (at line: %3)").arg(file).arg(allowItem.tagName()).arg(allowItem.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(allowItem.tagName()).arg(allowItem.lineNumber());
                    allowItem = allowItem.nextSiblingElement(DatapackGeneralLoader::text_allow);
                }
            }
            quest.rewards.allow.fromSet(quest.rewards.allow.toSet());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(rewards.tagName()).arg(rewards.lineNumber());
        rewards = rewards.nextSiblingElement(DatapackGeneralLoader::text_rewards);
    }

    QHash<quint8,CatchChallenger::Quest::Step> steps;
    //load step
    QDomElement step = root.firstChildElement(DatapackGeneralLoader::text_step);
    while(!step.isNull())
    {
        if(step.isElement())
        {
            if(step.hasAttribute(DatapackGeneralLoader::text_id))
            {
                const quint32 &id=step.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                if(ok)
                {
                    CatchChallenger::Quest::Step stepObject;
                    if(step.hasAttribute(DatapackGeneralLoader::text_bot))
                    {
                        const QStringList &tempStringList=step.attribute(DatapackGeneralLoader::text_bot).split(DatapackGeneralLoader::text_dotcomma);
                        int index=0;
                        while(index<tempStringList.size())
                        {
                            const quint32 &tempInt=tempStringList.at(index).toUInt(&ok);
                            if(ok)
                                stepObject.bots << tempInt;
                            index++;
                        }
                    }
                    else
                        stepObject.bots=defaultBots;
                    //do the item
                    {
                        QDomElement stepItem = step.firstChildElement(DatapackGeneralLoader::text_item);
                        while(!stepItem.isNull())
                        {
                            if(stepItem.isElement())
                            {
                                if(stepItem.hasAttribute(DatapackGeneralLoader::text_id))
                                {
                                    CatchChallenger::Quest::Item item;
                                    item.item=stepItem.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                                    item.quantity=1;
                                    if(ok)
                                    {
                                        if(!CommonDatapack::commonDatapack.items.item.contains(item.item))
                                        {
                                            qDebug() << QStringLiteral("Unable to open the file: %1, rewards item id is not into the item list: %4: child.tagName(): %2 (at line: %3)").arg(file).arg(stepItem.tagName()).arg(stepItem.lineNumber()).arg(stepItem.attribute(DatapackGeneralLoader::text_id));
                                            return QPair<bool,Quest>(false,quest);
                                        }
                                        if(stepItem.hasAttribute(DatapackGeneralLoader::text_quantity))
                                        {
                                            item.quantity=stepItem.attribute(DatapackGeneralLoader::text_quantity).toUInt(&ok);
                                            if(!ok)
                                                item.quantity=1;
                                        }
                                        stepObject.requirements.items << item;
                                        if(stepItem.hasAttribute(DatapackGeneralLoader::text_monster) && stepItem.hasAttribute(DatapackGeneralLoader::text_rate))
                                        {
                                            CatchChallenger::Quest::ItemMonster itemMonster;
                                            itemMonster.item=item.item;

                                            const QStringList &tempStringList=stepItem.attribute(DatapackGeneralLoader::text_monster).split(DatapackGeneralLoader::text_dotcomma);
                                            int index=0;
                                            while(index<tempStringList.size())
                                            {
                                                const quint32 &tempInt=tempStringList.at(index).toUInt(&ok);
                                                if(ok)
                                                    itemMonster.monsters << tempInt;
                                                index++;
                                            }

                                            QString rateString=stepItem.attribute(DatapackGeneralLoader::text_rate);
                                            rateString.remove(DatapackGeneralLoader::text_percent);
                                            itemMonster.rate=rateString.toUShort(&ok);
                                            if(ok)
                                                stepObject.itemsMonster << itemMonster;
                                        }
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, step id is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber()).arg(stepItem.attribute(DatapackGeneralLoader::text_id));
                                }
                                else
                                    qDebug() << QStringLiteral("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            stepItem = stepItem.nextSiblingElement(DatapackGeneralLoader::text_item);
                        }
                    }
                    //do the fight
                    {
                        QDomElement fightItem = step.firstChildElement(DatapackGeneralLoader::text_fight);
                        while(!fightItem.isNull())
                        {
                            if(fightItem.isElement())
                            {
                                if(fightItem.hasAttribute(DatapackGeneralLoader::text_id))
                                {
                                    const quint32 &fightId=fightItem.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                                    if(ok)
                                        stepObject.requirements.fightId << fightId;
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, step id is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber()).arg(fightItem.attribute(DatapackGeneralLoader::text_id));
                                }
                                else
                                    qDebug() << QStringLiteral("Has attribute: %1, step have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                            fightItem = fightItem.nextSiblingElement(DatapackGeneralLoader::text_fight);
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
        step = step.nextSiblingElement(DatapackGeneralLoader::text_step);
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
    QHash<QString,int> reputationNameToId;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    QHash<quint8, Plant> plants;
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile plantsFile(file);
        if(!plantsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the plants file: %1, error: %2").arg(file).arg(plantsFile.errorString());
            return plants;
        }
        const QByteArray &xmlContent=plantsFile.readAll();
        plantsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the plants file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return plants;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_plants)
    {
        qDebug() << QStringLiteral("Unable to open the plants file: %1, \"plants\" root balise not found for the xml file").arg(file);
        return plants;
    }

    //load the content
    bool ok,ok2;
    QDomElement plantItem = root.firstChildElement(DatapackGeneralLoader::text_plant);
    while(!plantItem.isNull())
    {
        if(plantItem.isElement())
        {
            if(plantItem.hasAttribute(DatapackGeneralLoader::text_id) && plantItem.hasAttribute(DatapackGeneralLoader::text_itemUsed))
            {
                const quint8 &id=plantItem.attribute(DatapackGeneralLoader::text_id).toUShort(&ok);
                const quint32 &itemUsed=plantItem.attribute(DatapackGeneralLoader::text_itemUsed).toUInt(&ok2);
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
                        {
                            QDomElement requirementsItem = plantItem.firstChildElement(DatapackGeneralLoader::text_requirements);
                            if(!requirementsItem.isNull() && requirementsItem.isElement())
                            {
                                QDomElement reputationItem = requirementsItem.firstChildElement(DatapackGeneralLoader::text_reputation);
                                while(!reputationItem.isNull())
                                {
                                    if(reputationItem.isElement())
                                    {
                                        if(reputationItem.hasAttribute(DatapackGeneralLoader::text_type) && reputationItem.hasAttribute(DatapackGeneralLoader::text_level))
                                        {
                                            if(reputationNameToId.contains(reputationItem.attribute(DatapackGeneralLoader::text_type)))
                                            {
                                                ReputationRequirements reputationRequirements;
                                                QString stringLevel=reputationItem.attribute(DatapackGeneralLoader::text_level);
                                                reputationRequirements.positif=!stringLevel.startsWith(DatapackGeneralLoader::text_less);
                                                if(!reputationRequirements.positif)
                                                    stringLevel.remove(0,1);
                                                reputationRequirements.level=stringLevel.toUShort(&ok);
                                                if(ok)
                                                {
                                                    reputationRequirements.reputationId=reputationNameToId.value(reputationItem.attribute(DatapackGeneralLoader::text_type));
                                                    plant.requirements.reputation << reputationRequirements;
                                                }
                                            }
                                            else
                                                qDebug() << QStringLiteral("Reputation type not found: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                        }
                                        else
                                            qDebug() << QStringLiteral("Unable to open the industries link file: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the industries link file: %1, is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                    reputationItem = reputationItem.nextSiblingElement(DatapackGeneralLoader::text_reputation);
                                }
                            }
                        }
                        {
                            QDomElement rewardsItem = plantItem.firstChildElement(DatapackGeneralLoader::text_rewards);
                            if(!rewardsItem.isNull() && rewardsItem.isElement())
                            {
                                QDomElement reputationItem = rewardsItem.firstChildElement(DatapackGeneralLoader::text_reputation);
                                while(!reputationItem.isNull())
                                {
                                    if(reputationItem.isElement())
                                    {
                                        if(reputationItem.hasAttribute(DatapackGeneralLoader::text_type) && reputationItem.hasAttribute(DatapackGeneralLoader::text_point))
                                        {
                                            if(reputationNameToId.contains(reputationItem.attribute(DatapackGeneralLoader::text_type)))
                                            {
                                                ReputationRewards reputationRewards;
                                                reputationRewards.point=reputationItem.attribute(DatapackGeneralLoader::text_point).toInt(&ok);
                                                if(ok)
                                                {
                                                    reputationRewards.reputationId=reputationNameToId.value(reputationItem.attribute(DatapackGeneralLoader::text_type));
                                                    plant.rewards.reputation << reputationRewards;
                                                }
                                            }
                                        }
                                        else
                                            qDebug() << QStringLiteral("Unable to open the industries link file: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the industries link file: %1, is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                    reputationItem = reputationItem.nextSiblingElement(DatapackGeneralLoader::text_reputation);
                                }
                            }
                        }
                        ok=false;
                        QDomElement quantity = plantItem.firstChildElement(DatapackGeneralLoader::text_quantity);
                        if(!quantity.isNull())
                        {
                            if(quantity.isElement())
                            {
                                const float &float_quantity=quantity.text().toFloat(&ok2);
                                const int &integer_part=float_quantity;
                                float random_part=float_quantity-integer_part;
                                random_part*=RANDOM_FLOAT_PART_DIVIDER;
                                plant.fix_quantity=integer_part;
                                plant.random_quantity=random_part;
                                ok=ok2;
                            }
                        }
                        int intermediateTimeCount=0;
                        QDomElement grow = plantItem.firstChildElement(DatapackGeneralLoader::text_grow);
                        if(!grow.isNull())
                        {
                            if(grow.isElement())
                            {
                                const QDomElement &fruits = grow.firstChildElement(DatapackGeneralLoader::text_fruits);
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
                                const QDomElement &sprouted = grow.firstChildElement(DatapackGeneralLoader::text_sprouted);
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
                                const QDomElement &taller = grow.firstChildElement(DatapackGeneralLoader::text_taller);
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
                                const QDomElement &flowering = grow.firstChildElement(DatapackGeneralLoader::text_flowering);
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
        plantItem = plantItem.nextSiblingElement(DatapackGeneralLoader::text_plant);
    }
    return plants;
}

QPair<QHash<quint16,CrafingRecipe>,QHash<quint16,quint16> > DatapackGeneralLoader::loadCraftingRecipes(const QString &file,const QHash<quint16, Item> &items)
{
    QHash<QString,int> reputationNameToId;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    QHash<quint16,CrafingRecipe> crafingRecipes;
    QHash<quint16,quint16> itemToCrafingRecipes;
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile craftingRecipesFile(file);
        if(!craftingRecipesFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, error: %2").arg(file).arg(craftingRecipesFile.errorString());
            return QPair<QHash<quint16,CrafingRecipe>,QHash<quint16,quint16> >(crafingRecipes,itemToCrafingRecipes);
        }
        const QByteArray &xmlContent=craftingRecipesFile.readAll();
        craftingRecipesFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return QPair<QHash<quint16,CrafingRecipe>,QHash<quint16,quint16> >(crafingRecipes,itemToCrafingRecipes);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_recipes)
    {
        qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, \"recipes\" root balise not found for the xml file").arg(file);
        return QPair<QHash<quint16,CrafingRecipe>,QHash<quint16,quint16> >(crafingRecipes,itemToCrafingRecipes);
    }

    //load the content
    bool ok,ok2,ok3;
    QDomElement recipeItem = root.firstChildElement(DatapackGeneralLoader::text_recipe);
    while(!recipeItem.isNull())
    {
        if(recipeItem.isElement())
        {
            if(recipeItem.hasAttribute(DatapackGeneralLoader::text_id) && recipeItem.hasAttribute(DatapackGeneralLoader::text_itemToLearn) && recipeItem.hasAttribute(DatapackGeneralLoader::text_doItemId))
            {
                quint8 success=100;
                if(recipeItem.hasAttribute(DatapackGeneralLoader::text_success))
                {
                    const quint8 &tempShort=recipeItem.attribute(DatapackGeneralLoader::text_success).toUShort(&ok);
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
                if(recipeItem.hasAttribute(DatapackGeneralLoader::text_quantity))
                {
                    const quint32 &tempShort=recipeItem.attribute(DatapackGeneralLoader::text_quantity).toUInt(&ok);
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

                const quint32 &id=recipeItem.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                const quint32 &itemToLearn=recipeItem.attribute(DatapackGeneralLoader::text_itemToLearn).toUInt(&ok2);
                const quint32 &doItemId=recipeItem.attribute(DatapackGeneralLoader::text_doItemId).toUInt(&ok3);
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
                        {
                            QDomElement requirementsItem = recipeItem.firstChildElement(DatapackGeneralLoader::text_requirements);
                            if(!requirementsItem.isNull() && requirementsItem.isElement())
                            {
                                QDomElement reputationItem = requirementsItem.firstChildElement(DatapackGeneralLoader::text_reputation);
                                while(!reputationItem.isNull())
                                {
                                    if(reputationItem.isElement())
                                    {
                                        if(reputationItem.hasAttribute(DatapackGeneralLoader::text_type) && reputationItem.hasAttribute(DatapackGeneralLoader::text_level))
                                        {
                                            if(reputationNameToId.contains(reputationItem.attribute(DatapackGeneralLoader::text_type)))
                                            {
                                                ReputationRequirements reputationRequirements;
                                                QString stringLevel=reputationItem.attribute(DatapackGeneralLoader::text_level);
                                                reputationRequirements.positif=!stringLevel.startsWith(DatapackGeneralLoader::text_less);
                                                if(!reputationRequirements.positif)
                                                    stringLevel.remove(0,1);
                                                reputationRequirements.level=stringLevel.toUShort(&ok);
                                                if(ok)
                                                {
                                                    reputationRequirements.reputationId=reputationNameToId.value(reputationItem.attribute(DatapackGeneralLoader::text_type));
                                                    recipe.requirements.reputation << reputationRequirements;
                                                }
                                            }
                                            else
                                                qDebug() << QStringLiteral("Reputation type not found: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                        }
                                        else
                                            qDebug() << QStringLiteral("Unable to open the industries link file: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the industries link file: %1, is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                    reputationItem = reputationItem.nextSiblingElement(DatapackGeneralLoader::text_reputation);
                                }
                            }
                        }
                        {
                            QDomElement rewardsItem = recipeItem.firstChildElement(DatapackGeneralLoader::text_rewards);
                            if(!rewardsItem.isNull() && rewardsItem.isElement())
                            {
                                QDomElement reputationItem = rewardsItem.firstChildElement(DatapackGeneralLoader::text_reputation);
                                while(!reputationItem.isNull())
                                {
                                    if(reputationItem.isElement())
                                    {
                                        if(reputationItem.hasAttribute(DatapackGeneralLoader::text_type) && reputationItem.hasAttribute(DatapackGeneralLoader::text_point))
                                        {
                                            if(reputationNameToId.contains(reputationItem.attribute(DatapackGeneralLoader::text_type)))
                                            {
                                                ReputationRewards reputationRewards;
                                                reputationRewards.point=reputationItem.attribute(DatapackGeneralLoader::text_point).toInt(&ok);
                                                if(ok)
                                                {
                                                    reputationRewards.reputationId=reputationNameToId.value(reputationItem.attribute(DatapackGeneralLoader::text_type));
                                                    recipe.rewards.reputation << reputationRewards;
                                                }
                                            }
                                        }
                                        else
                                            qDebug() << QStringLiteral("Unable to open the industries link file: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the industries link file: %1, is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                    reputationItem = reputationItem.nextSiblingElement(DatapackGeneralLoader::text_reputation);
                                }
                            }
                        }
                        QDomElement material = recipeItem.firstChildElement(DatapackGeneralLoader::text_material);
                        while(!material.isNull() && ok)
                        {
                            if(material.isElement())
                            {
                                if(material.hasAttribute(DatapackGeneralLoader::text_itemId))
                                {
                                    const quint32 &itemId=material.attribute(DatapackGeneralLoader::text_itemId).toUInt(&ok2);
                                    if(!ok2)
                                    {
                                        ok=false;
                                        qDebug() << QStringLiteral("preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(recipeItem.tagName()).arg(recipeItem.lineNumber());
                                        break;
                                    }
                                    quint16 quantity=1;
                                    if(material.hasAttribute(DatapackGeneralLoader::text_quantity))
                                    {
                                        const quint32 &tempShort=material.attribute(DatapackGeneralLoader::text_quantity).toUInt(&ok2);
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
                            material = material.nextSiblingElement(DatapackGeneralLoader::text_material);
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
        recipeItem = recipeItem.nextSiblingElement(DatapackGeneralLoader::text_recipe);
    }
    return QPair<QHash<quint16,CrafingRecipe>,QHash<quint16,quint16> >(crafingRecipes,itemToCrafingRecipes);
}

QHash<quint16,Industry> DatapackGeneralLoader::loadIndustries(const QString &folder,const QHash<quint16, Item> &items)
{
    QHash<quint16,Industry> industries;
    QDir dir(folder);
    const QFileInfoList &fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
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
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
            #endif
            QFile industryFile(file);
            if(!industryFile.open(QIODevice::ReadOnly))
            {
                qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, error: %2").arg(file).arg(industryFile.errorString());
                file_index++;
                continue;
            }
            const QByteArray &xmlContent=industryFile.readAll();
            industryFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        const QDomElement &root = domDocument.documentElement();
        if(root.tagName()!=DatapackGeneralLoader::text_industries)
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, \"industries\" root balise not found for the xml file").arg(file);
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2,ok3;
        QDomElement industryItem = root.firstChildElement(DatapackGeneralLoader::text_industrialrecipe);
        while(!industryItem.isNull())
        {
            if(industryItem.isElement())
            {
                if(industryItem.hasAttribute(DatapackGeneralLoader::text_id) && industryItem.hasAttribute(DatapackGeneralLoader::text_time) && industryItem.hasAttribute(DatapackGeneralLoader::text_cycletobefull))
                {
                    Industry industry;
                    const quint32 &id=industryItem.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                    industry.time=industryItem.attribute(DatapackGeneralLoader::text_time).toUInt(&ok2);
                    industry.cycletobefull=industryItem.attribute(DatapackGeneralLoader::text_cycletobefull).toUInt(&ok3);
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
                                QDomElement resourceItem = industryItem.firstChildElement(DatapackGeneralLoader::text_resource);
                                ok=true;
                                while(!resourceItem.isNull() && ok)
                                {
                                    if(resourceItem.isElement())
                                    {
                                        Industry::Resource resource;
                                        resource.quantity=1;
                                        if(resourceItem.hasAttribute(DatapackGeneralLoader::text_quantity))
                                        {
                                            resource.quantity=resourceItem.attribute(DatapackGeneralLoader::text_quantity).toUInt(&ok);
                                            if(!ok)
                                                qDebug() << QStringLiteral("quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(resourceItem.hasAttribute(DatapackGeneralLoader::text_id))
                                            {
                                                resource.item=resourceItem.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
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
                                    resourceItem = resourceItem.nextSiblingElement(DatapackGeneralLoader::text_resource);
                                }
                            }

                            //product
                            if(ok)
                            {
                                QDomElement productItem = industryItem.firstChildElement(DatapackGeneralLoader::text_product);
                                ok=true;
                                while(!productItem.isNull() && ok)
                                {
                                    if(productItem.isElement())
                                    {
                                        Industry::Product product;
                                        product.quantity=1;
                                        if(productItem.hasAttribute(DatapackGeneralLoader::text_quantity))
                                        {
                                            product.quantity=productItem.attribute(DatapackGeneralLoader::text_quantity).toUInt(&ok);
                                            if(!ok)
                                                qDebug() << QStringLiteral("quantity is not a number: %1: child.tagName(): %2 (at line: %3)").arg(file).arg(industryItem.tagName()).arg(industryItem.lineNumber());
                                        }
                                        if(ok)
                                        {
                                            if(productItem.hasAttribute(DatapackGeneralLoader::text_id))
                                            {
                                                product.item=productItem.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
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
                                    productItem = productItem.nextSiblingElement(DatapackGeneralLoader::text_product);
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
            industryItem = industryItem.nextSiblingElement(DatapackGeneralLoader::text_industrialrecipe);
        }
        file_index++;
    }
    return industries;
}

QHash<quint16,IndustryLink> DatapackGeneralLoader::loadIndustriesLink(const QString &file,const QHash<quint16,Industry> &industries)
{
    QHash<QString,int> reputationNameToId;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    QHash<quint16,IndustryLink> industriesLink;
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile industriesLinkFile(file);
        if(!industriesLinkFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, error: %2").arg(file).arg(industriesLinkFile.errorString());
            return industriesLink;
        }
        const QByteArray &xmlContent=industriesLinkFile.readAll();
        industriesLinkFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return industriesLink;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_industries)
    {
        qDebug() << QStringLiteral("Unable to open the crafting recipe file: %1, \"industries\" root balise not found for the xml file").arg(file);
        return industriesLink;
    }

    //load the content
    bool ok,ok2;
    QDomElement linkItem = root.firstChildElement(DatapackGeneralLoader::text_link);
    while(!linkItem.isNull())
    {
        if(linkItem.isElement())
        {
            if(linkItem.hasAttribute(DatapackGeneralLoader::text_industrialrecipe) && linkItem.hasAttribute(DatapackGeneralLoader::text_industry))
            {
                const quint32 &industry_id=linkItem.attribute(DatapackGeneralLoader::text_industrialrecipe).toUInt(&ok);
                const quint32 &factory_id=linkItem.attribute(DatapackGeneralLoader::text_industry).toUInt(&ok2);
                if(ok && ok2)
                {
                    if(!industriesLink.contains(factory_id))
                    {
                        if(industries.contains(industry_id))
                        {
                            industriesLink[factory_id].industry=industry_id;
                            IndustryLink *industryLink=&industriesLink[factory_id];
                            {
                                {
                                    QDomElement requirementsItem = linkItem.firstChildElement(DatapackGeneralLoader::text_requirements);
                                    if(!requirementsItem.isNull() && requirementsItem.isElement())
                                    {
                                        QDomElement reputationItem = requirementsItem.firstChildElement(DatapackGeneralLoader::text_reputation);
                                        while(!reputationItem.isNull())
                                        {
                                            if(reputationItem.isElement())
                                            {
                                                if(reputationItem.hasAttribute(DatapackGeneralLoader::text_type) && reputationItem.hasAttribute(DatapackGeneralLoader::text_level))
                                                {
                                                    if(reputationNameToId.contains(reputationItem.attribute(DatapackGeneralLoader::text_type)))
                                                    {
                                                        ReputationRequirements reputationRequirements;
                                                        QString stringLevel=reputationItem.attribute(DatapackGeneralLoader::text_level);
                                                        reputationRequirements.positif=!stringLevel.startsWith(DatapackGeneralLoader::text_less);
                                                        if(!reputationRequirements.positif)
                                                            stringLevel.remove(0,1);
                                                        reputationRequirements.level=stringLevel.toUShort(&ok);
                                                        if(ok)
                                                        {
                                                            reputationRequirements.reputationId=reputationNameToId.value(reputationItem.attribute(DatapackGeneralLoader::text_type));
                                                            industryLink->requirements.reputation << reputationRequirements;
                                                        }
                                                    }
                                                    else
                                                        qDebug() << QStringLiteral("Reputation type not found: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                                }
                                                else
                                                    qDebug() << QStringLiteral("Unable to open the industries link file: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                            }
                                            else
                                                qDebug() << QStringLiteral("Unable to open the industries link file: %1, is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                            reputationItem = reputationItem.nextSiblingElement(DatapackGeneralLoader::text_reputation);
                                        }
                                    }
                                }
                                {
                                    QDomElement rewardsItem = linkItem.firstChildElement(DatapackGeneralLoader::text_rewards);
                                    if(!rewardsItem.isNull() && rewardsItem.isElement())
                                    {
                                        QDomElement reputationItem = rewardsItem.firstChildElement(DatapackGeneralLoader::text_reputation);
                                        while(!reputationItem.isNull())
                                        {
                                            if(reputationItem.isElement())
                                            {
                                                if(reputationItem.hasAttribute(DatapackGeneralLoader::text_type) && reputationItem.hasAttribute(DatapackGeneralLoader::text_point))
                                                {
                                                    if(reputationNameToId.contains(reputationItem.attribute(DatapackGeneralLoader::text_type)))
                                                    {
                                                        ReputationRewards reputationRewards;
                                                        reputationRewards.point=reputationItem.attribute(DatapackGeneralLoader::text_point).toInt(&ok);
                                                        if(ok)
                                                        {
                                                            reputationRewards.reputationId=reputationNameToId.value(reputationItem.attribute(DatapackGeneralLoader::text_type));
                                                            industryLink->rewards.reputation << reputationRewards;
                                                        }
                                                    }
                                                }
                                                else
                                                    qDebug() << QStringLiteral("Unable to open the industries link file: %1, have not the id, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                            }
                                            else
                                                qDebug() << QStringLiteral("Unable to open the industries link file: %1, is not a element, child.tagName(): %2 (at line: %3)").arg(file).arg(reputationItem.tagName()).arg(reputationItem.lineNumber());
                                            reputationItem = reputationItem.nextSiblingElement(DatapackGeneralLoader::text_reputation);
                                        }
                                    }
                                }
                            }
                        }
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
        linkItem = linkItem.nextSiblingElement(DatapackGeneralLoader::text_link);
    }
    return industriesLink;
}

ItemFull DatapackGeneralLoader::loadItems(const QString &folder,const QHash<quint8,Buff> &monsterBuffs)
{
    #ifdef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    (void)monsterBuffs;
    #endif
    ItemFull items;
    QDir dir(folder);
    const QStringList &fileList=FacilityLibGeneral::listFolder(dir.absolutePath()+DatapackGeneralLoader::text_slash);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).endsWith(DatapackGeneralLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        const QString &file=folder+fileList.at(file_index);
        if(!QFileInfo(file).isFile())
        {
            file_index++;
            continue;
        }
        QDomDocument domDocument;
        //open and quick check the file
        if(!file.endsWith(DatapackGeneralLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
            #endif
            QFile itemsFile(file);
            if(!itemsFile.open(QIODevice::ReadOnly))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
                file_index++;
                continue;
            }
            const QByteArray &xmlContent=itemsFile.readAll();
            itemsFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        const QDomElement &root = domDocument.documentElement();
        if(root.tagName()!=DatapackGeneralLoader::text_items)
        {
            //qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file);
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        QDomElement item = root.firstChildElement(DatapackGeneralLoader::text_item);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(DatapackGeneralLoader::text_id))
                {
                    const quint32 &id=item.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                    if(ok)
                    {
                        if(!items.item.contains(id))
                        {
                            //load the price
                            {
                                if(item.hasAttribute(DatapackGeneralLoader::text_price))
                                {
                                    bool ok;
                                    items.item[id].price=item.attribute(DatapackGeneralLoader::text_price).toUInt(&ok);
                                    if(!ok)
                                    {
                                        qDebug() << QStringLiteral("price is not a number: child.tagName(): %1 (at line: %2)").arg(item.tagName()).arg(item.lineNumber());
                                        items.item[id].price=0;
                                    }
                                }
                                else
                                {
                                    /*if(!item.hasAttribute(DatapackGeneralLoader::text_quest) || item.attribute(DatapackGeneralLoader::text_quest)!=DatapackGeneralLoader::text_yes)
                                        qDebug() << QStringLiteral("For parse item: Price not found, default to 0 (not sellable): child.tagName(): %1 (%2 at line: %3)").arg(item.tagName()).arg(file).arg(item.lineNumber());*/
                                    items.item[id].price=0;
                                }
                            }
                            //load the consumeAtUse
                            {
                                if(item.hasAttribute(DatapackGeneralLoader::text_consumeAtUse))
                                {
                                    if(item.attribute(DatapackGeneralLoader::text_consumeAtUse)==DatapackGeneralLoader::text_false)
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
                                QDomElement trapItem = item.firstChildElement(DatapackGeneralLoader::text_trap);
                                if(!trapItem.isNull())
                                {
                                    if(trapItem.isElement())
                                    {
                                        Trap trap;
                                        trap.bonus_rate=1.0;
                                        if(trapItem.hasAttribute(DatapackGeneralLoader::text_bonus_rate))
                                        {
                                            float bonus_rate=trapItem.attribute(DatapackGeneralLoader::text_bonus_rate).toFloat(&ok);
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
                                QDomElement repelItem = item.firstChildElement(DatapackGeneralLoader::text_repel);
                                if(!repelItem.isNull())
                                {
                                    if(repelItem.isElement())
                                    {
                                        if(repelItem.hasAttribute(DatapackGeneralLoader::text_step))
                                        {
                                            const quint32 &step=repelItem.attribute(DatapackGeneralLoader::text_step).toUInt(&ok);
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
                                            qDebug() << QStringLiteral("Unable to open the file: %1, repel have not the attribute step: child.tagName(): %2 (at line: %3)").arg(file).arg(repelItem.tagName()).arg(repelItem.lineNumber());
                                    }
                                }
                            }
                            //load the monster effect
                            if(!haveAnEffect)
                            {
                                {
                                    QDomElement hpItem = item.firstChildElement(DatapackGeneralLoader::text_hp);
                                    while(!hpItem.isNull())
                                    {
                                        if(hpItem.isElement())
                                        {
                                            if(hpItem.hasAttribute(DatapackGeneralLoader::text_add))
                                            {
                                                if(hpItem.attribute(DatapackGeneralLoader::text_add)==DatapackGeneralLoader::text_all)
                                                {
                                                    MonsterItemEffect monsterItemEffect;
                                                    monsterItemEffect.type=MonsterItemEffectType_AddHp;
                                                    monsterItemEffect.value=-1;
                                                    items.monsterItemEffect.insert(id,monsterItemEffect);
                                                }
                                                else
                                                {
                                                    if(!hpItem.attribute(DatapackGeneralLoader::text_add).contains(DatapackGeneralLoader::text_percent))//todo this part
                                                    {
                                                        const qint32 &add=hpItem.attribute(DatapackGeneralLoader::text_add).toUInt(&ok);
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
                                                                qDebug() << QStringLiteral("Unable to open the file: %1, add is not greater than 0: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                                        }
                                                        else
                                                            qDebug() << QStringLiteral("Unable to open the file: %1, add is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                                    }
                                                }
                                            }
                                            else
                                                qDebug() << QStringLiteral("Unable to open the file: %1, hp have not the attribute add: child.tagName(): %2 (at line: %3)").arg(file).arg(hpItem.tagName()).arg(hpItem.lineNumber());
                                        }
                                        hpItem = hpItem.nextSiblingElement(DatapackGeneralLoader::text_hp);
                                    }
                                }
                                #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                                {
                                    QDomElement buffItem = item.firstChildElement(DatapackGeneralLoader::text_buff);
                                    while(!buffItem.isNull())
                                    {
                                        if(buffItem.isElement())
                                        {
                                            if(buffItem.hasAttribute(DatapackGeneralLoader::text_remove))
                                            {
                                                if(buffItem.attribute(DatapackGeneralLoader::text_remove)==DatapackGeneralLoader::text_all)
                                                {
                                                    MonsterItemEffect monsterItemEffect;
                                                    monsterItemEffect.type=MonsterItemEffectType_RemoveBuff;
                                                    monsterItemEffect.value=-1;
                                                    items.monsterItemEffect.insert(id,monsterItemEffect);
                                                }
                                                else
                                                {
                                                    const qint32 &remove=buffItem.attribute(DatapackGeneralLoader::text_remove).toUInt(&ok);
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
                                            /// \todo
                                             /* else
                                                qDebug() << QStringLiteral("Unable to open the file: %1, buff have not the attribute know attribute like remove: child.tagName(): %2 (at line: %3)").arg(file).arg(buffItem.tagName()).arg(buffItem.lineNumber());*/
                                        }
                                        buffItem = buffItem.nextSiblingElement(DatapackGeneralLoader::text_buff);
                                    }
                                }
                                #endif
                                if(items.monsterItemEffect.contains(id))
                                    haveAnEffect=true;
                            }
                            //load the monster offline effect
                            if(!haveAnEffect)
                            {
                                QDomElement levelItem = item.firstChildElement(DatapackGeneralLoader::text_level);
                                while(!levelItem.isNull())
                                {
                                    if(levelItem.isElement())
                                    {
                                        if(levelItem.hasAttribute(DatapackGeneralLoader::text_up))
                                        {
                                            const quint32 &levelUp=levelItem.attribute(DatapackGeneralLoader::text_up).toUInt(&ok);
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
                                    levelItem = levelItem.nextSiblingElement(DatapackGeneralLoader::text_level);
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
            item = item.nextSiblingElement(DatapackGeneralLoader::text_item);
        }
        file_index++;
    }
    return items;
}
#endif

QList<QString> DatapackGeneralLoader::loadSkins(const QString &folder)
{
    return FacilityLibGeneral::skinIdList(folder);
}

QPair<QList<QDomElement>, QList<Profile> > DatapackGeneralLoader::loadProfileList(const QString &datapackPath, const QString &file,
                                                                                  #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                                                  const QHash<quint16, Item> &items,
                                                                                  #endif // CATCHCHALLENGER_CLASS_MASTER
                                                                                  const QHash<quint16,Monster> &monsters,const QList<Reputation> &reputations)
{
    QSet<QString> idDuplicate;
    QHash<QString,int> reputationNameToId;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    QStringList defaultforcedskinList;
    QHash<QString,quint8> skinNameToId;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.skins.size())
        {
            skinNameToId[CommonDatapack::commonDatapack.skins.at(index)]=index;
            defaultforcedskinList << CommonDatapack::commonDatapack.skins.at(index);
            index++;
        }
    }
    QPair<QList<QDomElement>, QList<Profile> > returnVar;
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file to have new profile: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return returnVar;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_list)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return returnVar;
    }

    //load the content
    bool ok;
    QDomElement startItem = root.firstChildElement(DatapackGeneralLoader::text_start);
    while(!startItem.isNull())
    {
        if(startItem.isElement())
        {
            Profile profile;

            /* on server part
            const QDomElement &map = startItem.firstChildElement(DatapackGeneralLoader::text_map);
            if(!map.isNull() && map.isElement() && map.hasAttribute(DatapackGeneralLoader::text_file) && map.hasAttribute(DatapackGeneralLoader::text_x) && map.hasAttribute(DatapackGeneralLoader::text_y))
            {
                profile.map=map.attribute(DatapackGeneralLoader::text_file);
                if(!profile.map.endsWith(DatapackGeneralLoader::text_dottmx))
                    profile.map+=DatapackGeneralLoader::text_dottmx;
                if(!QFile::exists(datapackPath+QLatin1String(DATAPACK_BASE_PATH_MAP)+profile.map))
                {
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, map don't exists %2: child.tagName(): %3 (at line: %4)").arg(file).arg(profile.map).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                    continue;
                }
                profile.x=map.attribute(DatapackGeneralLoader::text_x).toUShort(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, map x is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                    continue;
                }
                profile.y=map.attribute(DatapackGeneralLoader::text_y).toUShort(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, map y is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                    continue;
                }
            }
            else
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, no correct map configuration: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                continue;
            }
            */

            if(startItem.hasAttribute(DatapackGeneralLoader::text_id))
                profile.id=startItem.attribute(DatapackGeneralLoader::text_id);

            if(idDuplicate.contains(profile.id))
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, id duplicate: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                continue;
            }

            if(!profile.id.isEmpty() && !idDuplicate.contains(profile.id))
            {
                const QDomElement &forcedskin = startItem.firstChildElement(DatapackGeneralLoader::text_forcedskin);

                QStringList forcedskinList;
                if(!forcedskin.isNull() && forcedskin.isElement() && forcedskin.hasAttribute(DatapackGeneralLoader::text_value))
                    forcedskinList=forcedskin.attribute(DatapackGeneralLoader::text_value).split(DatapackGeneralLoader::text_dotcomma);
                else
                    forcedskinList=defaultforcedskinList;
                {
                    int index=0;
                    while(index<forcedskinList.size())
                    {
                        if(skinNameToId.contains(forcedskinList.at(index)))
                            profile.forcedskin << skinNameToId.value(forcedskinList.at(index));
                        else
                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, skin %4 don't exists: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(forcedskinList.at(index)));
                        index++;
                    }
                }
                int index=0;
                while(index<profile.forcedskin.size())
                {
                    if(!QFile::exists(datapackPath+QLatin1String(DATAPACK_BASE_PATH_SKIN)+CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index))))
                    {
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, skin %4 don't exists into: %5: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(profile.forcedskin.at(index)).arg(datapackPath+QLatin1String(DATAPACK_BASE_PATH_SKIN)+CommonDatapack::commonDatapack.skins.at(profile.forcedskin.at(index))));
                        profile.forcedskin.removeAt(index);
                    }
                    else
                        index++;
                }

                profile.cash=0;
                const QDomElement &cash = startItem.firstChildElement(DatapackGeneralLoader::text_cash);
                if(!cash.isNull() && cash.isElement() && cash.hasAttribute(DatapackGeneralLoader::text_value))
                {
                    profile.cash=cash.attribute(DatapackGeneralLoader::text_value).toULongLong(&ok);
                    if(!ok)
                    {
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, cash is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        profile.cash=0;
                    }
                }
                QDomElement monstersElement = startItem.firstChildElement(DatapackGeneralLoader::text_monster);
                while(!monstersElement.isNull())
                {
                    Profile::Monster monster;
                    if(monstersElement.isElement() && monstersElement.hasAttribute(DatapackGeneralLoader::text_id) && monstersElement.hasAttribute(DatapackGeneralLoader::text_level) && monstersElement.hasAttribute(DatapackGeneralLoader::text_captured_with))
                    {
                        monster.id=monstersElement.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                        if(!ok)
                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, monster id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        if(ok)
                        {
                            monster.level=monstersElement.attribute(DatapackGeneralLoader::text_level).toUShort(&ok);
                            if(!ok)
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, monster level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        }
                        if(ok)
                        {
                            if(monster.level==0 || monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, monster level is not into the range: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        }
                        if(ok)
                        {
                            monster.captured_with=monstersElement.attribute(DatapackGeneralLoader::text_captured_with).toUInt(&ok);
                            if(!ok)
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, captured_with is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        }
                        if(ok)
                        {
                            if(!monsters.contains(monster.id))
                            {
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, starter don't found the monster %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(monster.id));
                                ok=false;
                            }
                        }
                        if(ok)
                        {
                            #ifndef CATCHCHALLENGER_CLASS_MASTER
                            if(!items.contains(monster.captured_with))
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, starter don't found the monster capture item %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(monster.id));
                            #endif // CATCHCHALLENGER_CLASS_MASTER
                        }
                        if(ok)
                            profile.monsters << monster;
                    }
                    monstersElement = monstersElement.nextSiblingElement(DatapackGeneralLoader::text_monster);
                }
                if(profile.monsters.empty())
                {
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, not monster to load: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                    continue;
                }
                QDomElement reputationElement = startItem.firstChildElement(DatapackGeneralLoader::text_reputation);
                while(!reputationElement.isNull())
                {
                    Profile::Reputation reputationTemp;
                    if(reputationElement.isElement() && reputationElement.hasAttribute(DatapackGeneralLoader::text_type) && reputationElement.hasAttribute(DatapackGeneralLoader::text_level))
                    {
                        reputationTemp.level=reputationElement.attribute(DatapackGeneralLoader::text_level).toShort(&ok);
                        if(!ok)
                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, reputation level is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        if(ok)
                        {
                            if(!reputationNameToId.contains(reputationElement.attribute(DatapackGeneralLoader::text_type)))
                            {
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, reputation type not found %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(reputationElement.attribute(DatapackGeneralLoader::text_type)));
                                ok=false;
                            }
                            if(ok)
                            {
                                reputationTemp.reputationId=reputationNameToId.value(reputationElement.attribute(DatapackGeneralLoader::text_type));
                                if(reputationTemp.level==0)
                                {
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, reputation level is useless if level 0: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                    ok=false;
                                }
                                else if(reputationTemp.level<0)
                                {
                                    if((-reputationTemp.level)>reputations.value(reputationTemp.reputationId).reputation_negative.size())
                                    {
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, reputation level is lower than minimal level for %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(reputationElement.attribute(DatapackGeneralLoader::text_type)));
                                        ok=false;
                                    }
                                }
                                else// if(reputationTemp.level>0)
                                {
                                    if((reputationTemp.level)>=reputations.value(reputationTemp.reputationId).reputation_positive.size())
                                    {
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, reputation level is higther than maximal level for %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(reputationElement.attribute(DatapackGeneralLoader::text_type)));
                                        ok=false;
                                    }
                                }
                            }
                            if(ok)
                            {
                                reputationTemp.point=0;
                                if(reputationElement.hasAttribute(DatapackGeneralLoader::text_point))
                                {
                                    reputationTemp.point=reputationElement.attribute(DatapackGeneralLoader::text_point).toInt(&ok);
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, reputation point is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                    if(ok)
                                    {
                                        if((reputationTemp.point>0 && reputationTemp.level<0) || (reputationTemp.point<0 && reputationTemp.level>=0))
                                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, reputation point is not negative/positive like the level: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                    }
                                }
                            }
                        }
                        if(ok)
                            profile.reputation << reputationTemp;
                    }
                    reputationElement = reputationElement.nextSiblingElement(DatapackGeneralLoader::text_reputation);
                }
                QDomElement itemElement = startItem.firstChildElement(DatapackGeneralLoader::text_item);
                while(!itemElement.isNull())
                {
                    Profile::Item itemTemp;
                    if(itemElement.isElement() && itemElement.hasAttribute(DatapackGeneralLoader::text_id))
                    {
                        itemTemp.id=itemElement.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                        if(!ok)
                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, item id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        if(ok)
                        {
                            itemTemp.quantity=0;
                            if(itemElement.hasAttribute(DatapackGeneralLoader::text_quantity))
                            {
                                itemTemp.quantity=itemElement.attribute(DatapackGeneralLoader::text_quantity).toUInt(&ok);
                                if(!ok)
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, item quantity is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                if(ok)
                                {
                                    if(itemTemp.quantity==0)
                                    {
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, item quantity is null: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                                        ok=false;
                                    }
                                }
                                #ifndef CATCHCHALLENGER_CLASS_MASTER
                                if(ok)
                                {
                                    if(!items.contains(itemTemp.id))
                                    {
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, item not found as know item %4: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(itemTemp.id));
                                        ok=false;
                                    }
                                }
                                #endif // CATCHCHALLENGER_CLASS_MASTER
                            }
                        }
                        if(ok)
                            profile.items << itemTemp;
                    }
                    itemElement = itemElement.nextSiblingElement(DatapackGeneralLoader::text_item);
                }
                idDuplicate << profile.id;
                returnVar.second << profile;
                returnVar.first << startItem;
            }
        }
        else
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
        startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
    }
    return returnVar;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
QList<MonstersCollision> DatapackGeneralLoader::loadMonstersCollision(const QString &file, const QHash<quint16, Item> &items,const QList<Event> &events)
{
    QHash<QString,quint8> eventStringToId;
    QHash<QString,QHash<QString,quint8> > eventValueStringToId;
    {
        int index=0;
        int sub_index;
        while(index<events.size())
        {
            const Event &event=events.at(index);
            eventStringToId[event.name]=index;
            sub_index=0;
            while(sub_index<event.values.size())
            {
                eventValueStringToId[event.name][event.values.at(sub_index)]=sub_index;
                sub_index++;
            }
            index++;
        }
    }
    QList<MonstersCollision> returnVar;
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file to monster collision: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return returnVar;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_layers)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return returnVar;
    }

    //load the content
    bool ok;
    QDomElement monstersCollisionItem = root.firstChildElement(DatapackGeneralLoader::text_monstersCollision);
    while(!monstersCollisionItem.isNull())
    {
        if(monstersCollisionItem.isElement())
        {
            if(!monstersCollisionItem.hasAttribute(DatapackGeneralLoader::text_type))
                qDebug() << (QStringLiteral("Have not the attribute type, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
            else if(!monstersCollisionItem.hasAttribute(DatapackGeneralLoader::text_monsterType))
                qDebug() << (QStringLiteral("Have not the attribute monsterType, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
            else
            {
                ok=true;
                MonstersCollision monstersCollision;
                if(monstersCollisionItem.attribute(DatapackGeneralLoader::text_type)==DatapackGeneralLoader::text_walkOn)
                    monstersCollision.type=MonstersCollisionType_WalkOn;
                else if(monstersCollisionItem.attribute(DatapackGeneralLoader::text_type)==DatapackGeneralLoader::text_actionOn)
                    monstersCollision.type=MonstersCollisionType_ActionOn;
                else
                {
                    ok=false;
                    qDebug() << (QStringLiteral("type is not walkOn or actionOn, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute(DatapackGeneralLoader::text_layer))
                        monstersCollision.layer=monstersCollisionItem.attribute(DatapackGeneralLoader::text_layer);
                }
                if(ok)
                {
                    if(monstersCollision.layer.isEmpty() && monstersCollision.type!=MonstersCollisionType_WalkOn)//need specific layer name to do that's
                    {
                        ok=false;
                        qDebug() << (QStringLiteral("To have blocking layer by item, have specific layer name, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                    }
                    else
                    {
                        monstersCollision.item=0;
                        if(monstersCollisionItem.hasAttribute(DatapackGeneralLoader::text_item))
                        {
                            monstersCollision.item=monstersCollisionItem.attribute(DatapackGeneralLoader::text_item).toUInt(&ok);
                            if(!ok)
                                qDebug() << (QStringLiteral("item attribute is not a number, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                            else if(!items.contains(monstersCollision.item))
                            {
                                ok=false;
                                qDebug() << (QStringLiteral("item is not into item list, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                            }
                        }
                    }
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute(DatapackGeneralLoader::text_tile))
                        monstersCollision.tile=monstersCollisionItem.attribute(DatapackGeneralLoader::text_tile);
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute(DatapackGeneralLoader::text_background))
                        monstersCollision.background=monstersCollisionItem.attribute(DatapackGeneralLoader::text_background);
                }
                if(ok)
                {
                    if(monstersCollisionItem.hasAttribute(DatapackGeneralLoader::text_monsterType))
                    {
                        monstersCollision.defautMonsterTypeList=monstersCollisionItem.attribute(DatapackGeneralLoader::text_monsterType).split(DatapackGeneralLoader::text_dotcomma,QString::SkipEmptyParts);
                        monstersCollision.defautMonsterTypeList.removeDuplicates();
                        monstersCollision.monsterTypeList=monstersCollision.defautMonsterTypeList;
                        //load the condition
                        QDomElement eventItem = monstersCollisionItem.firstChildElement(DatapackGeneralLoader::text_event);
                        while(!eventItem.isNull())
                        {
                            if(eventItem.isElement())
                            {
                                if(eventItem.hasAttribute(DatapackGeneralLoader::text_id) && eventItem.hasAttribute(DatapackGeneralLoader::text_value) && eventItem.hasAttribute(DatapackGeneralLoader::text_monsterType))
                                {
                                    if(eventStringToId.contains(eventItem.attribute(DatapackGeneralLoader::text_id)))
                                    {
                                        if(eventValueStringToId.value(eventItem.attribute(DatapackGeneralLoader::text_id)).contains(eventItem.attribute(DatapackGeneralLoader::text_value)))
                                        {
                                            MonstersCollision::MonstersCollisionEvent event;
                                            event.event=eventStringToId.value(eventItem.attribute(DatapackGeneralLoader::text_id));
                                            event.event_value=eventValueStringToId.value(eventItem.attribute(DatapackGeneralLoader::text_id)).value(eventItem.attribute(DatapackGeneralLoader::text_value));
                                            event.monsterTypeList=eventItem.attribute(DatapackGeneralLoader::text_monsterType).split(DatapackGeneralLoader::text_dotcomma,QString::SkipEmptyParts);
                                            if(!event.monsterTypeList.isEmpty())
                                            {
                                                monstersCollision.events << event;
                                                int index=0;
                                                while(index<event.monsterTypeList.size())
                                                {
                                                    if(!monstersCollision.monsterTypeList.contains(event.monsterTypeList.at(index)))
                                                        monstersCollision.monsterTypeList << event.monsterTypeList.at(index);
                                                    index++;
                                                }
                                            }
                                            else
                                                qDebug() << (QStringLiteral("monsterType is empty, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
                                        }
                                        else
                                            qDebug() << (QStringLiteral("event value not found, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
                                    }
                                    else
                                        qDebug() << (QStringLiteral("event not found, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
                                }
                                else
                                    qDebug() << (QStringLiteral("event have missing attribute, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
                            }
                            eventItem = eventItem.nextSiblingElement(DatapackGeneralLoader::text_event);
                        }
                    }
                }
                if(ok)
                {
                    int index=0;
                    while(index<returnVar.size())
                    {
                        if(returnVar.at(index).type==monstersCollision.type && returnVar.at(index).layer==monstersCollision.layer && returnVar.at(index).item==monstersCollision.item)
                        {
                            qDebug() << (QStringLiteral("similar monstersCollision previously found, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                            ok=false;
                            break;
                        }
                        if(monstersCollision.type==MonstersCollisionType_WalkOn && returnVar.at(index).layer==monstersCollision.layer)
                        {
                            qDebug() << (QStringLiteral("You can't have different item for same layer in walkOn mode, into: %1 at line %2").arg(file).arg(monstersCollisionItem.lineNumber()));
                            ok=false;
                            break;
                        }
                        index++;
                    }
                }
                if(ok && !monstersCollision.monsterTypeList.isEmpty())
                {
                    if(monstersCollision.type==MonstersCollisionType_WalkOn && monstersCollision.layer.isEmpty() && monstersCollision.item==0)
                    {
                        if(returnVar.isEmpty())
                            returnVar << monstersCollision;
                        else
                        {
                            returnVar << returnVar.last();
                            returnVar.first()=monstersCollision;
                        }
                    }
                    else
                        returnVar << monstersCollision;
                }
            }
        }
        monstersCollisionItem = monstersCollisionItem.nextSiblingElement(DatapackGeneralLoader::text_monstersCollision);
    }
    return returnVar;
}

LayersOptions DatapackGeneralLoader::loadLayersOptions(const QString &file)
{
    LayersOptions returnVar;
    returnVar.zoom=2;
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file to load layer options: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return returnVar;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_layers)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return returnVar;
    }
    if(root.hasAttribute(QLatin1Literal("zoom")))
    {
        bool ok;
        returnVar.zoom=root.attribute(QLatin1Literal("zoom")).toUShort(&ok);
        if(!ok)
        {
            returnVar.zoom=2;
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, zoom is not a number").arg(file));
        }
        else
        {
            if(returnVar.zoom<1 || returnVar.zoom>4)
            {
                returnVar.zoom=2;
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, zoom out of range 1-4").arg(file));
            }
        }
    }

    return returnVar;
}

QList<Event> DatapackGeneralLoader::loadEvents(const QString &file)
{
    QList<Event> returnVar;

    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file to load event: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return returnVar;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return returnVar;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_events)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return returnVar;
    }

    //load the content
    QDomElement eventItem = root.firstChildElement(DatapackGeneralLoader::text_event);
    while(!eventItem.isNull())
    {
        if(eventItem.isElement())
        {
            if(!eventItem.hasAttribute(DatapackGeneralLoader::text_id))
                qDebug() << (QStringLiteral("Have not the attribute id, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
            else if(eventItem.attribute(DatapackGeneralLoader::text_id).isEmpty())
                qDebug() << (QStringLiteral("Have id empty, into: %1 at line %2").arg(file).arg(eventItem.lineNumber()));
            else
            {
                Event event;
                event.name=eventItem.attribute(DatapackGeneralLoader::text_id);
                QDomElement valueItem = eventItem.firstChildElement(DatapackGeneralLoader::text_value);
                while(!valueItem.isNull())
                {
                    if(valueItem.isElement())
                        event.values << valueItem.text();
                    valueItem = valueItem.nextSiblingElement(DatapackGeneralLoader::text_value);
                }
                if(!event.values.isEmpty())
                    returnVar << event;
            }
        }
        eventItem = eventItem.nextSiblingElement(DatapackGeneralLoader::text_event);
    }
    return returnVar;
}

QHash<quint32,Shop> DatapackGeneralLoader::preload_shop(const QString &file, const QHash<quint16, Item> &items)
{
    QHash<quint32,Shop> shops;

    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile shopFile(file);
        QByteArray xmlContent;
        if(!shopFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the shops file: %1, error: %2").arg(file).arg(shopFile.errorString()));
            return shops;
        }
        xmlContent=shopFile.readAll();
        shopFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the shops file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return shops;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_shops)
    {
        qDebug() << (QStringLiteral("Unable to open the shops file: %1, \"shops\" root balise not found for the xml file").arg(file));
        return shops;
    }

    //load the content
    bool ok;
    QDomElement shopItem = root.firstChildElement(DatapackGeneralLoader::text_shop);
    while(!shopItem.isNull())
    {
        if(shopItem.isElement())
        {
            if(shopItem.hasAttribute(DatapackGeneralLoader::text_id))
            {
                quint32 id=shopItem.attribute(DatapackGeneralLoader::text_id).toUInt(&ok);
                if(ok)
                {
                    if(!shops.contains(id))
                    {
                        Shop shop;
                        QDomElement product = shopItem.firstChildElement(DatapackGeneralLoader::text_product);
                        while(!product.isNull())
                        {
                            if(product.isElement())
                            {
                                if(product.hasAttribute(DatapackGeneralLoader::text_itemId))
                                {
                                    quint32 itemId=product.attribute(DatapackGeneralLoader::text_itemId).toUInt(&ok);
                                    if(!ok)
                                        qDebug() << (QStringLiteral("preload_shop() product attribute itemId is not a number for shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                    else
                                    {
                                        if(!items.contains(itemId))
                                            qDebug() << (QStringLiteral("preload_shop() product itemId in not into items list for shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                        else
                                        {
                                            quint32 price=items.value(itemId).price;
                                            if(product.hasAttribute(DatapackGeneralLoader::text_overridePrice))
                                            {
                                                price=product.attribute(DatapackGeneralLoader::text_overridePrice).toUInt(&ok);
                                                if(!ok)
                                                    price=items.value(itemId).price;
                                            }
                                            if(price==0)
                                                qDebug() << (QStringLiteral("preload_shop() item can't be into the shop with price == 0' for shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                                            else
                                            {
                                                shop.prices << price;
                                                shop.items << itemId;
                                            }
                                        }
                                    }
                                }
                                else
                                    qDebug() << (QStringLiteral("preload_shop() material have not attribute itemId for shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                            }
                            else
                                qDebug() << (QStringLiteral("preload_shop() material is not an element for shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                            product = product.nextSiblingElement(DatapackGeneralLoader::text_product);
                        }
                        shops[id]=shop;
                    }
                    else
                        qDebug() << (QStringLiteral("Unable to open the shops file: %1, child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
                }
                else
                    qDebug() << (QStringLiteral("Unable to open the shops file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
            }
            else
                qDebug() << (QStringLiteral("Unable to open the shops file: %1, have not the shops id: child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
        }
        else
            qDebug() << (QStringLiteral("Unable to open the shops file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(shopItem.tagName()).arg(shopItem.lineNumber()));
        shopItem = shopItem.nextSiblingElement(DatapackGeneralLoader::text_shop);
    }
    return shops;
}
#endif

QList<ServerProfile> DatapackGeneralLoader::loadServerProfileList(const QString &datapackPath, const QString &mainDatapackCode, const QString &file,const QList<Profile> &profileCommon)
{
    QList<ServerProfile> serverProfile=loadServerProfileListInternal(datapackPath,mainDatapackCode,file);
    //index of base profile
    QSet<QString> profileId,serverProfileId;
    {
        int index=0;
        while(index<profileCommon.size())
        {
            //already deduplicated at loading
            profileId << profileCommon.at(index).id;
            index++;
        }
    }
    //drop serverProfileList
    {
        int index=0;
        while(index<serverProfile.size())
        {
            if(profileId.contains(serverProfile.at(index).id))
            {
                serverProfileId << serverProfile.at(index).id;
                index++;
            }
            else
            {
                qDebug() << (QStringLiteral("Profile xml file: %1, found id \"%2\" but not found in common, drop it").arg(file).arg(serverProfile.at(index).id));
                serverProfile.removeAt(index);
            }
        }
    }
    //add serverProfileList
    {
        int index=0;
        while(index<profileCommon.size())
        {
            if(!serverProfileId.contains(profileCommon.at(index).id))
            {
                qDebug() << (QStringLiteral("Profile xml file: %1, found common id \"%2\" but not found in server, add it").arg(file).arg(profileCommon.at(index).id));
                ServerProfile serverProfileTemp;
                serverProfileTemp.id=profileCommon.at(index).id;
                serverProfileTemp.orientation=Orientation_bottom;
                serverProfileTemp.x=0;
                serverProfileTemp.y=0;
                serverProfile << serverProfileTemp;
            }
            index++;
        }
    }

    return serverProfile;
}

QList<ServerProfile> DatapackGeneralLoader::loadServerProfileListInternal(const QString &datapackPath, const QString &mainDatapackCode, const QString &file)
{
    QSet<QString> idDuplicate;
    QList<ServerProfile> serverProfileList;

    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        #endif
        QFile xmlFile(file);
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file to have new server profile internal: %1, error: %2").arg(file).arg(xmlFile.errorString()));
            return serverProfileList;
        }
        const QByteArray &xmlContent=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return serverProfileList;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackGeneralLoader::text_list)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return serverProfileList;
    }

    //load the content
    bool ok;
    QDomElement startItem = root.firstChildElement(DatapackGeneralLoader::text_start);
    while(!startItem.isNull())
    {
        if(startItem.isElement())
        {
            ServerProfile serverProfile;
            serverProfile.orientation=Orientation_bottom;

            const QDomElement &map = startItem.firstChildElement(DatapackGeneralLoader::text_map);
            if(!map.isNull() && map.isElement() && map.hasAttribute(DatapackGeneralLoader::text_file) && map.hasAttribute(DatapackGeneralLoader::text_x) && map.hasAttribute(DatapackGeneralLoader::text_y))
            {
                serverProfile.mapString=map.attribute(DatapackGeneralLoader::text_file);
                if(!serverProfile.mapString.endsWith(DatapackGeneralLoader::text_dottmx))
                    serverProfile.mapString+=DatapackGeneralLoader::text_dottmx;
                if(!QFile::exists(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPMAIN).arg(mainDatapackCode)+serverProfile.mapString))
                {
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, map don't exists %2: child.tagName(): %3 (at line: %4)").arg(file).arg(serverProfile.mapString).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                    continue;
                }
                serverProfile.x=map.attribute(DatapackGeneralLoader::text_x).toUShort(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, map x is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                    continue;
                }
                serverProfile.y=map.attribute(DatapackGeneralLoader::text_y).toUShort(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, map y is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                    continue;
                }
            }
            else
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, no correct map configuration: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                continue;
            }

            if(startItem.hasAttribute(DatapackGeneralLoader::text_id))
                serverProfile.id=startItem.attribute(DatapackGeneralLoader::text_id);

            if(idDuplicate.contains(serverProfile.id))
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, id duplicate: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
                continue;
            }

            if(!serverProfile.id.isEmpty() && !idDuplicate.contains(serverProfile.id))
            {
                idDuplicate << serverProfile.id;
                serverProfileList << serverProfile;
            }
        }
        else
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(startItem.tagName()).arg(startItem.lineNumber()));
        startItem = startItem.nextSiblingElement(DatapackGeneralLoader::text_start);
    }

    return serverProfileList;
}
