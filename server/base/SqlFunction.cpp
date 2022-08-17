#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
#else
#include "SqlFunction.hpp"
#include "../../general/base/cpp11addition.hpp"
using namespace CatchChallenger;

std::string SqlFunction::text_antislash="\\";
std::string SqlFunction::text_double_antislash="\\\\";
std::string SqlFunction::text_antislash_quote="\"";
std::string SqlFunction::text_double_antislash_quote="\\\"";
std::string SqlFunction::text_single_quote="'";
std::string SqlFunction::text_antislash_single_quote="\\'";
#endif

#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
#else
std::string SqlFunction::quoteSqlVariable(const std::string &variable)
{
    std::string newVariable=variable;
    stringreplaceAll(newVariable,SqlFunction::text_antislash,SqlFunction::text_double_antislash);
    stringreplaceAll(newVariable,SqlFunction::text_antislash_quote,SqlFunction::text_double_antislash_quote);
    stringreplaceAll(newVariable,SqlFunction::text_single_quote,SqlFunction::text_antislash_single_quote);
    return newVariable;
}
#endif

#ifdef DEBUG_MESSAGE_CLIENT_SQL
std::string SqlFunction::replaceSQLValues(const std::string &query,const std::vector<std::string> &values)
{
    std::string newQuery(query);
    unsigned int index=values.size();
    while(index>0)
    {
        stringreplaceAll(newQuery,"$"+std::to_string(index),"\""+values.at(index-1)+"\"");
        index--;
    }
    return newQuery;
}
#endif
