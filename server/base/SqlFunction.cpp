#include "SqlFunction.h"
#include "../../general/base/cpp11addition.h"

using namespace CatchChallenger;

std::string SqlFunction::text_antislash="\\";
std::string SqlFunction::text_double_antislash="\\\\";
std::string SqlFunction::text_antislash_quote="\"";
std::string SqlFunction::text_double_antislash_quote="\\\"";
std::string SqlFunction::text_single_quote="'";
std::string SqlFunction::text_antislash_single_quote="\\'";

std::string SqlFunction::quoteSqlVariable(const std::string &variable)
{
    std::string newVariable=variable;
    stringreplaceAll(newVariable,SqlFunction::text_antislash,SqlFunction::text_double_antislash);
    stringreplaceAll(newVariable,SqlFunction::text_antislash_quote,SqlFunction::text_double_antislash_quote);
    stringreplaceAll(newVariable,SqlFunction::text_single_quote,SqlFunction::text_antislash_single_quote);
    return newVariable;
}
