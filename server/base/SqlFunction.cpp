#include "SqlFunction.h"
#include "../../general/base/cpp11addition.h"

using namespace CatchChallenger;

std::string SqlFunction::text_antislash="\\";
std::string SqlFunction::text_double_antislash="\\\\";
std::string SqlFunction::text_antislash_quote="\"";
std::string SqlFunction::text_double_antislash_quote="\\\"";
std::string SqlFunction::text_single_quote="'";
std::string SqlFunction::text_antislash_single_quote="\\'";

std::string SqlFunction::quoteSqlVariable(std::string variable)
{
    stringreplaceAll(variable,SqlFunction::text_antislash,SqlFunction::text_double_antislash);
    stringreplaceAll(variable,SqlFunction::text_antislash_quote,SqlFunction::text_double_antislash_quote);
    stringreplaceAll(variable,SqlFunction::text_single_quote,SqlFunction::text_antislash_single_quote);
    return variable;
}
