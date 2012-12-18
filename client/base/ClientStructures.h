#ifndef POKECRAFT_STRUCTURES_CLIENT_H
#define POKECRAFT_STRUCTURES_CLIENT_H

#include <QObject>
#include <QString>
#include <QPixmap>

namespace Pokecraft {

struct ItemToSell
{
    quint32 object;
    quint32 price;
    quint32 quantity;
};
enum BuyStat
{
    BuyStat_Done,
    BuyStat_BetterPrice,
    BuyStat_HaveNotQuantity,
    BuyStat_PriceHaveChanged
};
enum SoldStat
{
    SoldStat_Done,
    SoldStat_BetterPrice,
    SoldStat_WrongQuantity,
    SoldStat_PriceHaveChanged
};

}

#endif // STRUCTURES_CLIENT_H
