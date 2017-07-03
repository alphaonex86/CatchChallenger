#include "LoadMapAll.h"

#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"

#include <unordered_set>
#include <unordered_map>

std::vector<LoadMapAll::City> LoadMapAll::cities;

void LoadMapAll::addDebugCity(Tiled::Map &worldMap, unsigned int mapWidth, unsigned int mapHeight)
{
    Tiled::ObjectGroup *layerCity=new Tiled::ObjectGroup("City",0,0,worldMap.width(),worldMap.height());
    layerCity->setColor(QColor("#bb5555"));
    worldMap.addLayer(layerCity);

    unsigned int index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);

        std::string citySize;
        switch(city.type) {
        case CityType_small:
            citySize="small";
            break;
        case CityType_medium:
            citySize="medium";
            break;
        default:
            citySize="big";
            break;
        }
        const uint32_t x=city.x;
        const uint32_t y=city.y;
        QPolygonF poly(QRectF(0,0,mapWidth,mapHeight));
        Tiled::MapObject *objectPolygon = new Tiled::MapObject(QString::fromStdString(city.name+" ("+citySize+")"),"",QPointF(mapWidth*x,mapHeight*y), QSizeF(0.0,0.0));
        objectPolygon->setPolygon(poly);
        objectPolygon->setShape(Tiled::MapObject::Polygon);
        layerCity->addObject(objectPolygon);

        index++;
    }
}

void LoadMapAll::addCity(const Grid &grid, const std::vector<std::string> &citiesNames)
{
    std::unordered_map<uint32_t,std::unordered_map<uint32_t,uint32_t> > positionsAndIndex;
    unsigned int index=0;
    while(index<grid.size())
    {
        const Point &centroid=grid.at(index);

        const uint32_t x=centroid.x();
        const uint32_t y=centroid.y();
        if(positions.find(x)==positions.cend() || (positions.find(x)!=positions.cend() && positions.at(x).find(y)==positions.at(x).cend()))
        {
            City city;
            city.name=citiesNames.at(index);
            switch(rand()%3) {
            case 0:
                city.type=CityType_small;
                break;
            case 1:
                city.type=CityType_medium;
                break;
            default:
                city.type=CityType_big;
                break;
            }
            city.x=x;
            city.y=y;
            positionsAndIndex[x][y]=cities.size();
            cities.push_back(city);
        }

        index++;
    }

    //do top list of map with number of direct neighbor
    index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);
        if(x>0)
        {

        }
        index++;
    }
    //drop the first and decremente their neighbor
}
