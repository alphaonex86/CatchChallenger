#include "../base/ClientHeavyLoad.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

void ClientHeavyLoad::loadRecipes()
{
    //recipes
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT recipe FROM recipes WHERE player=%1").arg(player_informations->id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT recipe FROM recipes WHERE player=%1").arg(player_informations->id);
        break;
    }
    bool ok;
    quint32 recipeId;
    QSqlQuery recipesQuery;
    if(!recipesQuery.exec(queryText))
        emit message(recipesQuery.lastQuery()+": "+recipesQuery.lastError().text());
    while(recipesQuery.next())
    {
        recipeId=recipesQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(GlobalServerData::serverPrivateVariables.crafingRecipes.contains(recipeId))
                player_informations->public_and_private_informations.recipes << recipeId;
            else
                emit message(QString("recipeId: %1 is not into recipe list").arg(recipeId));
        }
        else
            emit message(QString("recipeId: %1 is not a number").arg(recipesQuery.value(0).toString()));
    }
}

void ClientHeavyLoad::loadItems()
{
    //do the query
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT item_id,quantity FROM item WHERE player_id=%1")
                .arg(player_informations->id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT item_id,quantity FROM item WHERE player_id=%1")
                .arg(player_informations->id);
        break;
    }
    bool ok;
    QSqlQuery itemQuery;
    if(!itemQuery.exec(queryText))
        emit message(itemQuery.lastQuery()+": "+itemQuery.lastError().text());
    //parse the result
    while(itemQuery.next())
    {
        quint32 id=itemQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            emit message(QString("item id is not a number, skip"));
            continue;
        }
        quint32 quantity=itemQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            emit message(QString("quantity is not a number, skip"));
            continue;
        }
        if(quantity==0)
        {
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    queryText=QString("DELETE FROM item WHERE player_id=%1 AND item_id=%2")
                                         .arg(player_informations->id)
                                         .arg(id);
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    queryText=QString("DELETE FROM item WHERE player_id=%1 AND item_id=%2")
                                     .arg(player_informations->id)
                                     .arg(id);
                break;
            }
            QSqlQuery loginQuery(queryText);
            emit message(QString("The item %1 have been dropped because the quantity is 0").arg(id));
            continue;
        }
        if(!GlobalServerData::serverPrivateVariables.items.contains(id))
        {
            emit message(QString("The item %1 is ignored because it's not into the items list").arg(id));
            continue;
        }
        player_informations->public_and_private_informations.items[id]=quantity;
    }
}

void ClientHeavyLoad::sendInventory()
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    out << (quint32)player_informations->public_and_private_informations.items.size();
    QHashIterator<quint32,quint32> i(player_informations->public_and_private_informations.items);
    while (i.hasNext()) {
        i.next();
        out << (quint32)i.key();
        out << (quint32)i.value();
    }

    //send the items
    emit sendPacket(0xD0,0x0001,outputData);
}
