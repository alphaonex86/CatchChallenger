#include "../base/Client.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

void Client::loadRecipes()
{
    //recipes
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_recipes_by_player_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadRecipes() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_select_recipes_by_player_id.arg(character_id);
    if(!GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::loadRecipes_static))
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        loadMonsters();
    }
}

void Client::loadRecipes_static(void *object)
{
    static_cast<Client *>(object)->loadRecipes_return();
}

void Client::loadRecipes_return()
{
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        const quint32 &recipeId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.crafingRecipes.contains(recipeId))
                public_and_private_informations.recipes << recipeId;
            else
                normalOutput(QStringLiteral("recipeId: %1 is not into recipe list").arg(recipeId));
        }
        else
            normalOutput(QStringLiteral("recipeId: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
    }
    loadMonsters();
}

void Client::loadItems()
{
    //do the query
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_items_by_player_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadItems() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_item_by_charater_item_place.isEmpty())
    {
        errorOutput(QStringLiteral("loadItems() Query remove is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_select_items_by_player_id.arg(character_id);
    if(!GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::loadItems_static))
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        loadRecipes();
    }
}

void Client::loadItems_static(void *object)
{
    static_cast<Client *>(object)->loadItems_return();
}

void Client::loadItems_return()
{
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        const quint32 &id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QLatin1String("item id is not a number, skip"));
            continue;
        }
        const quint32 &quantity=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QLatin1String("quantity is not a number, skip"));
            continue;
        }
        const QString &warehouseString(GlobalServerData::serverPrivateVariables.db.value(2));
        if(warehouseString.isEmpty())
        {
            normalOutput(QLatin1String("item warehouse is not a number, skip"));
            continue;
        }
        bool warehouse;
        if(warehouseString==Client::text_warehouse)
            warehouse=true;
        else
        {
            if(warehouseString==Client::text_wear)
                warehouse=false;
            else if(warehouseString==Client::text_market)
                continue;
            else
            {
                normalOutput(QStringLiteral("unknow wear type: %1 for item %2 and player %3").arg(warehouseString).arg(id).arg(character_id));
                continue;
            }
        }
        if(quantity==0)
        {
            normalOutput(QStringLiteral("The item %1 have been dropped because the quantity is 0").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.items.item.contains(id))
        {
            normalOutput(QStringLiteral("The item %1 is ignored because it's not into the items list").arg(id));
            continue;
        }
        if(!warehouse)
            public_and_private_informations.items[id]=quantity;
        else
            public_and_private_informations.warehouse_items[id]=quantity;
    }
    loadRecipes();
}

void Client::sendInventory()
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    out << (quint32)public_and_private_informations.items.size();
    QHashIterator<quint32,quint32> i(public_and_private_informations.items);
    while (i.hasNext()) {
        i.next();
        out << (quint32)i.key();
        out << (quint32)i.value();
    }
    out << (quint32)public_and_private_informations.warehouse_items.size();
    QHashIterator<quint32,quint32> j(public_and_private_informations.warehouse_items);
    while (j.hasNext()) {
        j.next();
        out << (quint32)j.key();
        out << (quint32)j.value();
    }

    //send the items
    sendFullPacket(0xD0,0x0001,outputData);
}
