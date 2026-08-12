#include "SettingsAll.h"
#include "../map-procedural-generation-terrain/LoadMap.h"
#include "LoadMapAll.h"

#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QXmlStreamReader>

void SettingsAll::putDefaultSettings(QSettings &settings)
{
    //do tile to zone converter
    if(!settings.contains("displaycity"))
        settings.setValue("displaycity",false);
    if(!settings.contains("displayregion"))
        settings.setValue("displayregion",false);
    if(!settings.contains("cleanTileset"))
        settings.setValue("cleanTileset",true);
    if(!settings.contains("cityDebug"))
        settings.setValue("cityDebug",false);
    if(!settings.contains("terrainDebug"))
        settings.setValue("terrainDebug",false);
    if(!settings.contains("scale_City"))
        settings.setValue("scale_City",1.0);
    if(!settings.contains("doallmap"))
        settings.setValue("doallmap",false);
    if(!settings.contains("cityRadius"))
        settings.setValue("cityRadius",3);
    if(!settings.contains("maxCityLinks"))
        settings.setValue("maxCityLinks",3);
    if(!settings.contains("levelmapscale"))
        settings.setValue("levelmapscale",0.05);
    if(!settings.contains("levelmapmin"))
        settings.setValue("levelmapmin",2);
    if(!settings.contains("levelmapmax"))
        settings.setValue("levelmapmax",50);

    settings.beginGroup("city");
    if(!settings.contains("flatten"))
        settings.setValue("flatten",true);
    if(!settings.contains("flattenShape"))
        settings.setValue("flattenShape","rectangle");
    if(!settings.contains("flattenMargin"))
        settings.setValue("flattenMargin",0);
    if(!settings.contains("flattenFalloff"))
        settings.setValue("flattenFalloff",44);
    {
        //index 0 small, 1 medium, 2 big — the CityType order
        static const char * const sizeGroup[3]={"small","medium","big"};
        static const char * const sizeTemplate[3]={"city-small","city-medium","city-big"};
        //a small town laid out over the whole chunk read as an empty field: it gets
        //a smaller hole, a denser packing and fewer buildings than a capital
        static const unsigned int sizeHolePercent[3]={55,75,100};
        static const unsigned int sizeDensityPercent[3]={45,40,35};
        static const unsigned int sizeMinBuilding[3]={2,4,6};
        unsigned int sizeIndex=0;
        while(sizeIndex<3)
        {
            settings.beginGroup(sizeGroup[sizeIndex]);
            if(!settings.contains("template"))
                settings.setValue("template",sizeTemplate[sizeIndex]);
            if(!settings.contains("useAsBase"))
                settings.setValue("useAsBase",false);
            if(!settings.contains("signTiles"))
                settings.setValue("signTiles","");
            if(!settings.contains("holePercent"))
                settings.setValue("holePercent",sizeHolePercent[sizeIndex]);
            if(!settings.contains("densityPercent"))
                settings.setValue("densityPercent",sizeDensityPercent[sizeIndex]);
            if(!settings.contains("minBuilding"))
                settings.setValue("minBuilding",sizeMinBuilding[sizeIndex]);
            settings.endGroup();
            sizeIndex++;
        }
    }
    settings.endGroup();

    settings.beginGroup("road");
    settings.beginGroup("cave");
    if(!settings.contains("percent"))
        settings.setValue("percent",15);
    if(!settings.contains("wallTile"))
        settings.setValue("wallTile","");
    if(!settings.contains("floorTile"))
        settings.setValue("floorTile","");
    if(!settings.contains("entranceTile"))
        settings.setValue("entranceTile","");
    if(!settings.contains("entranceTopTile"))
        settings.setValue("entranceTopTile","");
    if(!settings.contains("exitBottomTile"))
        settings.setValue("exitBottomTile","");
    if(!settings.contains("exitTopTile"))
        settings.setValue("exitTopTile","");
    if(!settings.contains("stairDownTile"))
        settings.setValue("stairDownTile","");
    if(!settings.contains("stairUpTile"))
        settings.setValue("stairUpTile","");
    if(!settings.contains("itemTile"))
        settings.setValue("itemTile","");
    if(!settings.contains("maxDepth"))
        settings.setValue("maxDepth",3);
    if(!settings.contains("itemPercent"))
        settings.setValue("itemPercent",60);
    if(!settings.contains("items"))
        settings.setValue("items","");
    settings.endGroup();
    settings.beginGroup("ledge");
    if(!settings.contains("doledge"))
        settings.setValue("doledge",true);
    if(!settings.contains("ledgeleft"))
        settings.setValue("ledgeleft",808);
    if(!settings.contains("ledgeright"))
        settings.setValue("ledgeright",810);
    if(!settings.contains("ledgebottom"))
        settings.setValue("ledgebottom",740);
    if(!settings.contains("ledgechance"))
        settings.setValue("ledgechance",0.7);
    if(!settings.contains("tsx"))
        settings.setValue("tsx","main/tileset/t1.tsx");
    settings.endGroup();

    settings.beginGroup("bot");
    if(!settings.contains("dobot"))
        settings.setValue("dobot",true);
    if(!settings.contains("maxbot"))
        settings.setValue("maxbot",15);
    if(!settings.contains("maxskin"))
        settings.setValue("maxskin", 100);
    if(!settings.contains("cashbase"))
        settings.setValue("cashbase", 100);
    if(!settings.contains("cashexp"))
        settings.setValue("cashexp", 1.5);
    if(!settings.contains("cashmonster"))
        settings.setValue("cashmonster", 0.2);
    settings.endGroup();

    settings.beginGroup("region");
    if(!settings.contains("grass"))
        settings.setValue("grass", "");
    if(!settings.contains("extratileset"))
        settings.setValue("extratileset", "");
    if(!settings.contains("mountain_terrain"))
        settings.setValue("mountain_terrain", "");
    if(!settings.contains("mountain_layer"))
        settings.setValue("mountain_layer", "Collisions");
    if(!settings.contains("mountain_tile"))
        settings.setValue("mountain_tile", "");
    if(!settings.contains("mountain_tsx"))
        settings.setValue("mountain_tsx", "");
    if(!settings.contains("walkway"))
        settings.setValue("walkway", "");
    if(!settings.contains("waterchance"))
        settings.setValue("waterchance", 0.4);
    if(!settings.contains("initialregion"))
        settings.setValue("initialregion", 300);
    if(!settings.contains("walkwayregion"))
        settings.setValue("walkwayregion", 20);
    if(!settings.contains("retrymax"))
        settings.setValue("retrymax", 500);
    settings.endGroup();
    settings.endGroup();

    settings.beginGroup("building");
    if(!settings.contains("doGym"))
        settings.setValue("doGym",true);
    if(!settings.contains("gymTrainers"))
        settings.setValue("gymTrainers",3);
    //numeric item ids sold by every generated shop; must exist in the target
    //datapack's items table or the engine silently drops the product.
    if(!settings.contains("shopItems"))
        settings.setValue("shopItems","1,2,3,5,6");
    //skins are folder NAMES under the datapack's skin/bot (or skin/fighter);
    //these defaults are real CatchChallenger-datapack skin names.
    if(!settings.contains("botSkins"))
        settings.setValue("botSkins","franck,oldman,florist,farmer,smith,alphonse,captain");
    if(!settings.contains("healSkin"))
        settings.setValue("healSkin","oldgirl");
    if(!settings.contains("shopSkin"))
        settings.setValue("shopSkin","bankier");
    if(!settings.contains("gymTrainerSkin"))
        settings.setValue("gymTrainerSkin","smith");
    if(!settings.contains("gymLeaderSkin"))
        settings.setValue("gymLeaderSkin","soldier");
    //gym type list: "type[:#color]->Monster1,Monster2;type2->..." (monster NAMES);
    //colors come from the datapack monsters/type.xml (or inline after ':')
    if(!settings.contains("gymTypes"))
        settings.setValue("gymTypes","");
    if(!settings.contains("cityTypeTerrains"))
        settings.setValue("cityTypeTerrains","");
    if(!settings.contains("cityStyleTerrains"))
        settings.setValue("cityStyleTerrains","sea-city->water,sea,beach;desert-city->desert,sand");
    if(!settings.contains("typeXml"))
        settings.setValue("typeXml","");
    settings.endGroup();

    settings.beginGroup("wildMonsters");
    settings.beginGroup("0");
    if(!settings.contains("comment"))
        settings.setValue("comment","key is the id, heightmoisurelist entries is height,moisure->mappercent,luckweight");
    if(!settings.contains("heightmoisurelist"))
        settings.setValue("heightmoisurelist","3,1->100,10;3,2->100,10;3,3->100,10;3,4->100,10;3,5->100,10;3,6->100,10");
    settings.endGroup();
    settings.endGroup();

    settings.sync();
}

void SettingsAll::populateSettings(QSettings &settings, SettingsAll::SettingsExtra& config)
{
    config.displaycity=settings.value("displaycity").toBool();
    config.displayregion=settings.value("displayregion").toBool();
    config.cleanTileset=settings.value("cleanTileset",true).toBool();
    config.cityDebug=settings.value("cityDebug",false).toBool();
    config.terrainDebug=settings.value("terrainDebug",false).toBool();

    settings.beginGroup("water");
    config.waterSeaMinTiles=settings.value("seaMinTiles",4000).toUInt();
    config.waterLakeMinTiles=settings.value("lakeMinTiles",200).toUInt();
    config.waterBodyDebugStep=settings.value("bodyDebugStep",32).toUInt();
    config.waterPathPercentOfLand=settings.value("pathPercentOfLand",0).toUInt();
    config.waterChunkSeaPercent=settings.value("chunkSeaPercent",80).toUInt();
    if(config.waterChunkSeaPercent>100)
        config.waterChunkSeaPercent=100;
    config.waterBoatPercent=settings.value("boatPercent",30).toUInt();
    if(config.waterBoatPercent>100)
        config.waterBoatPercent=100;
    config.waterHarbourChunkRadius=settings.value("harbourChunkRadius",3).toUInt();
    config.waterBorderTile=settings.value("borderTile","").toString();
    config.waterChannelHalfWidth=settings.value("channelHalfWidth",6).toUInt();
    config.waterWanderAmplitude=settings.value("wanderAmplitude",3).toUInt();
    config.waterIslandPercent=settings.value("islandPercent",35).toUInt();
    config.waterIslandMinTiles=settings.value("islandMinTiles",20).toUInt();
    config.waterIslandSandMax=settings.value("islandSandMax",2).toUInt();
    config.waterIslandLandablePercent=settings.value("islandLandablePercent",50).toUInt();
    config.waterMinFighter=settings.value("minFighter",1).toUInt();
    config.waterMaxFighter=settings.value("maxFighter",3).toUInt();
    if(config.waterMaxFighter<config.waterMinFighter)
        config.waterMaxFighter=config.waterMinFighter;
    config.waterShipDecoration=settings.value("shipDecoration","").toString();
    config.waterShipUsable=settings.value("shipUsable","").toString();
    if(config.waterBodyDebugStep<1)
        config.waterBodyDebugStep=1;
    settings.endGroup();
    config.scale_City=settings.value("scale_City").toFloat();
    config.doallmap=settings.value("doallmap").toBool();
    config.maxCityLinks=settings.value("maxCityLinks").toUInt();
    config.cityRadius=settings.value("cityRadius").toUInt();

    config.levelmapscale=settings.value("levelmapscale").toFloat();
    config.levelmapmin=settings.value("levelmapmin").toUInt();
    config.levelmapmax=settings.value("levelmapmax").toUInt();

    settings.beginGroup("city");
    config.cityFlatten=settings.value("flatten",true).toBool();
    config.cityFlattenShape=settings.value("flattenShape","rectangle").toString().trimmed().toLower();
    if(config.cityFlattenShape!="rectangle" && config.cityFlattenShape!="circle" && config.cityFlattenShape!="octagon")
    {
        std::cerr << "[city] flattenShape \"" << config.cityFlattenShape.toStdString()
                  << "\" is not rectangle/circle/octagon, using rectangle" << std::endl;
        config.cityFlattenShape="rectangle";
    }
    config.cityFlattenMargin=settings.value("flattenMargin",0).toUInt();
    config.cityFlattenFalloff=settings.value("flattenFalloff",44).toFloat();
    if(config.cityFlattenFalloff<0)
        config.cityFlattenFalloff=0;
    {
        static const char * const sizeGroup[3]={"small","medium","big"};
        static const char * const sizeTemplate[3]={"city-small","city-medium","city-big"};
        static const unsigned int sizeHolePercent[3]={55,75,100};
        static const unsigned int sizeDensityPercent[3]={45,40,35};
        static const unsigned int sizeMinBuilding[3]={2,4,6};
        unsigned int sizeIndex=0;
        while(sizeIndex<3)
        {
            SettingsExtra::CitySize &citySize=config.citySize[sizeIndex];
            settings.beginGroup(sizeGroup[sizeIndex]);
            citySize.templateName=settings.value("template",sizeTemplate[sizeIndex]).toString();
            citySize.useAsBase=settings.value("useAsBase",false).toBool();
            citySize.signTiles.clear();
            {
                const QStringList signList=settings.value("signTiles","").toString().split(",");
                unsigned int indexSign=0;
                while(indexSign<(unsigned int)signList.size())
                {
                    const std::string signTile=signList.at(indexSign).trimmed().toStdString();
                    if(!signTile.empty())
                        citySize.signTiles.push_back(signTile);
                    indexSign++;
                }
            }
            citySize.holePercent=settings.value("holePercent",sizeHolePercent[sizeIndex]).toUInt();
            //below ~20% of the chunk not even the heal center fits; above 100 is
            //the whole chunk anyway (the hole is clamped to the vegetation ring)
            if(citySize.holePercent<20 || citySize.holePercent>100)
            {
                std::cerr << "[city] " << sizeGroup[sizeIndex] << "\\holePercent "
                          << citySize.holePercent << " is out of 20..100, using "
                          << sizeHolePercent[sizeIndex] << std::endl;
                citySize.holePercent=sizeHolePercent[sizeIndex];
            }
            citySize.densityPercent=settings.value("densityPercent",sizeDensityPercent[sizeIndex]).toUInt();
            if(citySize.densityPercent<1 || citySize.densityPercent>100)
            {
                std::cerr << "[city] " << sizeGroup[sizeIndex] << "\\densityPercent "
                          << citySize.densityPercent << " is out of 1..100, using "
                          << sizeDensityPercent[sizeIndex] << std::endl;
                citySize.densityPercent=sizeDensityPercent[sizeIndex];
            }
            citySize.minBuilding=settings.value("minBuilding",sizeMinBuilding[sizeIndex]).toUInt();
            settings.endGroup();
            sizeIndex++;
        }
    }
    settings.endGroup();

    settings.beginGroup("road");
    settings.beginGroup("cave");
    config.cavePercent=settings.value("percent",15).toUInt();
    config.caveWallTile=settings.value("wallTile","").toString();
    config.caveFloorTile=settings.value("floorTile","").toString();
    config.caveEntranceTile=settings.value("entranceTile","").toString();
    config.caveEntranceTopTile=settings.value("entranceTopTile","").toString();
    config.caveExitBottomTile=settings.value("exitBottomTile","").toString();
    config.caveExitTopTile=settings.value("exitTopTile","").toString();
    config.caveStairDownTile=settings.value("stairDownTile","").toString();
    config.caveStairUpTile=settings.value("stairUpTile","").toString();
    config.caveItemTile=settings.value("itemTile","").toString();
    config.caveMaxDepth=settings.value("maxDepth",3).toUInt();
    if(config.caveMaxDepth<1)
        config.caveMaxDepth=1;
    config.caveItemPercent=settings.value("itemPercent",60).toUInt();
    config.caveItems.clear();
    {
        const QStringList caveItemsList=settings.value("items","").toString().split(",");
        unsigned int indexItem=0;
        while(indexItem<(unsigned int)caveItemsList.size())
        {
            bool ok=false;
            const unsigned int itemId=caveItemsList.at(indexItem).trimmed().toUInt(&ok);
            if(ok)
                config.caveItems.push_back(itemId);
            indexItem++;
        }
    }
    settings.endGroup();
    settings.beginGroup("ledge");
    config.doledge=settings.value("doledge").toBool();
    config.ledgeleft=settings.value("ledgeleft").toUInt();
    config.ledgeright=settings.value("ledgeright").toUInt();
    config.ledgebottom=settings.value("ledgebottom").toUInt();
    config.ledgechance=settings.value("ledgechance").toFloat();
    settings.endGroup();

    settings.beginGroup("region");
    config.grass = settings.value("grass").toString();
    config.extratileset = settings.value("extratileset").toString();
    config.walkway = settings.value("walkway").toString();


    config.roadWaterChance=settings.value("waterchance").toFloat();
    config.regionTry=settings.value("initialregion").toUInt();
    config.walkwayTry=settings.value("walkwayregion").toUInt();
    config.roadRetry=settings.value("retrymax").toUInt();

    LoadMapAll::RoadMountain mountain;
    mountain.terrain = settings.value("mountain_terrain").toString();
    mountain.layer = settings.value("mountain_layer").toString();
    mountain.tile = settings.value("mountain_tile").toString();
    mountain.tsx = settings.value("mountain_tsx").toString();
    LoadMapAll::mountain = mountain;
    settings.endGroup();

    //[road] level, NOT [road] region\: this is about the whole chunk
    config.roadExtraSpacePercentMin=settings.value("extraSpacePercentMin",8).toUInt();
    config.roadExtraSpacePercentMax=settings.value("extraSpacePercentMax",65).toUInt();
    config.roadExtraSpacePercentVariance=settings.value("extraSpacePercentVariance",100).toUInt();
    if(config.roadExtraSpacePercentMin>100)
        config.roadExtraSpacePercentMin=100;
    if(config.roadExtraSpacePercentMax>100)
        config.roadExtraSpacePercentMax=100;
    if(config.roadExtraSpacePercentMax<config.roadExtraSpacePercentMin)
    {
        std::cerr << "[road] extraSpacePercentMax " << config.roadExtraSpacePercentMax
                  << " is below extraSpacePercentMin " << config.roadExtraSpacePercentMin
                  << ", using the min for both" << std::endl;
        config.roadExtraSpacePercentMax=config.roadExtraSpacePercentMin;
    }
    if(config.roadExtraSpacePercentVariance>100)
        config.roadExtraSpacePercentVariance=100;
    settings.endGroup();

    settings.beginGroup("building");
    config.doGym=settings.value("doGym",true).toBool();
    config.gymTrainers=settings.value("gymTrainers",3).toUInt();
    config.shopItems.clear();
    {
        const QStringList shopItemsList=settings.value("shopItems","1,2,3").toString().split(",");
        unsigned int indexShopItem=0;
        while(indexShopItem<(unsigned int)shopItemsList.size())
        {
            bool ok=false;
            const unsigned int itemId=shopItemsList.at(indexShopItem).trimmed().toUInt(&ok);
            if(ok)
                config.shopItems.push_back(itemId);
            indexShopItem++;
        }
    }
    if(config.shopItems.empty())
    {
        config.shopItems.push_back(1);
        config.shopItems.push_back(2);
        config.shopItems.push_back(3);
    }
    config.botSkins.clear();
    {
        const QStringList botSkinsList=settings.value("botSkins","franck,oldman,florist,farmer,smith,alphonse,captain").toString().split(",");
        unsigned int indexSkin=0;
        while(indexSkin<(unsigned int)botSkinsList.size())
        {
            const std::string skinName=botSkinsList.at(indexSkin).trimmed().toStdString();
            if(!skinName.empty())
                config.botSkins.push_back(skinName);
            indexSkin++;
        }
    }
    if(config.botSkins.empty())
        config.botSkins.push_back("bot");
    config.healSkin=settings.value("healSkin","oldgirl").toString().toStdString();
    config.shopSkin=settings.value("shopSkin","bankier").toString().toStdString();
    config.gymTrainerSkin=settings.value("gymTrainerSkin","smith").toString().toStdString();
    config.gymLeaderSkin=settings.value("gymLeaderSkin","soldier").toString().toStdString();
    config.gymTypeNames.clear();
    config.gymTypeColors.clear();
    config.gymTypeMonsters.clear();
    {
        //"type[:#color]->Monster1,Monster2;type2->..."
        const QStringList typeList=settings.value("gymTypes","").toString().split(";");
        unsigned int indexType=0;
        while(indexType<(unsigned int)typeList.size())
        {
            const QString &typeEntry=typeList.at(indexType).trimmed();
            if(!typeEntry.isEmpty())
            {
                const QStringList typeSplit=typeEntry.split("->");
                if(typeSplit.size()==2)
                {
                    QString typeName=typeSplit.at(0).trimmed();
                    QString typeColor;
                    const int colonPos=typeName.indexOf(":");
                    if(colonPos>=0)
                    {
                        typeColor=typeName.mid(colonPos+1).trimmed();
                        typeName=typeName.left(colonPos).trimmed();
                    }
                    std::vector<std::string> typeMonsters;
                    const QStringList monsterList=typeSplit.at(1).split(",");
                    unsigned int indexMonster=0;
                    while(indexMonster<(unsigned int)monsterList.size())
                    {
                        const std::string monsterName=monsterList.at(indexMonster).trimmed().toLower().toStdString();
                        if(!monsterName.empty())
                            typeMonsters.push_back(monsterName);
                        indexMonster++;
                    }
                    if(!typeName.isEmpty() && !typeMonsters.empty())
                    {
                        config.gymTypeNames.push_back(typeName.toLower().toStdString());
                        config.gymTypeColors.push_back(typeColor);
                        config.gymTypeMonsters.push_back(typeMonsters);
                    }
                }
                else
                    qDebug() << "Syntaxe error into gymTypes entry: " << typeEntry;
            }
            indexType++;
        }
    }
    config.cityTypeTerrains.clear();
    {
        //"type->terrainKeyword,terrainKeyword;type2->..."
        const QStringList typeList=settings.value("cityTypeTerrains","").toString().split(";");
        unsigned int indexType=0;
        while(indexType<(unsigned int)typeList.size())
        {
            const QString &typeEntry=typeList.at(indexType).trimmed();
            if(!typeEntry.isEmpty())
            {
                const QStringList typeSplit=typeEntry.split("->");
                if(typeSplit.size()==2)
                {
                    const std::string typeName=typeSplit.at(0).trimmed().toLower().toStdString();
                    std::vector<std::string> keywords;
                    const QStringList keywordList=typeSplit.at(1).split(",");
                    unsigned int indexKeyword=0;
                    while(indexKeyword<(unsigned int)keywordList.size())
                    {
                        const std::string keyword=keywordList.at(indexKeyword).trimmed().toLower().toStdString();
                        if(!keyword.empty())
                            keywords.push_back(keyword);
                        indexKeyword++;
                    }
                    if(!typeName.empty() && !keywords.empty())
                        config.cityTypeTerrains.push_back(std::pair<std::string,std::vector<std::string> >(typeName,keywords));
                }
                else
                    qDebug() << "Syntaxe error into cityTypeTerrains entry: " << typeEntry;
            }
            indexType++;
        }
    }
    config.cityStyleTerrains.clear();
    {
        //"style-city->terrainKeyword,terrainKeyword;style2-city->..." : which
        //template/<style>/ folder the houses of a city come from. A style with
        //no rule here is only used as a random fallback.
        const QStringList styleList=settings.value("cityStyleTerrains","").toString().split(";");
        unsigned int indexStyle=0;
        while(indexStyle<(unsigned int)styleList.size())
        {
            const QString &styleEntry=styleList.at(indexStyle).trimmed();
            if(!styleEntry.isEmpty())
            {
                const QStringList styleSplit=styleEntry.split("->");
                if(styleSplit.size()==2)
                {
                    const std::string styleName=styleSplit.at(0).trimmed().toStdString();
                    std::vector<std::string> keywords;
                    const QStringList keywordList=styleSplit.at(1).split(",");
                    unsigned int indexKeyword=0;
                    while(indexKeyword<(unsigned int)keywordList.size())
                    {
                        const std::string keyword=keywordList.at(indexKeyword).trimmed().toLower().toStdString();
                        if(!keyword.empty())
                            keywords.push_back(keyword);
                        indexKeyword++;
                    }
                    if(!styleName.empty() && !keywords.empty())
                        config.cityStyleTerrains.push_back(std::pair<std::string,std::vector<std::string> >(styleName,keywords));
                }
                else
                    qDebug() << "Syntaxe error into cityStyleTerrains entry: " << styleEntry;
            }
            indexStyle++;
        }
    }
    config.typeXml=settings.value("typeXml","").toString();
    if(!config.typeXml.isEmpty() && !config.gymTypeNames.empty())
    {
        //type colors from the datapack monsters/type.xml: <type name="fire" color="#f38233"/>
        QFile typeFile(config.typeXml);
        if(typeFile.open(QIODevice::ReadOnly))
        {
            QXmlStreamReader xml(&typeFile);
            while(!xml.atEnd())
            {
                if(xml.readNext()==QXmlStreamReader::StartElement)
                {
                    if(xml.name().toString()=="type")
                    {
                        const std::string typeName=xml.attributes().value("name").toString().toLower().toStdString();
                        const QString typeColor=xml.attributes().value("color").toString();
                        if(!typeName.empty() && !typeColor.isEmpty())
                        {
                            unsigned int indexType=0;
                            while(indexType<config.gymTypeNames.size())
                            {
                                if(config.gymTypeNames.at(indexType)==typeName)
                                    config.gymTypeColors[indexType]=typeColor;
                                indexType++;
                            }
                        }
                    }
                }
            }
            typeFile.close();
        }
        else
            std::cerr << "typeXml configured but not readable: " << config.typeXml.toStdString() << std::endl;
    }
    settings.endGroup();

    settings.beginGroup("wildMonsters");
    QStringList monsterId=settings.childGroups();
    {
        bool ok=false;
        unsigned int indexGroupId=0;
        while(indexGroupId<(unsigned int)monsterId.size())
        {
            QString idString=monsterId.at(indexGroupId);
            const uint32_t monsterId=idString.toUInt(&ok);
            if(!ok)
                qDebug() << "Syntaxe error into: idString not number: " << idString;
            if(monsterId!=0)
            {
                settings.beginGroup(idString);
                if(settings.contains("name"))
                {
                    //display name: generated xml emits it lowercase instead of the id
                    const std::string monsterName=settings.value("name").toString().trimmed().toLower().toStdString();
                    if(!monsterName.empty())
                        config.monsterNames[monsterId]=monsterName;
                }
                if(settings.contains("heightmoisurelist"))
                {
                    QStringList heightmoisurelist=settings.value("heightmoisurelist").toString().split(";");
                    unsigned int heightmoisureId=0;
                    while(heightmoisureId<(unsigned int)heightmoisurelist.size())
                    {
                        QString heightmoisureEntry=heightmoisurelist.at(heightmoisureId);
                        heightmoisureEntry.replace("->",",");
                        QStringList heightmoisureSplit=heightmoisureEntry.split(",");
                        if(heightmoisureSplit.size()!=4)
                            qDebug() << "Syntaxe error into: heightmoisurelist: " << settings.value("heightmoisurelist").toString() << ": " << heightmoisureEntry;
                        else
                        {
                            const uint32_t height=heightmoisureSplit.at(0).toUInt(&ok)-1;
                            if(!ok)
                                qDebug() << "Syntaxe error into: height not number: " << heightmoisureSplit.at(0);
                            else
                            {
                                const uint32_t moisure=heightmoisureSplit.at(1).toUInt(&ok)-1;
                                if(!ok)
                                    qDebug() << "Syntaxe error into: moisure not number: " << heightmoisureSplit.at(1);
                                else
                                {
                                    const uint32_t mapweight=heightmoisureSplit.at(2).toUInt(&ok);
                                    if(!ok)
                                        qDebug() << "Syntaxe error into: mapweight not number: " << heightmoisureSplit.at(2);
                                    else
                                    {
                                        const uint32_t luckweight=heightmoisureSplit.at(3).toUInt(&ok);
                                        if(!ok)
                                            qDebug() << "Syntaxe error into: height not number: " << heightmoisureSplit.at(3);
                                        else
                                        {
                                            LoadMap::TerrainMonster terrainMonster;
                                            //terrainMonster.luckweight=luckweight;
                                            terrainMonster.mapweight=mapweight;
                                            terrainMonster.monster=monsterId;
                                            if(height>=5)
                                            {
                                                std::cerr << "height>=5" << std::endl;
                                                abort();
                                            }
                                            if(moisure>=6)
                                            {
                                                std::cerr << "moisure>=6" << std::endl;
                                                abort();
                                            }
                                            std::map<unsigned int,std::vector<LoadMap::TerrainMonster> > &terrainMonsters=LoadMap::terrainList[height][moisure].terrainMonsters;
                                            if(terrainMonsters.find(luckweight)==terrainMonsters.cend())
                                                terrainMonsters.insert(std::pair<unsigned int,std::vector<LoadMap::TerrainMonster> >(luckweight,std::vector<LoadMap::TerrainMonster>()));
                                            std::vector<LoadMap::TerrainMonster> &terrainMonstersVector=terrainMonsters[luckweight];
                                            terrainMonstersVector.push_back(terrainMonster);
                                        }
                                    }
                                }
                            }
                        }
                        heightmoisureId++;
                    }
                }
                settings.endGroup();
            }
            indexGroupId++;
        }
    }
    settings.endGroup();

    {
        QFile inputFile("cities.txt");
        if(inputFile.open(QIODevice::ReadOnly))
        {
            QTextStream in(&inputFile);
            while(!in.atEnd())
            {
                QString line = in.readLine();
                if(!line.isEmpty())
                    config.citiesNames.push_back(line.toStdString());
            }
            inputFile.close();
        }
    }
    {
        QFile inputFile("dialog.txt");
        if(inputFile.open(QIODevice::ReadOnly))
        {
            QTextStream in(&inputFile);
            while(!in.atEnd())
            {
                QString line = in.readLine();
                if(!line.isEmpty())
                    config.npcMessage.push_back(line.toStdString());
            }
            inputFile.close();
        }
    }
}
