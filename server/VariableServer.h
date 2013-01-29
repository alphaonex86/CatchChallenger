#ifndef POKECRAFT_VARIABLESERVER_H
#define POKECRAFT_VARIABLESERVER_H

#define DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
/*
#define DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
#define DEBUG_MESSAGE_CLIENT_MOVE
//*/

/*
#define DEBUG_MESSAGE_CLIENT_RAW_NETWORK
#define DEBUG_MESSAGE_MAP_LOAD
#define DEBUG_MESSAGE_MAP_PLANTS
#define DEBUG_MESSAGE_BUFF_LOAD
#define DEBUG_MESSAGE_SKILL_LOAD
#define DEBUG_MESSAGE_MONSTER_XP_LOAD
//*/
#define DEBUG_MESSAGE_MONSTER_LOAD

//in ms
#define POKECRAFT_SERVER_MAP_TIME_TO_SEND_MOVEMENT 150
#define POKECRAFT_SERVER_NORMAL_SPEED 250 //then 250ms, see the protocol

#define POKECRAFT_SERVER_OWNER_TIMEOUT 60*60*24
#define RANDOM_FLOAT_PART_DIVIDER 10000

/** map visibility bandwith optimisation
 Do in define to not drop cpu performance, due to heavy call **/

/** convert the overflow of move into insert, use more cpu than POKECRAFT_SERVER_VISIBILITY_CLEAR
 * Disable on very slow configuration, add <5% of cpu with simple algorithm with 400 benchmark's bot on same map */
#define POKECRAFT_SERVER_MAP_DROP_OVER_MOVE
/// not send all packet, drop move/insert if remove, ... use very few more cpu, < 1% more cpu
#define POKECRAFT_SERVER_VISIBILITY_CLEAR
/// \brief drop the previous if stopped step, need POKECRAFT_SERVER_MAP_DROP_OVER_MOVE
#define POKECRAFT_SERVER_MAP_DROP_STOP_MOVE

//one of this line
#define POKECRAFT_SERVER_FIGHT_SAVE_MODE_BY_TURN//more safe in crash case, but use more power
//#define POKECRAFT_SERVER_FIGHT_SAVE_MODE_AFTER_ALL//less safe in crash case, but use less power

//put for map, add for local thread
//put this size into the options
#define POKECRAFT_SERVER_RANDOM_LIST_SIZE 512
#define POKECRAFT_SERVER_MIN_RANDOM_LIST_SIZE 128

/// check more, then use more cpu, it's to develop and detect the internal bug
#define POKECRAFT_SERVER_EXTRA_CHECK

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#endif // VARIABLESERVER_H
