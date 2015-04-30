#include "DictionaryLogin.h"

using namespace CatchChallenger;

QList<ActionAllow> DictionaryLogin::dictionary_allow_database_to_internal;
QList<quint8> DictionaryLogin::dictionary_allow_internal_to_database;
QList<int> DictionaryLogin::dictionary_reputation_database_to_internal;//negative == not found
QList<quint8> DictionaryLogin::dictionary_skin_database_to_internal;
QList<quint32> DictionaryLogin::dictionary_skin_internal_to_database;
QList<quint8> DictionaryLogin::dictionary_starter_database_to_internal;
QList<quint32> DictionaryLogin::dictionary_starter_internal_to_database;