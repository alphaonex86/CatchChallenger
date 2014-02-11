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
    const QString &text_plant=QStringLiteral("plant");
    const QString &text_dotpng=QStringLiteral(".png");
    const QString &text_dotgif=QStringLiteral(".gif");
    const QString &basePath=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLANTS);
    QHash<quint8,CatchChallenger::Plant>::const_iterator i = CatchChallenger::CommonDatapack::commonDatapack.plants.constBegin();
    while (i != CatchChallenger::CommonDatapack::commonDatapack.plants.constEnd()) {
        //try load the tileset
        PlantExtra plant;
        plant.tileset = new Tiled::Tileset(text_plant,16,32);
        const QString &path=basePath+QString::number(i.key())+text_dotpng;
        if(!plant.tileset->loadFromImage(QImage(path),path))
        {
            const QString &path=basePath+QString::number(i.key())+text_dotgif;
            if(!plant.tileset->loadFromImage(QImage(path),path))
                if(!plant.tileset->loadFromImage(QImage(QStringLiteral(":/images/plant/unknow_plant.png")),QStringLiteral(":/images/plant/unknow_plant.png")))
                    qDebug() << "Unable the load the default plant tileset";
        }
        plantExtra[i.key()]=plant;
        itemToPlants[CatchChallenger::CommonDatapack::commonDatapack.plants[i.key()].itemUsed]=i.key();
        ++i;
    }

    qDebug() << QStringLiteral("%1 plant(s) extra loaded").arg(plantExtra.size());
}
