/** \file BotInterface.h
\brief Define the class of bot plugin
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef BOT_INTERFACE_H
#define BOT_INTERFACE_H

#include <string>
#include <vector>

#include "../../client/libcatchchallenger/Api_protocol.hpp"

/// \brief The bot "brain": what decides what a connected bot does.
///
/// Qt-free on purpose, so a Qt-free front-end can supply one. The api pointer
/// is the Qt-free CatchChallenger::Api_protocol base; a Qt implementation that
/// needs the Qt subclass static_cast<>s it back (it created the object, so the
/// dynamic type is known -- no RTTI needed).
class BotInterface
{
public:
    BotInterface();
    virtual ~BotInterface();
    /// \return false when the variable is unknown (value untouched)
    virtual bool getValue(const std::string &variable,bool &value) = 0;
    /// \return false when the variable is unknown
    virtual bool setValue(const std::string &variable,const bool value) = 0;
    virtual std::vector<std::string> variablesList() = 0;
    virtual void removeClient(CatchChallenger::Api_protocol *api) = 0;
    virtual std::string name() = 0;
    virtual std::string version() = 0;
    virtual void insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction) = 0;
    virtual void insert_player_all(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);
    virtual void remove_player(CatchChallenger::Api_protocol *api,const SIMPLIFIED_PLAYER_ID_FOR_MAP &id);
    virtual void dropAllPlayerOnTheMap(CatchChallenger::Api_protocol *api);
};

#endif // BOT_INTERFACE_H
