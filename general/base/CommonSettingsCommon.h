#ifndef COMMMONSETTINGSCOMMON_H
#define COMMMONSETTINGSCOMMON_H

#include <QtGlobal>
#include <QString>

class CommonSettingsCommon
{
public:
    bool automatic_account_creation;
    QString httpDatapackMirrorBase;
    QByteArray datapackHashBase;

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    quint8 max_character;//if 0, not allowed, else the character max allowed
    quint8 min_character;
    quint8 max_pseudo_size;
    quint32 character_delete_time;//in seconds
    #endif

    quint8 maxPlayerMonsters;
    quint16 maxWarehousePlayerMonsters;
    quint8 maxPlayerItems;
    quint16 maxWarehousePlayerItems;

    static CommonSettingsCommon commonSettingsCommon;
};

#endif // COMMMONSETTINGSCOMMON_H
