#ifndef CATCHCHALLENGER_DictionaryLogin_H
#define CATCHCHALLENGER_DictionaryLogin_H

#include <vector>
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"

namespace CatchChallenger {
class DictionaryLogin
{
public:
    static std::vector<ActionAllow> dictionary_allow_database_to_internal;
    static std::vector<quint8> dictionary_allow_internal_to_database;
    static std::vector<int> dictionary_reputation_database_to_internal;//negative == not found
    static std::vector<quint8> dictionary_skin_database_to_internal;
    static std::vector<quint32> dictionary_skin_internal_to_database;
    static std::vector<quint8> dictionary_starter_database_to_internal;
    static std::vector<quint32> dictionary_starter_internal_to_database;
};
}

#endif
