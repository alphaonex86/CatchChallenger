#include "SqlFunction.h"

using namespace CatchChallenger;

QString SqlFunction::text_antislash=QLatin1Literal("\\");
QString SqlFunction::text_double_antislash=QLatin1Literal("\\\\");
QString SqlFunction::text_antislash_quote=QLatin1Literal("\"");
QString SqlFunction::text_double_antislash_quote=QLatin1Literal("\\\"");
QString SqlFunction::text_single_quote=QLatin1Literal("'");
QString SqlFunction::text_antislash_single_quote=QLatin1Literal("\\'");

QString SqlFunction::quoteSqlVariable(QString variable)
{
    variable.replace(SqlFunction::text_antislash,SqlFunction::text_double_antislash);
    variable.replace(SqlFunction::text_antislash_quote,SqlFunction::text_double_antislash_quote);
    variable.replace(SqlFunction::text_single_quote,SqlFunction::text_antislash_single_quote);
    return variable;
}
