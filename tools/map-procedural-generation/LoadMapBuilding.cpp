// Building templates: discovery of the template/ folders, staging of their
// tilesets into the generated datapack, door/exit wiring and the regeneration
// of the interior content from each template's floor-N.xml skeleton.
//
// A template folder holds ONE exterior tmx (the facade brushed onto the city
// ground) and its floor-N.tmx interiors with a floor-N.xml skeleton. The
// skeleton says WHICH bots exist and which step types they run (text, shop,
// sell, heal, warehouse, fight); every content is regenerated here for the city
// the building is placed in, so the same template gives a coherent shop in a
// level 5 village and in a level 60 capital.
#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../../general/base/customtinyxml2.hpp"

#include <libtiled/tileset.h>
#include <libtiled/tile.h>
#include <libtiled/tilelayer.h>
#include <libtiled/objectgroup.h>
#include <libtiled/mapobject.h>
#include <libtiled/mapreader.h>

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QDomDocument>
#include <QTextStream>

#include <cstring>
#include <iostream>

std::map<std::string,LoadMapAll::BuildingGroup> LoadMapAll::buildingGroups;
std::vector<std::string> LoadMapAll::cityStyles;

//every text the generator produced, with the context needed to regenerate it
//with a local LLM (npcfill.py). One entry per CDATA of a generated xml.
std::vector<std::string> npcTextSlots;

//true when ANY layer named "Collisions" has a tile at that cell (the engine
//OR-merges them, so a cell is blocked as soon as one layer blocks it)
static bool collisionAt(Tiled::Map *map,const int x,const int y)
{
    if(x<0 || y<0 || x>=map->width() || y>=map->height())
        return true;
    bool blocked=false;
    int layerIndex=0;
    while(layerIndex<map->layerCount())
    {
        Tiled::Layer *layer=map->layerAt(layerIndex);
        if(layer->isTileLayer() && layer->name()=="Collisions")
        {
            const Tiled::TileLayer *tileLayer=static_cast<const Tiled::TileLayer *>(layer);
            if(tileLayer->cellAt(x,y).tile()!=NULL)
                blocked=true;
        }
        layerIndex++;
    }
    return blocked;
}

//every door/teleport object of a map, whatever its properties (getDoorsListAndTp
//only returns the ones already carrying a "map" property)
static std::vector<Tiled::MapObject*> teleportObjects(Tiled::Map *map)
{
    std::vector<Tiled::MapObject*> objects;
    int layerIndex=0;
    while(layerIndex<map->layerCount())
    {
        Tiled::Layer *layer=map->layerAt(layerIndex);
        if(layer->isObjectGroup())
        {
            Tiled::ObjectGroup *group=static_cast<Tiled::ObjectGroup *>(layer);
            const QList<Tiled::MapObject*> &list=group->objects();
            int index=0;
            while(index<list.size())
            {
                const QString &type=list.at(index)->type();
                if(type=="door" || type=="teleport on it" || type=="teleport on push")
                    objects.push_back(list.at(index));
                index++;
            }
        }
        layerIndex++;
    }
    return objects;
}

static Tiled::ObjectGroup *objectGroupOrCreate(Tiled::Map *map,const QString &name)
{
    Tiled::ObjectGroup *group=LoadMap::searchObjectGroupByName(*map,name);
    if(group==NULL)
    {
        group=new Tiled::ObjectGroup(name,0,0);
        map->addLayer(group);
    }
    return group;
}

//LoadMap::searchTilesetByName() aborts when the tileset is absent, and an
//interior legitimately may not use the invisible tileset at all
static Tiled::Tileset *tilesetByNameOrNull(Tiled::Map *map,const QString &name)
{
    int tilesetIndex=0;
    while(tilesetIndex<map->tilesetCount())
    {
        const Tiled::SharedTileset tileset=map->tilesetAt(tilesetIndex);
        if(tileset->name()==name)
            return tileset.get();
        tilesetIndex++;
    }
    return NULL;
}

//marker cell of the map's own invisible tileset: MapBrush only copies an object
//that HAS a cell, and Tiled shows the teleport where the author can see it
static void setMarkerCell(Tiled::Map *map,Tiled::MapObject *object,const int tileIndex)
{
    Tiled::Tileset *invisible=tilesetByNameOrNull(map,"invisible");
    if(invisible!=NULL)
    {
        Tiled::Cell cell;
        cell.setTile(invisible->tileAt(tileIndex));
        object->setCell(cell);
    }
}

void LoadMapAll::wireBuildingDoors(BuildingVariant &variant)
{
    Tiled::Map * const exterior=variant.mapTemplate.tiledMap;
    Tiled::Map *floor0=NULL;
    if(!variant.mapTemplate.otherMap.empty())
        floor0=variant.mapTemplate.otherMap.front();
    if(floor0==NULL)
    {
        std::cerr << "Building template without interior: " << variant.folder << std::endl;
        abort();
    }

    //1) the exterior doorstep. A hand placed door wins (the author knows where
    //the door graphic is); else take the centre column of the facade and drop
    //to the first free cell under the building body — which is what every
    //shipped template does.
    int doorX=-1,doorY=-1;
    std::vector<Tiled::MapObject*> exteriorDoors=teleportObjects(exterior);
    if(!exteriorDoors.empty())
    {
        doorX=(int)(exteriorDoors.front()->x()/exterior->tileWidth());
        doorY=(int)(exteriorDoors.front()->y()/exterior->tileHeight());
    }
    else
    {
        const int width=exterior->width();
        const int height=exterior->height();
        int column=0;
        while(column<width && doorX<0)
        {
            //centre first, then alternate left/right
            const int offset=(column+1)/2;
            const int candidate=(column%2==0) ? width/2-offset : width/2+offset;
            if(candidate>=0 && candidate<width)
            {
                int lowest=-1;
                int row=0;
                while(row<height)
                {
                    if(collisionAt(exterior,candidate,row))
                        lowest=row;
                    row++;
                }
                if(lowest>=0)
                {
                    int step=lowest+1;
                    while(step<height && collisionAt(exterior,candidate,step))
                        step++;
                    doorX=candidate;
                    doorY=step;//may be height: the doorstep is the city ground below
                }
            }
            column++;
        }
        if(doorX<0)
        {
            //a facade with no collision at all: put the door under its middle
            doorX=width/2;
            doorY=height;
        }
    }

    //2) the interior exit, on the bottom row of the interior (the doorway gap
    //when the wall has one, else the wall cell the player pushes into), and the
    //landing cell above it
    int exitX=-1,exitY=-1;
    std::vector<Tiled::MapObject*> interiorExits=teleportObjects(floor0);
    if(!interiorExits.empty())
    {
        exitX=(int)(interiorExits.front()->x()/floor0->tileWidth());
        exitY=(int)(interiorExits.front()->y()/floor0->tileHeight());
    }
    else
    {
        const int width=floor0->width();
        const int height=floor0->height();
        const int row=height-1;
        int best=-1;
        int column=0;
        while(column<width)
        {
            if(!collisionAt(floor0,column,row))
            {
                if(best<0 || abs(column-width/2)<abs(best-width/2))
                    best=column;
            }
            column++;
        }
        if(best<0)
        {
            //full wall on the bottom row: exit ON the wall, under a free cell
            column=0;
            while(column<width)
            {
                if(!collisionAt(floor0,column,row-1))
                {
                    if(best<0 || abs(column-width/2)<abs(best-width/2))
                        best=column;
                }
                column++;
            }
        }
        if(best<0)
            best=width/2;
        exitX=best;
        exitY=row+1;//object coordinates carry the engine -1 tile offset
    }

    //3) where the player lands inside: the free cell above the exit tile
    int spawnX=exitX;
    int spawnY=exitY-2;
    while(spawnY>0 && collisionAt(floor0,spawnX,spawnY))
        spawnY--;
    if(spawnY<0)
        spawnY=0;

    variant.doorX=(unsigned int)doorX;
    variant.doorY=(unsigned int)doorY;
    variant.spawnX=(unsigned int)spawnX;
    variant.spawnY=(unsigned int)spawnY;

    //4) (re)write both objects. The values of a hand made template are NOT
    //trusted: several shipped ones point at a spawn outside their interior.
    if(exteriorDoors.empty())
    {
        Tiled::MapObject *door=new Tiled::MapObject(QString(),"teleport on push",
            QPointF(doorX*exterior->tileWidth(),doorY*exterior->tileHeight()),
            QSizeF(exterior->tileWidth(),exterior->tileHeight()));
        setMarkerCell(exterior,door,3);
        objectGroupOrCreate(exterior,"Moving")->addObject(door);
        exteriorDoors.push_back(door);
    }
    {
        Tiled::Properties properties=exteriorDoors.front()->properties();
        properties["map"]=QString::fromStdString(variant.mapTemplate.otherMapName.front());
        properties["x"]=QString::number(spawnX);
        properties["y"]=QString::number(spawnY);
        exteriorDoors.front()->setProperties(properties);
    }
    if(interiorExits.empty())
    {
        Tiled::MapObject *exit=new Tiled::MapObject(QString(),"teleport on push",
            QPointF(exitX*floor0->tileWidth(),exitY*floor0->tileHeight()),
            QSizeF(floor0->tileWidth(),floor0->tileHeight()));
        setMarkerCell(floor0,exit,3);
        objectGroupOrCreate(floor0,"Moving")->addObject(exit);
        interiorExits.push_back(exit);
    }
    {
        //addBuildingChain() recognises the way out by its "map" property being
        //the exterior name, and offsets x/y by the position of the building
        Tiled::Properties properties=interiorExits.front()->properties();
        properties["map"]=QString::fromStdString(variant.exterior);
        properties["x"]=QString::number(doorX);
        properties["y"]=QString::number(doorY);
        interiorExits.front()->setProperties(properties);
    }
}

Tiled::SharedTileset LoadMapAll::stageTemplateTileset(Tiled::Map &worldMap,const QString &tsxPath)
{
    const QFileInfo source(tsxPath);
    const QString destinationDir=QCoreApplication::applicationDirPath()+"/dest/map/tileset/";
    const QString destination=destinationDir+source.fileName();
    if(!QFile::exists(destination))
    {
        QDir().mkpath(destinationDir);
        //the tsx references its image relatively, so the image goes next to it
        QFile tsxFile(source.absoluteFilePath());
        if(!tsxFile.open(QFile::ReadOnly))
        {
            std::cerr << "Unable to read the template tileset " << source.absoluteFilePath().toStdString() << std::endl;
            abort();
        }
        const QString content=QString::fromUtf8(tsxFile.readAll());
        tsxFile.close();
        int imageIndex=content.indexOf("<image source=\"");
        while(imageIndex>=0)
        {
            const int start=imageIndex+(int)strlen("<image source=\"");
            const int end=content.indexOf("\"",start);
            if(end>start)
            {
                const QString image=content.mid(start,end-start);
                const QString imageSource=QFileInfo(source.absolutePath()+"/"+image).absoluteFilePath();
                const QString imageDestination=destinationDir+QFileInfo(image).fileName();
                if(!QFile::exists(imageDestination))
                {
                    if(!QFile::copy(imageSource,imageDestination))
                    {
                        std::cerr << "Unable to stage the tileset image " << imageSource.toStdString()
                                  << " into " << imageDestination.toStdString() << std::endl;
                        abort();
                    }
                }
            }
            imageIndex=content.indexOf("<image source=\"",imageIndex+1);
        }
        if(!QFile::copy(source.absoluteFilePath(),destination))
        {
            std::cerr << "Unable to stage the tileset " << source.absoluteFilePath().toStdString()
                      << " into " << destination.toStdString() << std::endl;
            abort();
        }
    }
    //already in the world?
    int tilesetIndex=0;
    while(tilesetIndex<worldMap.tilesetCount())
    {
        const Tiled::SharedTileset tileset=worldMap.tilesetAt(tilesetIndex);
        const QString worldPath=QFileInfo(QCoreApplication::applicationDirPath()+
                                          "/dest/map/main/official/"+tileset->fileName()).absoluteFilePath();
        if(worldPath==QFileInfo(destination).absoluteFilePath())
            return tileset;
        tilesetIndex++;
    }
    LoadMap::readTileset("tileset/"+source.fileName(),&worldMap);
    return worldMap.tilesetAt(worldMap.tilesetCount()-1);
}

LoadMapAll::BuildingGroup *LoadMapAll::buildingGroup(const std::string &name)
{
    std::map<std::string,BuildingGroup>::iterator group=buildingGroups.find(name);
    if(group==buildingGroups.cend())
        return NULL;
    return &group->second;
}

//load one variant folder: the exterior tmx (the only tmx that is not a floor)
//and, through loadMapTemplate(), its interior chain
static bool loadBuildingVariant(const QString &groupName,const QString &variantName,
                                LoadMapAll::BuildingVariant &variant,
                                const unsigned int mapWidth,const unsigned int mapHeight,
                                Tiled::Map &worldMap)
{
    QString relative=groupName+"/";
    if(!variantName.isEmpty())
        relative+=variantName+"/";
    const QDir folder(QCoreApplication::applicationDirPath()+"/template/"+relative);
    const QStringList tmxList=folder.entryList(QStringList("*.tmx"),QDir::Files,QDir::Name);
    QString exterior;
    int index=0;
    while(index<tmxList.size())
    {
        if(!tmxList.at(index).startsWith("floor-"))
        {
            if(!exterior.isEmpty())
            {
                std::cerr << "Building template " << relative.toStdString()
                          << " has more than one exterior tmx" << std::endl;
                abort();
            }
            exterior=tmxList.at(index);
            exterior.remove(exterior.size()-4,4);
        }
        index++;
    }
    if(exterior.isEmpty())
        return false;
    variant.folder=relative.toStdString();
    variant.exterior=exterior.toStdString();
    LoadMapAll::loadMapTemplate(relative.toUtf8().constData(),variant.mapTemplate,exterior,
                                mapWidth,mapHeight,worldMap);
    LoadMapAll::wireBuildingDoors(variant);
    return true;
}

void LoadMapAll::scanBuildingTemplates(Tiled::Map &worldMap,const unsigned int mapWidth,const unsigned int mapHeight)
{
    buildingGroups.clear();
    cityStyles.clear();
    //like LoadMap::readMap(), every runtime path is relative to the BINARY
    const QDir templateDir(QCoreApplication::applicationDirPath()+"/template");
    if(!templateDir.exists())
    {
        std::cerr << "template/ not found (run the generator from its source directory)" << std::endl;
        abort();
    }
    //stage every tileset the templates use into the generated datapack, so a
    //written map never references a tsx that is not shipped with it
    {
        const QStringList groups=templateDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
        int groupIndex=0;
        while(groupIndex<groups.size())
        {
            QDirIterator tmxIterator(templateDir.absoluteFilePath(groups.at(groupIndex)),
                                     QStringList("*.tmx"),QDir::Files,QDirIterator::Subdirectories);
            while(tmxIterator.hasNext())
            {
                const QString tmxPath=tmxIterator.next();
                QFile tmxFile(tmxPath);
                if(!tmxFile.open(QFile::ReadOnly))
                {
                    std::cerr << "Unable to read " << tmxPath.toStdString() << std::endl;
                    abort();
                }
                const QString content=QString::fromUtf8(tmxFile.readAll());
                tmxFile.close();
                int sourceIndex=content.indexOf("<tileset firstgid=");
                while(sourceIndex>=0)
                {
                    const int start=content.indexOf("source=\"",sourceIndex);
                    const int end=(start<0) ? -1 : content.indexOf("\"",start+8);
                    if(start>=0 && end>start)
                    {
                        const QString tsx=QFileInfo(QFileInfo(tmxPath).absolutePath()+"/"+
                                                    content.mid(start+8,end-start-8)).absoluteFilePath();
                        if(QFile::exists(tsx))
                            stageTemplateTileset(worldMap,tsx);
                        else
                        {
                            std::cerr << "Template " << tmxPath.toStdString() << " references a missing tileset: "
                                      << content.mid(start+8,end-start-8).toStdString()
                                      << " (run template-check.py --fix)" << std::endl;
                            abort();
                        }
                    }
                    sourceIndex=content.indexOf("<tileset firstgid=",sourceIndex+1);
                }
            }
            groupIndex++;
        }
    }
    const QStringList groups=templateDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
    int groupIndex=0;
    while(groupIndex<groups.size())
    {
        const QString groupName=groups.at(groupIndex);
        BuildingGroup group;
        group.name=groupName.toStdString();
        const QDir groupDir(templateDir.absoluteFilePath(groupName));
        const QStringList variantNames=groupDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
        if(variantNames.isEmpty())
        {
            BuildingVariant variant;
            if(loadBuildingVariant(groupName,QString(),variant,mapWidth,mapHeight,worldMap))
                group.variants.push_back(variant);
        }
        else
        {
            int variantIndex=0;
            while(variantIndex<variantNames.size())
            {
                BuildingVariant variant;
                if(loadBuildingVariant(groupName,variantNames.at(variantIndex),variant,
                                       mapWidth,mapHeight,worldMap))
                    group.variants.push_back(variant);
                variantIndex++;
            }
        }
        if(!group.variants.empty())
        {
            buildingGroups[group.name]=group;
            if(groupName.endsWith("-city"))
                cityStyles.push_back(group.name);
        }
        groupIndex++;
    }
    std::cout << "Building templates: " << buildingGroups.size() << " groups, "
              << cityStyles.size() << " city styles" << std::endl;
}

//deterministic index from a text seed: the same building of the same city
//always gets the same line, so two runs of the generator are identical
static size_t seedPick(const std::string &seed,const size_t &count)
{
    if(count==0)
        return 0;
    size_t hash=1469598103934665603ULL;
    size_t index=0;
    while(index<seed.size())
    {
        hash^=(size_t)(unsigned char)seed.at(index);
        hash*=1099511628211ULL;
        index++;
    }
    return hash%count;
}

static QString jsonEscape(const QString &text)
{
    QString out;
    int index=0;
    while(index<text.size())
    {
        const QChar character=text.at(index);
        if(character=='"' || character=='\\')
            out+="\\"+QString(character);
        else if(character=='\n')
            out+="\\n";
        else if(character.unicode()<0x20)
            out+="\\u"+QString("%1").arg((int)character.unicode(),4,16,QChar('0'));
        else
            out+=character;
        index++;
    }
    return out;
}

//role of a bot, from the step types its skeleton runs
static std::string botRole(const QStringList &stepTypes)
{
    if(stepTypes.contains("heal"))
        return "healer";
    if(stepTypes.contains("shop") || stepTypes.contains("sell"))
        return "shopkeeper";
    if(stepTypes.contains("warehouse"))
        return "storage";
    if(stepTypes.contains("fight"))
        return "trainer";
    return "villager";
}

static std::string citySizeName(const LoadMapAll::CityType &type)
{
    if(type==LoadMapAll::CityType_small)
        return "small";
    if(type==LoadMapAll::CityType_medium)
        return "medium";
    return "big";
}

//record one generated text so npcfill.py can replace it with an LLM line, and
//return the OFFLINE fallback that is written right now (the datapack is always
//complete and playable without ever running the LLM pass)
static QString npcText(const std::string &destinationFile,const unsigned int &cdataIndex,
                       const std::string &role,const std::string &field,
                       const LoadMapAll::City &city,const std::string &buildingKind,
                       const SettingsAll::SettingsExtra &setting,const std::string &seed)
{
    QString fallback;
    if(field=="name")
    {
        static const char *const firstNames[]={
            "Aldric","Bryn","Cora","Dane","Edda","Finn","Gwen","Hale","Ivo","Juna",
            "Kael","Lyra","Mira","Nuri","Oren","Pell","Quin","Rhea","Soren","Tova",
            "Ulf","Vera","Wren","Xan","Yara","Zane","Bram","Cleo","Dora","Esra",
            "Faye","Gus","Hana","Iris","Jad","Kira","Loris","Mads","Noa","Otto"};
        const size_t count=sizeof(firstNames)/sizeof(firstNames[0]);
        const QString first=firstNames[seedPick(seed,count)];
        if(role=="healer")
            fallback="Nurse "+first;
        else if(role=="shopkeeper")
            fallback="Clerk "+first;
        else if(role=="storage")
            fallback="Storage system";
        else
            fallback=first;
    }
    else if(role=="healer")
    {
        static const char *const lines[]={
            "Let me restore your team.",
            "Your team is in perfect shape again.",
            "Rest here as long as you need."};
        fallback=lines[seedPick(seed,3)];
    }
    else if(role=="shopkeeper")
    {
        static const char *const lines[]={
            "Welcome to our shop!",
            "Everything on these shelves is for sale.",
            "Stock up before you take the road."};
        fallback=lines[seedPick(seed,3)];
    }
    else if(role=="storage")
    {
        static const char *const lines[]={
            "Welcome to the monster storage system.",
            "Your spare team members are safe with us.",
            "The storage is ready when you are."};
        fallback=lines[seedPick(seed,3)];
    }
    else if(role=="trainer" || role=="gym leader")
    {
        if(field=="win")
        {
            static const char *const lines[]={
                "Well fought.",
                "You earned that one.",
                "My team still has a lot to learn."};
            fallback=lines[seedPick(seed,3)];
        }
        else if(role=="gym leader")
            fallback="I am the leader here. Prove your worth!";
        else
        {
            static const char *const lines[]={
                "You look strong - let's battle!",
                "I have been waiting for a real opponent.",
                "Show me what your team can do!"};
            fallback=lines[seedPick(seed,3)];
        }
    }
    else if(!setting.npcMessage.empty())
        fallback=QString::fromStdString(setting.npcMessage.at(seedPick(seed,setting.npcMessage.size())));
    else
        fallback="...";

    npcTextSlots.push_back(std::string("{\"file\":\"")+destinationFile+
        "\",\"cdata\":"+std::to_string(cdataIndex)+
        ",\"field\":\""+field+
        "\",\"role\":\""+role+
        "\",\"building\":\""+buildingKind+
        "\",\"city\":\""+city.name+
        "\",\"style\":\""+city.style+
        "\",\"size\":\""+citySizeName(city.type)+
        "\",\"element\":\""+city.elementType+
        "\",\"level\":"+std::to_string((unsigned int)city.level)+
        ",\"fallback\":\""+jsonEscape(fallback).toStdString()+"\"}");
    return fallback;
}

QString LoadMapAll::fightStepXml(const unsigned int &stepId,const bool &leader,
                                 const SettingsAll::SettingsExtra &setting,
                                 const std::vector<RoadMonster> &monsterPool,const uint8_t &level,
                                 const std::string &gymTypeName,const std::vector<std::string> &gymTypeMonsters,
                                 const QString &startText,const QString &winText)
{
    (void)setting;
    //team size 1..4 (same math as the road trainers), the leader fields one more
    int monsterCount=rand()%2 + rand()%3 + 1;
    if(leader)
        monsterCount++;
    int reward=level*30+100;
    QString monsterXml;
    int indexMonster=0;
    while(indexMonster<monsterCount)
    {
        //in a typed gym part of the team comes from the type pool, and the
        //leader's last monster (the ace) always does
        bool useGymPool=false;
        if(!gymTypeMonsters.empty())
        {
            if(monsterPool.empty())
                useGymPool=true;
            else if(leader && indexMonster==monsterCount-1)
                useGymPool=true;
            else
                useGymPool=(rand()%2==0);
        }
        int monsterLevel=level*(95+rand()%10)/100;
        if(monsterLevel<1)
            monsterLevel=1;
        QString monsterId;
        if(useGymPool)
            monsterId=QString::fromStdString(gymTypeMonsters.at(rand()%gymTypeMonsters.size()));
        else
            monsterId=monsterRef(monsterPool.at(rand()%monsterPool.size()).monsterId,setting);
        monsterXml+="      <monster id=\""+monsterId+"\" level=\""+QString::number(monsterLevel)+"\"/>\n";
        reward+=monsterLevel*monsterLevel;
        indexMonster++;
    }
    if(leader)
        reward*=2;
    QString out="    <step id=\""+QString::number(stepId)+"\" type=\"fight\">\n";
    QString start=startText;
    if(start.isEmpty())
    {
        if(leader && !gymTypeName.empty())
            start="I am the leader here. My "+QString::fromStdString(gymTypeName)+" team will test you!";
        else if(leader)
            start="I am the leader here. Prove your worth!";
        else
            start="You look strong - let's battle!";
    }
    QString win=winText;
    if(win.isEmpty())
        win="Well fought.";
    start.replace("]]>","]]&gt;");
    win.replace("]]>","]]&gt;");
    out+="      <start><![CDATA["+start+"]]></start>\n";
    out+="      <win><![CDATA["+win+"]]></win>\n";
    out+=monsterXml;
    out+="      <gain cash=\""+QString::number(reward)+"\"/>\n";
    out+="    </step>\n";
    return out;
}

//the <a href="..">..</a> of a skeleton text ARE the bot flow (which step the
//player jumps to), so they are kept as is and only the prose is regenerated
static std::string extractLinks(const char * const text)
{
    std::string out;
    if(text==NULL)
        return out;
    const std::string content(text);
    std::string::size_type position=content.find("<a ");
    while(position!=std::string::npos)
    {
        const std::string::size_type end=content.find("</a>",position);
        if(end==std::string::npos)
            position=std::string::npos;
        else
        {
            if(!out.empty())
                out+="<br />";
            out+=content.substr(position,end+4-position);
            position=content.find("<a ",end+4);
        }
    }
    return out;
}

//highest city level of the world: the shop catalogue and the trainer teams
//scale on it, so the same template is a village store or a capital mall
static uint8_t maxCityLevel()
{
    uint8_t maximum=1;
    unsigned int index=0;
    while(index<LoadMapAll::cities.size())
    {
        if(LoadMapAll::cities.at(index).level>maximum)
            maximum=LoadMapAll::cities.at(index).level;
        index++;
    }
    return maximum;
}

struct SkeletonStep
{
    unsigned int id;
    std::string type;
    std::string links;
};

QString LoadMapAll::interiorBotXml(const BuildingVariant &variant,const std::string &floorName,
                                   const std::string &destinationFile,
                                   Tiled::Map *floorMap,const BotKind &kind,const City &city,
                                   const SettingsAll::SettingsExtra &setting,
                                   const std::vector<RoadMonster> &monsterPool,const uint8_t &level,
                                   const std::string &gymTypeName,const std::vector<std::string> &gymTypeMonsters,
                                   std::vector<Tiled::MapObject*> &injectedBots)
{
    //1) the skeleton of template/<folder>/<floor>.xml: which bot runs which
    //steps. Only the STRUCTURE is used, every content is regenerated below.
    std::map<unsigned int,std::vector<SkeletonStep> > skeleton;
    {
        const std::string path=QCoreApplication::applicationDirPath().toStdString()+
                               "/template/"+variant.folder+floorName+".xml";
        tinyxml2::XMLDocument document;
        if(document.LoadFile(path.c_str())==tinyxml2::XML_SUCCESS)
        {
            const tinyxml2::XMLElement * const root=document.RootElement();
            const tinyxml2::XMLElement *botElement=(root==NULL)?NULL:root->FirstChildElement("bot");
            while(botElement!=NULL)
            {
                unsigned int botId=0;
                if(botElement->Attribute("id")!=NULL)
                    botId=(unsigned int)atoi(botElement->Attribute("id"));
                if(botId>0)
                {
                    std::vector<SkeletonStep> steps;
                    const tinyxml2::XMLElement *stepElement=botElement->FirstChildElement("step");
                    while(stepElement!=NULL)
                    {
                        SkeletonStep step;
                        step.id=1;
                        if(stepElement->Attribute("id")!=NULL)
                            step.id=(unsigned int)atoi(stepElement->Attribute("id"));
                        if(stepElement->Attribute("type")!=NULL)
                            step.type=stepElement->Attribute("type");
                        const tinyxml2::XMLElement * const textElement=stepElement->FirstChildElement("text");
                        if(textElement!=NULL)
                            step.links=extractLinks(textElement->GetText());
                        steps.push_back(step);
                        stepElement=stepElement->NextSiblingElement("step");
                    }
                    skeleton[botId]=steps;
                }
                botElement=botElement->NextSiblingElement("bot");
            }
        }
        else
            std::cerr << "No bot skeleton for " << path << " (text bots only)" << std::endl;
    }

    //2) the bot OBJECTS of the floor: they carry the position, the look
    //direction and the skin the template author placed. Fix what the engine
    //would refuse: a missing type, a duplicated id, a skin the datapack has not.
    std::vector<Tiled::MapObject*> botObjects;
    {
        //the engine reads bots ONLY from the group named "Object"
        //(Map_loaderMain.cpp), so a bot the template author left in another
        //group (several shipped templates keep them in "Moving") is moved here
        Tiled::ObjectGroup * const group=objectGroupOrCreate(floorMap,"Object");
        {
            std::vector<Tiled::MapObject*> misplaced;
            int layerIndex=0;
            while(layerIndex<floorMap->layerCount())
            {
                Tiled::Layer * const layer=floorMap->layerAt(layerIndex);
                if(layer->isObjectGroup() && layer!=group)
                {
                    Tiled::ObjectGroup * const other=static_cast<Tiled::ObjectGroup *>(layer);
                    const QList<Tiled::MapObject*> &objects=other->objects();
                    int index=0;
                    while(index<objects.size())
                    {
                        Tiled::MapObject * const object=objects.at(index);
                        if(object->type()=="bot" || (object->properties().contains("id")
                                && object->type()!="door" && object->type()!="teleport on it"
                                && object->type()!="teleport on push"))
                            misplaced.push_back(object);
                        index++;
                    }
                }
                layerIndex++;
            }
            unsigned int misplacedIndex=0;
            while(misplacedIndex<misplaced.size())
            {
                Tiled::MapObject * const object=misplaced.at(misplacedIndex);
                std::cerr << "Template " << variant.folder << floorName
                          << ".tmx: bot object outside the \"Object\" group, moved" << std::endl;
                static_cast<Tiled::ObjectGroup *>(object->objectGroup())->removeObject(object);
                group->addObject(object);
                misplacedIndex++;
            }
        }
        {
            std::vector<unsigned int> usedIds;
            const QList<Tiled::MapObject*> &objects=group->objects();
            int index=0;
            while(index<objects.size())
            {
                Tiled::MapObject * const object=objects.at(index);
                Tiled::Properties properties=object->properties();
                if(properties.contains("id"))
                {
                    if(object->type()!="bot")
                    {
                        std::cerr << "Template " << variant.folder << floorName
                                  << ".tmx: bot object without type=\"bot\", fixed" << std::endl;
                        object->setType("bot");
                    }
                    unsigned int botId=properties.value("id").toUInt();
                    if(botId==0)
                        botId=1;
                    while(vectorcontainsAtLeastOne(usedIds,botId) && botId<255)
                        botId++;
                    usedIds.push_back(botId);
                    properties["id"]=QString::number(botId);
                    //an unknown skin makes a bot without sprite: fall back on the
                    //role skin configured for this datapack
                    const QString skin=properties.value("skin").toString();
                    if(skin.isEmpty() ||
                            !QDir(QCoreApplication::applicationDirPath()+"/skin").exists(skin))
                    {
                        std::string replacement=setting.botSkins.empty()?std::string():
                            setting.botSkins.at(seedPick(variant.folder+floorName+std::to_string(botId),
                                                         setting.botSkins.size()));
                        if(kind==BotKind_heal && botId==1)
                            replacement=setting.healSkin;
                        else if(kind==BotKind_shop && botId==1)
                            replacement=setting.shopSkin;
                        else if(kind==BotKind_fight)
                            replacement=setting.gymTrainerSkin;
                        if(!replacement.empty())
                            properties["skin"]=QString::fromStdString(replacement);
                    }
                    object->setProperties(properties);
                    botObjects.push_back(object);
                }
                index++;
            }
        }
    }

    //3) one <bot> per object, with the steps of the skeleton refilled
    QString out;
    unsigned int cdataIndex=0;
    unsigned int nameIndex=0;
    unsigned int lastFightBot=0;
    unsigned int highestBotId=0;
    {
        unsigned int index=0;
        while(index<botObjects.size())
        {
            const unsigned int botId=botObjects.at(index)->properties().value("id").toUInt();
            if(botId>highestBotId)
                highestBotId=botId;
            if(skeleton.find(botId)!=skeleton.cend())
            {
                const std::vector<SkeletonStep> &steps=skeleton.at(botId);
                unsigned int stepIndex=0;
                while(stepIndex<steps.size())
                {
                    if(steps.at(stepIndex).type=="fight")
                        lastFightBot=botId;
                    stepIndex++;
                }
            }
            index++;
        }
    }

    //a GYM keeps its historical shape: the template draws the room and its
    //leader (the fight bot it declares), the number of extra TRAINERS is the
    //[building] gymTrainers setting. They are added to the shared template and
    //removed by the caller once the map is written.
    if(kind==BotKind_fight && floorName=="floor-0" && setting.gymTrainers>0)
    {
        Tiled::ObjectGroup * const group=objectGroupOrCreate(floorMap,"Object");
        const int tileWidth=floorMap->tileWidth();
        const int tileHeight=floorMap->tileHeight();
        unsigned int placed=0;
        unsigned int nextBotId=highestBotId+1;
        //free cells the player can reach (at least one free orthogonal
        //neighbour), spread by two cells so the trainers do not block each
        //other in a narrow gym room
        std::vector<std::pair<int,int> > taken;
        int row=1;
        while(row<floorMap->height()-1 && placed<setting.gymTrainers)
        {
            int column=1;
            while(column<floorMap->width()-1 && placed<setting.gymTrainers)
            {
                bool usable=!collisionAt(floorMap,column,row);
                if(usable)
                    usable=(!collisionAt(floorMap,column,row+1) || !collisionAt(floorMap,column,row-1)
                            || !collisionAt(floorMap,column-1,row) || !collisionAt(floorMap,column+1,row));
                if(usable && column==(int)variant.spawnX && row>=(int)variant.spawnY)
                    usable=false;//never block the way in
                unsigned int takenIndex=0;
                while(takenIndex<taken.size() && usable)
                {
                    if(abs(taken.at(takenIndex).first-column)<2 && abs(taken.at(takenIndex).second-row)<2)
                        usable=false;
                    takenIndex++;
                }
                if(usable)
                {
                    Tiled::MapObject * const trainer=new Tiled::MapObject(QString(),"bot",
                        QPointF(column*tileWidth,(row+1)*tileHeight),QSizeF(tileWidth,tileHeight));
                    trainer->setProperty("id",QString::number(nextBotId));
                    trainer->setProperty("lookAt","bottom");
                    trainer->setProperty("skin",QString::fromStdString(setting.gymTrainerSkin));
                    setMarkerCell(floorMap,trainer,3);
                    group->addObject(trainer);
                    injectedBots.push_back(trainer);
                    botObjects.push_back(trainer);
                    taken.push_back(std::pair<int,int>(column,row));
                    //synthetic skeleton: a gym trainer only fights
                    SkeletonStep step;
                    step.id=1;
                    step.type="fight";
                    skeleton[nextBotId]=std::vector<SkeletonStep>(1,step);
                    nextBotId++;
                    placed++;
                }
                column++;
            }
            row++;
        }
        if(placed<setting.gymTrainers)
            std::cerr << "Gym template " << variant.folder << ": only " << placed << " of "
                      << setting.gymTrainers << " trainers fit in the room" << std::endl;
    }
    unsigned int index=0;
    while(index<botObjects.size())
    {
        Tiled::MapObject * const object=botObjects.at(index);
        const Tiled::Properties properties=object->properties();
        const unsigned int botId=properties.value("id").toUInt();
        std::vector<SkeletonStep> steps;
        if(skeleton.find(botId)!=skeleton.cend())
            steps=skeleton.at(botId);
        if(steps.empty())
        {
            //an object with no <bot> in the skeleton still has to talk
            SkeletonStep step;
            step.id=1;
            step.type="text";
            steps.push_back(step);
        }
        QStringList stepTypes;
        {
            unsigned int stepIndex=0;
            while(stepIndex<steps.size())
            {
                stepTypes.append(QString::fromStdString(steps.at(stepIndex).type));
                stepIndex++;
            }
        }
        const std::string role=botRole(stepTypes);
        const std::string seed=destinationFile+"/"+std::to_string(botId);

        out+="  <bot id=\""+QString::number(botId)+"\"";
        if(properties.contains("lookAt"))
            out+=" lookAt=\""+properties.value("lookAt").toString()+"\"";
        out+=">\n";
        out+="    <name>"+npcText(destinationFile,nameIndex,role,"name",city,variant.folder,
                                  setting,seed)+"</name>\n";
        nameIndex++;

        unsigned int stepIndex=0;
        while(stepIndex<steps.size())
        {
            const SkeletonStep &step=steps.at(stepIndex);
            const QString stepId=QString::number(step.id);
            if(step.type=="text")
            {
                QString message=npcText(destinationFile,cdataIndex,role,"text",city,variant.folder,
                                        setting,seed+"/"+std::to_string(step.id));
                cdataIndex++;
                //a literal ]]> would close the CDATA section early
                message.replace("]]>","]]&gt;");
                if(!step.links.empty())
                    message+="<br />"+QString::fromStdString(step.links);
                out+="    <step id=\""+stepId+"\" type=\"text\"><text><![CDATA["+message+
                     "]]></text></step>\n";
            }
            else if(step.type=="shop")
            {
                //the catalogue grows with the level of the city: a village sells
                //the basics, the capital the whole list
                out+="    <step id=\""+stepId+"\" type=\"shop\">\n";
                size_t productCount=setting.shopItems.size();
                if(productCount>1)
                {
                    productCount=1+(setting.shopItems.size()-1)*(size_t)level/(size_t)maxCityLevel();
                    if(productCount>setting.shopItems.size())
                        productCount=setting.shopItems.size();
                }
                size_t itemIndex=0;
                while(itemIndex<productCount)
                {
                    out+="      <product item=\""+QString::number(setting.shopItems.at(itemIndex))+"\"/>\n";
                    itemIndex++;
                }
                out+="    </step>\n";
            }
            else if(step.type=="fight")
            {
                if(monsterPool.empty() && gymTypeMonsters.empty())
                {
                    //no usable monster around: a fight step would reference an
                    //unknown monster, so the bot only talks
                    QString message=npcText(destinationFile,cdataIndex,"villager","text",city,
                                            variant.folder,setting,seed+"/"+std::to_string(step.id));
                    cdataIndex++;
                    message.replace("]]>","]]&gt;");
                    out+="    <step id=\""+stepId+"\" type=\"text\"><text><![CDATA["+message+
                         "]]></text></step>\n";
                }
                else
                {
                    const bool leader=(botId==lastFightBot && kind==BotKind_fight);
                    //the taunt and the defeat line are npc text too
                    const QString startText=npcText(destinationFile,cdataIndex,
                        leader?"gym leader":"trainer","start",city,variant.folder,setting,
                        seed+"/"+std::to_string(step.id)+"/start");
                    cdataIndex++;
                    const QString winText=npcText(destinationFile,cdataIndex,
                        leader?"gym leader":"trainer","win",city,variant.folder,setting,
                        seed+"/"+std::to_string(step.id)+"/win");
                    cdataIndex++;
                    out+=fightStepXml(step.id,leader,setting,
                                      monsterPool,level,gymTypeName,gymTypeMonsters,startText,winText);
                }
            }
            else if(step.type.empty())
                std::cerr << "Skeleton step without type in " << variant.folder << floorName
                          << ".xml (skipped)" << std::endl;
            else
                out+="    <step id=\""+stepId+"\" type=\""+
                     QString::fromStdString(step.type)+"\"/>\n";
            stepIndex++;
        }
        out+="  </bot>\n";
        index++;
    }
    return out;
}

void LoadMapAll::writeNpcSlots(const SettingsAll::SettingsExtra &setting)
{
    (void)setting;
    const QString path=QCoreApplication::applicationDirPath()+"/dest/npc-slots.json";
    QFile file(path);
    if(!file.open(QFile::WriteOnly))
    {
        std::cerr << "Unable to write " << path.toStdString() << std::endl;
        return;
    }
    std::string content="{\"slots\":[\n";
    unsigned int index=0;
    while(index<npcTextSlots.size())
    {
        content+=npcTextSlots.at(index);
        if(index+1<npcTextSlots.size())
            content+=",";
        content+="\n";
        index++;
    }
    content+="]}\n";
    if(file.write(content.c_str(),content.size())!=(qint64)content.size())
        std::cerr << "Short write on " << path.toStdString() << std::endl;
    file.close();
    std::cout << "npc text slots: " << npcTextSlots.size() << " (dest/npc-slots.json)" << std::endl;
}
