#include "SqlFunction.h"
#include "../../general/base/cpp11addition.h"

using namespace CatchChallenger;

std::basic_string<char> SqlFunction::text_antislash="\\";
std::basic_string<char> SqlFunction::text_double_antislash="\\\\";
std::basic_string<char> SqlFunction::text_antislash_quote="\"";
std::basic_string<char> SqlFunction::text_double_antislash_quote="\\\"";
std::basic_string<char> SqlFunction::text_single_quote="'";
std::basic_string<char> SqlFunction::text_antislash_single_quote="\\'";

std::basic_string<char> SqlFunction::quoteSqlVariable(std::basic_string<char> variable)
{
    replaceAll(variable,SqlFunction::text_antislash,SqlFunction::text_double_antislash);
    replaceAll(variable,SqlFunction::text_antislash_quote,SqlFunction::text_double_antislash_quote);
    replaceAll(variable,SqlFunction::text_single_quote,SqlFunction::text_antislash_single_quote);
    return variable;
}
