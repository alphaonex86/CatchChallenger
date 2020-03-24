#include "DictionaryLogin.hpp"

using namespace CatchChallenger;

std::vector<int> DictionaryLogin::dictionary_reputation_database_to_internal;//negative == not found
std::vector<uint8_t> DictionaryLogin::dictionary_skin_database_to_internal;
std::vector<uint8_t> DictionaryLogin::dictionary_starter_database_to_internal;
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
std::vector<uint32_t> DictionaryLogin::dictionary_skin_internal_to_database;
std::vector<uint32_t> DictionaryLogin::dictionary_starter_internal_to_database;
#endif
