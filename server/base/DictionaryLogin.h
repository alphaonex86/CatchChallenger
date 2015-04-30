#ifndef CATCHCHALLENGER_DictionaryLogin_H
#define CATCHCHALLENGER_DictionaryLogin_H

#include <QList>
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"

namespace CatchChallenger {
class DictionaryLogin
{
public:
    static QList<ActionAllow> dictionary_allow_database_to_internal;
    static QList<quint8> dictionary_allow_internal_to_database;
    static QList<int> dictionary_reputation_database_to_internal;//negative == not found
    static QList<quint8> dictionary_skin_database_to_internal;
    static QList<quint32> dictionary_skin_internal_to_database;
    static QList<quint8> dictionary_starter_database_to_internal;
    static QList<quint32> dictionary_starter_internal_to_database;
};
}

#endif
