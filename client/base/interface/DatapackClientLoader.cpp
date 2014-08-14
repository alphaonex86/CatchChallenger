#include "../interface/DatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../LanguagesSelect.h"
#include "../../tiled/tiled_tileset.h"
#include "../../tiled/tiled_mapreader.h"

#include <QDebug>
#include <QFile>
#include <QDomElement>
#include <QDomDocument>
#include <QByteArray>
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>
#include <QCryptographicHash>

DatapackClientLoader DatapackClientLoader::datapackLoader;

const QString DatapackClientLoader::text_list=QLatin1Literal("list");
const QString DatapackClientLoader::text_reputation=QLatin1Literal("reputation");
const QString DatapackClientLoader::text_type=QLatin1Literal("type");
const QString DatapackClientLoader::text_name=QLatin1Literal("name");
const QString DatapackClientLoader::text_en=QLatin1Literal("en");
const QString DatapackClientLoader::text_lang=QLatin1Literal("lang");
const QString DatapackClientLoader::text_level=QLatin1Literal("level");
const QString DatapackClientLoader::text_point=QLatin1Literal("point");
const QString DatapackClientLoader::text_text=QLatin1Literal("text");
const QString DatapackClientLoader::text_id=QLatin1Literal("id");
const QString DatapackClientLoader::text_image=QLatin1Literal("image");
const QString DatapackClientLoader::text_description=QLatin1Literal("description");
const QString DatapackClientLoader::text_item=QLatin1Literal("item");
const QString DatapackClientLoader::text_slashdefinitiondotxml=QLatin1Literal("/definition.xml");
const QString DatapackClientLoader::text_quest=QLatin1Literal("quest");
const QString DatapackClientLoader::text_rewards=QLatin1Literal("rewards");
const QString DatapackClientLoader::text_show=QLatin1Literal("show");
const QString DatapackClientLoader::text_autostep=QLatin1Literal("autostep");
const QString DatapackClientLoader::text_yes=QLatin1Literal("yes");
const QString DatapackClientLoader::text_true=QLatin1Literal("true");
const QString DatapackClientLoader::text_step=QLatin1Literal("step");
const QString DatapackClientLoader::text_bot=QLatin1Literal("bot");
const QString DatapackClientLoader::text_dotcomma=QLatin1Literal(";");
const QString DatapackClientLoader::text_client_logic=QLatin1Literal("client_logic");
const QString DatapackClientLoader::text_map=QLatin1Literal("map");
const QString DatapackClientLoader::text_items=QLatin1Literal("items");
const QString DatapackClientLoader::text_zone=QLatin1Literal("zone");

const QString DatapackClientLoader::text_monster=QLatin1Literal("monster");
const QString DatapackClientLoader::text_monsters=QLatin1Literal("monsters");
const QString DatapackClientLoader::text_kind=QLatin1Literal("kind");
const QString DatapackClientLoader::text_habitat=QLatin1Literal("habitat");
const QString DatapackClientLoader::text_slash=QLatin1Literal("/");
const QString DatapackClientLoader::text_types=QLatin1Literal("types");
const QString DatapackClientLoader::text_buff=QLatin1Literal("buff");
const QString DatapackClientLoader::text_skill=QLatin1Literal("skill");
const QString DatapackClientLoader::text_buffs=QLatin1Literal("buffs");
const QString DatapackClientLoader::text_skills=QLatin1Literal("skills");
const QString DatapackClientLoader::text_fight=QLatin1Literal("fight");
const QString DatapackClientLoader::text_fights=QLatin1Literal("fights");
const QString DatapackClientLoader::text_start=QLatin1Literal("start");
const QString DatapackClientLoader::text_win=QLatin1Literal("win");
const QString DatapackClientLoader::text_dotxml=QLatin1Literal(".xml");
const QString DatapackClientLoader::text_dottsx=QLatin1Literal(".tsx");
const QString DatapackClientLoader::text_visual=QLatin1Literal("visual");
const QString DatapackClientLoader::text_category=QLatin1Literal("category");
const QString DatapackClientLoader::text_alpha=QLatin1Literal("alpha");
const QString DatapackClientLoader::text_color=QLatin1Literal("color");
const QString DatapackClientLoader::text_event=QLatin1Literal("event");
const QString DatapackClientLoader::text_value=QLatin1Literal("value");

DatapackClientLoader::DatapackClientLoader()
{
    mDefaultInventoryImage=NULL;
    inProgress=false;
    start();
    moveToThread(this);
}

DatapackClientLoader::~DatapackClientLoader()
{
    if(mDefaultInventoryImage==NULL)
        delete mDefaultInventoryImage;
    quit();
    wait();
}

QPixmap DatapackClientLoader::defaultInventoryImage()
{
    return *mDefaultInventoryImage;
}

void DatapackClientLoader::run()
{
    exec();
}

bool DatapackClientLoader::isParsingDatapack()
{
    return inProgress;
}

void DatapackClientLoader::parseDatapack(const QString &datapackPath)
{
    if(inProgress)
    {
        qDebug() << QStringLiteral("already in progress");
        return;
    }
    inProgress=true;

    if(!CommonSettings::commonSettings.httpDatapackMirror.isEmpty())
    {
        QCryptographicHash hash(QCryptographicHash::Sha224);

        QSet<QString> extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
        QRegularExpression datapack_rightFileName=QRegularExpression(DATAPACK_FILE_REGEX);
        QStringList returnList=CatchChallenger::FacilityLib::listFolder(datapackPath);
        returnList.sort();
        int index=0;
        const int &size=returnList.size();
        while(index<size)
        {
            const QString &fileName=returnList.at(index);
            if(fileName.contains(datapack_rightFileName))
            {
                if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
                {
                    QFile file(datapackPath+returnList.at(index));
                    if(file.size()<=8*1024*1024)
                    {
                        if(file.open(QIODevice::ReadOnly))
                        {
                            const QByteArray &data=file.readAll();
                            {
                                QCryptographicHash hashFile(QCryptographicHash::Sha224);
                                hashFile.addData(data);
                                qDebug() << QStringLiteral("%1 %2").arg(file.fileName()).arg(QString(hashFile.result().toHex()));
                            }
                            hash.addData(data);
                            file.close();
                        }
                        else
                        {
                            qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                            emit datapackChecksumError();
                            inProgress=false;
                            return;
                        }
                    }
                }
            }
            index++;
        }

        if(CommonSettings::commonSettings.datapackHash!=hash.result())
        {
            qDebug() << QStringLiteral("CommonSettings::commonSettings.datapackHash!=hash.result(): %1!=%2")
                        .arg(QString(CommonSettings::commonSettings.datapackHash.toHex()))
                        .arg(QString(hash.result().toHex()));
            emit datapackChecksumError();
            inProgress=false;
            return;
        }
    }

    this->datapackPath=datapackPath;
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/images/inventory/unknow-object.png"));
    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapackPath);
    language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    parseVisualCategory();
    parseTypesExtra();
    parseItemsExtra();
    parseMaps();
    parseSkins();
    parseMonstersExtra();
    parseBuffExtra();
    parseSkillsExtra();
    parseQuestsLink();
    parseQuestsExtra();
    parseQuestsText();
    parseBotFightsExtra();
    parsePlantsExtra();
    parseAudioAmbiance();
    parseZoneExtra();
    parseTileset();
    parseReputationExtra();
    inProgress=false;
    emit datapackParsed();
}

QString DatapackClientLoader::getDatapackPath()
{
    return datapackPath;
}

void DatapackClientLoader::parseVisualCategory()
{
    QDomDocument domDocument;
    //open and quick check the file
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+QStringLiteral("visualcategory.xml");
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile itemsFile(file);
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString()));
            return;
        }
        const QByteArray &xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return;
        }
        //qDebug() << (QStringLiteral("Xml not already loaded: %1").arg(file));
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackClientLoader::text_visual)
    {
        qDebug() << (QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file));
        return;
    }

    //load the content
    QDomElement item = root.firstChildElement(DatapackClientLoader::text_category);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(DatapackClientLoader::text_id))
            {
                if(!item.attribute(DatapackClientLoader::text_id).isEmpty())
                {
                    if(!DatapackClientLoader::datapackLoader.visualCategories.contains(item.attribute(DatapackClientLoader::text_id)))
                    {
                        bool ok;
                        DatapackClientLoader::datapackLoader.visualCategories[item.attribute(DatapackClientLoader::text_id)].defaultColor=Qt::transparent;
                        int alpha=255;
                        if(item.hasAttribute(DatapackClientLoader::text_alpha))
                        {
                            alpha=item.attribute(DatapackClientLoader::text_alpha).toUInt(&ok);
                            if(!ok || alpha>255)
                            {
                                alpha=255;
                                qDebug() << (QStringLiteral("Unable to open the file: %1, alpha is not number or greater than 255: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                            }
                        }
                        if(item.hasAttribute(DatapackClientLoader::text_color))
                        {
                            QColor color;
                            color.setNamedColor(item.attribute(DatapackClientLoader::text_color));
                            if(color.isValid())
                            {
                                color.setAlpha(alpha);
                                DatapackClientLoader::datapackLoader.visualCategories[item.attribute(DatapackClientLoader::text_id)].defaultColor=color;
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the file: %1, color is not valid: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                        QDomElement event = item.firstChildElement(DatapackClientLoader::text_event);
                        while(!event.isNull())
                        {
                            if(event.isElement())
                            {
                                if(event.hasAttribute(DatapackClientLoader::text_id) && event.hasAttribute(DatapackClientLoader::text_value))
                                {
                                    int index=0;
                                    while(index<CatchChallenger::CommonDatapack::commonDatapack.events.size())
                                    {
                                        if(CatchChallenger::CommonDatapack::commonDatapack.events.at(index).name==event.attribute(DatapackClientLoader::text_id))
                                        {
                                            int sub_index=0;
                                            while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.events.at(index).values.size())
                                            {
                                                if(CatchChallenger::CommonDatapack::commonDatapack.events.at(index).values.at(sub_index)==event.attribute(DatapackClientLoader::text_value))
                                                {
                                                    VisualCategory::VisualCategoryCondition visualCategoryCondition;
                                                    visualCategoryCondition.event=index;
                                                    visualCategoryCondition.eventValue=sub_index;
                                                    visualCategoryCondition.color=DatapackClientLoader::datapackLoader.visualCategories.value(item.attribute(DatapackClientLoader::text_id)).defaultColor;
                                                    int alpha=255;
                                                    if(event.hasAttribute(DatapackClientLoader::text_alpha))
                                                    {
                                                        alpha=event.attribute(DatapackClientLoader::text_alpha).toUInt(&ok);
                                                        if(!ok || alpha>255)
                                                        {
                                                            alpha=255;
                                                            qDebug() << (QStringLiteral("Unable to open the file: %1, alpha is not number or greater than 255: child.tagName(): %2 (at line: %3)").arg(file).arg(event.tagName()).arg(event.lineNumber()));
                                                        }
                                                    }
                                                    if(event.hasAttribute(DatapackClientLoader::text_color))
                                                    {
                                                        QColor color;
                                                        color.setNamedColor(event.attribute(DatapackClientLoader::text_color));
                                                        if(color.isValid())
                                                            visualCategoryCondition.color=QColor(color.red(),color.green(),color.blue(),alpha);
                                                        else
                                                            qDebug() << (QStringLiteral("Unable to open the file: %1, color is not valid: child.tagName(): %2 (at line: %3)").arg(file).arg(event.tagName()).arg(event.lineNumber()));
                                                    }
                                                    if(visualCategoryCondition.color!=DatapackClientLoader::datapackLoader.visualCategories.value(item.attribute(DatapackClientLoader::text_id)).defaultColor)
                                                        DatapackClientLoader::datapackLoader.visualCategories[item.attribute(DatapackClientLoader::text_id)].conditions << visualCategoryCondition;
                                                    else
                                                        qDebug() << (QStringLiteral("Unable to open the file: %1, color same than the default color: child.tagName(): %2 (at line: %3)").arg(file).arg(event.tagName()).arg(event.lineNumber()));
                                                    break;
                                                }
                                                sub_index++;
                                            }
                                            if(sub_index==CatchChallenger::CommonDatapack::commonDatapack.events.at(index).values.size())
                                                qDebug() << (QStringLiteral("Unable to open the file: %1, event value not found: child.tagName(): %2 (at line: %3)").arg(file).arg(event.tagName()).arg(event.lineNumber()));
                                            break;
                                        }
                                        index++;
                                    }
                                    if(index==CatchChallenger::CommonDatapack::commonDatapack.events.size())
                                        qDebug() << (QStringLiteral("Unable to open the file: %1, event not found: child.tagName(): %2 (at line: %3)").arg(file).arg(event.tagName()).arg(event.lineNumber()));
                                }
                                else
                                    qDebug() << (QStringLiteral("Unable to open the file: %1, attribute id is already found: child.tagName(): %2 (at line: %3)").arg(file).arg(event.tagName()).arg(event.lineNumber()));
                            }
                            event = event.nextSiblingElement(DatapackClientLoader::text_event);
                        }
                    }
                    else
                        qDebug() << (QStringLiteral("Unable to open the file: %1, attribute id is already found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                }
                else
                    qDebug() << (QStringLiteral("Unable to open the file: %1, attribute id can't be empty: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                qDebug() << (QStringLiteral("Unable to open the file: %1, have not the attribute id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        item = item.nextSiblingElement(DatapackClientLoader::text_category);
    }

    qDebug() << QStringLiteral("%1 visual cat loaded").arg(DatapackClientLoader::datapackLoader.visualCategories.size());
}

void DatapackClientLoader::parseReputationExtra()
{
    {
        int index=0;
        while(index<CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CatchChallenger::CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    QDomDocument domDocument;
    //open and quick check the file
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLAYER)+QStringLiteral("reputation.xml");
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile itemsFile(file);
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << (QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString()));
            return;
        }
        const QByteArray &xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << (QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
            return;
        }
        //qDebug() << (QStringLiteral("Xml not already loaded: %1").arg(file));
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackClientLoader::text_list)
    {
        qDebug() << (QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file));
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement(DatapackClientLoader::text_reputation);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(DatapackClientLoader::text_type))
            {
                ok=true;

                //load the name
                {
                    bool name_found=false;
                    QDomElement name = item.firstChildElement(DatapackClientLoader::text_name);
                    if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                        while(!name.isNull())
                        {
                            if(name.isElement())
                            {
                                if(name.hasAttribute(DatapackClientLoader::text_lang) && name.attribute(DatapackClientLoader::text_lang)==language)
                                {
                                    reputationExtra[item.attribute(DatapackClientLoader::text_type)].name=name.text();
                                    name_found=true;
                                    break;
                                }
                            }
                            name = name.nextSiblingElement(DatapackClientLoader::text_name);
                        }
                    if(!name_found)
                    {
                        name = item.firstChildElement(DatapackClientLoader::text_name);
                        while(!name.isNull())
                        {
                            if(name.isElement())
                            {
                                if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                {
                                    reputationExtra[item.attribute(DatapackClientLoader::text_type)].name=name.text();
                                    name_found=true;
                                    break;
                                }
                            }
                            name = name.nextSiblingElement(DatapackClientLoader::text_name);
                        }
                    }
                    if(!name_found)
                    {
                        reputationExtra[item.attribute(DatapackClientLoader::text_type)].name=tr("Unknown");
                        qDebug() << QStringLiteral("English name not found for the reputation with id: %1").arg(item.attribute(DatapackClientLoader::text_type));
                    }
                }

                QList<qint32> point_list_positive,point_list_negative;
                QStringList text_positive,text_negative;
                QDomElement level = item.firstChildElement(DatapackClientLoader::text_level);
                ok=true;
                while(!level.isNull() && ok)
                {
                    if(level.isElement())
                    {
                        if(level.hasAttribute(DatapackClientLoader::text_point))
                        {
                            const qint32 &point=level.attribute(DatapackClientLoader::text_point).toInt(&ok);
                            //QString text_val;
                            if(ok)
                            {
                                ok=true;

                                QString text;
                                //load the name
                                {
                                    bool name_found=false;
                                    QDomElement name = level.firstChildElement(DatapackClientLoader::text_text);
                                    if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                        while(!name.isNull())
                                        {
                                            if(name.isElement())
                                            {
                                                if(name.hasAttribute(DatapackClientLoader::text_lang) && name.attribute(DatapackClientLoader::text_lang)==language)
                                                {
                                                    text=name.text();
                                                    name_found=true;
                                                    break;
                                                }
                                            }
                                            name = name.nextSiblingElement(DatapackClientLoader::text_text);
                                        }
                                    if(!name_found)
                                    {
                                        name = level.firstChildElement(DatapackClientLoader::text_text);
                                        while(!name.isNull())
                                        {
                                            if(name.isElement())
                                            {
                                                if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                                {
                                                    text=name.text();
                                                    name_found=true;
                                                    break;
                                                }
                                            }
                                            name = name.nextSiblingElement(DatapackClientLoader::text_text);
                                        }
                                    }
                                    if(!name_found)
                                    {
                                        text=tr("Unknown");
                                        qDebug() << QStringLiteral("English name not found for the reputation with id: %1").arg(item.attribute("type"));
                                    }
                                }

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
                                            text_positive.insert(index,text);
                                            found=true;
                                            break;
                                        }
                                        index++;
                                    }
                                    if(!found)
                                    {
                                        point_list_positive << point;
                                        text_positive << text;
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
                                            text_negative.insert(index,text);
                                            found=true;
                                            break;
                                        }
                                        index++;
                                    }
                                    if(!found)
                                    {
                                        point_list_negative << point;
                                        text_negative << text;
                                    }
                                }
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the file: %1, point is not number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        }
                    }
                    else
                        qDebug() << (QStringLiteral("Unable to open the file: %1, point attribute not found: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    level = level.nextSiblingElement(DatapackClientLoader::text_level);
                }
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
                        qDebug() << (QStringLiteral("Unable to open the file: %1, no starting level for the negative: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!item.attribute("type").contains(QRegExp("^[a-z]{1,32}$")))
                    {
                        qDebug() << (QStringLiteral("Unable to open the file: %1, the type %4 don't match wiuth the regex: ^[a-z]{1,32}$: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("type")));
                        ok=false;
                    }
                if(ok)
                {
                    reputationExtra[item.attribute(DatapackClientLoader::text_type)].reputation_positive=text_positive;
                    reputationExtra[item.attribute(DatapackClientLoader::text_type)].reputation_negative=text_negative;
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            qDebug() << (QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement(DatapackClientLoader::text_reputation);
    }
    {
        int index=0;
        while(index<CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
        {
            const CatchChallenger::Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(index);
            if(!reputationExtra.contains(reputation.name))
                reputationExtra[reputation.name].name=tr("Unknown");
            while(reputationExtra[reputation.name].reputation_negative.size()<reputation.reputation_negative.size())
                reputationExtra[reputation.name].reputation_negative << tr("Unknown");
            while(reputationExtra[reputation.name].reputation_positive.size()<reputation.reputation_positive.size())
                reputationExtra[reputation.name].reputation_positive << tr("Unknown");
            index++;
        }
    }

    qDebug() << QStringLiteral("%1 reputation(s) extra loaded").arg(reputationExtra.size());
}

void DatapackClientLoader::parseItemsExtra()
{
    QDir dir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM));
    const QStringList &fileList=CatchChallenger::FacilityLib::listFolder(dir.absolutePath()+DatapackClientLoader::text_slash);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).endsWith(DatapackClientLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM)+fileList.at(file_index);
        if(!QFileInfo(file).isFile())
        {
            file_index++;
            continue;
        }
        QDomDocument domDocument;
        if(!file.endsWith(DatapackClientLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        //open and quick check the file
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
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
            //qDebug() << (QStringLiteral("Xml not already loaded: %1").arg(file));
            CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        const QDomElement &root = domDocument.documentElement();
        if(root.tagName()!=DatapackClientLoader::text_items)
        {
            //qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file);
            file_index++;
            continue;
        }

        const QString &folder=QFileInfo(file).absolutePath()+DatapackClientLoader::text_slash;
        //load the content
        bool ok;
        QDomElement item = root.firstChildElement(DatapackClientLoader::text_item);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(DatapackClientLoader::text_id))
                {
                    const quint32 &id=item.attribute(DatapackClientLoader::text_id).toULongLong(&ok);
                    if(ok)
                    {
                        if(!DatapackClientLoader::itemsExtra.contains(id))
                        {
                            //load the image
                            if(item.hasAttribute(DatapackClientLoader::text_image))
                            {
                                QPixmap image(folder+item.attribute("image"));
                                if(image.isNull())
                                {
                                    qDebug() << QStringLiteral("Unable to open the items image: %1: child.tagName(): %2 (at line: %3)").arg(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM)+item.attribute(QStringLiteral("image"))).arg(item.tagName()).arg(item.lineNumber());
                                    DatapackClientLoader::itemsExtra[id].image=*mDefaultInventoryImage;
                                    DatapackClientLoader::itemsExtra[id].imagePath=QStringLiteral(":/images/inventory/unknow-object.png");
                                }
                                else
                                {
                                    DatapackClientLoader::itemsExtra[id].imagePath=QFileInfo(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM)+item.attribute(DatapackClientLoader::text_image)).absoluteFilePath();
                                    DatapackClientLoader::itemsExtra[id].image=image;
                                }
                            }
                            else
                            {
                                qDebug() << QStringLiteral("For parse item: Have not image attribute: child.tagName(): %1 (%2 at line: %3)").arg(item.tagName()).arg(file).arg(item.lineNumber());
                                DatapackClientLoader::itemsExtra[id].image=*mDefaultInventoryImage;
                                DatapackClientLoader::itemsExtra[id].imagePath=QStringLiteral(":/images/inventory/unknow-object.png");
                            }
                            // base size: 24x24
                            DatapackClientLoader::itemsExtra[id].image=DatapackClientLoader::itemsExtra.value(id).image.scaled(72,72);//then zoom: 3x

                            //load the name
                            {
                                bool name_found=false;
                                QDomElement name = item.firstChildElement(DatapackClientLoader::text_name);
                                if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                    while(!name.isNull())
                                    {
                                        if(name.isElement())
                                        {
                                            if(name.hasAttribute(DatapackClientLoader::text_lang) && name.attribute(DatapackClientLoader::text_lang)==language)
                                            {
                                                DatapackClientLoader::itemsExtra[id].name=name.text();
                                                name_found=true;
                                                break;
                                            }
                                        }
                                        name = name.nextSiblingElement(DatapackClientLoader::text_name);
                                    }
                                if(!name_found)
                                {
                                    name = item.firstChildElement(DatapackClientLoader::text_name);
                                    while(!name.isNull())
                                    {
                                        if(name.isElement())
                                        {
                                            if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                            {
                                                DatapackClientLoader::itemsExtra[id].name=name.text();
                                                name_found=true;
                                                break;
                                            }
                                        }
                                        name = name.nextSiblingElement(DatapackClientLoader::text_name);
                                    }
                                }
                                if(!name_found)
                                {
                                    DatapackClientLoader::itemsExtra[id].name=tr("Unknown object");
                                    qDebug() << QStringLiteral("English name not found for the item with id: %1").arg(id);
                                }
                            }

                            //load the description
                            {
                                bool description_found=false;
                                QDomElement description = item.firstChildElement(DatapackClientLoader::text_description);
                                if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                    while(!description.isNull())
                                    {
                                        if(description.isElement())
                                        {
                                            if(description.hasAttribute(DatapackClientLoader::text_lang) && description.attribute(DatapackClientLoader::text_lang)==language)
                                            {
                                                DatapackClientLoader::itemsExtra[id].description=description.text();
                                                description_found=true;
                                                break;
                                            }
                                        }
                                        description = description.nextSiblingElement(DatapackClientLoader::text_description);
                                    }
                                if(!description_found)
                                {
                                    description = item.firstChildElement(DatapackClientLoader::text_description);
                                    while(!description.isNull())
                                    {
                                        if(description.isElement())
                                        {
                                            if(!description.hasAttribute(DatapackClientLoader::text_lang) || description.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                            {
                                                DatapackClientLoader::itemsExtra[id].description=description.text();
                                                description_found=true;
                                                break;
                                            }
                                        }
                                        description = description.nextSiblingElement(DatapackClientLoader::text_description);
                                    }
                                }
                                if(!description_found)
                                {
                                    DatapackClientLoader::itemsExtra[id].description=tr("This object is not listed as know object. The information can't be found.");
                                    //qDebug() << QStringLiteral("English description not found for the item with id: %1").arg(id);
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
            item = item.nextSiblingElement(DatapackClientLoader::text_item);
        }
        file_index++;
    }

    qDebug() << QStringLiteral("%1 item(s) extra loaded").arg(DatapackClientLoader::itemsExtra.size());
}

void DatapackClientLoader::parseMaps()
{
    const QStringList &returnList=CatchChallenger::FacilityLib::listFolder(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP));

    //load the map
    const int &size=returnList.size();
    int index=0;
    QRegularExpression mapFilter(QStringLiteral("\\.tmx$"));
    QRegularExpression mapExclude(QLatin1String("[\"']"));
    QHash<QString,QString> sortToFull;
    QStringList tempMapList;
    while(index<size)
    {
        const QString &fileName=returnList.at(index);
        QString sortFileName=fileName;
        if(fileName.contains(mapFilter) && !fileName.contains(mapExclude))
        {
            sortFileName.remove(mapFilter);
            sortToFull[sortFileName]=fileName;
            tempMapList << sortFileName;
        }
        index++;
    }
    tempMapList.sort();
    const QString &basePath=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP);
    index=0;
    while(index<tempMapList.size())
    {
        mapToId[sortToFull.value(tempMapList.at(index))]=index;
        fullMapPathToId[QFileInfo(basePath+sortToFull.value(tempMapList.at(index))).absoluteFilePath()]=index;
        maps << sortToFull.value(tempMapList.at(index));
        index++;
    }

    qDebug() << QStringLiteral("%1 map(s) extra loaded").arg(tempMapList.size());
}

void DatapackClientLoader::parseSkins()
{
    skins=CatchChallenger::FacilityLib::skinIdList(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_SKIN));

    qDebug() << QStringLiteral("%1 skin(s) loaded").arg(skins.size());
}

void DatapackClientLoader::resetAll()
{
    CatchChallenger::CommonDatapack::commonDatapack.unload();
    language.clear();
    mapToId.clear();
    fullMapPathToId.clear();
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/images/inventory/unknow-object.png"));
    datapackPath.clear();
    itemsExtra.clear();
    maps.clear();
    skins.clear();

    {
        QHashIterator<quint8,PlantExtra> i(plantExtra);
        while (i.hasNext()) {
            i.next();
            delete i.value().tileset;
        }
    }
    plantExtra.clear();
    itemToPlants.clear();
    questsExtra.clear();
    questsText.clear();
    botToQuestStart.clear();
    botFightsExtra.clear();
    monsterExtra.clear();
    monsterBuffsExtra.clear();
    monsterSkillsExtra.clear();
    audioAmbiance.clear();
    zonesExtra.clear();
    typeExtra.clear();
    {
         QHashIterator<QString,Tiled::Tileset *> i(Tiled::Tileset::preloadedTileset);
         while (i.hasNext()) {
             i.next();
             delete i.value();
         }
         Tiled::Tileset::preloadedTileset.clear();
    }
}

void DatapackClientLoader::parseQuestsExtra()
{
    //open and quick check the file
    const QFileInfoList &entryList=QDir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_QUESTS)).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    while(index<entryList.size())
    {
        bool showRewards=false;
        bool autostep=false;
        if(!entryList.at(index).isDir())
        {
            index++;
            continue;
        }
        bool ok;
        const quint32 &id=entryList.at(index).fileName().toUInt(&ok);
        if(!ok)
        {
            qDebug() << QStringLiteral("Unable to open the folder: %1, because is folder name is not a number").arg(entryList.at(index).fileName());
            index++;
            continue;
        }
        QDomDocument domDocument;
        const QString &file=entryList.at(index).absoluteFilePath()+DatapackClientLoader::text_slashdefinitiondotxml;
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
            QFile itemsFile(file);
            if(!itemsFile.open(QIODevice::ReadOnly))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
                index++;
                continue;
            }
            const QByteArray &xmlContent=itemsFile.readAll();
            itemsFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
                index++;
                continue;
            }
            //qDebug() << (QStringLiteral("Xml not already loaded: %1").arg(file));
            CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        const QDomElement &root = domDocument.documentElement();
        if(root.tagName()!=DatapackClientLoader::text_quest)
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(file);
            index++;
            continue;
        }

        DatapackClientLoader::QuestExtra quest;

        //load name
        {
            QDomElement name = root.firstChildElement(DatapackClientLoader::text_name);
            bool found=false;
            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(name.hasAttribute(DatapackClientLoader::text_lang) && name.attribute(DatapackClientLoader::text_lang)==language)
                        {
                            quest.name=name.text();
                            found=true;
                            break;
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(name.tagName()).arg(name.lineNumber());
                    }
                    name = name.nextSiblingElement(DatapackClientLoader::text_name);
                }
            if(!found)
            {
                name = root.firstChildElement(DatapackClientLoader::text_name);
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                        {
                            quest.name=name.text();
                            break;
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(name.tagName()).arg(name.lineNumber());
                    }
                    name = name.nextSiblingElement(DatapackClientLoader::text_name);
                }
            }
        }

        //load showRewards
        {
            const QDomElement &rewards = root.firstChildElement(DatapackClientLoader::text_rewards);
            if(!rewards.isNull() && rewards.isElement())
            {
                if(rewards.hasAttribute(DatapackClientLoader::text_show))
                    if(rewards.attribute(DatapackClientLoader::text_show)==DatapackClientLoader::text_true)
                        showRewards=true;
            }
        }
        //load autostep
        {
            if(root.hasAttribute(DatapackClientLoader::text_autostep))
                if(root.attribute(DatapackClientLoader::text_autostep)==DatapackClientLoader::text_yes)
                    autostep=true;
        }

        QHash<quint32,QString> steps;
        {
            //load step
            QDomElement step = root.firstChildElement(DatapackClientLoader::text_step);
            while(!step.isNull())
            {
                if(step.isElement())
                {
                    if(step.hasAttribute(DatapackClientLoader::text_id))
                    {
                        const quint32 &id=step.attribute(DatapackClientLoader::text_id).toULongLong(&ok);
                        if(ok)
                        {
                            CatchChallenger::Quest::Step stepObject;
                            if(step.hasAttribute(DatapackClientLoader::text_bot))
                            {
                                QStringList tempStringList=step.attribute(DatapackClientLoader::text_bot).split(DatapackClientLoader::text_dotcomma);
                                int index=0;
                                while(index<tempStringList.size())
                                {
                                    quint32 tempInt=tempStringList.at(index).toUInt(&ok);
                                    if(ok)
                                        stepObject.bots << tempInt;
                                    index++;
                                }
                            }
                            QDomElement stepItem = step.firstChildElement(DatapackClientLoader::text_text);
                            bool found=false;
                            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                            {
                                while(!stepItem.isNull())
                                {
                                    if(stepItem.isElement())
                                    {
                                        if(stepItem.hasAttribute(DatapackClientLoader::text_lang) || stepItem.attribute(DatapackClientLoader::text_lang)==language)
                                        {
                                            found=true;
                                            steps[id]=stepItem.text();
                                        }
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                                    stepItem = stepItem.nextSiblingElement(DatapackClientLoader::text_text);
                                }
                            }
                            if(!found)
                            {
                                stepItem = step.firstChildElement(DatapackClientLoader::text_text);
                                while(!stepItem.isNull())
                                {
                                    if(stepItem.isElement())
                                    {
                                        if(!stepItem.hasAttribute(DatapackClientLoader::text_lang) || stepItem.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                            steps[id]=stepItem.text();
                                        /*else can be into another lang
                                            qDebug() << QStringLiteral("Has attribute: %1, is not lang en: child.tagName(): %2 (at line: %3)").arg(file).arg(stepItem.tagName()).arg(stepItem.lineNumber());*/
                                    }
                                    else
                                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(stepItem.tagName()).arg(stepItem.lineNumber());
                                    stepItem = stepItem.nextSiblingElement(DatapackClientLoader::text_text);
                                }
                                if(!steps.contains(id))
                                    steps[id]=tr("No text");
                            }
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                    }
                    else
                        qDebug() << QStringLiteral("Has attribute: %1, have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(step.tagName()).arg(step.lineNumber());
                step = step.nextSiblingElement(DatapackClientLoader::text_step);
            }
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
        if(indexLoop>=(steps.size()+1))
        {
            //add it, all seam ok
            questsExtra[id]=quest;
            questsExtra[id].showRewards=showRewards;
            questsExtra[id].autostep=autostep;
        }
        questsPathToId[entryList.at(index).absoluteFilePath()]=id;

        index++;
    }

    qDebug() << QStringLiteral("%1 quest(s) extra loaded").arg(questsExtra.size());
}

void DatapackClientLoader::parseQuestsText()
{
    //open and quick check the file
    const QFileInfoList &entryList=QDir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_QUESTS)).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    while(index<entryList.size())
    {
        if(!entryList.at(index).isDir())
        {
            index++;
            continue;
        }
        const QString &file=entryList.at(index).absoluteFilePath()+QStringLiteral("/text.xml");
        QFile itemsFile(file);
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            index++;
            continue;
        }
        const QByteArray &xmlContent=itemsFile.readAll();
        itemsFile.close();
        QDomDocument domDocument;
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        const QDomElement &root = domDocument.documentElement();
        if(root.tagName()!=DatapackClientLoader::text_text)
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(file);
            index++;
            continue;
        }

        //load the content
        bool ok;
        QHash<quint32,QString> client_logic_texts;
        //load text
        QDomElement client_logic = root.firstChildElement(DatapackClientLoader::text_client_logic);
        while(!client_logic.isNull())
        {
            if(client_logic.isElement())
            {
                if(client_logic.hasAttribute(DatapackClientLoader::text_id))
                {
                    const quint32 &id=client_logic.attribute(DatapackClientLoader::text_id).toULongLong(&ok);
                    if(ok)
                    {
                        QDomElement text = client_logic.firstChildElement(DatapackClientLoader::text_text);
                        bool found=false;
                        if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                            while(!text.isNull())
                            {
                                if(text.isElement())
                                {
                                    if(text.hasAttribute(DatapackClientLoader::text_lang) && text.attribute(DatapackClientLoader::text_lang)==language)
                                    {
                                        client_logic_texts[id]=text.text();
                                        found=true;
                                    }
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                                text = text.nextSiblingElement(DatapackClientLoader::text_text);
                            }
                        if(!found)
                        {
                            text = client_logic.firstChildElement(DatapackClientLoader::text_text);
                            while(!text.isNull())
                            {
                                if(text.isElement())
                                {
                                    if(!text.hasAttribute(DatapackClientLoader::text_lang) || text.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                        client_logic_texts[id]=text.text();
                                    /*else can be another language than english
                                        qDebug() << QStringLiteral("Has attribute: %1, is not lang en: child.tagName(): %2 (at line: %3)").arg(file).arg(text.tagName()).arg(text.lineNumber());*/
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(text.tagName()).arg(text.lineNumber());
                                text = text.nextSiblingElement(DatapackClientLoader::text_text);
                            }
                            if(!client_logic_texts.contains(id))
                                client_logic_texts[id]=tr("No text");
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Has attribute: %1, have not id attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(client_logic.tagName()).arg(client_logic.lineNumber());
            client_logic = client_logic.nextSiblingElement(DatapackClientLoader::text_client_logic);
        }
        #ifdef DEBUG_CLIENT_QUEST
        qDebug() << QStringLiteral("%1 quest(s) text loaded for quest %2").arg(client_logic_texts.size()).arg(questsPathToId.value(entryList.at(index).absoluteFilePath()));
        #endif
        questsText[questsPathToId.value(entryList.at(index).absoluteFilePath())].text=client_logic_texts;
        index++;
    }

    qDebug() << QStringLiteral("%1 quest(s) text loaded").arg(questsExtra.size());
}

void DatapackClientLoader::parseAudioAmbiance()
{
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+QStringLiteral("music.xml");
    QDomDocument domDocument;
    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile itemsFile(file);
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return;
        }
        const QByteArray &xmlContent=itemsFile.readAll();
        itemsFile.close();

        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return;
        }
        //qDebug() << (QStringLiteral("Xml not already loaded: %1").arg(file));
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackClientLoader::text_list)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(file);
        return;
    }

    //load the content
    QDomElement item = root.firstChildElement(DatapackClientLoader::text_map);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(DatapackClientLoader::text_type))
            {
                const QString &type=item.attribute(DatapackClientLoader::text_type);
                if(!DatapackClientLoader::datapackLoader.audioAmbiance.contains(type))
                    audioAmbiance[type]=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+item.text();
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement(DatapackClientLoader::text_map);
    }

    qDebug() << QStringLiteral("%1 audio ambiance(s) link loaded").arg(audioAmbiance.size());
}

void DatapackClientLoader::parseQuestsLink()
{
    QHashIterator<quint16,CatchChallenger::Quest> i(CatchChallenger::CommonDatapack::commonDatapack.quests);
    while(i.hasNext()) {
        i.next();
        if(!i.value().steps.isEmpty())
        {
            QList<quint16> bots=i.value().steps.first().bots;
            int index=0;
            while(index<bots.size())
            {
                botToQuestStart.insert(bots.at(index),i.key());
                index++;
            }
        }
    }
    qDebug() << QStringLiteral("%1 bot linked with quest(s) loaded").arg(botToQuestStart.size());
}

void DatapackClientLoader::parseZoneExtra()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ZONE)).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    QRegularExpression xmlFilter(QStringLiteral("^[a-zA-Z0-9\\- _]+\\.xml$"));
    QRegularExpression removeXml(QStringLiteral("\\.xml$"));
    while(index<entryList.size())
    {
        if(!entryList.at(index).isFile())
        {
            index++;
            continue;
        }
        if(!entryList.at(index).fileName().contains(xmlFilter))
        {
            qDebug() << QStringLiteral("%1 the zone file name not match").arg(entryList.at(index).fileName());
            index++;
            continue;
        }
        QString zoneCodeName=entryList.at(index).fileName();
        const QString &file=entryList.at(index).absoluteFilePath();
        zoneCodeName.remove(removeXml);
        QFile itemsFile(file);
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            index++;
            continue;
        }
        const QByteArray &xmlContent=itemsFile.readAll();
        itemsFile.close();
        QDomDocument domDocument;
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        const QDomElement &root = domDocument.documentElement();
        if(root.tagName()!=DatapackClientLoader::text_zone)
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"zone\" root balise not found for the xml file").arg(file);
            index++;
            continue;
        }

        //load the content
        bool haveName=false;
        DatapackClientLoader::ZoneExtra zone;

        //load name
        QDomElement name = root.firstChildElement(DatapackClientLoader::text_name);
        bool found=false;
        if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                    {
                        haveName=true;
                        zone.name=name.text();
                        break;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(name.tagName()).arg(name.lineNumber());
                }
                name = name.nextSiblingElement(DatapackClientLoader::text_name);
            }
        if(!found)
        {
            name = root.firstChildElement(DatapackClientLoader::text_name);
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                    {
                        haveName=true;
                        zone.name=name.text();
                        break;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(name.tagName()).arg(name.lineNumber());
                }
                name = name.nextSiblingElement(DatapackClientLoader::text_name);
            }
        }
        if(haveName)
            zonesExtra[zoneCodeName]=zone;

        index++;
    }

    qDebug() << QStringLiteral("%1 zone(s) extra loaded").arg(zonesExtra.size());
}

void DatapackClientLoader::parseTileset()
{
    const QStringList &fileList=CatchChallenger::FacilityLib::listFolder(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP));
    int index=0;
    while(index<fileList.size())
    {
        const QString &filePath=fileList.at(index);
        if(filePath.endsWith(DatapackClientLoader::text_dottsx))
        {
            const QString &source=QFileInfo(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+filePath).absoluteFilePath();
            QFile file(source);
            if(file.open(QIODevice::ReadOnly))
            {
                Tiled::MapReader mapReader;
                Tiled::Tileset *tileset = mapReader.readTileset(&file, source);
                if (tileset)
                {
                    tileset->setFileName(source);
                    Tiled::Tileset::preloadedTileset[source]=tileset;
                }
                file.close();
            }
            else
                qDebug() << QStringLiteral("Tileset: %1 can't be open: %2").arg(source).arg(file.errorString());
        }
        index++;
    }

    qDebug() << QStringLiteral("%1 tileset(s) loaded").arg(Tiled::Tileset::preloadedTileset.size());
}
