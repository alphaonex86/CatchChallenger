#include "SqlFunction.h"

using namespace CatchChallenger;

QString SqlFunction::quoteSqlVariable(QString variable)
{
    variable.replace(QLatin1String("\\"),QLatin1String("\\\\"));
    variable.replace(QLatin1String("\""),QLatin1String("\\\""));
    variable.replace(QLatin1String("'"),QLatin1String("\\'"));
    return variable;
}
