#include "SqlFunction.h"

using namespace CatchChallenger;

QString SqlFunction::quoteSqlVariable(QString variable)
{
	variable.replace("\\","\\\\");
	variable.replace("\"","\\\"");
	variable.replace("'","\\'");
	return variable;
}
