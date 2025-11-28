#ifndef CATCHCHALLENGER_EPOLLCLIENTLIST_H
#define CATCHCHALLENGER_EPOLLCLIENTLIST_H

#include "ClientWithMapEpoll.hpp"
#include "../base/ClientList.hpp"

class EpollClientList : public CatchChallenger::ClientList
{
public:
     /* list start at (max player*map id)
     * size is 8Bits if max connected player < 255 else 16Bit
     * only used in individual player diff
     * that's resolve visible player to connected player
     *
     * DIFF
       * if ordened no problem
       * else NEED: add new player at end, remove imply:
         * move all the rest
         * or invalid the entry
           * and browsing invalid entry
           * or have status invalid entry too with index corelation
     * diff profile is more diff than insert/remove
     * diff CHOOSEN via vector:
       * insert O(n), can be resolved into O(1) if removed index list
       * remove O(n), can be resolved into O(1) if save into Client internal map index
       * diff O(n) should be fast, large hole is rare
       * while(index) if map player[index].id_db==local player[index].id_db send move, else send insert/remove
       * memory: map count*max visible player*8Bits/16Bits index of connected player
       * memory: static array: NO memory allocation, lot of not used but allocated memory, can be dynamic
     * diff BAD via unordered_map:
       * insert O(log(n))
       * remove O(log(n))
       * diff O(2n*log(n))
     * diff via map (imply index is id_db), see doc/algo/visibility/constant-time-player-visibility.png:
       * insert O(2log(n))
       * remove O(2log(n))
       * diff O(n) doble iterator to scan each (imply status player is map too to keep same order)
       * if found then send pos else send insert/remove
       * memory: id_db 32Bits map+8Bits/16Bits index of connected player*player on map
       * memory: only with dynamic memory allocation
     * diff linked list, see doc/algo/visibility/linked-list-player-visibility.png (std::forward_list can't be used, need more pointer):
       * insert O(1)
       * remove O(1)
       * diff O(n) doble iterator to scan each
         * random acess is not predictible then generate cache miss, the need of new player at end for the diff algo randomize the data
         * or reinsert at first hole + index corelantion with player status
       * if found then send pos else send insert/remove
       * memory: dynamic and O(n) players, can use index 8 or 8/16Bits and not pointer, but due to index to the next entry it duplicate the memory size (divide by /2 the data density)
       * memory: only with dynamic memory allocation
     * move BAD: have local copy: imply resolve each time to change position/orientation at EACH move + function call (and may abstraction resolv)
     * move CHOOSEN: just keep index connected to access only at move when need
     * store CHOOSEN index connected player imply read each player attribute
     * store id_db imply read each player attribute only for insert + store resolution id_db to Object
     */
    EpollClientList();
    std::vector<ClientWithMapEpoll> clients;//65535 = empty slot, else pending auth
public:
    ClientWithMapEpoll &getByReference();
    void remove(const CatchChallenger::Client &client);

    SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED size() const;
    bool empty(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const;
    const CatchChallenger::Client &at(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const;//abort if index is not valid
    CatchChallenger::Client &rw(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index);//abort if index is not valid
    SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED connected_size() const;
    CatchChallenger::ClientWithMap &rwWithMap(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const;//abort if index is not valid
};

#endif // CATCHCHALLENGER_CLIENTLIST_H
