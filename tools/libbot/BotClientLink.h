/** \file BotClientLink.h
\brief One bot<->server connection, as the Qt-free bot core sees it
\licence GPL3, see the file COPYING */

#ifndef CATCHCHALLENGER_BOTCLIENTLINK_H
#define CATCHCHALLENGER_BOTCLIENTLINK_H

#include <string>
#include <stdint.h>

#include "../../client/libcatchchallenger/Api_protocol.hpp"

/// \brief The transport of ONE bot connection.
///
/// This is the only thing the bot core (MultipleBotConnectionCore) knows about
/// how a bot is wired to the server. The Qt front-end implements it with its
/// ssl socket plus Api_client_real; a CLI front-end implements it with a POSIX
/// socket and its own Api_protocol subclass. Nothing here may mention a type of
/// either toolkit.
///
/// The protocol object itself is exposed as the Qt-free CatchChallenger::
/// Api_protocol base: every protocol call the core makes (tryLogin,
/// addCharacter, selectCharacter, setMapNumber, stage, ...) is declared there.
/// The one exception is the main+sub datapack request: it is pure virtual on the
/// toolkit protocol subclass and not on Api_protocol, so it lives on the link.
/// (The BASE datapack request is not here: the core never issues it.)
class BotClientLink
{
public:
    BotClientLink();
    virtual ~BotClientLink();

    /// \brief The protocol object driving this link. Never NULL after the link
    /// has been constructed.
    virtual CatchChallenger::Api_protocol *api() const = 0;

    /// \brief Open the transport. An empty proxyHost means "no proxy".
    /// \return false when the transport could not even be started.
    virtual bool connectToHost(const std::string &host,const uint16_t port,
                               const std::string &proxyHost,const uint16_t proxyPort) = 0;
    virtual void disconnectFromHost() = 0;

    virtual void sendDatapackContentMainSub() = 0;
};

#endif // CATCHCHALLENGER_BOTCLIENTLINK_H
