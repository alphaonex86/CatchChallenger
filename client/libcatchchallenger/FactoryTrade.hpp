#ifndef CATCHCHALLENGER_FACTORYTRADE_H
#define CATCHCHALLENGER_FACTORYTRADE_H

#include <cstdint>
#include <vector>

#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/lib.h"

namespace CatchChallenger {
/// \brief factory buy/sell bookkeeping shared by the clients: the queries waiting for the server reply,
/// what each reply means (cash, items, factory stock) and the factory prices/production time.
/// The client applies the Answer to its UI; the stock is the IndustryStatus kept in the player informations.
class DLL_PUBLIC FactoryTrade
{
public:
    enum Result : uint8_t
    {
        Result_Bought,
        Result_BoughtBetterPrice,
        Result_Sold,
        Result_SoldBetterPrice,
        Result_NotTheQuantity,
        Result_PriceWorse,
        Result_WrongReply//no query waiting, or a reply the protocol does not allow
    };
    struct Answer
    {
        Result result;
        bool done;//then apply the industry reputation rewards
        CATCHCHALLENGER_TYPE_ITEM item;
        uint32_t itemsToAdd;
        uint32_t cashToAdd;
        uint32_t cashToRemove;
    };
    void reset();
    //unitPrice is the price shown; return the cash to remove now
    uint32_t buy(const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantity,const uint32_t &unitPrice);
    //the items are removed from the inventory now
    void sell(const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantity,const uint32_t &unitPrice);
    Answer buyAnswer(const BuyStat &stat,const uint32_t &newUnitPrice,IndustryStatus &status,const Industry &industry,const uint64_t &nowSec);
    Answer sellAnswer(const SoldStat &stat,const uint32_t &newUnitPrice,IndustryStatus &status,const Industry &industry,const uint64_t &nowSec);

    static IndustryStatus &industryStatus(Player_private_and_public_informations &informations,const CATCHCHALLENGER_TYPE_MAPID &mapId,const uint16_t &factoryId);
    //resources quantity = what the factory still accepts, a resource not listed is full
    static void statusFromServerList(IndustryStatus &status,const Industry &industry,const uint32_t &remainingProductionTime,const uint64_t &nowSec,
                                     const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products);
    static uint32_t remainingProductionTime(const IndustryStatus &status,const Industry &industry,const uint64_t &nowSec);
    //price after a local buy/sell, quantity = what remains in the list; false if the item is not in the industry
    static bool productPrice(const Industry &industry,const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantityLeft,uint32_t &price);
    static bool resourcePrice(const Industry &industry,const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantityAccepted,uint32_t &price);
private:
    static Answer emptyAnswer();

    std::vector<ItemToSellOrBuy> buys_;
    std::vector<ItemToSellOrBuy> sells_;
};
}

#endif // CATCHCHALLENGER_FACTORYTRADE_H
