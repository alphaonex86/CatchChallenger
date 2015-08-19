#include "SqlFunction.h"

using namespace CatchChallenger;

std::basic_string<char> SqlFunction::text_antislash=QLatin1Literal("\\");
std::basic_string<char> SqlFunction::text_double_antislash=QLatin1Literal("\\\\");
std::basic_string<char> SqlFunction::text_antislash_quote=QLatin1Literal("\"");
std::basic_string<char> SqlFunction::text_double_antislash_quote=QLatin1Literal("\\\"");
std::basic_string<char> SqlFunction::text_single_quote=QLatin1Literal("'");
std::basic_string<char> SqlFunction::text_antislash_single_quote=QLatin1Literal("\\'");

std::basic_string<char> SqlFunction::quoteSqlVariable(std::basic_string<char> variable)
{
    variable.replace(SqlFunction::text_antislash,SqlFunction::text_double_antislash);
    variable.replace(SqlFunction::text_antislash_quote,SqlFunction::text_double_antislash_quote);
    variable.replace(SqlFunction::text_single_quote,SqlFunction::text_antislash_single_quote);
    return variable;
}
