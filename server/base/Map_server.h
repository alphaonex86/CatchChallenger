#ifndef MAP_SERVER_H
#define MAP_SERVER_H

#include "../../general/base/Map.h"

namespace Pokecraft {
class MapVisibilityAlgorithm_Simple;

class Map_server_MapVisibility_simple : public Map
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
	
	Map_server_MapVisibility_simple();

	QList<MapVisibilityAlgorithm_Simple *> clients;//manipulated by thread of ClientMapManagement()

	bool show;
};
}

#endif // MAP_SERVER_H
