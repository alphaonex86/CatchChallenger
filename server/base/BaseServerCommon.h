#ifndef CATCHCHALLENGER_BASESERVERCOMMON_H
#define CATCHCHALLENGER_BASESERVERCOMMON_H

#include <QList>
#include "../../general/base/GeneralStructures.h"

namespace CatchChallenger {
class BaseServerCommon
{
public:
    explicit BaseServerCommon();
    virtual ~BaseServerCommon();

    void SQL_common_load_start();

protected:
    QList<ActionAllow> dictionary_allow_database_to_internal;
    QList<quint8> dictionary_allow_internal_to_database;
    QList<int> dictionary_reputation_database_to_internal;//negative == not found
    QList<quint8> dictionary_skin_database_to_internal;
    QList<quint32> dictionary_skin_internal_to_database;
private:
    virtual void SQL_common_load_finish() = 0;

    void preload_dictionary_allow();
    void preload_dictionary_allow_static(void *object);
    void preload_dictionary_allow_return();
    void preload_dictionary_reputation();
    void preload_dictionary_reputation_static(void *object);
    void preload_dictionary_reputation_return();
    void preload_dictionary_skin();
    void preload_dictionary_skin_static(void *object);
    void preload_dictionary_skin_return();
    void preload_dictionary_starter();
    void preload_dictionary_starter_static(void *object);
    void preload_dictionary_starter_return();

protected:
    void unload();
};
}

#endif
