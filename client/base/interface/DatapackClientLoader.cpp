#include "../interface/DatapackClientLoader.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/DatapackGeneralLoader.h"
#include "../DatapackChecksum.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "../../../general/base/CommonSettingsServer.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/base/FacilityLibGeneral.h"
#include "../../../general/base/DatapackGeneralLoader.h"
#include "../../../general/base/Map_loader.h"
#include "../LanguagesSelect.h"
#include "../../tiled/tiled_tileset.h"
#include "../../tiled/tiled_mapreader.h"
#include "../FacilityLibClient.h"

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

const QString DatapackClientLoader::text_list=QStringLiteral("list");
const QString DatapackClientLoader::text_reputation=QStringLiteral("reputation");
const QString DatapackClientLoader::text_type=QStringLiteral("type");
const QString DatapackClientLoader::text_name=QStringLiteral("name");
const QString DatapackClientLoader::text_en=QStringLiteral("en");
const QString DatapackClientLoader::text_lang=QStringLiteral("lang");
const QString DatapackClientLoader::text_level=QStringLiteral("level");
const QString DatapackClientLoader::text_point=QStringLiteral("point");
const QString DatapackClientLoader::text_text=QStringLiteral("text");
const QString DatapackClientLoader::text_id=QStringLiteral("id");
const QString DatapackClientLoader::text_image=QStringLiteral("image");
const QString DatapackClientLoader::text_description=QStringLiteral("description");
const QString DatapackClientLoader::text_item=QStringLiteral("item");
const QString DatapackClientLoader::text_slashdefinitiondotxml=QStringLiteral("/definition.xml");
const QString DatapackClientLoader::text_quest=QStringLiteral("quest");
const QString DatapackClientLoader::text_rewards=QStringLiteral("rewards");
const QString DatapackClientLoader::text_show=QStringLiteral("show");
const QString DatapackClientLoader::text_autostep=QStringLiteral("autostep");
const QString DatapackClientLoader::text_yes=QStringLiteral("yes");
const QString DatapackClientLoader::text_true=QStringLiteral("true");
const QString DatapackClientLoader::text_step=QStringLiteral("step");
const QString DatapackClientLoader::text_bot=QStringLiteral("bot");
const QString DatapackClientLoader::text_dotcomma=QStringLiteral(";");
const QString DatapackClientLoader::text_client_logic=QStringLiteral("client_logic");
const QString DatapackClientLoader::text_map=QStringLiteral("map");
const QString DatapackClientLoader::text_items=QStringLiteral("items");
const QString DatapackClientLoader::text_zone=QStringLiteral("zone");
const QString DatapackClientLoader::text_music=QStringLiteral("music");
const QString DatapackClientLoader::text_backgroundsound=QStringLiteral("backgroundsound");

const QString DatapackClientLoader::text_monster=QStringLiteral("monster");
const QString DatapackClientLoader::text_monsters=QStringLiteral("monsters");
const QString DatapackClientLoader::text_kind=QStringLiteral("kind");
const QString DatapackClientLoader::text_habitat=QStringLiteral("habitat");
const QString DatapackClientLoader::text_slash=QStringLiteral("/");
const QString DatapackClientLoader::text_types=QStringLiteral("types");
const QString DatapackClientLoader::text_buff=QStringLiteral("buff");
const QString DatapackClientLoader::text_skill=QStringLiteral("skill");
const QString DatapackClientLoader::text_buffs=QStringLiteral("buffs");
const QString DatapackClientLoader::text_skills=QStringLiteral("skills");
const QString DatapackClientLoader::text_fight=QStringLiteral("fight");
const QString DatapackClientLoader::text_fights=QStringLiteral("fights");
const QString DatapackClientLoader::text_start=QStringLiteral("start");
const QString DatapackClientLoader::text_win=QStringLiteral("win");
const QString DatapackClientLoader::text_dotxml=QStringLiteral(".xml");
const QString DatapackClientLoader::text_dottsx=QStringLiteral(".tsx");
const QString DatapackClientLoader::text_visual=QStringLiteral("visual");
const QString DatapackClientLoader::text_category=QStringLiteral("category");
const QString DatapackClientLoader::text_alpha=QStringLiteral("alpha");
const QString DatapackClientLoader::text_color=QStringLiteral("color");
const QString DatapackClientLoader::text_event=QStringLiteral("event");
const QString DatapackClientLoader::text_value=QStringLiteral("value");

const QString DatapackClientLoader::text_tileheight=QStringLiteral("tileheight");
const QString DatapackClientLoader::text_tilewidth=QStringLiteral("tilewidth");
const QString DatapackClientLoader::text_x=QStringLiteral("x");
const QString DatapackClientLoader::text_y=QStringLiteral("y");
const QString DatapackClientLoader::text_object=QStringLiteral("object");
const QString DatapackClientLoader::text_objectgroup=QStringLiteral("objectgroup");
const QString DatapackClientLoader::text_Object=QStringLiteral("Object");
const QString DatapackClientLoader::text_layer=QStringLiteral("layer");
const QString DatapackClientLoader::text_Dirt=QStringLiteral("Dirt");
const QString DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPBASE=DATAPACK_BASE_PATH_MAPBASE;
QString DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN=DATAPACK_BASE_PATH_MAPMAIN;
QString DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=DATAPACK_BASE_PATH_MAPSUB1 "na" DATAPACK_BASE_PATH_MAPSUB2;

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

    if(!CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
    {
        const std::vector<char> &hash=CatchChallenger::DatapackChecksum::doChecksumBase(datapackPath.toStdString());
        if(hash.empty())
        {
            std::cerr << "DatapackClientLoader::parseDatapack(): hash is empty" << std::endl;
            emit datapackChecksumError();
            inProgress=false;
            return;
        }

        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase!=hash)
        {
            qDebug() << QStringLiteral("DatapackClientLoader::parseDatapack() CommonSettingsCommon::commonSettingsCommon.datapackHashBase!=hash.result(): %1!=%2")
                        .arg(QString::fromStdString(binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase)))
                        .arg(QString::fromStdString(binarytoHexa(hash)));
            emit datapackChecksumError();
            inProgress=false;
            return;
        }
    }

    this->datapackPath=datapackPath;
    DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN=DATAPACK_BASE_PATH_MAPMAIN "na/";
    DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=QString(DATAPACK_BASE_PATH_MAPSUB1)+"na/"+QString(DATAPACK_BASE_PATH_MAPSUB2)+"nabis/";
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/images/inventory/unknown-object.png"));
    #ifndef BOTTESTCONNECT
    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapackPath.toStdString());
    language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    parseVisualCategory();
    parseTypesExtra();
    parseItemsExtra();
    parseSkins();
    parseMonstersExtra();
    parseBuffExtra();
    parseSkillsExtra();
    parsePlantsExtra();
    parseAudioAmbiance();
    parseReputationExtra();
    #endif
    inProgress=false;
    emit datapackParsed();
}

void DatapackClientLoader::parseDatapackMainSub(const QString &mainDatapackCode, const QString &subDatapackCode)
{
    DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN=DATAPACK_BASE_PATH_MAPMAIN+mainDatapackCode+"/";
    DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=DATAPACK_BASE_PATH_MAPSUB1+mainDatapackCode+DATAPACK_BASE_PATH_MAPSUB2+subDatapackCode+"/";

    if(inProgress)
    {
        qDebug() << QStringLiteral("already in progress");
        return;
    }
    inProgress=true;
    this->mainDatapackCode=mainDatapackCode;
    this->subDatapackCode=subDatapackCode;

    if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
    {
        {
            const std::vector<char> &hash=CatchChallenger::DatapackChecksum::doChecksumMain((datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN).toStdString());
            if(hash.empty())
            {
                std::cerr << "DatapackClientLoader::parseDatapackMainSub(): hash is empty" << std::endl;
                emit datapackChecksumError();
                inProgress=false;
                return;
            }

            if(CommonSettingsServer::commonSettingsServer.datapackHashServerMain!=hash)
            {
                qDebug() << QStringLiteral("DatapackClientLoader::parseDatapack() Main CommonSettingsServer::commonSettingsServer.datapackHashServerMain!=hash.result(): %1!=%2")
                            .arg(QString::fromStdString(binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerMain)))
                            .arg(QString::fromStdString(binarytoHexa(hash)));
                emit datapackChecksumError();
                inProgress=false;
                return;
            }
        }
        if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
        {
            const std::vector<char> &hash=CatchChallenger::DatapackChecksum::doChecksumSub((datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB).toStdString());
            if(hash.empty())
            {
                std::cerr << "DatapackClientLoader::parseDatapackSub(): hash is empty" << std::endl;
                emit datapackChecksumError();
                inProgress=false;
                return;
            }

            if(CommonSettingsServer::commonSettingsServer.datapackHashServerSub!=hash)
            {
                qDebug() << QStringLiteral("DatapackClientLoader::parseDatapack() Sub CommonSettingsServer::commonSettingsServer.datapackHashServerSub!=hash.result(): %1!=%2")
                            .arg(QString::fromStdString(binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerSub)))
                            .arg(QString::fromStdString(binarytoHexa(hash)));
                emit datapackChecksumError();
                inProgress=false;
                return;
            }
        }
    }
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/images/inventory/unknown-object.png"));
    CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapack(datapackPath.toStdString(),mainDatapackCode.toStdString(),subDatapackCode.toStdString());

    parseMaps();
    parseQuestsLink();
    parseQuestsExtra();
    parseQuestsText();
    parseBotFightsExtra();
    parseZoneExtra();
    parseTileset();

    inProgress=false;

    emit datapackParsedMainSub();
}

QString DatapackClientLoader::getDatapackPath()
{
    return datapackPath;
}

QString DatapackClientLoader::getMainDatapackPath()
{
    return DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN;
}

QString DatapackClientLoader::getSubDatapackPath()
{
    return DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB;
}

void DatapackClientLoader::parseVisualCategory()
{
    QDomDocument domDocument;
    //open and quick check the file
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPBASE)+QStringLiteral("visualcategory.xml");
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
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
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt[file.toStdString()]=domDocument;
    }
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=DatapackClientLoader::text_visual)
    {
        qDebug() << (QStringLiteral("Unable to open the file: %1, \"visual\" root balise not found for the xml file").arg(file));
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
                                    unsigned int index=0;
                                    while(index<CatchChallenger::CommonDatapack::commonDatapack.events.size())
                                    {
                                        if(CatchChallenger::CommonDatapack::commonDatapack.events.at(index).name==event.attribute(DatapackClientLoader::text_id).toStdString())
                                        {
                                            unsigned int sub_index=0;
                                            while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.events.at(index).values.size())
                                            {
                                                if(CatchChallenger::CommonDatapack::commonDatapack.events.at(index).values.at(sub_index)==event.attribute(DatapackClientLoader::text_value).toStdString())
                                                {
                                                    VisualCategory::VisualCategoryCondition visualCategoryCondition;
                                                    visualCategoryCondition.event=static_cast<uint8_t>(index);
                                                    visualCategoryCondition.eventValue=static_cast<uint8_t>(sub_index);
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
        uint8_t index=0;
        while(index<CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[QString::fromStdString(CatchChallenger::CommonDatapack::commonDatapack.reputation.at(index).name)]=index;
            index++;
        }
    }
    QDomDocument domDocument;
    //open and quick check the file
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLAYERBASE)+QStringLiteral("reputation.xml");
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
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
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt[file.toStdString()]=domDocument;
    }
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="reputations")
    {
        qDebug() << (QStringLiteral("Unable to open the file: %1, \"reputations\" root balise not found for the xml file").arg(file));
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

                QList<int32_t> point_list_positive,point_list_negative;
                QStringList text_positive,text_negative;
                QDomElement level = item.firstChildElement(DatapackClientLoader::text_level);
                ok=true;
                while(!level.isNull() && ok)
                {
                    if(level.isElement())
                    {
                        if(level.hasAttribute(DatapackClientLoader::text_point))
                        {
                            const int32_t &point=level.attribute(DatapackClientLoader::text_point).toInt(&ok);
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
                        //qDebug() << (QStringLiteral("Unable to open the file: %1, no starting level for the negative client: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        QList<int32_t> point_list_negative_new;
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
        unsigned int index=0;
        while(index<CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
        {
            const CatchChallenger::Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(index);
            if(!reputationExtra.contains(QString::fromStdString(reputation.name)))
                reputationExtra[QString::fromStdString(reputation.name)].name=tr("Unknown");
            while((uint32_t)reputationExtra[QString::fromStdString(reputation.name)].reputation_negative.size()<reputation.reputation_negative.size())
                reputationExtra[QString::fromStdString(reputation.name)].reputation_negative << tr("Unknown");
            while((uint32_t)reputationExtra[QString::fromStdString(reputation.name)].reputation_positive.size()<reputation.reputation_positive.size())
                reputationExtra[QString::fromStdString(reputation.name)].reputation_positive << tr("Unknown");
            index++;
        }
    }

    qDebug() << QStringLiteral("%1 reputation(s) extra loaded").arg(reputationExtra.size());
}

void DatapackClientLoader::parseItemsExtra()
{
    QDir dir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM));
    const QStringList &fileList=CatchChallenger::FacilityLibClient::stdvectorstringToQStringList(CatchChallenger::FacilityLibGeneral::listFolder((dir.absolutePath()+DatapackClientLoader::text_slash).toStdString()));
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
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
            domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
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
            CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt[file.toStdString()]=domDocument;
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
                    const uint32_t &tempid=item.attribute(DatapackClientLoader::text_id).toUInt(&ok);
                    if(ok && tempid<65536)
                    {
                        const uint16_t &id=static_cast<uint16_t>(tempid);
                        if(!DatapackClientLoader::itemsExtra.contains(id))
                        {
                            ItemExtra itemExtra;
                            //load the image
                            if(item.hasAttribute(DatapackClientLoader::text_image))
                            {
                                const QString &imagePath=folder+item.attribute(DatapackClientLoader::text_image);
                                QPixmap image(imagePath);
                                if(image.isNull())
                                {
                                    qDebug() << QStringLiteral("Unable to open the items image: %1: child.tagName(): %2 (at line: %3)").arg(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM)+item.attribute(QStringLiteral("image"))).arg(item.tagName()).arg(item.lineNumber());
                                    itemExtra.image=*mDefaultInventoryImage;
                                    itemExtra.imagePath=QStringLiteral(":/images/inventory/unknown-object.png");
                                }
                                else
                                {
                                    itemExtra.imagePath=QFileInfo(imagePath).absoluteFilePath();
                                    itemExtra.image=image;
                                }
                            }
                            else
                            {
                                qDebug() << QStringLiteral("For parse item: Have not image attribute: child.tagName(): %1 (%2 at line: %3)").arg(item.tagName()).arg(file).arg(item.lineNumber());
                                itemExtra.image=*mDefaultInventoryImage;
                                itemExtra.imagePath=QStringLiteral(":/images/inventory/unknown-object.png");
                            }
                            // base size: 24x24
                            itemExtra.image=itemExtra.image.scaled(72,72);//then zoom: 3x

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
                                                itemExtra.name=name.text();
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
                                                itemExtra.name=name.text();
                                                name_found=true;
                                                break;
                                            }
                                        }
                                        name = name.nextSiblingElement(DatapackClientLoader::text_name);
                                    }
                                }
                                if(!name_found)
                                {
                                    itemExtra.name=tr("Unknown object");
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
                                                itemExtra.description=description.text();
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
                                                itemExtra.description=description.text();
                                                description_found=true;
                                                break;
                                            }
                                        }
                                        description = description.nextSiblingElement(DatapackClientLoader::text_description);
                                    }
                                }
                                if(!description_found)
                                {
                                    itemExtra.description=tr("This object is not listed as know object. The information can't be found.");
                                    //qDebug() << QStringLiteral("English description not found for the item with id: %1").arg(id);
                                }
                            }
                            DatapackClientLoader::itemsExtra[id]=itemExtra;
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
    /// \todo do a sub overlay
    const QStringList &returnList=CatchChallenger::FacilityLibClient::stdvectorstringToQStringList(CatchChallenger::FacilityLibGeneral::listFolder((datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN).toStdString()));

    //load the map
    uint16_t pointOnMapIndexItem=0;
    uint16_t pointOnMapIndexPlant=0;
    const int &size=returnList.size();
    int index=0;
    QRegularExpression mapFilter(QStringLiteral("\\.tmx$"));
    QRegularExpression mapExclude(QStringLiteral("[\"']"));
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
    const QString &basePath=datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN;
    index=0;
    while(index<tempMapList.size())
    {
        mapToId[sortToFull.value(tempMapList.at(index))]=index;
        fullMapPathToId[QFileInfo(basePath+sortToFull.value(tempMapList.at(index))).absoluteFilePath()]=index;
        maps << sortToFull.value(tempMapList.at(index));

        const QString &fileName=sortToFull.value(tempMapList.at(index));
        {
            QDomDocument domDocument;
            QFile itemsFile(basePath+fileName);
            if(!itemsFile.open(QIODevice::ReadOnly))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(fileName).arg(itemsFile.errorString());
                index++;
                continue;
            }
            const QByteArray &xmlContent=itemsFile.readAll();
            itemsFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(fileName).arg(errorLine).arg(errorColumn).arg(errorStr);
                index++;
                continue;
            }
            const QDomElement &root = domDocument.documentElement();
            if(root.tagName()!=DatapackClientLoader::text_map)
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, \"map\" root balise not found for the xml file").arg(fileName);
                index++;
                continue;
            }
            bool ok;
            int tilewidth=16;
            int tileheight=16;
            if(root.hasAttribute(DatapackClientLoader::text_tilewidth))
            {
                tilewidth=root.attribute(DatapackClientLoader::text_tilewidth).toUShort(&ok);
                if(!ok)
                {
                    qDebug() << QStringLiteral("Unable to open the file: %1, tilewidth is not a number").arg(fileName);
                    tilewidth=16;
                }
            }
            if(root.hasAttribute(DatapackClientLoader::text_tileheight))
            {
                tileheight=root.attribute(DatapackClientLoader::text_tileheight).toUShort(&ok);
                if(!ok)
                {
                    qDebug() << QStringLiteral("Unable to open the file: %1, tilewidth is not a number").arg(fileName);
                    tileheight=16;
                }
            }

            bool haveDirtLayer=false;
            {
                QDomElement layergroup = root.firstChildElement(DatapackClientLoader::text_layer);
                while(!layergroup.isNull())
                {
                    if(layergroup.isElement())
                    {
                        if(layergroup.hasAttribute(DatapackClientLoader::text_name) && layergroup.attribute(DatapackClientLoader::text_name)==DatapackClientLoader::text_Dirt)
                        {
                            haveDirtLayer=true;
                            break;
                        }
                    }
                    layergroup = layergroup.nextSiblingElement(DatapackClientLoader::text_layer);
                }
            }
            if(haveDirtLayer)
            {
                CatchChallenger::Map_loader mapLoader;
                if(mapLoader.tryLoadMap((basePath+fileName).toStdString()))
                {
                    unsigned int index=0;
                    while(index<mapLoader.map_to_send.dirts.size())
                    {
                        const CatchChallenger::Map_to_send::DirtOnMap_Semi &dirt=mapLoader.map_to_send.dirts.at(index);
                        plantOnMap[basePath+fileName][QPair<uint8_t,uint8_t>(dirt.point.x,dirt.point.y)]=pointOnMapIndexPlant;
                        PlantIndexContent plantIndexContent;
                        plantIndexContent.map=basePath+fileName;
                        plantIndexContent.x=dirt.point.x;
                        plantIndexContent.y=dirt.point.y;
                        plantIndexOfOnMap[pointOnMapIndexPlant]=plantIndexContent;
                        pointOnMapIndexPlant++;
                        index++;
                    }
                }
            }

            //load name
            {
                QDomElement objectgroup = root.firstChildElement(DatapackClientLoader::text_objectgroup);
                while(!objectgroup.isNull())
                {
                    if(objectgroup.isElement())
                    {
                        if(objectgroup.hasAttribute(DatapackClientLoader::text_name) && objectgroup.attribute(DatapackClientLoader::text_name)==DatapackClientLoader::text_Object)
                        {
                            QDomElement object = objectgroup.firstChildElement(DatapackClientLoader::text_object);
                            while(!object.isNull())
                            {
                                if(object.isElement())
                                {
                                    if(
                                            object.hasAttribute(DatapackClientLoader::text_type) && object.attribute(DatapackClientLoader::text_type)==DatapackClientLoader::text_object
                                            && object.hasAttribute(DatapackClientLoader::text_x)
                                            && object.hasAttribute(DatapackClientLoader::text_y)
                                            )
                                    {
                                        /** the -1 is important to fix object layer bug into tiled!!!
                                         * Don't remove! */
                                        const uint32_t &object_y=(object.attribute(DatapackClientLoader::text_y).toUInt(&ok)/tileheight)-1;
                                        if(ok && object_y<256)
                                        {
                                            const uint32_t &object_x=object.attribute(DatapackClientLoader::text_x).toUInt(&ok)/tilewidth;
                                            if(ok && object_x<256)
                                            {
                                                itemOnMap[datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN+fileName]
                                                        [QPair<uint8_t,uint8_t>(static_cast<uint8_t>(object_x),static_cast<uint8_t>(object_y))]=pointOnMapIndexItem;
                                                pointOnMapIndexItem++;
                                            }
                                            else
                                                qDebug() << QStringLiteral("object_y too big or not number");
                                        }
                                        else
                                            qDebug() << QStringLiteral("object_x too big or not number");
                                    }
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(fileName).arg(object.tagName()).arg(object.lineNumber());
                                object = object.nextSiblingElement(DatapackClientLoader::text_object);
                            }
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(fileName).arg(objectgroup.tagName()).arg(objectgroup.lineNumber());
                    objectgroup = objectgroup.nextSiblingElement(DatapackClientLoader::text_objectgroup);
                }
            }
        }
        index++;
    }

    qDebug() << QStringLiteral("%1 map(s) extra loaded").arg(DatapackClientLoader::datapackLoader.maps.size());
}

void DatapackClientLoader::parseSkins()
{
    skins=CatchChallenger::FacilityLibClient::stdvectorstringToQStringList(CatchChallenger::FacilityLibGeneral::skinIdList((datapackPath+DATAPACK_BASE_PATH_SKIN).toStdString()));

    qDebug() << QStringLiteral("%1 skin(s) loaded").arg(skins.size());
}

void DatapackClientLoader::resetAll()
{
    CatchChallenger::CommonDatapack::commonDatapack.unload();
    language.clear();
    mapToId.clear();
    fullMapPathToId.clear();
    visualCategories.clear();
    if(mDefaultInventoryImage==NULL)
    {
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/images/inventory/unknown-object.png"));
        if(mDefaultInventoryImage->isNull())
        {
            qDebug() << "default internal image bug for item";
            abort();
        }
    }
    datapackPath.clear();
    itemsExtra.clear();
    maps.clear();
    skins.clear();

    {
        QHashIterator<uint8_t,PlantExtra> i(plantExtra);
        while (i.hasNext()) {
            i.next();
            delete i.value().tileset;
        }
    }
    itemOnMap.clear();
    plantOnMap.clear();
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
    const QFileInfoList &entryList=QDir(datapackPath+DATAPACK_BASE_PATH_QUESTS1+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+DATAPACK_BASE_PATH_QUESTS2).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
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
        const uint32_t &tempid=entryList.at(index).fileName().toUInt(&ok);
        if(!ok)
        {
            qDebug() << QStringLiteral("Unable to open the folder: %1, because is folder name is not a number").arg(entryList.at(index).fileName());
            index++;
            continue;
        }
        if(tempid>=256)
        {
            qDebug() << QStringLiteral("parseQuestsExtra too big: %1").arg(entryList.at(index).fileName());
            index++;
            continue;
        }
        const uint16_t &id=static_cast<uint16_t>(tempid);

        QDomDocument domDocument;
        const QString &file=entryList.at(index).absoluteFilePath()+DatapackClientLoader::text_slashdefinitiondotxml;
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
            domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
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
            CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt[file.toStdString()]=domDocument;
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

        QHash<uint32_t,QString> steps;
        {
            //load step
            QDomElement step = root.firstChildElement(DatapackClientLoader::text_step);
            while(!step.isNull())
            {
                if(step.isElement())
                {
                    if(step.hasAttribute(DatapackClientLoader::text_id))
                    {
                        const uint32_t &tempid=step.attribute(DatapackClientLoader::text_id).toUInt(&ok);
                        if(ok)
                        {
                            if(tempid<256)
                            {
                                const uint16_t &id=static_cast<uint16_t>(tempid);
                                CatchChallenger::Quest::Step stepObject;
                                if(step.hasAttribute(DatapackClientLoader::text_bot))
                                {
                                    QStringList tempStringList=step.attribute(DatapackClientLoader::text_bot).split(DatapackClientLoader::text_dotcomma);
                                    int index=0;
                                    while(index<tempStringList.size())
                                    {
                                        uint32_t tempInt=tempStringList.at(index).toUInt(&ok);
                                        if(ok && tempInt<65536)
                                            stepObject.bots.push_back(static_cast<uint16_t>(tempInt));
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
    const QFileInfoList &entryList=QDir(datapackPath+DATAPACK_BASE_PATH_QUESTS1+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+DATAPACK_BASE_PATH_QUESTS2).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
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
        QHash<uint32_t,QString> client_logic_texts;
        //load text
        QDomElement client_logic = root.firstChildElement(DatapackClientLoader::text_client_logic);
        while(!client_logic.isNull())
        {
            if(client_logic.isElement())
            {
                if(client_logic.hasAttribute(DatapackClientLoader::text_id))
                {
                    const uint32_t &tempid=client_logic.attribute(DatapackClientLoader::text_id).toUInt(&ok);
                    if(ok && tempid<65536)
                    {
                        const uint16_t &id=static_cast<uint16_t>(tempid);
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
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPBASE)+QStringLiteral("music.xml");
    QDomDocument domDocument;
    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
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
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt[file.toStdString()]=domDocument;
    }
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!="musics")
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"musics\" root balise not found for the xml file").arg(file);
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
                    audioAmbiance[type]=datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN+item.text();
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
    auto i=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.begin();
    while(i!=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
    {
        if(!i->second.steps.empty())
        {
            std::vector<uint16_t> bots=i->second.steps.front().bots;
            unsigned int index=0;
            while(index<bots.size())
            {
                botToQuestStart.insert(bots.at(index),i->first);
                index++;
            }
        }
        ++i;
    }
    qDebug() << QStringLiteral("%1 bot linked with quest(s) loaded").arg(botToQuestStart.size());
}

void DatapackClientLoader::parseZoneExtra()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(datapackPath+DATAPACK_BASE_PATH_ZONE1+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+DATAPACK_BASE_PATH_ZONE2).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
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

        //load the audio ambiance
        {
            QDomElement item = root.firstChildElement(DatapackClientLoader::text_music);
            while(!item.isNull())
            {
                if(item.isElement())
                {
                    if(item.hasAttribute(DatapackClientLoader::text_type) && item.hasAttribute(DatapackClientLoader::text_backgroundsound))
                    {
                        const QString &type=item.attribute(DatapackClientLoader::text_type);
                        const QString &backgroundsound=item.attribute(DatapackClientLoader::text_backgroundsound);
                        zonesExtra[zoneCodeName].audioAmbiance[type]=backgroundsound;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, have not the music attribute: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber());
                item = item.nextSiblingElement(DatapackClientLoader::text_music);
            }
        }

        index++;
    }

    qDebug() << QStringLiteral("%1 zone(s) extra loaded").arg(zonesExtra.size());
}

void DatapackClientLoader::parseTileset()
{
    const QStringList &fileList=CatchChallenger::FacilityLibClient::stdvectorstringToQStringList(CatchChallenger::FacilityLibGeneral::listFolder((datapackPath+DATAPACK_BASE_PATH_MAPBASE).toStdString()));
    int index=0;
    while(index<fileList.size())
    {
        const QString &filePath=fileList.at(index);
        if(filePath.endsWith(DatapackClientLoader::text_dottsx))
        {
            const QString &source=QFileInfo(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPBASE)+filePath).absoluteFilePath();
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
