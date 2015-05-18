#include "../base/Client.h"
#include "../base/PreparedDBQuery.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

void Client::loadRecipes()
{
    //recipes
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_recipes_by_player_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadRecipes() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=PreparedDBQueryCommon::db_query_select_recipes_by_player_id.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::loadRecipes_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        loadMonsters();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadRecipes_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadRecipes_return();
}

void Client::loadRecipes_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const quint32 &recipeId=QString(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.crafingRecipes.contains(recipeId))
                public_and_private_informations.recipes << recipeId;
            else
                normalOutput(QStringLiteral("recipeId: %1 is not into recipe list").arg(recipeId));
        }
        else
            normalOutput(QStringLiteral("recipeId: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
    }
    loadMonsters();
}

void Client::loadItems()
{
    //do the query
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_items_by_player_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadItems() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=PreparedDBQueryCommon::db_query_select_items_by_player_id.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::loadItems_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        loadItemsWarehouse();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadItems_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadItems_return();
}

void Client::loadItems_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const quint32 &id=QString(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QLatin1String("item id is not a number, skip"));
            continue;
        }
        const quint32 &quantity=QString(GlobalServerData::serverPrivateVariables.db_common->value(1)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QLatin1String("quantity is not a number, skip"));
            continue;
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
        public_and_private_informations.items[id]=quantity;
    }
    loadItemsWarehouse();
}

void Client::loadItemsWarehouse()
{
    //do the query
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadItems() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=PreparedDBQueryCommon::db_query_select_items_warehouse_by_player_id.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::loadItemsWarehouse_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        loadRecipes();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadItemsWarehouse_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadItemsWarehouse_return();
}

void Client::loadItemsWarehouse_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const quint32 &id=QString(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QLatin1String("item id is not a number, skip"));
            continue;
        }
        const quint32 &quantity=QString(GlobalServerData::serverPrivateVariables.db_common->value(1)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QLatin1String("quantity is not a number, skip"));
            continue;
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
        public_and_private_informations.warehouse_items[id]=quantity;
    }
    loadRecipes();
}

void Client::sendInventory()
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    out << (quint16)public_and_private_informations.items.size();
    QHashIterator<quint16,quint32> i(public_and_private_informations.items);
    while (i.hasNext()) {
        i.next();
        out << (quint16)i.key();
        out << (quint32)i.value();
    }
    out << (quint16)public_and_private_informations.warehouse_items.size();
    QHashIterator<quint16,quint32> j(public_and_private_informations.warehouse_items);
    while (j.hasNext()) {
        j.next();
        out << (quint16)j.key();
        out << (quint32)j.value();
    }
    //send the items
    sendFullPacket(0xD0,0x01,outputData.constData(),outputData.size());
}
