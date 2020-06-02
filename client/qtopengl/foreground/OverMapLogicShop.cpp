#include "OverMapLogic.hpp"
#include "../ConnexionManager.hpp"

void OverMapLogic::haveShopList(const std::vector<CatchChallenger::ItemToSellOrBuy> &items)
{
    abort();
}

void OverMapLogic::haveBuyObject(const CatchChallenger::BuyStat &stat,const uint32_t &newPrice)
{
    const CatchChallenger::ItemToSellOrBuy &itemToSellOrBuy=itemsToBuy.front();
    //std::unordered_map<uint32_t,uint32_t> items;
    switch(stat)
    {
        case CatchChallenger::BuyStat_Done:
            add_to_inventory(itemToSellOrBuy.object,itemToSellOrBuy.quantity);
        break;
        case CatchChallenger::BuyStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() Can't buy at 0$!";
                return;
            }
            addCash(itemToSellOrBuy.price);
            removeCash(newPrice*itemToSellOrBuy.quantity);
            add_to_inventory(itemToSellOrBuy.object,itemToSellOrBuy.quantity);
        break;
        case CatchChallenger::BuyStat_HaveNotQuantity:
            addCash(itemToSellOrBuy.price);
            showTip(tr("Sorry but have not the quantity of this item").toStdString());
        break;
        case CatchChallenger::BuyStat_PriceHaveChanged:
            addCash(itemToSellOrBuy.price);
            showTip(tr("Sorry but now the price is worse").toStdString());
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
    itemsToBuy.erase(itemsToBuy.cbegin());
}

void OverMapLogic::haveSellObject(const CatchChallenger::SoldStat &stat,const uint32_t &newPrice)
{
    waitToSell=false;
    switch(stat)
    {
        case CatchChallenger::SoldStat_Done:
            addCash(itemsToSell.front().price*itemsToSell.front().quantity);
            showTip(tr("Item sold").toStdString());
        break;
        case CatchChallenger::SoldStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() the price 0$ can't be better price!";
                return;
            }
            addCash(newPrice*itemsToSell.front().quantity);
            showTip(tr("Item sold at better price").toStdString());
        break;
        case CatchChallenger::SoldStat_WrongQuantity:
            add_to_inventory(itemsToSell.front().object,itemsToSell.front().quantity,false);
            showTip(tr("Sorry but have not the quantity of this item").toStdString());
        break;
        case CatchChallenger::SoldStat_PriceHaveChanged:
            add_to_inventory(itemsToSell.front().object,itemsToSell.front().quantity,false);
            showTip(tr("Sorry but now the price is worse").toStdString());
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
    itemsToSell.erase(itemsToSell.cbegin());
    //displaySellList();
    abort();
}
