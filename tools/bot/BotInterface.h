/** \file BotInterface.h
\brief Define the class of bot plugin
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef BOT_INTERFACE_H
#define BOT_INTERFACE_H

#include <QVariant>
#include <QString>
#include <QStringList>

#include "../../client/base/Api_protocol.h"
#include "../../general/base/CommonSettings.h"

/// \brief To define the interface, to pass the facility object from Ultracopier to the plugins without compatibility problem
//not possible to be static, because in the plugin it's not resolved
class BotInterface : public QObject
{
    Q_OBJECT
public:
    virtual QVariant getValue(const QString &variable) = 0;
    virtual bool setValue(const QString &variable,const QVariant &value) = 0;
    virtual QStringList variablesList() = 0;
    virtual void removeClient(CatchChallenger::Api_protocol *api) = 0;
    virtual QString name() = 0;
    virtual QString version() = 0;
    virtual void insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction) = 0;
};

#endif // BOT_INTERFACE_H
