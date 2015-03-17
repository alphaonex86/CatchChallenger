#ifndef CATCHCHALLENGER_BASESERVERCOMMON_H
#define CATCHCHALLENGER_BASESERVERCOMMON_H

namespace CatchChallenger {
class BaseServerCommon
{
public:
    explicit BaseServerCommon();
    virtual ~BaseServerCommon();

    void SQL_common_load_start();
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
};
}

#endif
