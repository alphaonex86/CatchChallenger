#include "../base/ClientHeavyLoad.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

void ClientHeavyLoad::loadRecipes()
{
    //recipes
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_recipes_by_player_id.isEmpty())
    {
        /*emit */error(QStringLiteral("loadRecipes() Query is empty, bug"));
        return;
    }
    #endif
    bool ok;
    QSqlQuery recipesQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!recipesQuery.exec(GlobalServerData::serverPrivateVariables.db_query_select_recipes_by_player_id.arg(player_informations->character_id)))
        /*emit */message(recipesQuery.lastQuery()+QLatin1String(": ")+recipesQuery.lastError().text());
    while(recipesQuery.next())
    {
        const quint32 &recipeId=recipesQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.crafingRecipes.contains(recipeId))
                player_informations->public_and_private_informations.recipes << recipeId;
            else
                /*emit */message(QStringLiteral("recipeId: %1 is not into recipe list").arg(recipeId));
        }
        else
            /*emit */message(QStringLiteral("recipeId: %1 is not a number").arg(recipesQuery.value(0).toString()));
    }
}

void ClientHeavyLoad::loadItems()
{
    //do the query
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_items_by_player_id.isEmpty())
    {
        /*emit */error(QStringLiteral("loadItems() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_item_by_charater_item_place.isEmpty())
    {
        /*emit */error(QStringLiteral("loadItems() Query remove is empty, bug"));
        return;
    }
    #endif
    bool ok;
    QSqlQuery itemQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!itemQuery.exec(GlobalServerData::serverPrivateVariables.db_query_select_items_by_player_id.arg(player_informations->character_id)))
        /*emit */message(itemQuery.lastQuery()+QLatin1String(": ")+itemQuery.lastError().text());
    //parse the result
    while(itemQuery.next())
    {
        const quint32 &id=itemQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            /*emit */message(QLatin1String("item id is not a number, skip"));
            continue;
        }
        const quint32 &quantity=itemQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            /*emit */message(QLatin1String("quantity is not a number, skip"));
            continue;
        }
        if(itemQuery.value(2).toString().isEmpty())
        {
            /*emit */message(QLatin1String("item warehouse is not a number, skip"));
            continue;
        }
        bool warehouse;
        if(itemQuery.value(2).toString()==ClientHeavyLoad::text_warehouse)
            warehouse=true;
        else
        {
            if(itemQuery.value(2).toString()==ClientHeavyLoad::text_wear)
                warehouse=false;
            else if(itemQuery.value(2).toString()==ClientHeavyLoad::text_market)
                continue;
            else
            {
                /*emit */message(QStringLiteral("unknow wear type: %1 for item %2 and player %3").arg(itemQuery.value(9).toString()).arg(id).arg(player_informations->character_id));
                continue;
            }
        }
        if(quantity==0)
        {
            QSqlQuery removeItemQuery(*GlobalServerData::serverPrivateVariables.db);
            if(!removeItemQuery.exec(GlobalServerData::serverPrivateVariables.db_query_delete_item_by_charater_item_place.arg(player_informations->character_id).arg(id).arg(itemQuery.value(2).toString())))
                /*emit */message(itemQuery.lastQuery()+QLatin1String(": ")+itemQuery.lastError().text());
            /*emit */message(QStringLiteral("The item %1 have been dropped because the quantity is 0").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.items.item.contains(id))
        {
            /*emit */message(QStringLiteral("The item %1 is ignored because it's not into the items list").arg(id));
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
    /*emit */sendFullPacket(0xD0,0x0001,outputData);
}
