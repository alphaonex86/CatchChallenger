#include "QtDatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "QtDatapackChecksum.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../../general/base/Map_loader.h"
#include "LanguagesSelect.h"
#include "tiled/tiled_tileset.h"
#include "tiled/tiled_mapreader.h"
#include "FacilityLibClient.h"

#include <QDebug>
#include <QFile>
#include <QByteArray>
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <iostream>

QtDatapackClientLoader *QtDatapackClientLoader::datapackLoader=nullptr;

/*const std::string QtDatapackClientLoader::text_list="list";
const std::string QtDatapackClientLoader::text_reputation="reputation";
const std::string QtDatapackClientLoader::text_type="type";
const std::string QtDatapackClientLoader::text_name="name";
const std::string QtDatapackClientLoader::text_en="en";
const std::string QtDatapackClientLoader::text_lang="lang";
const std::string QtDatapackClientLoader::text_level="level";
const std::string QtDatapackClientLoader::text_point="point";
const std::string QtDatapackClientLoader::text_text="text";
const std::string QtDatapackClientLoader::text_id="id";
const std::string QtDatapackClientLoader::text_image="image";
const std::string QtDatapackClientLoader::text_description="description";
const std::string QtDatapackClientLoader::text_item="item";
const std::string QtDatapackClientLoader::text_slashdefinitiondotxml="/definition.xml";
const std::string QtDatapackClientLoader::text_quest="quest";
const std::string QtDatapackClientLoader::text_rewards="rewards";
const std::string QtDatapackClientLoader::text_show="show";
const std::string QtDatapackClientLoader::text_autostep="autostep";
const std::string QtDatapackClientLoader::text_yes="yes";
const std::string QtDatapackClientLoader::text_true="true";
const std::string QtDatapackClientLoader::text_bot="bot";
const std::string QtDatapackClientLoader::text_dotcomma=";";
const std::string QtDatapackClientLoader::text_client_logic="client_logic";
const std::string QtDatapackClientLoader::text_map="map";
const std::string QtDatapackClientLoader::text_items="items";
const std::string QtDatapackClientLoader::text_zone="zone";
const std::string QtDatapackClientLoader::text_music="music";
const std::string QtDatapackClientLoader::text_backgroundsound="backgroundsound";

const std::string QtDatapackClientLoader::text_monster="monster";
const std::string QtDatapackClientLoader::text_monsters="monsters";
const std::string QtDatapackClientLoader::text_kind="kind";
const std::string QtDatapackClientLoader::text_habitat="habitat";
const std::string QtDatapackClientLoader::text_slash="/";
const std::string QtDatapackClientLoader::text_types="types";
const std::string QtDatapackClientLoader::text_buff="buff";
const std::string QtDatapackClientLoader::text_skill="skill";
const std::string QtDatapackClientLoader::text_buffs="buffs";
const std::string QtDatapackClientLoader::text_skills="skills";
const std::string QtDatapackClientLoader::text_fight="fight";
const std::string QtDatapackClientLoader::text_fights="fights";
const std::string QtDatapackClientLoader::text_start="start";
const std::string QtDatapackClientLoader::text_win="win";
const std::string QtDatapackClientLoader::text_dotxml=".xml";
const std::string QtDatapackClientLoader::text_dottsx=".tsx";
const std::string QtDatapackClientLoader::text_visual="visual";
const std::string QtDatapackClientLoader::text_category="category";
const std::string QtDatapackClientLoader::text_alpha="alpha";
const std::string QtDatapackClientLoader::text_color="color";
const std::string QtDatapackClientLoader::text_event="event";
const std::string QtDatapackClientLoader::text_value="value";

const std::string QtDatapackClientLoader::text_tileheight="tileheight";
const std::string QtDatapackClientLoader::text_tilewidth="tilewidth";
const std::string QtDatapackClientLoader::text_x="x";
const std::string QtDatapackClientLoader::text_y="y";
const std::string QtDatapackClientLoader::text_object="object";
const std::string QtDatapackClientLoader::text_objectgroup="objectgroup";
const std::string QtDatapackClientLoader::text_Object="Object";
const std::string QtDatapackClientLoader::text_layer="layer";
const std::string QtDatapackClientLoader::text_Dirt="Dirt";
const std::string QtDatapackClientLoader::text_DATAPACK_BASE_PATH_MAPBASE=DATAPACK_BASE_PATH_MAPBASE;
std::string QtDatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN=DATAPACK_BASE_PATH_MAPMAIN;
std::string QtDatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=DATAPACK_BASE_PATH_MAPSUB1 "na" DATAPACK_BASE_PATH_MAPSUB2;*/

QtDatapackClientLoader::QtDatapackClientLoader()
{
    mDefaultInventoryImage=NULL;
    #ifndef NOTHREADS
    start();
    moveToThread(this);
    #endif
    setObjectName("DatapackClientLoader");
}

QtDatapackClientLoader::~QtDatapackClientLoader()
{
    if(mDefaultInventoryImage==NULL)
        delete mDefaultInventoryImage;
    #ifndef NOTHREADS
    quit();
    wait();
    #endif
}

QPixmap QtDatapackClientLoader::defaultInventoryImage()
{
    return *mDefaultInventoryImage;
}

#ifndef NOTHREADS
void QtDatapackClientLoader::run()
{
    exec();
}
#endif

void QtDatapackClientLoader::parseDatapack(const std::string &datapackPath)
{
    if(inProgress)
    {
        qDebug() << QStringLiteral("already in progress");
        return;
    }
    DatapackClientLoader::parseDatapack(datapackPath);
    parseMonstersExtra();
    parseBuffExtra();
    parsePlantsExtra();
    inProgress=false;
    emit datapackParsed();
}

void QtDatapackClientLoader::parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode)
{
    DatapackClientLoader::parseDatapackMainSub(mainDatapackCode,subDatapackCode);
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/CC/images/inventory/unknown-object.png"));
    CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapack(
                datapackPath,mainDatapackCode,subDatapackCode);

    parseTileset();

    inProgress=false;

    emit datapackParsedMainSub();
}

void QtDatapackClientLoader::emitdatapackParsed()
{
    emit datapackParsed();
}

void QtDatapackClientLoader::emitdatapackParsedMainSub()
{
    emit datapackParsedMainSub();
}

void QtDatapackClientLoader::emitdatapackChecksumError()
{
    emit datapackChecksumError();
}

void QtDatapackClientLoader::parseTopLib()
{
    parseTileset();
}

void QtDatapackClientLoader::resetAll()
{
    CatchChallenger::CommonDatapack::commonDatapack.unload();
    if(mDefaultInventoryImage==NULL)
    {
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/CC/images/inventory/unknown-object.png"));
        if(mDefaultInventoryImage->isNull())
        {
            qDebug() << "default internal image bug for item";
            abort();
        }
    }
    for (const auto &n : QtplantExtra)
        delete n.second.tileset;
    QtplantExtra.clear();
    {
         QHashIterator<QString,Tiled::Tileset *> i(Tiled::Tileset::preloadedTileset);
         while (i.hasNext()) {
             i.next();
             delete i.value();
         }
         Tiled::Tileset::preloadedTileset.clear();
    }
    QtitemsExtra.clear();
    QtmonsterExtra.clear();
    QtmonsterBuffsExtra.clear();
    QtplantExtra.clear();
    DatapackClientLoader::resetAll();
}

void QtDatapackClientLoader::parseTileset()
{
    const std::vector<std::string> &fileList=
                CatchChallenger::FacilityLibGeneral::listFolder(datapackPath+DATAPACK_BASE_PATH_MAPBASE);
    unsigned int index=0;
    while(index<fileList.size())
    {
        const std::string &filePath=fileList.at(index);
        if(stringEndsWith(filePath,".tsx"))
        {
            const std::string &source=QFileInfo(QString::fromStdString(datapackPath+DATAPACK_BASE_PATH_MAPBASE+filePath))
                    .absoluteFilePath().toStdString();
            QFile file(QString::fromStdString(source));
            if(file.open(QIODevice::ReadOnly))
            {
                Tiled::MapReader mapReader;
                Tiled::Tileset *tileset = mapReader.readTileset(&file, QString::fromStdString(source));
                if (tileset)
                {
                    tileset->setFileName(QString::fromStdString(source));
                    Tiled::Tileset::preloadedTileset[QString::fromStdString(source)]=tileset;
                }
                file.close();
            }
            else
                qDebug() << QStringLiteral("Tileset: %1 can't be open: %2")
                            .arg(QString::fromStdString(source))
                            .arg(file.errorString());
        }
        index++;
    }

    qDebug() << QStringLiteral("%1 tileset(s) loaded").arg(Tiled::Tileset::preloadedTileset.size());
}

void QtDatapackClientLoader::parseItemsExtra()
{
    DatapackClientLoader::parseItemsExtra();
    for( const auto& n : itemsExtra ) {
        QString path=QString::fromStdString(n.second.imagePath);
        QPixmap pix(path);
        if(pix.isNull())
            qDebug() << "bug: " << path;
        else
            QtitemsExtra[n.first].image=pix;
    }
}

std::string QtDatapackClientLoader::getLanguage()
{
    return LanguagesSelect::languagesSelect->getCurrentLanguages();
}

