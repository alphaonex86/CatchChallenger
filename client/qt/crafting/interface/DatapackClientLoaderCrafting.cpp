#include "../../QtDatapackClientLoader.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/GeneralVariable.hpp"
#include "../../../../general/base/FacilityLib.hpp"

#include <QFile>
#include <QByteArray>
#include <QDebug>

void QtDatapackClientLoader::parsePlantsExtra()
{
    const std::string &text_plant="plant";
    const std::string &text_dotpng=".png";
    const std::string &text_dotgif=".gif";
    const std::string &basePath=datapackPath+DATAPACK_BASE_PATH_PLANTS;
    auto i = CatchChallenger::CommonDatapack::commonDatapack.plants.begin();
    while (i != CatchChallenger::CommonDatapack::commonDatapack.plants.cend()) {
        //try load the tileset
        QtDatapackClientLoader::QtPlantExtra plant;
        plant.tileset = new Tiled::Tileset(QString::fromStdString(text_plant),16,32);
        const std::string &path=basePath+std::to_string(i->first)+text_dotpng;
        if(!plant.tileset->loadFromImage(QImage(QString::fromStdString(path)),QString::fromStdString(path)))
        {
            const std::string &path=basePath+std::to_string(i->first)+text_dotgif;
            if(!plant.tileset->loadFromImage(QImage(QString::fromStdString(path)),QString::fromStdString(path)))
                if(!plant.tileset->loadFromImage(QImage(QStringLiteral(":/CC/images/plant/unknow_plant.png")),QStringLiteral(":/CC/images/plant/unknow_plant.png")))
                    qDebug() << "Unable the load the default plant tileset";
        }
        QtDatapackClientLoader::datapackLoader->QtplantExtra[i->first]=plant;
        itemToPlants[CatchChallenger::CommonDatapack::commonDatapack.plants[i->first].itemUsed]=i->first;
        ++i;
    }

    qDebug() << QStringLiteral("%1 plant(s) extra loaded").arg(QtDatapackClientLoader::datapackLoader->QtplantExtra.size());
}
