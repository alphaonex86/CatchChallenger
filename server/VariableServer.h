#ifndef VARIABLESERVER_H
#define VARIABLESERVER_H

//*
#define DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
#define DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
#define DEBUG_MESSAGE_CLIENT_MOVE
#define DEBUG_MESSAGE_CLIENT_MAP
#define DEBUG_MESSAGE_CLIENT_MAP_BORDER
#define DEBUG_MESSAGE_CLIENT_MAP_OBJECT
#define DEBUG_MESSAGE_CLIENT_RAW_NETWORK
#define DEBUG_MESSAGE_CLIENT_RAW_MAP
//*/

//in ms
#define POKECRAFT_SERVER_MAP_TIME_TO_SEND_MOVEMENT 150
#define POKECRAFT_SERVER_NORMAL_SPEED 200

/** convert the overflow of move into insert, use more cpu than POKECRAFT_SERVER_VISIBILITY_CLEAR
 * need POKECRAFT_SERVER_VISIBILITY_CLEAR enabled
 * Disable on very slow configuration, add <5% of cpu with simple algorithm with 400 benchmark's bot on same map */
#define POKECRAFT_SERVER_MAP_DROP_OVER_MOVE

//put for map, add for local thread
//put this size into the options
#define POKECRAFT_SERVER_RANDOM_LIST_SIZE 512

/// check more, then use more cpu, it's to develop and detect the internal bug
#define POKECRAFT_SERVER_EXTRA_CHECK

/// not end all packet, drop move/insert if remove, ... use very few more cpu, < 1% more cpu
#define POKECRAFT_SERVER_VISIBILITY_CLEAR

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#endif // VARIABLESERVER_H
