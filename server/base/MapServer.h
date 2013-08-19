#ifndef CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVER_H
#define CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVER_H

#include "../../general/base/Map.h"
#include "../crafting/MapServerCrafting.h"

#include <QSet>
#include <QList>
#include <QMultiHash>

namespace CatchChallenger {
class MapVisibilityAlgorithm_Simple;
class ClientLocalBroadcast;

class MapServer : public Map, public MapServerCrafting
{
public:
    QList<ClientLocalBroadcast *> clientsForBroadcast;//manipulated by thread of ClientLocalBroadcast(), frequent remove/insert due to map change
    QMultiHash<QPair<quint8,quint8>,quint32> shops;
    QSet<QPair<quint8,quint8> > learn;
    QSet<QPair<quint8,quint8> > heal;
    QSet<QPair<quint8,quint8> > market;
    QHash<QPair<quint8,quint8>,QString> zonecapture;
    QHash<QPair<quint8,quint8>,Orientation> rescue;
    QMultiHash<QPair<quint8,quint8>,quint32> botsFight;
};

class Map_server_MapVisibility_simple : public MapServer
{
public:
    /** \todo
     * Posibility to use:
     * int getBit(int a, int nb)
    {
        return (nb >> a) & 1;
    }
    or:
    #define get(s, r, n, o) 	(((s) >> ((r) * (n) + (o))) & ((1 << (n)) - 1))
    #define set(s, r, n, o, b) \
    ((s) = ((s) & ~(((1 << (n)) - 1) << \
        ((r) * (n) + (o)))) | ((b) << ((r) * (n) + (o))))
        or:
        (map[n/8]>>(n%8))&1
     or:
     typedef uint32_t word_t;
    enum { BITS_PER_WORD = sizeof(word_t) * CHAR_BIT };
    #define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
    #define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)

    void set_bit(word_t *words, int n) {
    words[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
    }

    void clear_bit(word_t *words, int n) {
    words[WORD_OFFSET(n)] &= ~(1 << BIT_OFFSET(n));
    }

    int get_bit(word_t *words, int n) {
    word_t bit = words[WORD_OFFSET(n)] & (1 << BIT_OFFSET(n));
    return bit != 0;
    }
    Or quadtree usage, cf google: quadtree
     * */

    QList<MapVisibilityAlgorithm_Simple *> clients;//manipulated by thread of ClientMapManagement()

    bool show;
};
}

#endif
