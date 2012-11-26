#include "../base/LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "../base/GlobalData.h"

using namespace Pokecraft;

void LocalClientHandler::useSeed(const quint8 &plant_id)
{
    if(!player_informations->public_and_private_informations.items.contains(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed))
    {
        emit error(QString("The player have not the id: %1").arg(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed));
        return;
    }
    else
    {
        player_informations->public_and_private_informations.items[GlobalData::serverPrivateVariables.plants[plant_id].itemUsed]--;
        if(player_informations->public_and_private_informations.items[GlobalData::serverPrivateVariables.plants[plant_id].itemUsed]==0)
        {
            player_informations->public_and_private_informations.items.remove(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed);
            switch(GlobalData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2")
                                 .arg(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed)
                                 .arg(player_informations->id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2")
                             .arg(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed)
                             .arg(player_informations->id)
                             );
                break;
            }
        }
        else
        {
            switch(GlobalData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3;")
                                 .arg(player_informations->public_and_private_informations.items[GlobalData::serverPrivateVariables.plants[plant_id].itemUsed])
                                 .arg(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed)
                                 .arg(player_informations->id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3;")
                             .arg(player_informations->public_and_private_informations.items[GlobalData::serverPrivateVariables.plants[plant_id].itemUsed])
                             .arg(GlobalData::serverPrivateVariables.plants[plant_id].itemUsed)
                             .arg(player_informations->id)
                             );
                break;
            }
        }
        emit seedValidated();
    }
}

void LocalClientHandler::addObject(const quint32 &item,const quint32 &quantity)
{
    if(player_informations->public_and_private_informations.items.contains(item))
    {
        player_informations->public_and_private_informations.items[item]+=quantity;
        switch(GlobalData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3;")
                             .arg(player_informations->public_and_private_informations.items[item])
                             .arg(item)
                             .arg(player_informations->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3;")
                         .arg(player_informations->public_and_private_informations.items[item])
                         .arg(item)
                         .arg(player_informations->id)
                         );
            break;
        }
    }
    else
    {
        switch(GlobalData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("INSERT INTO item(item_id,player_id,quantity) VALUES(%1,%2,%3);")
                             .arg(item)
                             .arg(player_informations->id)
                             .arg(quantity)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("INSERT INTO item(item_id,player_id,quantity) VALUES(%1,%2,%3);")
                         .arg(item)
                         .arg(player_informations->id)
                         .arg(quantity)
                         );
            break;
        }
        player_informations->public_and_private_informations.items[item]=quantity;
    }
}
