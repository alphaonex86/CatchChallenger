#ifndef POKECRAFT_CLIENT_STRUCTURES_H
#define POKECRAFT_CLIENT_STRUCTURES_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QDomElement>

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
struct Bot
{
    QHash<quint8,QDomElement> step;
    QHash<QString,QString> properties;
};

}

#endif // POKECRAFT_CLIENT_STRUCTURES_H
