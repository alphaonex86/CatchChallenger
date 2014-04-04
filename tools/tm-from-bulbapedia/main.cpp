#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QDomElement>
#include <QDomDocument>
#include <QDebug>
#include <QImage>
#include <QPainter>

//item, skill, monster
#define DATAPACK_BASE_PATH_ITEM "items/"
#define DATAPACK_BASE_PATH_MONSTERS "monsters/"

QString text_list=QLatin1Literal("list");
QString text_reputation=QLatin1Literal("reputation");
QString text_type=QLatin1Literal("type");
QString text_name=QLatin1Literal("name");
QString text_en=QLatin1Literal("en");
QString text_lang=QLatin1Literal("lang");
QString text_level=QLatin1Literal("level");
QString text_point=QLatin1Literal("point");
QString text_text=QLatin1Literal("text");
QString text_id=QLatin1Literal("id");
QString text_image=QLatin1Literal("image");
QString text_description=QLatin1Literal("description");
QString text_item=QLatin1Literal("item");
QString text_slashdefinitiondotxml=QLatin1Literal("/definition.xml");
QString text_quest=QLatin1Literal("quest");
QString text_rewards=QLatin1Literal("rewards");
QString text_show=QLatin1Literal("show");
QString text_true=QLatin1Literal("true");
QString text_step=QLatin1Literal("step");
QString text_bot=QLatin1Literal("bot");
QString text_dotcomma=QLatin1Literal(";");
QString text_client_logic=QLatin1Literal("client_logic");
QString text_map=QLatin1Literal("map");
QString text_items=QLatin1Literal("items");
QString text_zone=QLatin1Literal("zone");
QString text_kind=QLatin1Literal("kind");
QString text_habitat=QLatin1Literal("habitat");
QString text_slash=QLatin1Literal("/");
QString text_types=QLatin1Literal("types");
QString text_buff=QLatin1Literal("buff");
QString text_skill=QLatin1Literal("skill");
QString text_fight=QLatin1Literal("buff");
QString text_fights=QLatin1Literal("skill");
QString text_start=QLatin1Literal("start");
QString text_win=QLatin1Literal("win");
QString text_multiplicator=QLatin1String("multiplicator");
QString text_number=QLatin1String("number");
QString text_to=QLatin1String("to");
QString text_dotcoma=QLatin1String(";");
QString text_monster=QLatin1String("monster");
QString text_egg_step=QLatin1String("egg_step");
QString text_xp_for_max_level=QLatin1String("xp_for_max_level");
QString text_xp_max=QLatin1String("xp_max");
QString text_hp=QLatin1String("hp");
QString text_attack=QLatin1String("attack");
QString text_defense=QLatin1String("defense");
QString text_special_attack=QLatin1String("special_attack");
QString text_special_defense=QLatin1String("special_defense");
QString text_speed=QLatin1String("speed");
QString text_give_sp=QLatin1String("give_sp");
QString text_give_xp=QLatin1String("give_xp");
QString text_catch_rate=QLatin1String("catch_rate");
QString text_type2=QLatin1String("type2");
QString text_pow=QLatin1String("pow");
QString text_ratio_gender=QLatin1String("ratio_gender");
QString text_percent=QLatin1String("%");
QString text_attack_list=QLatin1String("attack_list");
QString text_skill_level=QLatin1String("skill_level");
QString text_attack_level=QLatin1String("attack_level");
QString text_byitem=QLatin1String("byitem");
QString text_evolution=QLatin1String("evolution");
QString text_evolutions=QLatin1String("evolutions");
QString text_trade=QLatin1String("trade");
QString text_evolveTo=QLatin1String("evolveTo");
QString text_gain=QLatin1String("gain");
QString text_cash=QLatin1String("cash");
QString text_sp=QLatin1String("sp");
QString text_effect=QLatin1String("effect");
QString text_endurance=QLatin1String("endurance");
QString text_life=QLatin1String("life");
QString text_applyOn=QLatin1String("applyOn");
QString text_aloneEnemy=QLatin1String("aloneEnemy");
QString text_themself=QLatin1String("themself");
QString text_allEnemy=QLatin1String("allEnemy");
QString text_allAlly=QLatin1String("allAlly");
QString text_nobody=QLatin1String("nobody");
QString text_quantity=QLatin1String("quantity");
QString text_more=QLatin1String("+");
QString text_success=QLatin1String("success");
QString text_capture_bonus=QLatin1String("capture_bonus");
QString text_duration=QLatin1String("duration");
QString text_Always=QLatin1String("Always");
QString text_NumberOfTurn=QLatin1String("NumberOfTurn");
QString text_durationNumberOfTurn=QLatin1String("durationNumberOfTurn");
QString text_ThisFight=QLatin1String("ThisFight");
QString text_inFight=QLatin1String("inFight");
QString text_inWalk=QLatin1String("inWalk");
QString text_steps=QLatin1String("steps");

QString datapackPath;

struct Link
{
    QString tm;
    QString skill;
};
QHash<QString,QList<Link> > monsterToLink;

QHash<QString,QString> TMtoType;
QHash<QString,quint32> itemNameToId;
QHash<QString,quint32> skillNameToId;
QHash<QString,quint32> monsterNameToId;
QHash<quint32,QString> monsterIdToName;

bool loadHtmlTM(QString data,QString fulldata)
{
    if(!fulldata.contains(QRegularExpression("List of Pok.{1,2}mon by National Pok.{1,2}dex number")) && !data.contains(QRegularExpression("By TM/HM")))
        return false;

    if(!data.contains(QRegularExpression("<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"en\">([a-zA-Z0-9]+)( \\(Pokémon\\))?</h1>")) && !data.contains(QRegularExpression("By TM/HM")))
        return false;
    QString name=data;
    if(data.contains(QRegularExpression("<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"en\">([a-zA-Z0-9]+)( \\(Pokémon\\))?</h1>")))
        name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"en\">([a-zA-Z0-9]+)( \\(Pokémon\\))?</h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    else
        name.replace(QRegularExpression(".*<title>([a-zA-Z0-9]+) \\(Pok.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");

    fulldata.remove(QRegularExpression("^.*id=\"By_TM",QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption));
    fulldata.remove(QRegularExpression("id=\"By_breeding.*$",QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption));
    QStringList itemStringTempList=fulldata.split(QRegularExpression("</tr>"));

    const QString &preg=QStringLiteral(
               "<tr[^>]*>[\n\r\t ]*"
               "<td[^>]*>[\n\r\t ]*<a href[^>]*><img alt=\"Bag (TM|HM) ([a-zA-Z]+) Sprite.png\"[^>]*></a>[\n\r\t ]*"
               "</td>[\n\r\t ]*"
               "<td[^>]*>[\n\r\t ]*<a href=[^>]* title=\"((TM|HM)[^\"]*)\">(<span[^>]*>)?[^<]*(</span>)?</a>[\n\r\t ]*"
               "</td>[\n\r\t ]*"
               "<td[^>]*>[\n\r\t ]*<a href=[^>]*>(<span[^>]*>)?([^<]*)(</span>)?</a>[\n\r\t ]*"
 /*              "</td>[\n\r\t ]*"
               "<td[^>]*>[\n\r\t ]*<a href=[^>]*>(<span[^>]*>)?[^<]*(</span>)?</a>[\n\r\t ]*"
               "</td>[\n\r\t ]*"
               "<td[^>]*>[\n\r\t ]*(<a href=[^>]*)?>[\n\r\t ]*(<span[^>]*>)?[^<]*(</span>)?[\n\r\t ]*(</a>)?[\n\r\t ]*"
               "</td>[\n\r\t ]*"
               "<td[^>]*>[\n\r\t ]*(<span[^>]*>)?[^<]*(</span>)?[^<]*[\n\r\t ]*"
               "</td>[\n\r\t ]*"
               "<td[^>]*>[\n\r\t ]*(<span[^>]*>)?[^<]*(</span>)?[^<]*[\n\r\t ]*"
               "</td>[\n\r\t ]*"
               "<td[^>]*>[\n\r\t ]*[^<]*[\n\r\t ]*"
               "</td>[\n\r\t ]*" */
    );
    int index=0;
    while(index<itemStringTempList.size())
    {
        Link link;
        QString tempString=itemStringTempList.at(index);

        if(tempString.contains(QRegularExpression(preg,QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption)))
        {
            link.tm=tempString;
            link.tm.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\3");
            link.tm.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            link.tm.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            link.skill=tempString;
            link.skill.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\8");
            link.skill.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            link.skill.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            QString type=tempString;
            type.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\2");
            type.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            type.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            TMtoType[link.tm]=type.toLower();
            monsterToLink[name] << link;
        }
        /*else
            qDebug() << QStringLiteral("link ignored:") << tempString;*/

        index++;
    }
    return true;
}

void parseItemsExtra()
{
    //open and quick check the file
    QFile itemsFile(datapackPath+"items/items.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
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
                item.attribute("id").toULongLong(&ok);
                if(ok)
                {
                    //load the name
                    {
                        QDomElement name = item.firstChildElement("name");
                        while(!name.isNull())
                        {
                            if(name.isElement())
                            {
                                if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                {
                                    QString tempName=name.text();
                                    tempName.remove(QRegularExpression("[\r\n\t]+"));
                                    if(tempName.contains("TM") || tempName.contains("HM"))
                                        if(tempName.contains(QRegularExpression("^.*((TM|HM)[0-9]{2}) (.*)$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption)))
                                        {
                                            tempName.replace(QRegularExpression("^.*((TM|HM)[0-9]{2}) (.*)$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\3");
                                            //tempName=tempName.toLower();
                                            itemNameToId[tempName]=item.attribute("id").toUInt();
                                        }
                                    break;
                                }
                            }
                            name = name.nextSiblingElement("name");
                        }
                    }
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement("item");
    }

    qDebug() << QStringLiteral("%1 item(s) extra loaded").arg(itemNameToId.size());
}

void parseSkillsExtra()
{
    //open and quick check the file
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("skill.xml");
    QDomDocument domDocument;
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=text_list)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return;
    }

    const QString &language="en";
    bool found;
    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(text_skill);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(text_id))
            {
                quint32 id=item.attribute(text_id).toUInt(&ok);
                if(!ok)
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                    qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                    #endif
                    found=false;
                    QString nameText;
                    QDomElement name = item.firstChildElement(text_name);
                    if(!language.isEmpty() && language!=text_en)
                        while(!name.isNull())
                        {
                            if(name.isElement())
                            {
                                if(name.hasAttribute(text_lang) && name.attribute(text_lang)==language)
                                {
                                    nameText=name.text();
                                    found=true;
                                    break;
                                }
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                            name = name.nextSiblingElement(text_name);
                        }
                    if(!found)
                    {
                        name = item.firstChildElement(text_name);
                        while(!name.isNull())
                        {
                            if(name.isElement())
                            {
                                if(!name.hasAttribute(text_lang) || name.attribute(text_lang)==text_en)
                                {
                                    nameText=name.text();
                                    break;
                                }
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                            name = name.nextSiblingElement(text_name);
                        }
                    }
                    if(!nameText.isEmpty())
                        skillNameToId[nameText]=id;
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the skill id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(text_skill);
    }

    qDebug() << QStringLiteral("%1 skill(s) extra loaded").arg(skillNameToId.size());
}

void parseMonstersExtra()
{
    QString STATIC_DATAPACK_BASE_PATH_MONSTERS=QLatin1Literal(DATAPACK_BASE_PATH_MONSTERS);
    const QString &file=datapackPath+STATIC_DATAPACK_BASE_PATH_MONSTERS+QStringLiteral("monster.xml");
    QDomDocument domDocument;
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        qDebug() << (QStringLiteral("Unable to open the xml monster extra file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=text_list)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return;
    }

    const QString &language="en";
    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(text_monster);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(text_id))
            {
                quint32 id=item.attribute(text_id).toUInt(&ok);
                if(!ok)
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    QString nameText;
                    #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                    qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                    #endif
                    {
                        bool found=false;
                        QDomElement name = item.firstChildElement(text_name);
                        if(!language.isEmpty() && language!=text_en)
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute(text_lang) && name.attribute(text_lang)==language)
                                    {
                                        nameText=name.text();
                                        found=true;
                                        break;
                                    }
                                }
                                else
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                name = name.nextSiblingElement(text_name);
                            }
                        if(!found)
                        {
                            name = item.firstChildElement(text_name);
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute(text_lang) || name.attribute(text_lang)==text_en)
                                    {
                                        nameText=name.text();
                                        break;
                                    }
                                }
                                else
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                name = name.nextSiblingElement(text_name);
                            }
                        }
                    }
                    if(!nameText.isEmpty())
                    {
                        monsterNameToId[nameText]=id;
                        monsterIdToName[id]=nameText;
                    }
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the monster id at monster extra: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(text_monster);
    }

    qDebug() << QStringLiteral("%1 monster(s) extra loaded").arg(monsterNameToId.size());
}

void updateMonsterXml(const QString &file)
{
    QDomDocument domDocument;
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        qDebug() << (QStringLiteral("Unable to open the xml monster file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=text_list)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(file));
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(text_monster);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            bool attributeIsOk=true;
            if(!item.hasAttribute(text_id))
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the monster attribute \"id\": child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                attributeIsOk=false;
            }
            if(attributeIsOk)
            {
                quint32 id=item.attribute(text_id).toUInt(&ok);
                if(!ok)
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                    qDebug() << (QStringLiteral("monster loading: %1").arg(id));
                    #endif
                    if(ok)
                    {
                        {
                            QDomElement attack_list = item.firstChildElement(text_attack_list);
                            if(!attack_list.isNull())
                            {
                                if(attack_list.isElement())
                                {
                                    //QSet<quint32> learnByItem;
                                    QDomElement attack = attack_list.firstChildElement(text_attack);
                                    while(!attack.isNull())
                                    {
                                        if(attack.isElement())
                                        {
                                            if(attack.hasAttribute(text_skill) || attack.hasAttribute(text_id))
                                            {
                                                if(attack.hasAttribute(text_byitem))
                                                {
                                                    attack.parentNode().removeChild(attack);
                                                    /*quint32 itemId;
                                                    if(ok)
                                                    {
                                                        itemId=attack.attribute(text_byitem).toUShort(&ok);
                                                        if(!ok)
                                                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, item to learn is not a number %4: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()).arg(attack.attribute(text_byitem)));
                                                    }
                                                    if(ok)
                                                    {
                                                        if(learnByItem.contains(itemId))
                                                        {
                                                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, item to learn is already used %4: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()).arg(itemId));
                                                            ok=false;
                                                        }
                                                    }
                                                    if(ok)
                                                        learnByItem << itemId;*/
                                                }
                                            }
                                            else
                                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, missing arguements (level or skill): child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()));
                                        }
                                        else
                                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(attack.tagName()).arg(attack.lineNumber()));
                                        attack = attack.nextSiblingElement(text_attack);
                                    }
                                    QSet<quint32> byitemAlreadySet;
                                    QSet<quint32> skillAlreadySet;
                                    //add the missing skill
                                    if(monsterIdToName.contains(id))
                                        if(monsterToLink.contains(monsterIdToName.value(id)))
                                        {
                                            const QList<Link> &link_list=monsterToLink.value(monsterIdToName.value(id));
                                            int index=0;
                                            while(index<link_list.size())
                                            {
                                                const Link &link=link_list.at(index);
                                                if(itemNameToId.contains(link.skill))
                                                    if(skillNameToId.contains(link.skill))
                                                    {
                                                        QDomElement newElement=attack_list.ownerDocument().createElement("attack");
                                                        quint32 byitem=itemNameToId.value(link.skill);
                                                        quint32 skill=skillNameToId.value(link.skill);
                                                        if(!byitemAlreadySet.contains(byitem) && !skillAlreadySet.contains(skill))
                                                        {
                                                            byitemAlreadySet << byitem;
                                                            skillAlreadySet << skill;
                                                            newElement.setAttribute("byitem",byitem);
                                                            newElement.setAttribute("id",skill);
                                                            if(!newElement.isNull())
                                                                attack_list.appendChild(newElement);
                                                        }
                                                    }
                                                index++;
                                            }
                                        }
                                }
                                else
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, attack_list balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not attack_list: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                    }
                    else
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, one of the attribute is wrong or is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the monster id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(text_monster);
    }
    {
        if(!xmlFile.open(QIODevice::WriteOnly))
        {
            qDebug() << QString("Unable to open the file %1:\n%2").arg(xmlFile.fileName()).arg(xmlFile.errorString());
            return;
        }
        xmlFile.write(domDocument.toByteArray(4));
        xmlFile.close();
    }
}

QStringList listFolder(const QString& folder,const QString& suffix=QString())
{
    QStringList returnList;
    QFileInfoList entryList=QDir(folder+suffix).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot,QDir::DirsFirst|QDir::Name);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnList+=listFolder(folder,suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else if(fileInfo.isFile())
            returnList+=suffix+fileInfo.fileName();
    }
    return returnList;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStringList arguments=QCoreApplication::arguments();
    if(arguments.size()!=2)
    {
        qDebug() << "pass only the file to parse and datapack path to update as arguments";
        return 1;
    }
    if(!QFile(arguments.last()+"/items/items.xml").exists())
    {
        qDebug() << "missing items/items.xml into the path given";
        return 2;
    }
    if(!QFile(arguments.last()+"/monsters/skill.xml").exists())
    {
        qDebug() << "missing monsters/skill.xml into the path given";
        return 2;
    }
    if(!QFile(arguments.last()+"/monsters/monster.xml").exists())
    {
        qDebug() << "missing monsters/monster.xml into the path given";
        return 2;
    }
    datapackPath=arguments.last()+"/";

    QByteArray endOfString;
    endOfString[0]=0x00;
    {
        bool test=false;
        if(!test)
        {
            QString currentPath=QDir::currentPath();
            if(!currentPath.endsWith("/") || !currentPath.endsWith("\\"))
                currentPath+="/";
            const QStringList &fileList=listFolder(currentPath);
            int index=0;
            while(index<fileList.size())
            {
                if(index%1000==0 && index>0)
                    qDebug() << "Number of file done" << index << "on" << fileList.size();
                //filter by generation
                const QString &filePath=currentPath+fileList.at(index);
                QFile file(filePath);
                if(file.open(QIODevice::ReadOnly))
                {
                    QByteArray rawData=file.readAll();
                    rawData.replace(endOfString,QByteArray());
                    QString data=QString::fromUtf8(rawData);
                    QString fulldata=data;
                    file.close();
                    data.remove(QRegularExpression("<span[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
                    data.remove("</span>");
                    data.remove(QRegularExpression("<a[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
                    data.remove("</a>");
                    data.remove(QRegularExpression("<p[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
                    data.remove("</p>");
                    data.replace(QRegularExpression("<br />[^>]+[ \r\t\n]*</td>"),"</td>");
                    loadHtmlTM(data,fulldata);//bool used=
                    //if(!used)
                    //    qDebug() << "Not used:" << file.fileName();
                }
                else
                    qDebug() << "Unable to open the file" << filePath;
                index++;
            }
        }
        else
        {
            QFile file(QDir::currentPath()+"/Celebi/Generation_III_learnset.html");
            if(file.open(QIODevice::ReadOnly))
            {
                QByteArray rawData=file.readAll();
                rawData.replace(endOfString,QByteArray());
                QString data=QString::fromUtf8(rawData);
                QString fulldata=data;
                file.close();
                data.remove(QRegularExpression("<span[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
                data.remove("</span>");
                data.remove(QRegularExpression("<a[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
                data.remove("</a>");
                data.remove(QRegularExpression("<p[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
                data.remove("</p>");
                data.replace(QRegularExpression("<br />[^>]+[ \r\t\n]*</td>"),"</td>");
                bool used=loadHtmlTM(data,fulldata);
                if(!used)
                    qDebug() << "Not used:" << file.fileName();
            }
        }
    }

    parseItemsExtra();
    parseSkillsExtra();
    parseMonstersExtra();
    updateMonsterXml(arguments.last()+QLatin1Literal(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("monster.xml"));

    return 0;
}
