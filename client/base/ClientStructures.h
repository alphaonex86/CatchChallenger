#ifndef POKECRAFT_CLIENT_STRUCTURES_H
#define POKECRAFT_CLIENT_STRUCTURES_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QDomElement>

namespace Pokecraft {

struct ItemToSellOrBuy
{
    quint32 object;
    quint32 price;
    quint32 quantity;
};
struct Bot
{
    QHash<quint8,QDomElement> step;
    QHash<QString,QString> properties;
};

}

#endif // POKECRAFT_CLIENT_STRUCTURES_H
