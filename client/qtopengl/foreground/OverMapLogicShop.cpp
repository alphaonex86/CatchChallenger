#include "OverMapLogic.hpp"
#include "../ConnexionManager.hpp"

void OverMapLogic::haveShopList(const std::vector<CatchChallenger::ItemToSellOrBuy> &/*items*/)
{
    // handled by Shop screen directly via UniqueConnection
}

void OverMapLogic::haveBuyObject(const CatchChallenger::BuyStat &stat,const uint32_t &newPrice)
{
    // handled by Shop screen directly via UniqueConnection
    Q_UNUSED(stat);
    Q_UNUSED(newPrice);
}

void OverMapLogic::haveSellObject(const CatchChallenger::SoldStat &stat,const uint32_t &newPrice)
{
    // handled by Shop screen directly via UniqueConnection
    Q_UNUSED(stat);
    Q_UNUSED(newPrice);
}
