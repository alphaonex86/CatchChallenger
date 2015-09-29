#ifndef COMMMONSETTINGSCOMMON_H
#define COMMMONSETTINGSCOMMON_H

#include <string>
#include <vector>

class CommonSettingsCommon
{
public:
    bool automatic_account_creation;
    std::string httpDatapackMirrorBase;
    std::vector<char> datapackHashBase;

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    uint8_t max_character;//if 0, not allowed, else the character max allowed
    uint8_t min_character;
    uint8_t max_pseudo_size;
    uint32_t character_delete_time;//in seconds
    #endif

    uint8_t maxPlayerMonsters;
    uint16_t maxWarehousePlayerMonsters;
    uint8_t maxPlayerItems;
    uint16_t maxWarehousePlayerItems;

    static CommonSettingsCommon commonSettingsCommon;
};

#endif // COMMMONSETTINGSCOMMON_H
