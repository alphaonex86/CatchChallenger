/** \file SimpleBotInterface.h
\brief Minimal bot brain: remember where each bot is
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef SIMPLE_BOT_INTERFACE_H
#define SIMPLE_BOT_INTERFACE_H

#include <unordered_map>

#include "../BotInterface.h"

class SimpleBotInterface : public BotInterface
{
public:
    struct Player
    {
        CatchChallenger::Player_public_informations player;
        uint8_t mapId;
        uint8_t x;
        uint8_t y;
        CatchChallenger::Direction direction;
    };

    SimpleBotInterface();
    ~SimpleBotInterface();
    bool getValue(const std::string &variable,bool &value);
    bool setValue(const std::string &variable,const bool value);
    std::vector<std::string> variablesList();
    virtual void removeClient(CatchChallenger::Api_protocol *api);
    std::string name();
    std::string version();
    virtual void insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);
protected:
    bool move;
    bool randomText;
    bool bugInDirection;
    bool globalChatRandomReply;
    std::unordered_map<CatchChallenger::Api_protocol *,Player> clientList;
};

#endif // SIMPLE_BOT_INTERFACE_H
