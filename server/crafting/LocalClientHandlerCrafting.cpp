#include "../base/LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "../base/GlobalData.h"

using namespace Pokecraft;

void LocalClientHandler::useSeed(const quint8 &plant_id)
{
    if(objectQuantity(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed)==0)
    {
        emit error(QString("The player have not the item id: %1 to plant as seed").arg(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed));
        return;
    }
    else
    {
        removeObject(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed);
        emit seedValidated();
    }
}
