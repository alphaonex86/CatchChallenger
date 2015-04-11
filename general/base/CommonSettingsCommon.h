#ifndef COMMMONSETTINGSLOGIN_H
#define COMMMONSETTINGSLOGIN_H

#include <QtGlobal>
#include <QString>

class CommonSettingsLogin
{
public:
    QString httpDatapackMirrorBase;
    quint8 max_character;//if 0, not allowed, else the character max allowed
    quint8 min_character;
    quint8 max_pseudo_size;
    quint32 character_delete_time;//in seconds
    QByteArray datapackHashBase;
    quint8 maxPlayerMonsters;
    quint16 maxWarehousePlayerMonsters;
    quint8 maxPlayerItems;
    quint16 maxWarehousePlayerItems;

    static CommonSettingsLogin commonSettingsLogin;
};

#endif // COMMMONSETTINGSLOGIN_H
