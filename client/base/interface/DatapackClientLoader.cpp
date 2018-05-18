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
#include <QByteArray>
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <iostream>

DatapackClientLoader DatapackClientLoader::datapackLoader;

const std::string DatapackClientLoader::text_list="list";
const std::string DatapackClientLoader::text_reputation="reputation";
const std::string DatapackClientLoader::text_type="type";
const std::string DatapackClientLoader::text_name="name";
const std::string DatapackClientLoader::text_en="en";
const std::string DatapackClientLoader::text_lang="lang";
const std::string DatapackClientLoader::text_level="level";
const std::string DatapackClientLoader::text_point="point";
const std::string DatapackClientLoader::text_text="text";
const std::string DatapackClientLoader::text_id="id";
const std::string DatapackClientLoader::text_image="image";
const std::string DatapackClientLoader::text_description="description";
const std::string DatapackClientLoader::text_item="item";
const std::string DatapackClientLoader::text_slashdefinitiondotxml="/definition.xml";
const std::string DatapackClientLoader::text_quest="quest";
const std::string DatapackClientLoader::text_rewards="rewards";
const std::string DatapackClientLoader::text_show="show";
const std::string DatapackClientLoader::text_autostep="autostep";
const std::string DatapackClientLoader::text_yes="yes";
const std::string DatapackClientLoader::text_true="true";
const std::string DatapackClientLoader::text_step="step";
const std::string DatapackClientLoader::text_bot="bot";
const std::string DatapackClientLoader::text_dotcomma=";";
const std::string DatapackClientLoader::text_client_logic="client_logic";
const std::string DatapackClientLoader::text_map="map";
const std::string DatapackClientLoader::text_items="items";
const std::string DatapackClientLoader::text_zone="zone";
const std::string DatapackClientLoader::text_music="music";
const std::string DatapackClientLoader::text_backgroundsound="backgroundsound";

const std::string DatapackClientLoader::text_monster="monster";
const std::string DatapackClientLoader::text_monsters="monsters";
const std::string DatapackClientLoader::text_kind="kind";
const std::string DatapackClientLoader::text_habitat="habitat";
const std::string DatapackClientLoader::text_slash="/";
const std::string DatapackClientLoader::text_types="types";
const std::string DatapackClientLoader::text_buff="buff";
const std::string DatapackClientLoader::text_skill="skill";
const std::string DatapackClientLoader::text_buffs="buffs";
const std::string DatapackClientLoader::text_skills="skills";
const std::string DatapackClientLoader::text_fight="fight";
const std::string DatapackClientLoader::text_fights="fights";
const std::string DatapackClientLoader::text_start="start";
const std::string DatapackClientLoader::text_win="win";
const std::string DatapackClientLoader::text_dotxml=".xml";
const std::string DatapackClientLoader::text_dottsx=".tsx";
const std::string DatapackClientLoader::text_visual="visual";
const std::string DatapackClientLoader::text_category="category";
const std::string DatapackClientLoader::text_alpha="alpha";
const std::string DatapackClientLoader::text_color="color";
const std::string DatapackClientLoader::text_event="event";
const std::string DatapackClientLoader::text_value="value";

const std::string DatapackClientLoader::text_tileheight="tileheight";
const std::string DatapackClientLoader::text_tilewidth="tilewidth";
const std::string DatapackClientLoader::text_x="x";
const std::string DatapackClientLoader::text_y="y";
const std::string DatapackClientLoader::text_object="object";
const std::string DatapackClientLoader::text_objectgroup="objectgroup";
const std::string DatapackClientLoader::text_Object="Object";
const std::string DatapackClientLoader::text_layer="layer";
const std::string DatapackClientLoader::text_Dirt="Dirt";
const std::string DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPBASE=DATAPACK_BASE_PATH_MAPBASE;
std::string DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN=DATAPACK_BASE_PATH_MAPMAIN;
std::string DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=DATAPACK_BASE_PATH_MAPSUB1 "na" DATAPACK_BASE_PATH_MAPSUB2;

DatapackClientLoader::DatapackClientLoader()
{
    mDefaultInventoryImage=NULL;
    inProgress=false;
    start();
    moveToThread(this);
    setObjectName("DatapackClientLoader");
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

void DatapackClientLoader::parseDatapack(const std::string &datapackPath)
{
    if(inProgress)
    {
        qDebug() << QStringLiteral("already in progress");
        return;
    }
    inProgress=true;

    if(!CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
    {
        const std::vector<char> &hash=CatchChallenger::DatapackChecksum::doChecksumBase(datapackPath);
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
    DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=std::string(DATAPACK_BASE_PATH_MAPSUB1)+"na/"+std::string(DATAPACK_BASE_PATH_MAPSUB2)+"nabis/";
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/images/inventory/unknown-object.png"));
    #ifndef BOTTESTCONNECT
    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapackPath);
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

void DatapackClientLoader::parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode)
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
            const std::vector<char> &hash=CatchChallenger::DatapackChecksum::doChecksumMain((datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN));
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
            const std::vector<char> &hash=CatchChallenger::DatapackChecksum::doChecksumSub(
                        (datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB));
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
    CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapack(
                datapackPath,mainDatapackCode,subDatapackCode);

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

std::string DatapackClientLoader::getDatapackPath()
{
    return datapackPath;
}

std::string DatapackClientLoader::getMainDatapackPath()
{
    return DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN;
}

std::string DatapackClientLoader::getSubDatapackPath()
{
    return DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB;
}

void DatapackClientLoader::parseVisualCategory()
{
    tinyxml2::XMLDocument *domDocument=NULL;
    //open and quick check the file
    const std::string &file=datapackPath+DATAPACK_BASE_PATH_MAPBASE+"visualcategory.xml";
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=
            CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return;
        }
    }
    const tinyxml2::XMLElement *root = domDocument->RootElement();
    if(root==NULL || strcmp(root->Name(),"visual")!=0)
    {
        qDebug() << (QStringLiteral("Unable to open the file: %1, \"visual\" root balise not found for the xml file").arg(QString::fromStdString(file)));
        return;
    }

    //load the content
    const tinyxml2::XMLElement *item = root->FirstChildElement("category");
    while(item!=NULL)
    {
        if(item->Attribute("id")!=NULL)
        {
            if(strcmp(item->Attribute("id"),"")!=0)
            {
                if(DatapackClientLoader::datapackLoader.visualCategories.find(item->Attribute("id"))==
                        DatapackClientLoader::datapackLoader.visualCategories.cend())
                {
                    bool ok;
                    DatapackClientLoader::datapackLoader.visualCategories[item->Attribute("id")].defaultColor=Qt::transparent;
                    int alpha=255;
                    if(item->Attribute("alpha")!=NULL)
                    {
                        alpha=stringtouint8(item->Attribute("alpha"),&ok);
                        if(!ok || alpha>255)
                        {
                            alpha=255;
                            qDebug() << (QStringLiteral("Unable to open the file: %1, alpha is not number or greater than 255: child.Name(): %2")
                                         .arg(QString::fromStdString(file))
                                         .arg(item->Name())
                                         );
                        }
                    }
                    if(item->Attribute("color")!=NULL)
                    {
                        QColor color;
                        color.setNamedColor(item->Attribute("color"));
                        if(color.isValid())
                        {
                            color.setAlpha(alpha);
                            DatapackClientLoader::datapackLoader.visualCategories[item->Attribute("id")].defaultColor=color;
                        }
                        else
                            qDebug() << (QStringLiteral("Unable to open the file: %1, color is not valid: child.Name(): %2")
                                         .arg(QString::fromStdString(file))
                                         .arg(item->Name())
                                         );
                    }
                    const tinyxml2::XMLElement *event = item->FirstChildElement("event");
                    while(event!=NULL)
                    {
                        if(event->Attribute("id")!=NULL && event->Attribute("value")!=NULL)
                        {
                            unsigned int index=0;
                            while(index<CatchChallenger::CommonDatapack::commonDatapack.events.size())
                            {
                                if(CatchChallenger::CommonDatapack::commonDatapack.events.at(index).name==
                                        event->Attribute("id"))
                                {
                                    unsigned int sub_index=0;
                                    while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.events.at(index).values.size())
                                    {
                                        if(CatchChallenger::CommonDatapack::commonDatapack.events.at(index).values.at(sub_index)==
                                                event->Attribute("value"))
                                        {
                                            VisualCategory::VisualCategoryCondition visualCategoryCondition;
                                            visualCategoryCondition.event=static_cast<uint8_t>(index);
                                            visualCategoryCondition.eventValue=static_cast<uint8_t>(sub_index);
                                            visualCategoryCondition.color=DatapackClientLoader::datapackLoader.visualCategories.at(
                                                        item->Attribute("id")).defaultColor;
                                            int alpha=255;
                                            if(event->Attribute("alpha")!=NULL)
                                            {
                                                alpha=stringtouint32(event->Attribute("alpha"),&ok);
                                                if(!ok || alpha>255)
                                                {
                                                    alpha=255;
                                                    qDebug() << (QStringLiteral("Unable to open the file: %1, alpha is not number or greater than 255: child.Name(): %2")
                                                                 .arg(QString::fromStdString(file)).arg(QString::fromStdString(event->Name())));
                                                }
                                            }
                                            if(event->Attribute("color")!=NULL)
                                            {
                                                QColor color;
                                                color.setNamedColor(event->Attribute("color"));
                                                if(color.isValid())
                                                    visualCategoryCondition.color=QColor(color.red(),color.green(),color.blue(),alpha);
                                                else
                                                    qDebug() << (QStringLiteral("Unable to open the file: %1, color is not valid: child.Name(): %2")
                                                                 .arg(QString::fromStdString(file)).arg(QString::fromStdString(event->Name())));
                                            }
                                            if(visualCategoryCondition.color!=DatapackClientLoader::datapackLoader.visualCategories.at(
                                                        item->Attribute("id")).defaultColor)
                                                DatapackClientLoader::datapackLoader.visualCategories[item->Attribute("id")]
                                                        .conditions.push_back(visualCategoryCondition);
                                            else
                                                qDebug() << (QStringLiteral("Unable to open the file: %1, color same than the default color: child.Name(): %2").arg(QString::fromStdString(file)).arg(event->Name()));
                                            break;
                                        }
                                        sub_index++;
                                    }
                                    if(sub_index==CatchChallenger::CommonDatapack::commonDatapack.events.at(index).values.size())
                                        qDebug() << (QStringLiteral("Unable to open the file: %1, event value not found: child.Name(): %2").arg(QString::fromStdString(file)).arg(event->Name()));
                                    break;
                                }
                                index++;
                            }
                            if(index==CatchChallenger::CommonDatapack::commonDatapack.events.size())
                                qDebug() << (QStringLiteral("Unable to open the file: %1, event not found: child.Name(): %2").arg(QString::fromStdString(file)).arg(event->Name()));
                        }
                        else
                            qDebug() << (QStringLiteral("Unable to open the file: %1, attribute id is already found: child.Name(): %2").arg(QString::fromStdString(file)).arg(event->Name()));
                        event = event->NextSiblingElement("event");
                    }
                }
                else
                    qDebug() << (QStringLiteral("Unable to open the file: %1, attribute id is already found: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
            }
            else
                qDebug() << (QStringLiteral("Unable to open the file: %1, attribute id can't be empty: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
        }
        else
            qDebug() << (QStringLiteral("Unable to open the file: %1, have not the attribute id: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
        item = item->NextSiblingElement("category");
    }

    qDebug() << QStringLiteral("%1 visual cat loaded").arg(DatapackClientLoader::datapackLoader.visualCategories.size());
}

void DatapackClientLoader::parseReputationExtra()
{
    {
        uint8_t index=0;
        while(index<CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CatchChallenger::CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    tinyxml2::XMLDocument *domDocument=NULL;
    //open and quick check the file
    const std::string &file=datapackPath+DATAPACK_BASE_PATH_PLAYERBASE+"reputation.xml";
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return;
        }
    }
    const tinyxml2::XMLElement *root = domDocument->RootElement();
    if(root==NULL || strcmp(root->Name(),"reputations")!=0)
    {
        qDebug() << (QStringLiteral("Unable to open the file: %1, \"reputations\" root balise not found for the xml file").arg(QString::fromStdString(file)));
        return;
    }

    //load the content
    bool ok;
    const tinyxml2::XMLElement *item = root->FirstChildElement("reputation");
    while(item!=NULL)
    {
        if(item->Attribute("type")!=NULL)
        {
            ok=true;

            //load the name
            {
                bool name_found=false;
                const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                if(!language.empty() && language!="en")
                    while(name!=NULL)
                    {
                        if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language)
                        {
                            reputationExtra[item->Attribute("type")].name=name->GetText();
                            name_found=true;
                            break;
                        }
                        name = name->NextSiblingElement("name");
                    }
                if(!name_found)
                {
                    name = item->FirstChildElement("name");
                    while(name!=NULL)
                    {
                        if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                        {
                            reputationExtra[item->Attribute("type")].name=name->GetText();
                            name_found=true;
                            break;
                        }
                        name = name->NextSiblingElement("name");
                    }
                }
                if(!name_found)
                {
                    reputationExtra[item->Attribute("type")].name=tr("Unknown").toStdString();
                    qDebug() << QStringLiteral("English name not found for the reputation with id: %1")
                                .arg(item->Attribute("type"));
                }
            }

            std::vector<int32_t> point_list_positive,point_list_negative;
            std::vector<std::string> text_positive,text_negative;
            const tinyxml2::XMLElement *level = item->FirstChildElement("level");
            ok=true;
            while(level!=NULL && ok)
            {
                if(level->Attribute("point")!=NULL)
                {
                    const int32_t &point=stringtoint32(level->Attribute("point"),&ok);
                    //std::string text_val;
                    if(ok)
                    {
                        ok=true;

                        std::string text;
                        //load the name
                        {
                            bool name_found=false;
                            const tinyxml2::XMLElement *name = level->FirstChildElement("text");
                            if(!language.empty() && language!="en")
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang") && name->Attribute("lang")==language)
                                    {
                                        text=name->GetText();
                                        name_found=true;
                                        break;
                                    }
                                    name = name->NextSiblingElement("text");
                                }
                            if(!name_found)
                            {
                                name = level->FirstChildElement("text");
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                    {
                                        text=name->GetText();
                                        name_found=true;
                                        break;
                                    }
                                    name = name->NextSiblingElement("text");
                                }
                            }
                            if(!name_found)
                            {
                                text=tr("Unknown").toStdString();
                                qDebug() << QStringLiteral("English name not found for the reputation with id: %1").arg(item->Attribute("type"));
                            }
                        }

                        bool found=false;
                        unsigned int index=0;
                        if(point>=0)
                        {
                            while(index<point_list_positive.size())
                            {
                                if(point_list_positive.at(index)==point)
                                {
                                    qDebug() << (QStringLiteral("Unable to open the file: %1, reputation level with same number of point found!: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
                                    found=true;
                                    ok=false;
                                    break;
                                }
                                if(point_list_positive.at(index)>point)
                                {
                                    point_list_positive.insert(point_list_positive.cbegin()+index,point);
                                    text_positive.insert(text_positive.cbegin()+index,text);
                                    found=true;
                                    break;
                                }
                                index++;
                            }
                            if(!found)
                            {
                                point_list_positive.push_back(point);
                                text_positive.push_back(text);
                            }
                        }
                        else
                        {
                            while(index<point_list_negative.size())
                            {
                                if(point_list_negative.at(index)==point)
                                {
                                    qDebug() << (QStringLiteral("Unable to open the file: %1, reputation level with same number of point found!: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
                                    found=true;
                                    ok=false;
                                    break;
                                }
                                if(point_list_negative.at(index)<point)
                                {
                                    point_list_negative.insert(point_list_negative.cbegin()+index,point);
                                    text_negative.insert(text_negative.cbegin()+index,text);
                                    found=true;
                                    break;
                                }
                                index++;
                            }
                            if(!found)
                            {
                                point_list_negative.push_back(point);
                                text_negative.push_back(text);
                            }
                        }
                    }
                    else
                        qDebug() << (QStringLiteral("Unable to open the file: %1, point is not number: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
                }
                level = level->NextSiblingElement("level");
            }
            qSort(point_list_positive);
            qSort(point_list_negative.end(),point_list_negative.begin());
            if(ok)
                if(point_list_positive.size()<2)
                {
                    qDebug() << (QStringLiteral("Unable to open the file: %1, reputation have to few level: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
                    ok=false;
                }
            if(ok)
                if(!vectorcontainsAtLeastOne(point_list_positive,0))
                {
                    qDebug() << (QStringLiteral("Unable to open the file: %1, no starting level for the positive: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
                    ok=false;
                }
            if(ok)
                if(!point_list_negative.empty() && !vectorcontainsAtLeastOne(point_list_negative,-1))
                {
                    //qDebug() << (QStringLiteral("Unable to open the file: %1, no starting level for the negative client: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
                    std::vector<int32_t> point_list_negative_new;
                    int lastValue=-1;
                    unsigned int index=0;
                    while(index<point_list_negative.size())
                    {
                        point_list_negative_new.push_back(lastValue);
                        lastValue=point_list_negative.at(index);//(1 less to negative value)
                        index++;
                    }
                    point_list_negative=point_list_negative_new;
                }
            if(ok)
                if(!QString(item->Attribute("type")).contains(QRegExp("^[a-z]{1,32}$")))
                {
                    qDebug() << (QStringLiteral("Unable to open the file: %1, the type %4 don't match wiuth the regex: ^[a-z]{1,32}$: child.Name(): %2")
                                 .arg(QString::fromStdString(file))
                                 .arg(item->Name()).arg(item->Attribute("type")));
                    ok=false;
                }
            if(ok)
            {
                reputationExtra[item->Attribute("type")].reputation_positive=text_positive;
                reputationExtra[item->Attribute("type")].reputation_negative=text_negative;
            }
        }
        else
            qDebug() << (QStringLiteral("Unable to open the file: %1, have not the item id: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name()));
        item = item->NextSiblingElement("reputation");
    }
    {
        unsigned int index=0;
        while(index<CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
        {
            const CatchChallenger::Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(index);
            if(reputationExtra.find(reputation.name)==reputationExtra.cend())
                reputationExtra[reputation.name].name=tr("Unknown").toStdString();
            while((uint32_t)reputationExtra[reputation.name].reputation_negative.size()<reputation.reputation_negative.size())
                reputationExtra[reputation.name].reputation_negative.push_back(tr("Unknown").toStdString());
            while((uint32_t)reputationExtra[reputation.name].reputation_positive.size()<reputation.reputation_positive.size())
                reputationExtra[reputation.name].reputation_positive.push_back(tr("Unknown").toStdString());
            index++;
        }
    }

    qDebug() << QStringLiteral("%1 reputation(s) extra loaded").arg(reputationExtra.size());
}

void DatapackClientLoader::parseItemsExtra()
{
    QDir dir(QString::fromStdString(datapackPath)+QStringLiteral(DATAPACK_BASE_PATH_ITEM));
    const std::vector<std::string> &fileList=CatchChallenger::FacilityLibGeneral::listFolder(
                (dir.absolutePath().toStdString()+DatapackClientLoader::text_slash));
    unsigned int file_index=0;
    while(file_index<fileList.size())
    {
        const std::string &file=datapackPath+DATAPACK_BASE_PATH_ITEM+fileList.at(file_index);
        if(!QFileInfo(QString::fromStdString(file)).isFile())
        {
            file_index++;
            continue;
        }
        tinyxml2::XMLDocument *domDocument=NULL;
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        //open and quick check the file
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
        else
        {
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
            const auto loadOkay = domDocument->LoadFile(file.c_str());
            if(loadOkay!=0)
            {
                std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
                file_index++;
                continue;
            }
        }
        const tinyxml2::XMLElement *root = domDocument->RootElement();
        if(root==NULL || strcmp(root->Name(),"items")!=0)
        {
            //qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(QString::fromStdString(file));
            file_index++;
            continue;
        }

        const std::string &folder=QFileInfo(QString::fromStdString(file)).absolutePath().toStdString()+
                DatapackClientLoader::text_slash;
        //load the content
        bool ok;
        const tinyxml2::XMLElement *item = root->FirstChildElement("item");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                const uint32_t &tempid=stringtouint32(item->Attribute("id"),&ok);
                if(ok && tempid<65536)
                {
                    const uint16_t &id=static_cast<uint16_t>(tempid);
                    if(DatapackClientLoader::itemsExtra.find(id)==DatapackClientLoader::itemsExtra.cend())
                    {
                        ItemExtra itemExtra;
                        //load the image
                        if(item->Attribute("image")!=NULL)
                        {
                            const std::string &imagePath=folder+item->Attribute("image");
                            QPixmap image(QString::fromStdString(imagePath));
                            if(image.isNull())
                            {
                                qDebug() << QStringLiteral("Unable to open the items image: %1: child.Name(): %2")
                                            .arg(QString::fromStdString(datapackPath)+
                                                 QStringLiteral(DATAPACK_BASE_PATH_ITEM)+
                                                 item->Attribute("image")).arg(item->Name());
                                itemExtra.image=*mDefaultInventoryImage;
                                itemExtra.imagePath=":/images/inventory/unknown-object.png";
                            }
                            else
                            {
                                itemExtra.imagePath=QFileInfo(QString::fromStdString(imagePath))
                                        .absoluteFilePath().toStdString();
                                itemExtra.image=image;
                            }
                        }
                        else
                        {
                            qDebug() << QStringLiteral("For parse item: Have not image attribute: child.Name(): %1 (%2)")
                                        .arg(item->Name())
                                        .arg(QString::fromStdString(file));
                            itemExtra.image=*mDefaultInventoryImage;
                            itemExtra.imagePath=":/images/inventory/unknown-object.png";
                        }
                        // base size: 24x24
                        itemExtra.image=itemExtra.image.scaled(72,72);//then zoom: 3x

                        //load the name
                        {
                            bool name_found=false;
                            const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                            if(!language.empty() && language!="en")
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang") && name->Attribute("lang")==language)
                                    {
                                        itemExtra.name=name->GetText();
                                        name_found=true;
                                        break;
                                    }
                                    name = name->NextSiblingElement("name");
                                }
                            if(!name_found)
                            {
                                name = item->FirstChildElement("name");
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                    {
                                        itemExtra.name=name->GetText();
                                        name_found=true;
                                        break;
                                    }
                                    name = name->NextSiblingElement("name");
                                }
                            }
                            if(!name_found)
                            {
                                itemExtra.name=tr("Unknown object").toStdString();
                                qDebug() << QStringLiteral("English name not found for the item with id: %1").arg(id);
                            }
                        }

                        //load the description
                        {
                            bool description_found=false;
                            const tinyxml2::XMLElement *description = item->FirstChildElement("description");
                            if(!language.empty() && language!="en")
                                while(description!=NULL)
                                {
                                    if(description->Attribute("lang") && description->Attribute("lang")==language)
                                    {
                                        itemExtra.description=description->GetText();
                                        description_found=true;
                                        break;
                                    }
                                    description = description->NextSiblingElement("description");
                                }
                            if(!description_found)
                            {
                                description = item->FirstChildElement("description");
                                while(description!=NULL)
                                {
                                    if(description->Attribute("lang")==NULL || strcmp(description->Attribute("lang"),"en")==0)
                                    {
                                        itemExtra.description=description->GetText();
                                        description_found=true;
                                        break;
                                    }
                                    description = description->NextSiblingElement("description");
                                }
                            }
                            if(!description_found)
                            {
                                itemExtra.description=tr("This object is not listed as know object. The information can't be found.").toStdString();
                                //qDebug() << QStringLiteral("English description not found for the item with id: %1").arg(id);
                            }
                        }
                        DatapackClientLoader::itemsExtra[id]=itemExtra;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, id number already set: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name());
            item = item->NextSiblingElement("item");
        }
        file_index++;
    }

    qDebug() << QStringLiteral("%1 item(s) extra loaded").arg(DatapackClientLoader::itemsExtra.size());
}

void DatapackClientLoader::parseMaps()
{
    /// \todo do a sub overlay
    const std::vector<std::string> &returnList=CatchChallenger::FacilityLibClient::stdvectorstringToQStringList(
                CatchChallenger::FacilityLibGeneral::listFolder((datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN)));

    //load the map
    uint16_t pointOnMapIndexItem=0;
    uint16_t pointOnMapIndexPlant=0;
    const int &size=returnList.size();
    int index=0;
    std::unordered_map<std::string,std::string> sortToFull;
    std::vector<std::string> tempMapList;
    while(index<size)
    {
        const std::string &fileName=returnList.at(index);
        std::string sortFileName=fileName;
        if(stringEndsWith(fileName,".tmx") &&
                fileName.find("\"") == std::string::npos &&
                fileName.find("'\\'") == std::string::npos
                )
        {
            sortFileName.resize(sortFileName.size()-4);
            sortToFull[sortFileName]=fileName;
            tempMapList.push_back(sortFileName);
        }
        index++;
    }
    qSort(tempMapList);
    const std::string &basePath=datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN;
    index=0;
    while(index<tempMapList.size())
    {
        mapToId[sortToFull.at(tempMapList.at(index))]=index;
        fullMapPathToId[QFileInfo(basePath+sortToFull.at(tempMapList.at(index)))
                .absoluteFilePath().toStdString()]=index;
        maps.push_back(sortToFull.at(tempMapList.at(index)));

        const std::string &file=sortToFull.at(tempMapList.at(index));
        {
            tinyxml2::XMLDocument *domDocument=NULL;

            const auto loadOkay = domDocument->LoadFile((basePath+file).c_str());
            if(loadOkay!=0)
            {
                std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
                index++;
                continue;
            }

            const tinyxml2::XMLElement *root = domDocument->RootElement();
            if(root==NULL || strcmp(root->Name(),"map")!=0)
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, \"map\" root balise not found for the xml file").arg(QString::fromStdString(file));
                index++;
                continue;
            }
            bool ok;
            int tilewidth=16;
            int tileheight=16;
            if(root->Attribute("tilewidth")!=NULL)
            {
                tilewidth=stringtouint8(root->Attribute("tilewidth"),&ok);
                if(!ok)
                {
                    qDebug() << QStringLiteral("Unable to open the file: %1, tilewidth is not a number").arg(QString::fromStdString(file));
                    tilewidth=16;
                }
            }
            if(root.hasAttribute(DatapackClientLoader::text_tileheight))
            {
                tileheight=root.attribute(DatapackClientLoader::text_tileheight).toUShort(&ok);
                if(!ok)
                {
                    qDebug() << QStringLiteral("Unable to open the file: %1, tilewidth is not a number").arg(QString::fromStdString(file));
                    tileheight=16;
                }
            }

            bool haveDirtLayer=false;
            {
                const tinyxml2::XMLElement *layergroup = root->FirstChildElement("layer");
                while(layergroup!=NULL)
                {
                    if(layergroup->Attribute("name") &&
                            strcmp(layergroup.attribute("name"),"Dirt")==0)
                    {
                        haveDirtLayer=true;
                        break;
                    }
                    layergroup = layergroup->NextSiblingElement("layer");
                }
            }
            if(haveDirtLayer)
            {
                CatchChallenger::Map_loader mapLoader;
                if(mapLoader.tryLoadMap(basePath+file))
                {
                    unsigned int index=0;
                    while(index<mapLoader.map_to_send.dirts.size())
                    {
                        const CatchChallenger::Map_to_send::DirtOnMap_Semi &dirt=mapLoader.map_to_send.dirts.at(index);
                        plantOnMap[basePath+file][std::pair<uint8_t,uint8_t>(dirt.point.x,dirt.point.y)]=pointOnMapIndexPlant;
                        PlantIndexContent plantIndexContent;
                        plantIndexContent.map=basePath+file;
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
                const tinyxml2::XMLElement *objectgroup = root->FirstChildElement("objectgroup");
                while(!objectgroup.isNull())
                {
                    if(objectgroup.isElement())
                    {
                        if(objectgroup->Attribute("name")!=NULL &&
                                objectgroup.attribute(DatapackClientLoader::text_name)==DatapackClientLoader::text_Object)
                        {
                            tinyxml2::XMLElement object = objectgroup.FirstChildElement(DatapackClientLoader::text_object);
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
                                                itemOnMap[datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN+file]
                                                        [std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(object_x),static_cast<uint8_t>(object_y))]=pointOnMapIndexItem;
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
                                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(file).arg(object->Name());
                                object = object.NextSiblingElement(DatapackClientLoader::text_object);
                            }
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(file).arg(objectgroup->Name());
                    objectgroup = objectgroup.NextSiblingElement(DatapackClientLoader::text_objectgroup);
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

        tinyxml2::XMLDocument *domDocument=NULL;
        const std::string &file=entryList.at(index).absoluteFilePath()+DatapackClientLoader::text_slashdefinitiondotxml;
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
        else
        {
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
            const auto loadOkay = domDocument->LoadFile(file.c_str());
            if(loadOkay!=0)
            {
                std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
                index++;
                continue;
            }
        }
        const tinyxml2::XMLElement &root = domDocument.RootElement();
        if(root==NULL || strcmp(root->Name(),"quest")!=0)
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(QString::fromStdString(file));
            index++;
            continue;
        }

        DatapackClientLoader::QuestExtra quest;

        //load name
        {
            tinyxml2::XMLElement name = root.FirstChildElement(DatapackClientLoader::text_name);
            bool found=false;
            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(name.hasAttribute("lang") && name.attribute("lang")==language)
                        {
                            quest.name=name->GetText();
                            found=true;
                            break;
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(name->Name());
                    }
                    name = name.NextSiblingElement(DatapackClientLoader::text_name);
                }
            if(!found)
            {
                name = root.FirstChildElement(DatapackClientLoader::text_name);
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(!name.hasAttribute("lang") || name.attribute("lang")==DatapackClientLoader::text_en)
                        {
                            quest.name=name->GetText();
                            break;
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(name->Name());
                    }
                    name = name.NextSiblingElement(DatapackClientLoader::text_name);
                }
            }
        }

        //load showRewards
        {
            const tinyxml2::XMLElement &rewards = root.FirstChildElement(DatapackClientLoader::text_rewards);
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

        std::unordered_map<uint32_t,QString> steps;
        {
            //load step
            tinyxml2::XMLElement step = root.FirstChildElement(DatapackClientLoader::text_step);
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
                                tinyxml2::XMLElement stepItem = step.FirstChildElement("text");
                                bool found=false;
                                if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                {
                                    while(!stepItem.isNull())
                                    {
                                        if(stepItem.isElement())
                                        {
                                            if(stepItem.hasAttribute("lang") || stepItem.attribute("lang")==language)
                                            {
                                                found=true;
                                                steps[id]=stepItem->GetText();
                                            }
                                        }
                                        else
                                            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(step->Name());
                                        stepItem = stepItem.NextSiblingElement("text");
                                    }
                                }
                                if(!found)
                                {
                                    stepItem = step.FirstChildElement("text");
                                    while(!stepItem.isNull())
                                    {
                                        if(stepItem.isElement())
                                        {
                                            if(!stepItem.hasAttribute("lang") || stepItem.attribute("lang")==DatapackClientLoader::text_en)
                                                steps[id]=stepItem->GetText();
                                            /*else can be into another lang
                                                qDebug() << QStringLiteral("Has attribute: %1, is not lang en: child.Name(): %2").arg(QString::fromStdString(file)).arg(stepItem->Name()).arg(stepItem.lineNumber());*/
                                        }
                                        else
                                            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(stepItem->Name());
                                        stepItem = stepItem.NextSiblingElement("text");
                                    }
                                    if(!steps.contains(id))
                                        steps[id]=tr("No text");
                                }
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.Name(): %2").arg(QString::fromStdString(file)).arg(step->Name());
                        }
                        else
                            qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.Name(): %2").arg(QString::fromStdString(file)).arg(step->Name());
                    }
                    else
                        qDebug() << QStringLiteral("Has attribute: %1, have not id attribute: child.Name(): %2").arg(QString::fromStdString(file)).arg(step->Name());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(step->Name());
                step = step.NextSiblingElement(DatapackClientLoader::text_step);
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
        const std::string &file=entryList.at(index).absoluteFilePath()+QStringLiteral("/text.xml");

        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            index++;
            continue;
        }

        const tinyxml2::XMLElement &root = domDocument.RootElement();
        if(root==NULL || strcmp(root->Name(),"text")!=0)
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"quest\" root balise not found for the xml file").arg(QString::fromStdString(file));
            index++;
            continue;
        }

        //load the content
        bool ok;
        std::unordered_map<uint32_t,QString> client_logic_texts;
        //load text
        tinyxml2::XMLElement client_logic = root.FirstChildElement(DatapackClientLoader::text_client_logic);
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
                        tinyxml2::XMLElement text = client_logic.FirstChildElement("text");
                        bool found=false;
                        if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                            while(!text.isNull())
                            {
                                if(text.isElement())
                                {
                                    if(text.hasAttribute("lang") && text.attribute("lang")==language)
                                    {
                                        client_logic_texts[id]=text->GetText();
                                        found=true;
                                    }
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(client_logic->Name());
                                text = text.NextSiblingElement("text");
                            }
                        if(!found)
                        {
                            text = client_logic.FirstChildElement("text");
                            while(!text.isNull())
                            {
                                if(text.isElement())
                                {
                                    if(!text.hasAttribute("lang") || text.attribute("lang")==DatapackClientLoader::text_en)
                                        client_logic_texts[id]=text->GetText();
                                    /*else can be another language than english
                                        qDebug() << QStringLiteral("Has attribute: %1, is not lang en: child.Name(): %2").arg(QString::fromStdString(file)).arg(text->Name()).arg(text.lineNumber());*/
                                }
                                else
                                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(text->Name());
                                text = text.NextSiblingElement("text");
                            }
                            if(!client_logic_texts.contains(id))
                                client_logic_texts[id]=tr("No text");
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.Name(): %2").arg(QString::fromStdString(file)).arg(client_logic->Name());
                }
                else
                    qDebug() << QStringLiteral("Has attribute: %1, have not id attribute: child.Name(): %2").arg(QString::fromStdString(file)).arg(client_logic->Name());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(client_logic->Name()).arg(client_logic.lineNumber());
            client_logic = client_logic.NextSiblingElement(DatapackClientLoader::text_client_logic);
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
    const std::string &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPBASE)+QStringLiteral("music.xml");
    tinyxml2::XMLDocument *domDocument=NULL;
    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return;
        }
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file.toStdString()]=domDocument;
    }
    const tinyxml2::XMLElement &root = domDocument.RootElement();
    if(root==NULL || strcmp(root->Name(),"musics")!=0)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"musics\" root balise not found for the xml file").arg(QString::fromStdString(file));
        return;
    }

    //load the content
    tinyxml2::XMLElement item = root.FirstChildElement(DatapackClientLoader::text_map);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(DatapackClientLoader::text_type))
            {
                const std::string &type=item.attribute(DatapackClientLoader::text_type);
                if(!DatapackClientLoader::datapackLoader.audioAmbiance.contains(type))
                    audioAmbiance[type]=datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN+item->GetText();
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id number already set: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name());
        item = item.NextSiblingElement(DatapackClientLoader::text_map);
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
        std::string zoneCodeName=entryList.at(index).fileName();
        const std::string &file=entryList.at(index).absoluteFilePath();
        zoneCodeName.remove(removeXml);

        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            index++;
            continue;
        }

        const tinyxml2::XMLElement &root = domDocument.RootElement();
        if(root==NULL || strcmp(root->Name(),"zone")!=0)
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"zone\" root balise not found for the xml file").arg(QString::fromStdString(file));
            index++;
            continue;
        }

        //load the content
        bool haveName=false;
        DatapackClientLoader::ZoneExtra zone;

        //load name
        tinyxml2::XMLElement name = root.FirstChildElement(DatapackClientLoader::text_name);
        bool found=false;
        if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute("lang") || name.attribute("lang")==DatapackClientLoader::text_en)
                    {
                        haveName=true;
                        zone.name=name->GetText();
                        break;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(name->Name());
                }
                name = name.NextSiblingElement(DatapackClientLoader::text_name);
            }
        if(!found)
        {
            name = root.FirstChildElement(DatapackClientLoader::text_name);
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute("lang") || name.attribute("lang")==DatapackClientLoader::text_en)
                    {
                        haveName=true;
                        zone.name=name->GetText();
                        break;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(name->Name());
                }
                name = name.NextSiblingElement(DatapackClientLoader::text_name);
            }
        }
        if(haveName)
            zonesExtra[zoneCodeName]=zone;

        //load the audio ambiance
        {
            tinyxml2::XMLElement item = root.FirstChildElement(DatapackClientLoader::text_music);
            while(!item.isNull())
            {
                if(item.isElement())
                {
                    if(item.hasAttribute(DatapackClientLoader::text_type) && item.hasAttribute(DatapackClientLoader::text_backgroundsound))
                    {
                        const std::string &type=item.attribute(DatapackClientLoader::text_type);
                        const std::string &backgroundsound=item.attribute(DatapackClientLoader::text_backgroundsound);
                        zonesExtra[zoneCodeName].audioAmbiance[type]=backgroundsound;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, have not the music attribute: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.Name(): %2").arg(QString::fromStdString(file)).arg(item->Name());
                item = item.NextSiblingElement(DatapackClientLoader::text_music);
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
        const std::string &filePath=fileList.at(index);
        if(filePath.endsWith(DatapackClientLoader::text_dottsx))
        {
            const std::string &source=QFileInfo(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPBASE)+filePath).absoluteFilePath();
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
