#include "../base/ClientHeavyLoad.h"
#include "../base/GlobalData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"

using namespace Pokecraft;

void ClientHeavyLoad::loadRecipes()
{
    //recipes
    QString queryText;
    switch(GlobalData::serverSettings.database.type)
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
    QSqlQuery recipesQuery(queryText);
    while(recipesQuery.next())
    {
        recipeId=recipesQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(GlobalData::serverPrivateVariables.crafingRecipes.contains(recipeId))
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
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    QByteArray outputData2;
    QDataStream out2(&outputData2, QIODevice::WriteOnly);
    out2.setVersion(QDataStream::Qt_4_4);

    //do the query
    QString queryText;
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT item_id,quantity FROM item WHERE player_id=\"%1\"")
                .arg(player_informations->id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT item_id,quantity FROM item WHERE player_id=\"%1\"")
                .arg(player_informations->id);
        break;
    }
    bool ok;
    QSqlQuery loginQuery(queryText);

    //parse the result
    while(loginQuery.next())
    {
        quint32 id=loginQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            emit message(QString("item id is not a number, skip"));
            continue;
        }
        quint32 quantity=loginQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            emit message(QString("quantity is not a number, skip"));
            continue;
        }
        if(quantity==0)
        {
            QString queryText;
            switch(GlobalData::serverSettings.database.type)
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
        if(!GlobalData::serverPrivateVariables.items.contains(id))
        {
            emit message(QString("The item %1 is ignored because it's not into the items list").arg(id));
            continue;
        }
        player_informations->public_and_private_informations.items[id]=quantity;

        out2 << (quint32)id;
        out2 << (quint32)quantity;
    }

    out << (quint32)player_informations->public_and_private_informations.items.size();
    //send the items
    emit sendPacket(0xD0,0x0001,outputData+outputData2);
}
