#ifndef POKECRAFT_STRUCTURES_CLIENT_H
#define POKECRAFT_STRUCTURES_CLIENT_H

#include <QObject>
#include <QString>
#include <QPixmap>

namespace Pokecraft {

enum Plant_collect
{
    Plant_collect_correctly_collected=0x01,
    Plant_collect_empty_dirt=0x02,
    Plant_collect_owned_by_another_player=0x03,
    Plant_collect_impossible=0x04
};

}

#endif // STRUCTURES_CLIENT_H
