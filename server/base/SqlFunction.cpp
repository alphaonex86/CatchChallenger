#include "SqlFunction.h"

using namespace Pokecraft;

QString SqlFunction::quoteSqlVariable(QString variable)
{
	variable.replace("\\","\\\\");
	variable.replace("\"","\\\"");
	variable.replace("'","\\'");
	return variable;
}
