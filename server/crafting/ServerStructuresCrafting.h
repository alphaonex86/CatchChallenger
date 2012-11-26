#ifndef POKECRAFT_SERVER_STRUCTURES_CRAFTING_H
#define POKECRAFT_SERVER_STRUCTURES_CRAFTING_H

namespace Pokecraft {

struct Plant
{
    quint32 itemUsed;
    quint8 mature_hours;
    float quantity;
};

}

#endif // POKECRAFT_SERVER_STRUCTURES_CRAFTING_H
