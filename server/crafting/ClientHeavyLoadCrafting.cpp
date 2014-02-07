#include "../base/ClientHeavyLoad.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
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
            queryText=QStringLiteral("SELECT `recipe` FROM `recipes` WHERE `character`=%1").arg(player_informations->character_id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT recipe FROM recipes WHERE character=%1").arg(player_informations->character_id);
        break;
    }
    bool ok;
    quint32 recipeId;
    QSqlQuery recipesQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!recipesQuery.exec(queryText))
        emit message(recipesQuery.lastQuery()+QLatin1String(": ")+recipesQuery.lastError().text());
    while(recipesQuery.next())
    {
        recipeId=recipesQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.crafingRecipes.contains(recipeId))
                player_informations->public_and_private_informations.recipes << recipeId;
            else
                emit message(QStringLiteral("recipeId: %1 is not into recipe list").arg(recipeId));
        }
        else
            emit message(QStringLiteral("recipeId: %1 is not a number").arg(recipesQuery.value(0).toString()));
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
            queryText=QStringLiteral("SELECT `item`,`quantity`,`place` FROM `item` WHERE `character`=%1")
                .arg(player_informations->character_id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT item,quantity,place FROM item WHERE character=%1")
                .arg(player_informations->character_id);
        break;
    }
    bool ok;
    QSqlQuery itemQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!itemQuery.exec(queryText))
        emit message(itemQuery.lastQuery()+QLatin1String(": ")+itemQuery.lastError().text());
    //parse the result
    while(itemQuery.next())
    {
        quint32 id=itemQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            emit message(QLatin1String("item id is not a number, skip"));
            continue;
        }
        quint32 quantity=itemQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            emit message(QLatin1String("quantity is not a number, skip"));
            continue;
        }
        if(itemQuery.value(2).toString().isEmpty())
        {
            emit message(QLatin1String("item warehouse is not a number, skip"));
            continue;
        }
        bool warehouse;
        if(itemQuery.value(2).toString()==QLatin1String("warehouse"))
            warehouse=true;
        else
        {
            if(itemQuery.value(2).toString()==QLatin1String("wear"))
                warehouse=false;
            else if(itemQuery.value(2).toString()==QLatin1String("market"))
                continue;
            else
            {
                emit message(QStringLiteral("unknow wear type: %1 for item %2 and player %3").arg(itemQuery.value(9).toString()).arg(id).arg(player_informations->character_id));
                continue;
            }
        }
        if(quantity==0)
        {
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    queryText=QStringLiteral("DELETE FROM `item` WHERE `character`=%1 AND `item`=%2 AND `place`='%3'")
                                         .arg(player_informations->character_id)
                                         .arg(id)
                                         .arg(itemQuery.value(2).toString());
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    queryText=QStringLiteral("DELETE FROM item WHERE character=%1 AND item=%2 AND place='%3'")
                                         .arg(player_informations->character_id)
                                         .arg(id)
                                         .arg(itemQuery.value(2).toString());
                break;
            }
            emit message(QStringLiteral("The item %1 have been dropped because the quantity is 0").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.items.item.contains(id))
        {
            emit message(QStringLiteral("The item %1 is ignored because it's not into the items list").arg(id));
            continue;
        }
        if(!warehouse)
            player_informations->public_and_private_informations.items[id]=quantity;
        else
            player_informations->public_and_private_informations.warehouse_items[id]=quantity;
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
    out << (quint32)player_informations->public_and_private_informations.warehouse_items.size();
    QHashIterator<quint32,quint32> j(player_informations->public_and_private_informations.warehouse_items);
    while (j.hasNext()) {
        j.next();
        out << (quint32)j.key();
        out << (quint32)j.value();
    }

    //send the items
    emit sendFullPacket(0xD0,0x0001,outputData);
}
