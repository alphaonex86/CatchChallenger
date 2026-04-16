#ifndef CATCHCHALLENGER_DictionaryLogin_H
#define CATCHCHALLENGER_DictionaryLogin_H

#include <vector>
#include <cstdint>

namespace CatchChallenger {
class DictionaryLogin
{
public:
    static std::vector<uint8_t> dictionary_reputation_database_to_internal;//255 == not found
    static std::vector<uint8_t> dictionary_skin_database_to_internal;//255 == not found
    static std::vector<uint8_t> dictionary_starter_database_to_internal;//255 == not found
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    static std::vector<uint8_t> dictionary_skin_internal_to_database;
    static std::vector<uint8_t> dictionary_starter_internal_to_database;
    #endif
};
}

#endif
