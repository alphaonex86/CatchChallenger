#include "../base/interface/DatapackClientLoader.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"

#include <QDomElement>
#include <QDomDocument>
#include <QFile>
#include <QByteArray>
#include <QDebug>

void DatapackClientLoader::parsePlantsExtra()
{
    QHash<quint8,CatchChallenger::Plant>::const_iterator i = CatchChallenger::CommonDatapack::commonDatapack.plants.constBegin();
    while (i != CatchChallenger::CommonDatapack::commonDatapack.plants.constEnd()) {
        //try load the tileset
        PlantExtra plant;
        plant.tileset = new Tiled::Tileset(QStringLiteral("plant"),16,32);
        if(!plant.tileset->loadFromImage(QImage(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLANTS)+QString::number(i.key())+QStringLiteral(".png")),datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLANTS)+QString::number(i.key())+QStringLiteral(".png")))
            if(!plant.tileset->loadFromImage(QImage(QStringLiteral(":/images/plant/unknow_plant.png")),QStringLiteral(":/images/plant/unknow_plant.png")))
                qDebug() << "Unable the load the default plant tileset";
        plantExtra[i.key()]=plant;
        itemToPlants[CatchChallenger::CommonDatapack::commonDatapack.plants[i.key()].itemUsed]=i.key();
        ++i;
    }

    qDebug() << QStringLiteral("%1 plant(s) extra loaded").arg(plantExtra.size());
}
