#include "BaseServer.hpp"
#include "../DictionaryServer.hpp"
#include "../GlobalServerData.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"

using namespace CatchChallenger;

void BaseServer::preload_15_async_map_semi_after_db_id()
{
    if(DictionaryServer::dictionary_map_database_to_internal.size()==0)
    {
        std::cerr << "Need be called after preload_dictionary_map()" << std::endl;
        abort();
    }

    mapPathToId.clear();

    preload_16_async_zone_sql();
}
